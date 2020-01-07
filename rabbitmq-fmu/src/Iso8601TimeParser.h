//
// Created by Kenneth Guldbrandt Lausdahl on 07/01/2020.
//

#ifndef RABBITMQFMUPROJECT_ISO8601TIMEPARSER_H
#define RABBITMQFMUPROJECT_ISO8601TIMEPARSER_H

#include <iostream>
#include <ctime>
using namespace std;

namespace Iso8601 {



    std::time_t parseIso8601ToMilliseconds(const std::string &input) ;
}

#endif //RABBITMQFMUPROJECT_ISO8601TIMEPARSER_H
