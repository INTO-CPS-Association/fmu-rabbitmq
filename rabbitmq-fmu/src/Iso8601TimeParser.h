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

    date::sys_time<std::chrono::milliseconds> parseIso8601ToMilliseconds(const std::string input) ;
}

#endif //RABBITMQFMUPROJECT_ISO8601TIMEPARSER_H
