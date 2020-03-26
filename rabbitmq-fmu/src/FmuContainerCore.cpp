//
// Created by Kenneth Guldbrandt Lausdahl on 09/03/2020.
//

#include "FmuContainerCore.h"

#include <iostream>
FmuContainerCore::FmuContainerCore( std::chrono::milliseconds maxAge,  std::map<ScalarVariableId ,int>  lookAhead)
        :  maxAge(maxAge), lookahead(lookAhead), startOffsetTime(std::chrono::milliseconds(0)) {}

void FmuContainerCore::add(ScalarVariableId id, TimedScalarBasicValue value) {

//    if (this->incomingUnprocessed.count(id) > 0) {
    this->incomingUnprocessed[id].push_back(value);
//    } else {
//        list<TimedScalarBasicValue> l;
//        l.push_back(value);
//        this->incomingUnprocessed[id] = l;
//    }

}

std::chrono::milliseconds FmuContainerCore::messageTimeToSim(date::sys_time<std::chrono::milliseconds> messageTime) {
    return (messageTime - this->startOffsetTime);
}

void FmuContainerCore::processIncoming() {
    //sort

    for (auto &pair: this->incomingUnprocessed) {

        auto id = pair.first;

        std::cout << "\t --  Incoming unprocessed: "<<id<<" - "<< this->incomingUnprocessed[id].size() <<std::endl;
        std::cout << "\t --  Incoming lookahead  : "<<id<<" - "<< this->incomingLookahead[id].size() <<std::endl;

        // read until lookahead or end
        auto it = this->incomingUnprocessed[id].begin();
        auto c=0;
        for (int i = 0; i < this->lookahead[id]; i++) {
            it++;
            c++;
        }

        std::cout << "\t --  Incoming lookahead  slice: "<<id<<" - " <<c <<endl;

        //move
        this->incomingLookahead[id].splice(this->incomingLookahead[id].end(), this->incomingUnprocessed[id],
                                           this->incomingUnprocessed[id].begin(), it);

        //sort
        pair.second.sort(
                [](const TimedScalarBasicValue &a, const TimedScalarBasicValue &b) { return a.first < b.first; });
    std::cout << "\t --> Incoming unprocessed: "<<id<<" - "<< this->incomingUnprocessed[id].size() <<std::endl;
    std::cout << "\t --> Incoming lookahead  : "<<id<<" - "<< this->incomingLookahead[id].size() <<std::endl;
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
    bool initial=true;

    for (auto &pair:this->currentData) {

        auto valueTime = pair.second.first;
        if(initial)
        {
            time = valueTime;
            initial = false;
            continue;
        }

        if(time < valueTime)
        {
            time = valueTime;
        }

    }
    return std::make_pair(!initial,time);
}




template<typename Predicate>
void FmuContainerCore::processLookahead(Predicate predicate){
    for (auto &pair:  this->incomingLookahead) {
        auto id = pair.first;

        auto itr = pair.second.begin();
        while (itr != pair.second.end()) {
            auto timeValue = itr;

            if (predicate(*timeValue)) {

                this->currentData.erase(id);
                this->currentData.insert(this->currentData.begin(),
                                         std::make_pair(id, std::make_pair(timeValue->first, timeValue->second)));

                itr = pair.second.erase(itr);
            } else {
                //stop if value is newer than time
                break;
            }
        }
    }
}

bool FmuContainerCore::initialize() {
    processIncoming();

    bool initial = this->currentData.empty();

    //process all lookahead messages
    auto predicate = [this,initial](FmuContainerCore::TimedScalarBasicValue& value){
        if(initial)
        {
            return true;
        } else{
            return value.first <= this->startOffsetTime;
        }

    };
    processLookahead(predicate);

    if(initial){
        auto initialTimePair= calculateStartTime();

        if(initialTimePair.first)
        {
            this->startOffsetTime = initialTimePair.second;
     //no longer initial mode since time is found
            initial = false;
        }
    }

    //run the age check for time 0
    return this->check(0);

//    for (auto &pair:  this->incomingLookahead) {
//        auto id = pair.first;
//
//        auto itr = pair.second.begin();
//        while (itr != pair.second.end()) {
//            auto timeValue = itr;
//
//            if (this->currentData.size() != knownIds.size() || !hasValueFor(this->currentData, knownIds)) {
//
//                this->currentData.erase(id);
//                this->currentData.insert(this->currentData.begin(),
//                                         std::make_pair(id, std::make_pair(timeValue->first, timeValue->second)));
//
//                itr = pair.second.erase(itr);
//            } else {
//                //stop if value is newer than time
//                break;
//            }
//        }
//    }

//   auto initialTimePair= calculateStartTime();
//
//    if(initialTimePair.first)
//    {
//        this->startOffsetTime = initialTimePair.second;
//    }
//    return initialTimePair.first;

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

    auto predicate = [time,this](FmuContainerCore::TimedScalarBasicValue& value){
       return messageTimeToSim(value.first).count() <= time;
    };
    processLookahead(predicate);
//    for (auto &pair:  this->incomingLookahead) {
//        auto id = pair.first;
//
//        auto itr = pair.second.begin();
//        while (itr != pair.second.end()) {
//            auto timeValue = itr;
//
//            if (messageTimeToSim(timeValue->first).count() <= time) {
////                timeValue->swap(this->currentData[id]);
//
//                this->currentData.erase(id);
//                this->currentData.insert(this->currentData.begin(),
//                                         std::make_pair(id, std::make_pair(timeValue->first, timeValue->second)));
//
//                itr = pair.second.erase(itr);
//            } else {
//                //stop if value is newer than time
//                break;
//            }
//        }
//    }


    // now all available lookahead values are read until time

    //check that we are ok
    return this->check(time);

}

std::map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue> FmuContainerCore::getData(){
    return this->currentData;
}

date::sys_time<std::chrono::milliseconds> FmuContainerCore::getStartOffsetTime(){
    return this->startOffsetTime;
}


bool FmuContainerCore::check(double time) {

    for (auto &lookaheadPair: this->lookahead) {

        auto id = lookaheadPair.first;
        if (this->currentData.count(id) == 0) {
            //missing known id
            return false;
        }


        auto valueTime = this->currentData.at(id).first;

        if ((messageTimeToSim(valueTime) + this->maxAge).count() <= time || time < messageTimeToSim(valueTime).count()) {
            return false;
        }
    }

    return true;
}


