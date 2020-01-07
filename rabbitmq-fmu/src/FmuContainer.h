//
// Created by Kenneth Guldbrandt Lausdahl on 11/12/2019.
//

#ifndef RABBITMQ_FMU_FMUCONTAINER_H
#define RABBITMQ_FMU_FMUCONTAINER_H

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include "fmi2Functions.h"
#include "DataPoint.h"
#include "modeldescription/ModelDescriptionParser.h"
#include <list>
#include <iterator>
#include "rabbitmq/RabbitmqHandler.h"

#define RABBITMQ_FMU_HOSTNAME_ID 0
#define RABBITMQ_FMU_PORT 1
#define RABBITMQ_FMU_USER 2
#define RABBITMQ_FMU_PWD 3
#define RABBITMQ_FMU_ROUTING_KEY 4
#define RABBITMQ_FMU_START_TIMESTAMP 5
#define RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT 6

using namespace std;
class FmuContainer {
public:
    const fmi2CallbackFunctions *m_functions;
    const string m_name;


    FmuContainer(const fmi2CallbackFunctions *mFunctions, const char *mName,map<string,ModelDescriptionParser::ScalarVariable> nameToValueReference, DataPoint initialDataPoint);
~FmuContainer();

    fmi2ComponentEnvironment getComponentEnvironment();

    bool step(fmi2Real currentCommunicationPoint,
              fmi2Real communicationStepSize);

    bool fmi2GetMaxStepsize(fmi2Real *size);

    bool getString(const fmi2ValueReference vr[], size_t nvr, fmi2String value[]);

    bool getReal(const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]);

    bool getBoolean(const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]);

    bool getInteger(const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]);

    bool setString(const fmi2ValueReference vr[], size_t nvr,const fmi2String value[]);

    bool setReal(const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]);

    bool setBoolean(const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]);

    bool setInteger(const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]);


    bool terminate();

    bool setup(fmi2Real startTime);

    bool initialize();

private:
    std::time_t time;
    std::time_t startOffsetTime;
    int communicationTimeout;

    map<string,ModelDescriptionParser::ScalarVariable> nameToValueReference;

    list< DataPoint> data;
    DataPoint currentData;

    RabbitmqHandler *rabbitMqHandler;

};


#endif //RABBITMQ_FMU_FMUCONTAINER_H
