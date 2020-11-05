//
// Created by Kenneth Guldbrandt Lausdahl on 09/03/2020.
//

#include "FmuContainerCore.h"

#include <iostream>

FmuContainerCore::FmuContainerCore(std::chrono::milliseconds maxAge, std::map<ScalarVariableId, int> lookAhead)
        : maxAge(maxAge), lookahead(lookAhead), startOffsetTime(std::chrono::milliseconds(0)), verbose(false), inputFlags(), inputVals() {


}

void FmuContainerCore::add(ScalarVariableId id, TimedScalarBasicValue value) {
    this->incomingUnprocessed[id].push_back(value);
}

std::chrono::milliseconds FmuContainerCore::messageTimeToSim(date::sys_time<std::chrono::milliseconds> messageTime) {
    return (messageTime - this->startOffsetTime);
}

void showL(list<FmuContainerCore::TimedScalarBasicValue> &list) {
    for (auto &p : list)
        cout << " ( " << p.first.time_since_epoch().count() << " , " << p.second << ") ";
}

void FmuContainerCore::processIncoming() {
    //sort

    for (auto &pair: this->incomingUnprocessed) {

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

        //sort
        this->incomingLookahead[id].sort(
                [](const TimedScalarBasicValue &a, const TimedScalarBasicValue &b) { return a.first < b.first; });

        if (verbose) {
            std::cout << "\t --> Incoming unprocessed: id=" << id << " - size=" << this->incomingUnprocessed[id].size()
                      << ": ";
            showL(this->incomingUnprocessed[id]);
            cout << std::endl;
            std::cout << "\t --> Incoming lookahead  : id=" << id << " - size=" << this->incomingLookahead[id].size()
                      << ": ";
            showL(this->incomingLookahead[id]);
            cout << std::endl << endl;
        }
    }


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

void FmuContainerCore::add_flag(int flagVRef, pair<string, bool> nameVal){
    this->inputFlags.insert(pair<int, pair<string, bool>>(flagVRef, nameVal));
    /*for(auto it = this->inputFlags.cbegin(); it != this->inputFlags.cend(); it++){
            cout << "ADD_FLAG:" << it->first << it->second.first << it->second.second << endl;
        }*/
}

void FmuContainerCore::update_flag(int flagVRef, pair<string, bool> nameVal){
    //this->inputFlags.insert(pair<string, pair<int, bool>>(flagName, vrVal));
    this->inputFlags[flagVRef].second = nameVal.second;
    //cout << "UPDATE_FLAG:" << flagVRef << this->inputFlags[flagVRef].first << this->inputFlags[flagVRef].second << endl;
}

void FmuContainerCore::add_input_val(int inputVRef, pair<string, string> nameVal){
    this->inputVals.insert(pair<int, pair<string, string>>(inputVRef, nameVal));
    /*for(auto it = this->inputFlags.cbegin(); it != this->inputFlags.cend(); it++){
            cout << "ADD_FLAG:" << it->first << it->second.first << it->second.second << endl;
        }*/
}

void FmuContainerCore::update_input_val(int inputVRef, pair<string, string> nameVal){
    //this->inputVals.insert(pair<string, pair<int, string>>(inputName, vrVal));
    this->inputVals[inputVRef].second = nameVal.second;
    //cout << "UPDATE_FLAG:" << inputVRef << this->inputVals[inputVRef].first << this->inputVals[inputVRef].second << endl;
}

//Left it here - look at todo
void FmuContainerCore::sendCheckCompose(string &message){
    for(auto it = this->inputFlags.cbegin(); it != this->inputFlags.cend(); it++){
        if (it->second.second){
            //if the flag is set, then get the input value with value reference incremented by 1
            string cmd = this->inputVals[it->first+1].first;
            //TODO change the message to send the content of the input and not the flag
            message += R"(")" + cmd + R"(":)" + this->inputVals[it->first+1].second + R"(,)";
        }
    }
}

void FmuContainerCore::printFlagsInputs(){
    for(auto it = this->inputFlags.cbegin(); it != this->inputFlags.cend(); it++){
            cout << "FLAG: " << it->first << " " << it->second.first << " " << it->second.second << endl;
        }
    for(auto it = this->inputVals.cbegin(); it != this->inputVals.cend(); it++){
        cout << "INPUT: " << it->first << " " << it->second.first << " " << it->second.second << endl;
    }
}


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


