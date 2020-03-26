//
// Created by Kenneth Guldbrandt Lausdahl on 11/12/2019.
//

#ifndef RABBITMQ_FMU_FMUCONTAINER_H
#define RABBITMQ_FMU_FMUCONTAINER_H

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <cmath>
#include "fmi2Functions.h"
#include "DataPoint.h"
#include "modeldescription/ModelDescriptionParser.h"
#include <list>
#include <iterator>
#include "rabbitmq/RabbitmqHandler.h"
#include "Iso8601TimeParser.h"
#include "FmuContainerCore.h"

#define RABBITMQ_FMU_HOSTNAME_ID 0
#define RABBITMQ_FMU_PORT 1
#define RABBITMQ_FMU_USER 2
#define RABBITMQ_FMU_PWD 3
#define RABBITMQ_FMU_ROUTING_KEY 4
#define RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT 5
#define RABBITMQ_FMU_PRECISION 6

using namespace std;

class FmuContainer {
public:
    const fmi2CallbackFunctions *m_functions;
    const string m_name;


    FmuContainer(const fmi2CallbackFunctions *mFunctions,bool loggingOn, const char *mName,
                 map<string, ModelDescriptionParser::ScalarVariable> nameToValueReference, DataPoint initialDataPoint);

    ~FmuContainer();

    fmi2ComponentEnvironment getComponentEnvironment();

    bool step(fmi2Real currentCommunicationPoint,
              fmi2Real communicationStepSize);

    bool fmi2GetMaxStepsize(fmi2Real *size);

    bool getString(const fmi2ValueReference vr[], size_t nvr, fmi2String value[]);

    bool getReal(const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]);

    bool getBoolean(const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]);

    bool getInteger(const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]);

    bool setString(const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]);

    bool setReal(const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]);

    bool setBoolean(const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]);

    bool setInteger(const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]);


    bool terminate();

    bool setup(fmi2Real startTime);

    bool initialize();

    bool isLoggingOn();

private:

    FmuContainerCore *core;

    date::sys_time<std::chrono::milliseconds> startOffsetTime;
    int communicationTimeout;

    map<string, ModelDescriptionParser::ScalarVariable> nameToValueReference;

    list<DataPoint> data;
    DataPoint currentData;

    RabbitmqHandler *rabbitMqHandler;

   // bool readMessage(DataPoint *dataPoint, int timeout, bool *timeoutOccured);

    std::chrono::milliseconds messageTimeToSim( date::sys_time<std::chrono::milliseconds> messageTime);

    virtual RabbitmqHandler * createCommunicationHandler( const string &hostname, int port, const string& username, const string &password,const string &exchange,const string &queueBindingKey);

    const bool loggingOn;

    unsigned long precision;


    bool initializeCoreState();

};


#endif //RABBITMQ_FMU_FMUCONTAINER_H
