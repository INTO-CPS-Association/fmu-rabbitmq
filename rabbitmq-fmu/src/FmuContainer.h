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
#include <thread>
#include "fmi2Functions.h"
#include "DataPoint.h"
#include "modeldescription/ModelDescriptionParser.h"
#include <list>
#include <iterator>
#include "rabbitmq/RabbitmqHandler.h"
#include "Iso8601TimeParser.h"
#include "FmuContainerCore.h"
#include <condition_variable>

#define RABBITMQ_FMU_HOSTNAME_ID 0
#define RABBITMQ_FMU_PORT 1
#define RABBITMQ_FMU_USER 2
#define RABBITMQ_FMU_PWD 3
#define RABBITMQ_FMU_ROUTING_KEY 4
#define RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT 5
#define RABBITMQ_FMU_PRECISION 6
#define RABBITMQ_FMU_MAX_AGE 7
#define RABBITMQ_FMU_LOOKAHEAD 8
#define RABBITMQ_FMU_EXCHANGE_NAME 9
#define RABBITMQ_FMU_EXCHANGE_TYPE 10
#define RABBITMQ_FMU_SH_EXCHANGE_NAME 11
#define RABBITMQ_FMU_SH_EXCHANGE_TYPE 12
#define RABBITMQ_FMU_ROUTING_KEY_FROM_COSIM 13
#define RABBITMQ_FMU_SEQNO_OUTPUT 14
#define RABBITMQ_FMU_ENABLE_SEND_INPUT 15


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

    int coreIncomingSize();

private:

    bool seqnoPresent;
    bool sendEnablePresent;
    
    FmuContainerCore *core;

    date::sys_time<std::chrono::milliseconds> startOffsetTime;
    int communicationTimeout;

    map<string, ModelDescriptionParser::ScalarVariable> nameToValueReference;

    list<DataPoint> data;
    DataPoint currentData;
    DataPoint previousInputs;
    enum SvType{Real,Integer,Boolean,String};

    //this connection is for exchanging data regarding actual state content, e.g., robot data
    RabbitmqHandler *rabbitMqHandler;
    //this connection is for exchanging data regarding system health
    RabbitmqHandler *rabbitMqHandlerSystemHealth;

   // bool readMessage(DataPoint *dataPoint, int timeout, bool *timeoutOccured);

    std::chrono::milliseconds messageTimeToSim( date::sys_time<std::chrono::milliseconds> messageTime);

    void checkInputs(string &message);

    void addToCore(DataPoint result);

    virtual RabbitmqHandler * createCommunicationHandler( const string &hostname, int port, const string& username, const string &password,
    const string &exchange,const string &exchangetype,const string &queueBindingKey, const string &queueBindingKey_from_cosim);

    const bool loggingOn;

    unsigned long precision;

    bool timeOutputPresent;
    int timeOutputVRef;
    bool simtimeOutputPresent;
    int simtimeOutputVRef;
    double previousTimeOutputVal;
    double simpreviousTimeOutputVal;


    bool initializeCoreState();

    map<FmuContainerCore::ScalarVariableId, int> calculateLookahead(int lookaheadBound);

#ifdef USE_RBMQ_FMU_THREAD
    void consumerThreadFunc();
    thread consumerThread;
    bool consumerThreadStop;
    std::condition_variable cv;
#endif

#ifdef USE_RBMQ_FMU_HEALTH_THREAD
    void healthThreadFunc();
    thread healthThread;
    bool healthThreadStop;
#endif
};


#endif //RABBITMQ_FMU_FMUCONTAINER_H
