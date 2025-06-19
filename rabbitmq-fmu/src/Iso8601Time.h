//
// Created by Kenneth Guldbrandt Lausdahl on 07/01/2020.
//

#ifndef RABBITMQFMUPROJECT_ISO8601TIMEPARSER_H
#define RABBITMQFMUPROJECT_ISO8601TIMEPARSER_H

#include <iostream>
#include <ctime>
#include "date/date.h"
using namespace std;
using namespace std::chrono;
using namespace date;

namespace Iso8601 {
    std::chrono::system_clock::time_point parseIso8601String(const std::string &input);

    string toIso8601ToString(system_clock::time_point timePoint);
}

#endif //RABBITMQFMUPROJECT_ISO8601TIMEPARSER_H
