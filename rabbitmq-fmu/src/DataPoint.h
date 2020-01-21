//
// Created by Kenneth Guldbrandt Lausdahl on 11/12/2019.
//

#ifndef RABBITMQ_FMU_DATAPOINT_H
#define RABBITMQ_FMU_DATAPOINT_H

#include <string>
#include <map>
#include <ctime>

#define mergeFieldMap(mapName, other) \
    for(auto& pair: other.mapName) \
    { \
        this->mapName[pair.first]=pair.second; \
    }
#include "date/date.h"

using namespace std;

struct DataPoint {
    date::sys_time<std::chrono::milliseconds> time;

    std::map<unsigned int, int> integerValues;
    std::map<unsigned int, double> doubleValues;
    std::map<unsigned int, bool> booleanValues;
    std::map<unsigned int, std::string> stringValues;


    DataPoint merge(DataPoint other) {
        this->time = other.time;
//C 17 not properly supported on mac so we use old style
        mergeFieldMap(integerValues, other)
        mergeFieldMap(doubleValues, other)
        mergeFieldMap(booleanValues, other)
        mergeFieldMap(stringValues, other)
        return *this;
    }
};

#endif //RABBITMQ_FMU_DATAPOINT_H
