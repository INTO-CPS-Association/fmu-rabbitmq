//
// Created by Kenneth Guldbrandt Lausdahl on 09/03/2020.
//

#include "FmuContainerCore.h"

#include <iostream>

FmuContainerCore::FmuContainerCore(std::chrono::milliseconds maxAge, std::map<ScalarVariableId, int> lookAhead)
        : maxAge(maxAge), lookahead(lookAhead), startOffsetTime(std::chrono::milliseconds(0)), verbose(false) {


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
            std::cout << "\t --  Incoming unprocessed: id=" << id << " - size=" << this->incomingUnprocessed[id].size()
                      << ": ";
            showL(this->incomingUnprocessed[id]);
            cout << std::endl;
            std::cout << "\t --  Incoming lookahead  : id=" << id << " - size=" << this->incomingLookahead[id].size()
                      << ": ";
            showL(this->incomingLookahead[id]);
            cout << std::endl;
        }

        // read until lookahead or end
        auto it = this->incomingUnprocessed[id].begin();
        auto c = 0;
        for (int i = 0; i < this->lookahead[id] && it != this->incomingUnprocessed[id].end(); i++) {
            it++;
            c++;
        }
        if (verbose) {
            std::cout << "\t --  Incoming lookahead  slice  id=: " << id << " - count=" << c << endl;
        }
        //move
        this->incomingLookahead[id].splice(this->incomingLookahead[id].end(), this->incomingUnprocessed[id],
                                           this->incomingUnprocessed[id].begin(), it);

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

                // if the current state does not contain the value or if the new value is newer than the current state value then use the new value
                if (this->currentData.find(id) == this->currentData.end() ||
                    this->currentData[id].first < timeValue->first) {
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

    bool initial = this->currentData.empty();

    if (verbose && initial) {
        cout << "Initial initialize!" << endl;
    }

    //process all lookahead messages
    auto predicate = [this, initial](FmuContainerCore::TimedScalarBasicValue &value) {
        if (initial) {
            return true;
        } else {
            return value.first <= this->startOffsetTime;
        }

    };
    processLookahead(predicate);

    if (initial) {
        auto initialTimePair = calculateStartTime();

        if (initialTimePair.first) {
            this->startOffsetTime = initialTimePair.second;
            //no longer initial mode since time is found
            initial = false;
        }
    }

    //run the age check for time 0
    return this->check(0);
}

bool FmuContainerCore::process(double time) {

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

