//
// Created by Kenneth Guldbrandt Lausdahl on 12/12/2019.
//

#ifndef RABBITMQ_FMU_MESSAGEPARSER_H
#define RABBITMQ_FMU_MESSAGEPARSER_H

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include "DataPoint.h"
#include "modeldescription/ModelDescriptionParser.h"

class MessageParser {
public :
   static bool parse(map<string, ModelDescriptionParser::ScalarVariable> *nameToValueReference,const char *json,  DataPoint* output);
   static bool parseSystemHealthMessage(date::sys_time<std::chrono::milliseconds> &simTime, date::sys_time<std::chrono::milliseconds> &rTime, const char *json);
};

#endif //RABBITMQ_FMU_MESSAGEPARSER_H
