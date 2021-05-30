//
// Created by Kenneth Guldbrandt Lausdahl on 09/03/2020.
//

#include "FmuContainerCore.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

#define SEQNOID 10

#define FmuContainerCore_LOG(status, category, message, args...)       \
  if (m_functions != NULL) {                                             \
    if (m_functions->logger != NULL) {                                   \
      m_functions->logger(m_functions->componentEnvironment, m_name.c_str(), status, \
                        category, message, args);                      \
    }                                                                  \
  } else {                                                             \
    fprintf(stderr, "Name '%s', Category: '%s'\n", m_name.c_str(), category);    \
    fprintf(stderr, message, args);                                    \
    fprintf(stderr, "\n");                                             \
  }

FmuContainerCore::FmuContainerCore(std::chrono::milliseconds maxAge, std::map<ScalarVariableId, int> lookAhead,const fmi2CallbackFunctions *mFunctions,
                     const char *mName)
        : maxAge(maxAge), lookahead(lookAhead), startOffsetTime(std::chrono::milliseconds(0)), verbose(false), m_functions(mFunctions),
        m_name(mName){

}

FmuContainerCore::FmuContainerCore(std::chrono::milliseconds maxAge, std::map<ScalarVariableId, int> lookAhead)
        : maxAge(maxAge), lookahead(lookAhead), startOffsetTime(std::chrono::milliseconds(0)), verbose(false){

}

void FmuContainerCore::add(ScalarVariableId id, TimedScalarBasicValue value) {
    if (verbose && id == SEQNOID)
    {
       cout << "adding value=" << value.first.time_since_epoch().count() << ", " << value.second << endl;
    }
    this->incomingUnprocessed[id].push(value);
}

std::chrono::milliseconds FmuContainerCore::messageTimeToSim(date::sys_time<std::chrono::milliseconds> messageTime) {
    return (messageTime - this->startOffsetTime);
}

std::chrono::milliseconds FmuContainerCore::simTimeToReal(long long simTime) {
    if(verbose){
        cout << "startoffset " << this->startOffsetTime.time_since_epoch().count() << endl;
        cout << "startoffset + time passed in [ms] " << this->startOffsetTime.time_since_epoch().count() + std::chrono::milliseconds(simTime).count() << endl;
    }
    return (this->startOffsetTime.time_since_epoch() + std::chrono::milliseconds(simTime));
}

void FmuContainerCore::convertTimeToString(long long milliSecondsSinceEpoch, string &message){                
    const auto durationSinceEpoch = std::chrono::milliseconds(milliSecondsSinceEpoch);
    const std::chrono::time_point<std::chrono::system_clock> tp_after_duration(durationSinceEpoch);
    time_t time_after_duration = std::chrono::system_clock::to_time_t(tp_after_duration);
    std::tm* formattedTime = std::gmtime(&time_after_duration);
    long long int milliseconds_remainder = milliSecondsSinceEpoch % 1000;
    stringstream transTime, formatString;
    //transTime << put_time(std::localtime(&time_after_duration), "%Y-%m-%dT%H:%M:%S.") << milliseconds_remainder << "+01:00";
    int no_digits = 0;
    string appendZeros = "";
    if(milliseconds_remainder>0){
        no_digits = floor(log10(milliseconds_remainder))+1;
        //cout << "Number of digits: " << no_digits << endl;
        if(no_digits==1)appendZeros.append("00");
        if(no_digits==2)appendZeros.append("0");
    }
    formatString << "%FT%T."<< appendZeros.c_str() << milliseconds_remainder <<"%Ez";
    //cout << "Format string: " << formatString.str().c_str() << endl;
    transTime << put_time(formattedTime, formatString.str().c_str());
    message = transTime.str().insert(transTime.str().length()-2, ":");
    //cout <<"SIM time to REAL time"<< message << endl;
}

void showL(list<FmuContainerCore::TimedScalarBasicValue> &list) {
    for (auto &p : list){
        cout << "showL ( " << p.first.time_since_epoch().count() << " , " << p.second << ") ";
     }
        
}

template<typename Predicate>
void FmuContainerCore::processIncoming(Predicate predicate) {

    for (auto &pair: this->incomingUnprocessed) {

#ifdef USE_RBMQ_FMU_THREAD
        m.lock();
#endif
        auto id = pair.first;
        auto existingValue = this->currentData.find(id);
        auto cnt = 0;
        date::sys_time<std::chrono::milliseconds> t;
	TimedScalarBasicValue value = std::make_pair(t, ScalarVariableBaseValue(0));
        bool valid = false;

        // read until end, time (predicate) or lookahead size
        while (!this->incomingUnprocessed[id].empty() &&
               cnt++ < this->lookahead[id] &&
               predicate(this->incomingUnprocessed[id].top())) {

            value = this->incomingUnprocessed[id].top();
            this->incomingUnprocessed[id].pop();
            valid = true;
        }

        if (valid &&
            (existingValue == this->currentData.end() ||
             existingValue->second.first < value.first)) {
            this->currentData.erase(id);
            this->currentData.insert(this->currentData.begin(),
                std::make_pair(id, std::make_pair(value.first, value.second)));
            if (verbose) {
                if(id == SEQNOID){
                  cout << "Updated state with id=" << id << " time value=" << value.second.i.i << endl;
                  FmuContainerCore_LOG(fmi2Fatal, "logAll", "FmuContainerCore_LOG Updated state with id=%d value=%d",id, value.second.i.i);
                }
            }
        }

#ifdef USE_RBMQ_FMU_THREAD
        m.unlock();
#endif
    }

#ifdef USE_RBMQ_FMU_THREAD
    m.lock();
#endif
    if (!this->incomingUnprocessed.empty()) {
        if (verbose) {
            cout << "Cleaning incomingUnprocessed" << endl;
        }

        for (auto itr = this->incomingUnprocessed.begin(); itr != this->incomingUnprocessed.end();) {
            auto id = itr->first;
            if (itr->second.empty()) {
                this->incomingUnprocessed.erase(itr++);
            } else {
                ++itr;
            }
        }

    }
#ifdef USE_RBMQ_FMU_THREAD
    m.unlock();
#endif
}

bool FmuContainerCore::hasValueFor(map<ScalarVariableId, TimedScalarBasicValue> &currentData,
                                   list<ScalarVariableId> &knownIds) {

    for (auto &id : knownIds) {
        auto itr = currentData.find(id);
        if (itr == currentData.end()) {
            //key missing
            return false;
        }
    }
    return true;
}

pair<bool, date::sys_time<std::chrono::milliseconds>> FmuContainerCore::calculateStartTime() {

    date::sys_time<std::chrono::milliseconds> time;
    bool initial = true;

    for (auto &pair:this->currentData) {

        auto valueTime = pair.second.first;
        if (initial) {
            time = valueTime;
            initial = false;
            continue;
        }

        if (time < valueTime) {
            time = valueTime;
        }

    }
    return std::make_pair(!initial, time);
}

bool FmuContainerCore::initialize() {

    if (verbose) {
        cout << "Initial initialize! Max age: " << this->maxAge.count() << " - Lookahead: ";
        for (auto pair: this->lookahead) {
            cout << "(id: " << pair.first << " value: " << pair.second << ")";
        }
        cout << endl;
    }

    //process all lookahead messages
    auto predicate = [](const FmuContainerCore::TimedScalarBasicValue &value) {
        return true;
    };
    processIncoming(predicate);

    auto initialTimePair = calculateStartTime();

    if (initialTimePair.first) {
        this->startOffsetTime = initialTimePair.second;
        //no longer initial mode since time is found
    }

    //run the age check for time 0
    return this->check(0);
}

bool FmuContainerCore:: process(double time ) {

//check messages for acceptable aged values

    /*if (this->check(time)) {
        //all ok do nothing
        return true;
    }*/

//read all incoming and sort
    /* processIncoming(); */

//read until time
    auto predicate = [time, this](const FmuContainerCore::TimedScalarBasicValue &value) {
        return messageTimeToSim(value.first).count() <= time;
    };
    processIncoming(predicate);

    // now all available lookahead values are read until time

    //check that we are ok
    return this->check(time);

}

std::map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue> FmuContainerCore::getData() {
    return this->currentData;
}

date::sys_time<std::chrono::milliseconds> FmuContainerCore::getStartOffsetTime() {
    return this->startOffsetTime;
}


//bool FmuContainerCore::check(double time) {
//
//    for (auto &lookaheadPair: this->lookahead) {
//
//        auto id = lookaheadPair.first;
//        if (this->currentData.count(id) == 0) {
//            //missing known id
//            if (verbose) {
//                printf("Failing check on %d\n", id);
//            }
//            return false;
//        }
//
//
//        auto valueTime = this->currentData.at(id).first;
//        auto valueTimeInSimtime = messageTimeToSim(valueTime);
//
//        if (time < valueTimeInSimtime.count()) {
//            if (verbose) {
//                printf("Future value discovered. Failing check on %d. maxage %lld, t1 %lld, t1+age %lld, t %f\n", id,
//                       this->maxAge.count(), valueTimeInSimtime.count(),
//                       (messageTimeToSim(valueTime) + this->maxAge).count(), time);
//            }
//            return false;
//
//        } else {
//            if ((valueTimeInSimtime + this->maxAge).count() < time) {
//                if (verbose)
//                    printf("Value is too old for %d. maxage %lld, t1 %lld, t1+age %lld, t %9.f\n", id, this->maxAge.count(),
//                           valueTimeInSimtime.count(), (valueTimeInSimtime + this->maxAge).count(),
//                           time);
//                return false;
//            }
//            else {
//                // There are no error cases, so we pass!
//                return true;
//            }
//        }
//    }
//
//    // We have been through the entire list and no item passed the tests.
//    return false;
//}

bool FmuContainerCore::check(double time) {

    for (auto &lookaheadPair: this->lookahead) {

        auto id = lookaheadPair.first;
        if (this->currentData.count(id) == 0) {
            //missing known id
            if (verbose) {
                printf("Failing check on %d\n", id);
            }
            return false;
        }


        auto valueTime = this->currentData.at(id).first;
        
        std::stringstream temp;
        temp << valueTime.time_since_epoch().count();
        if(verbose){
            if(id == SEQNOID){
            cout << "The time value of datapoint " << temp.str().c_str() << endl;
            cout << "The time value of datapoint " << messageTimeToSim(valueTime).count() << endl;
            }
        }

        if (time < messageTimeToSim(valueTime).count()) {
            if (verbose) {
                if(id == SEQNOID){
                printf("Future value discovered. Failing check on %d. maxage %lld, t1 %lld, t1+age %lld, t %f\n", id,
                       this->maxAge.count(), messageTimeToSim(valueTime).count(),
                       (messageTimeToSim(valueTime) + this->maxAge).count(), time);
                }
            }
            return false;
        }
        if ((messageTimeToSim(valueTime) + this->maxAge).count() < time) {
            if(id == SEQNOID){
            printf("Failing check on %d. seqno %d, maxage %lld, t1 %lld, t1+age %lld, t %9.f\n", id, this->currentData.at(id).second.i.i, this->maxAge.count(),
                   messageTimeToSim(valueTime).count(), (messageTimeToSim(valueTime) + this->maxAge).count(), time);
            }
            return false;
        }
    }

    return true;
}

void FmuContainerCore::setVerbose(bool verbose) {
    this->verbose = verbose;

}

void showValue(ostream &os, const char *prefix, date::sys_time<std::chrono::milliseconds> offset,
               FmuContainerCore::TimedScalarBasicValue val) {
    os << prefix << "Time: " << val.first.time_since_epoch().count() << " (" << (val.first - offset).count() << ")"
       << " Value: " << val.second << "\n";
}

void FmuContainerCore::messageCompose(pair<string,string>input, string &message){
    if(!message.empty()){
        message += R"(")" + input.first + R"(":)" + input.second + R"(,)";
    }
    else{
        message = R"(")" + input.first + R"(":)" + input.second + R"(,)";
    }
}

std::chrono::milliseconds FmuContainerCore::message2SimTime(date::sys_time<std::chrono::milliseconds> rTime){
    std::stringstream rtimeString, temp;
    //cout << "RTIME: "  << endl;
    rtimeString << this->messageTimeToSim(rTime).count() << endl;
    //cout << "RTIME: " << rtimeString.str().c_str();
    return this->messageTimeToSim(rTime);
}
/*
void FmuContainerCore::setTimeDiscrepancyOutput(double timeDiff, int vref){
    auto getValue = this->currentData.find(vref);
    if (!(getValue == this->currentData.end())){
        //cout << "data we care about, royal we: " << getValue->second.second.d.d << "with vref: " << vref << endl;
        this->currentData.erase(vref);
        getValue->second.second.d.d = timeDiff;
        //cout << "data we care about, royal we: " << getValue->second.second.d.d << "with vref: " << vref << endl;
        this->currentData.insert(this->currentData.begin(),std::make_pair(vref, std::make_pair(getValue->second.first, getValue->second.second)));
        if(verbose){
            for(auto it = this->currentData.cbegin(); it != this->currentData.cend(); it++){
                cout << "MY DATA: " << it->first << " " << it->second.second << endl;
            }
        }
    }
}*/

void FmuContainerCore::setTimeDiscrepancyOutput(bool valid, double timeDiffNew, double timeDiffOld, int vref){
    auto getValue = this->currentData.find(vref);
    if (!(getValue == this->currentData.end())){
        //cout << "data we care about, royal we: " << getValue->second.second.d.d << "with vref: " << vref << endl;
        this->currentData.erase(vref);
        if(valid){
            getValue->second.second.d.d = timeDiffNew;
        }
        else{
            getValue->second.second.d.d = timeDiffOld;
        }
        //cout << "data we care about, royal we: " << getValue->second.second.d.d << "with vref: " << vref << endl;
        this->currentData.insert(this->currentData.begin(),std::make_pair(vref, std::make_pair(getValue->second.first, getValue->second.second)));
        if(verbose){
            for(auto it = this->currentData.cbegin(); it != this->currentData.cend(); it++){
                //cout << "MY DATA: " << it->first << " " << it->second.second << endl;
            }
        }
    }

}

double FmuContainerCore::getTimeDiscrepancyOutput(int vref){
    auto getValue = this->currentData.find(vref);
    if (!(getValue == this->currentData.end())){
        return getValue->second.second.d.d;
    }
}

int FmuContainerCore::getSeqNO(int vref){
    auto getValue = this->currentData.find(vref);
    if (!(getValue == this->currentData.end())){
        return getValue->second.second.i.i;
    }
}

#ifdef USE_RBMQ_FMU_THREAD
bool FmuContainerCore::hasUnprocessed(void){
    /* cout << "HERE: " << this->incomingUnprocessed.empty() << " " << this->incomingLookahead.empty() << endl; */
    return !this->incomingUnprocessed.empty();
}
#endif
int FmuContainerCore::incomingSize(void){
    return this->incomingUnprocessed[10].size();
}

#ifdef USE_RBMQ_FMU_HEALTH_THREAD
bool FmuContainerCore::hasUnprocessedHealth(void){
    return !this->incomingUnprocessedHealth.empty();
}
#endif

ostream &operator<<(ostream &os, const FmuContainerCore &c) {
    os << "------------------------------ INFO ------------------------------" << "\n";
    os << "Max age: " << c.maxAge.count() << "\n";
    os << "StartTime: " << c.startOffsetTime.time_since_epoch().count() << "\n";
    os << "Lookahead ids: [";
    for (auto &id:c.lookahead) {
        cout << id.first << "->" << id.second << " ";
    }
    os << "]" << "\n";
    /* os << "Incoming" << "\n"; */
    /* for (auto &pair: c.incomingUnprocessed) { */
    /*     cout << "\tId: " << pair.first << "\n"; */
    /*     for (auto &val: pair.second) { */
    /*         showValue(os, "\t\t", c.startOffsetTime, val); */

    /*     } */
    /* } */

    /* os << "Lookahead" << "\n"; */
    /* for (auto &pair: c.incomingLookahead) { */
    /*     os << "\tId: " << pair.first << "\n"; */
    /*     for (auto &val: pair.second) { */
    /*         showValue(os, "\t\t", c.startOffsetTime, val); */

    /*     } */
    /* } */

    os << "Data" << "\n";
    for (auto &pair: c.currentData) {
        os << "\tId: " << pair.first;

        showValue(os, " ", c.startOffsetTime, pair.second);


    }
    os << "\n";
    os << "------------------------------------------------------------------" << "\n";
    return os;
}


