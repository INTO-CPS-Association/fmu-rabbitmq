//
// Created by Kenneth Guldbrandt Lausdahl on 09/03/2020.
//

#include "FmuContainerCore.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

FmuContainerCore::FmuContainerCore(std::chrono::milliseconds maxAge, std::map<ScalarVariableId, int> lookAhead)
        : maxAge(maxAge), lookahead(lookAhead), startOffsetTime(std::chrono::milliseconds(0)), verbose(false){


}

void FmuContainerCore::add(ScalarVariableId id, TimedScalarBasicValue value) {
    this->incomingUnprocessed[id].push_back(value);
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
    for (auto &p : list)
        cout << " ( " << p.first.time_since_epoch().count() << " , " << p.second << ") ";
}

void FmuContainerCore::processIncoming() {
    //sort


    for (auto &pair: this->incomingUnprocessed) {

#ifdef USE_RBMQ_FMU_THREAD
        m.lock();
#endif
        auto id = pair.first;
        if (verbose) {
            cout << "\t --  Incoming unprocessed: id=" << id << " - size=" << pair.second.size()
                 << ": ";
            showL(this->incomingUnprocessed[id]);
            cout << std::endl;
            std::cout << "\t --  Incoming lookahead  : id=" << id << " - size=" << this->incomingLookahead[id].size()
                      << ": ";
            showL(this->incomingLookahead[id]);
            cout << std::endl;
        }

        // read until lookahead or end
        auto it = pair.second.begin();
        auto c = 0;
        for (int i = 0; i < this->lookahead[id] && it != pair.second.end(); i++) {
            it++;
            c++;
        }
        if (verbose) {
            std::cout << "\t --  Incoming lookahead  slice  id=: " << id << " - count=" << c << endl;
        }
        //move
        this->incomingLookahead[id].splice(this->incomingLookahead[id].end(), pair.second,
                pair.second.begin(), it);

#ifdef USE_RBMQ_FMU_THREAD
        m.unlock();
#endif
        //sort
        this->incomingLookahead[id].sort(
                [](const TimedScalarBasicValue &a, const TimedScalarBasicValue &b) { return a.first < b.first; });

        if (verbose) {
            std::cout << "\t --> Incoming unprocessed: id=" << id << " - size=" << this->incomingUnprocessed[id].size()
                      << ": ";
#ifdef USE_RBMQ_FMU_THREAD
            m.lock();
#endif
            showL(this->incomingUnprocessed[id]);
            cout << std::endl;
            std::cout << "\t --> Incoming lookahead  : id=" << id << " - size=" << this->incomingLookahead[id].size()
                      << ": ";
            showL(this->incomingLookahead[id]);
            cout << std::endl << endl;
#ifdef USE_RBMQ_FMU_THREAD
            m.unlock();
#endif
        }
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
            if (verbose) {
                std::cout << "\t --  Incoming unprocessed  : id=" << id << " - size="
                          << this->incomingUnprocessed[id].size()
                          << ": ";
                showL(this->incomingUnprocessed[id]);
                cout << std::endl;
            }
            if (itr->second.empty()) {
                if (verbose) {
                    printf("deleting list id=%d\n", itr->first);
                }
                this->incomingUnprocessed.erase(itr++);
            } else {
                ++itr;
            }
        }
#ifdef USE_RBMQ_FMU_THREAD
    m.unlock();
#endif
    }
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


template<typename Predicate>
void FmuContainerCore::processLookahead(Predicate predicate) {

    if (verbose) {
        cout << "Lookaheads:" << endl;
        for (auto &p : this->lookahead) {
            cout << "\t" << p.first << " = " << p.second << endl;
        }
    }

    for (auto &pair:  this->incomingLookahead) {
        auto id = pair.first;

        auto itr = pair.second.begin();
        while (itr != pair.second.end()) {
            auto timeValue = itr;

            if (predicate(*timeValue)) {

                auto existingValue = this->currentData.find(id);
                // if the current state does not contain the value or if the new value is newer than the current state value then use the new value
                if (existingValue == this->currentData.end() ||
                    existingValue->second.first < timeValue->first) {
                    this->currentData.erase(id);
                    this->currentData.insert(this->currentData.begin(),
                                             std::make_pair(id, std::make_pair(timeValue->first, timeValue->second)));
                    if (verbose) {
                        cout << "Updated state with id=" << id << " value=" << timeValue->second.d.d << endl;
                    }
                }

                itr = pair.second.erase(itr);
            } else {
                //stop if value is newer than time
                break;
            }
        }

    }

    if (!this->incomingLookahead.empty()) {
        if (verbose) {
            cout << "Cleaning incoming lookahead" << endl;
        }
        for (auto itr = this->incomingLookahead.begin(); itr != this->incomingLookahead.end();) {
            auto id = itr->first;
            if (verbose) {
                std::cout << "\t --  Incoming lookahead  : Id=" << id << " - Size="
                          << this->incomingLookahead[id].size()
                          << ": ";
                showL(this->incomingLookahead[id]);
                cout << std::endl;
            }
            if (itr->second.empty()) {
                if (verbose) {
                    printf("deleting list id=%d\n", itr->first);
                }
                this->incomingLookahead.erase(itr++);
            } else {
                ++itr;
            }
        }
    }
}

bool FmuContainerCore::initialize() {
    processIncoming();

    if (verbose) {
        cout << "Initial initialize! Max age: " << this->maxAge.count() << " - Lookahead: ";
        for (auto pair: this->lookahead) {
            cout << "(id: " << pair.first << " value: " << pair.second << ")";
        }
        cout << endl;
    }

    //process all lookahead messages
    auto predicate = [](FmuContainerCore::TimedScalarBasicValue &value) {
        return true;
    };
    processLookahead(predicate);

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

    if (this->check(time)) {
        //all ok do nothing
        return true;
    }

//read all incoming and sort
    processIncoming();

//read until time
    auto predicate = [time, this](FmuContainerCore::TimedScalarBasicValue &value) {
        return messageTimeToSim(value.first).count() <= time;
    };
    processLookahead(predicate);

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
            cout << "The time value of datapoint " << temp.str().c_str() << endl;
            cout << "The time value of datapoint " << messageTimeToSim(valueTime).count() << endl;
        }

        if (time < messageTimeToSim(valueTime).count()) {
            if (verbose) {
                printf("Future value discovered. Failing check on %d. maxage %lld, t1 %lld, t1+age %lld, t %f\n", id,
                       this->maxAge.count(), messageTimeToSim(valueTime).count(),
                       (messageTimeToSim(valueTime) + this->maxAge).count(), time);
            }
            return false;
        }
        if ((messageTimeToSim(valueTime) + this->maxAge).count() < time) {
            if (verbose) {
                printf("Failing check on %d. maxage %lld, t1 %lld, t1+age %lld, t %9.f\n", id, this->maxAge.count(),
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

#ifdef USE_RBMQ_FMU_THREAD
bool FmuContainerCore::hasUnprocessed(void){
    return !this->incomingUnprocessed.empty();
}
#endif

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
    os << "Incoming" << "\n";
    for (auto &pair: c.incomingUnprocessed) {
        cout << "\tId: " << pair.first << "\n";
        for (auto &val: pair.second) {
            showValue(os, "\t\t", c.startOffsetTime, val);

        }
    }

    os << "Lookahead" << "\n";
    for (auto &pair: c.incomingLookahead) {
        os << "\tId: " << pair.first << "\n";
        for (auto &val: pair.second) {
            showValue(os, "\t\t", c.startOffsetTime, val);

        }
    }

    os << "Data" << "\n";
    for (auto &pair: c.currentData) {
        os << "\tId: " << pair.first;

        showValue(os, " ", c.startOffsetTime, pair.second);


    }
    os << "\n";
    os << "------------------------------------------------------------------" << "\n";
    return os;
}


