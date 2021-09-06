//
// Created by Kenneth Guldbrandt Lausdahl on 11/12/2019.
//

#include "FmuContainer.h"

#include <utility>
#include <thread>
#include <message/MessageParser.h>
#include "Iso8601TimeParser.h"


#define FmuContainer_LOG(status, category, message, args...)       \
  if (m_functions != NULL) {                                             \
    if (m_functions->logger != NULL) {                                   \
      m_functions->logger(m_functions->componentEnvironment, m_name.c_str(), status, \
                        category, message, args);                      \
    }                                                                  \
  } else {                                                             \
    fprintf(stderr, "Name '%s', Category: '%s'\n", m_name.c_str(), category);    \
    fprintf(stderr, message, args);                                    \
    fprintf(stderr, "\n");                                             \
  }

std::map<FmuContainerCore::ScalarVariableId, int> FmuContainer::calculateLookahead(int lookaheadBound){
    std::map<FmuContainerCore::ScalarVariableId, int> lookahead;

    for (auto &pair: nameToValueReference) {
        if (pair.second.output) {
            lookahead[pair.second.valueReference] = lookaheadBound;
        }
    }
    return lookahead;

}

FmuContainer::FmuContainer(const fmi2CallbackFunctions *mFunctions, bool logginOn, const char *mName,
                           map<string, ModelDescriptionParser::ScalarVariable> nameToValueReference,
                           DataPoint initialDataPoint)
        : m_functions(mFunctions), m_name(mName), nameToValueReference((nameToValueReference)),
          currentData(std::move(initialDataPoint)), rabbitMqHandler(NULL),
          startOffsetTime(floor<milliseconds>(std::chrono::system_clock::now())),
          communicationTimeout(30), loggingOn(logginOn), precision(10), previousInputs(), 
          timeOutputPresent(false), timeOutputVRef(-1), previousTimeOutputVal(0.42), 
          simtimeOutputPresent(false), simtimeOutputVRef(-1), simpreviousTimeOutputVal(0.43){

    auto intConfigs = {std::make_pair(RABBITMQ_FMU_MAX_AGE, "max_age"),
                       std::make_pair(RABBITMQ_FMU_LOOKAHEAD, "lookahead")};


    int maxAgeMs = 0;
    int lookaheadBound = 1;

#ifdef USE_RBMQ_FMU_THREAD
    consumerThreadStop = false;
#endif 
#ifdef USE_RBMQ_FMU_HEALTH_THREAD
    healthThreadStop = false;
#endif 

    for (auto const &value: intConfigs) {
        auto vRef = value.first;
        auto description = value.second;

        if (this->currentData.integerValues.find(vRef) == this->currentData.integerValues.end()) {
            FmuContainer_LOG(fmi2Warning, "logWarn",
                             "Missing parameter. Value reference '%d', Description '%s'.", vRef,
                             description);
        }

    }

}

FmuContainer::~FmuContainer() {
#ifdef USE_RBMQ_FMU_THREAD
    consumerThreadStop = true;
    if (this->consumerThread.joinable())
    {
        this->consumerThread.join();
    }
#endif 

#ifdef USE_RBMQ_FMU_HEALTH_THREAD
    healthThreadStop = true;
    if (this->healthThread.joinable())
    {
        this->healthThread.join();
    }
#endif

    if (this->rabbitMqHandler) {
        this->rabbitMqHandler->close();
        delete this->rabbitMqHandler;
    }

}

bool FmuContainer::isLoggingOn() {
    return this->loggingOn;
}

/*####################################################
 *  Other
 ###################################################*/

bool FmuContainer::setup(fmi2Real startTime) {
    // this->time = startTime;
    return true;
}

#ifdef USE_RBMQ_FMU_THREAD
void FmuContainer::consumerThreadFunc(void) {
    string json;

    while (!consumerThreadStop) {
        if (this->rabbitMqHandler->consume(json)) {
            DataPoint result;
            if (MessageParser::parse(&this->nameToValueReference, json.c_str(), &result)) {
                std::stringstream startTimeStamp;
                startTimeStamp << result.time;
                /* FmuContainer_LOG(fmi2OK, "logOk", "message time to sim time %ld, at simtime", this->core->messageTimeToSim(result.time)); */
                /* FmuContainer_LOG(fmi2OK, "logOk", "Got data '%s', '%s', '%lld'", startTimeStamp.str().c_str(), */
                /*     json.c_str(), std::chrono::high_resolution_clock::now()); */

                std::unique_lock<std::mutex> lock(this->core->m);
                for (auto &pair: result.integerValues) {
                    this->core->add(pair.first, std::make_pair(result.time, pair.second));
                    /* if (pair.first == 10) { */
                    /*     cout << "HE: Got seqno=" << pair.second << endl; */
                    /* } */
                }
                for (auto &pair: result.stringValues) {
                    this->core->add(pair.first, std::make_pair(result.time, pair.second));
                }
                for (auto &pair: result.doubleValues) {
                    this->core->add(pair.first, std::make_pair(result.time, pair.second));
                }
                for (auto &pair: result.booleanValues) {
                    this->core->add(pair.first, std::make_pair(result.time, pair.second));
                }
                lock.unlock();
                FmuContainer_LOG(fmi2OK, "logWarn", "unlocked '%s'", "");
                cv.notify_one();
            } else {
                FmuContainer_LOG(fmi2OK, "logWarn", "Got unknown json '%s'", json.c_str());
            }
        }
    }
}
#endif //USE_RBMQ_FMU_THREAD

#ifdef USE_RBMQ_FMU_HEALTH_THREAD
void FmuContainer::healthThreadFunc(void) {
    string systemHealthData;
    bool tryAgain = true;
    bool validData = false;
    double simTime_d, rTime_d;

    while (!healthThreadStop) {
        if (this->rabbitMqHandlerSystemHealth->consume(systemHealthData)){

            FmuContainer_LOG(fmi2OK, "logAll", "New health message %s", systemHealthData.c_str());
            //Extract rtime value from message
            date::sys_time<std::chrono::milliseconds> simTime, rTime;
            if(MessageParser::parseSystemHealthMessage(simTime, rTime, systemHealthData.c_str())){
                FmuContainerCore::HealthData healthData = std::make_pair(rTime, simTime); 
                std::unique_lock<std::mutex> lock(this->core->mHealth);
                this->core->incomingUnprocessedHealth.push_back(healthData);
                lock.unlock();
            }
            else{
                FmuContainer_LOG(fmi2OK, "logAll", "Ignoring (either bad json or own message): %s", systemHealthData.c_str());
            }
        }
    }
}
#endif //USE_RBMQ_FMU_HEALTH_THREAD

bool FmuContainer::initialize() {
    FmuContainer_LOG(fmi2OK, "logAll", "Preparing initialization. Looking Up configuration parameters%s", "");

    auto stringMap = this->currentData.stringValues;

    auto stringConfigs = {std::make_pair(RABBITMQ_FMU_HOSTNAME_ID, "hostname"),
                          std::make_pair(RABBITMQ_FMU_USER, "username"),
                          std::make_pair(RABBITMQ_FMU_PWD, "password"),
                          std::make_pair(RABBITMQ_FMU_ROUTING_KEY, "routing key"),
                          std::make_pair(RABBITMQ_FMU_EXCHANGE_NAME, "exchangename"),
                          std::make_pair(RABBITMQ_FMU_EXCHANGE_TYPE, "exchangetype")};


    auto allParametersPresent = true;
    for (auto const &value: stringConfigs) {
        auto vRef = value.first;
        auto description = value.second;

        if (stringMap.find(vRef) == stringMap.end()) {
            FmuContainer_LOG(fmi2Fatal, "logError", "Missing parameter. Value reference '%d', Description '%s' ", vRef,
                             description);
            allParametersPresent = false;
        }
    }

    auto intConfigs = {std::make_pair(RABBITMQ_FMU_PORT, "port"),
                       std::make_pair(RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT, "communicationtimeout"),
                       std::make_pair(RABBITMQ_FMU_PRECISION, "precision"),
                       std::make_pair(RABBITMQ_FMU_LOOKAHEAD, "lookahead"),
                       std::make_pair(RABBITMQ_FMU_MAX_AGE, "maxage")};

    int lookaheadBound = 1;
    int maxAgeBound = 0;

    for (auto const &value: intConfigs) {
        auto vRef = value.first;
        auto description = value.second;

       if (vRef == RABBITMQ_FMU_LOOKAHEAD){
            auto v = this->currentData.integerValues[vRef];
                if (v < 1) {
                    FmuContainer_LOG(fmi2Warning, "logWarn",
                                     "Invalid parameter value. Value reference '%d', Description '%s' Value '%d'. Defaulting to %d.",
                                     vRef,
                                     description, v, lookaheadBound);
                    v = lookaheadBound;
                }
                lookaheadBound = v;
        }
        else if (vRef == RABBITMQ_FMU_MAX_AGE) {
            auto v = this->currentData.integerValues[vRef];
            if (v < 0) {
                FmuContainer_LOG(fmi2Warning, "logWarn",
                                    "Invalid parameter value. Value reference '%d', Description '%s' Value '%d'. Defaulting to %d.",
                                    vRef,
                                    description, v, maxAgeBound);
                v = maxAgeBound;
            }
            maxAgeBound = v;
        }
    }

    std::chrono::milliseconds maxAge = std::chrono::milliseconds(maxAgeBound);

    if (!allParametersPresent) {
        return false;
    }

    auto hostname = stringMap[RABBITMQ_FMU_HOSTNAME_ID];
    auto username = stringMap[RABBITMQ_FMU_USER];
    auto password = stringMap[RABBITMQ_FMU_PWD];
    auto routingKey = stringMap[RABBITMQ_FMU_ROUTING_KEY];
    auto exchangeName = stringMap[RABBITMQ_FMU_EXCHANGE_NAME];
    auto exchangeType = stringMap[RABBITMQ_FMU_EXCHANGE_TYPE];

    auto port = this->currentData.integerValues[RABBITMQ_FMU_PORT];
    this->communicationTimeout = this->currentData.integerValues[RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT];
    auto precisionDecimalPlaces = this->currentData.integerValues[RABBITMQ_FMU_PRECISION];

    if (precisionDecimalPlaces < 1) {
        FmuContainer_LOG(fmi2Fatal, "logAll",
                         "Precision must be a positive number %d",
                         precisionDecimalPlaces);
        return false;
    }

    this->precision = std::pow(10, precisionDecimalPlaces);


    FmuContainer_LOG(fmi2OK, "logAll",
                     "Preparing initialization. Hostname='%s', Port='%d', Username='%s', routingkey='%s', communication timeout %d s, precision %lu (%d)",
                     hostname.c_str(), port, username.c_str(), routingKey.c_str(),
                     this->communicationTimeout, this->precision, precisionDecimalPlaces);
    /////////////////////////////////////////////////////////////////////////////////////
    //create a separate connection that deals with publishing to the rabbitmq server/////
    this->rabbitMqHandler = createCommunicationHandler(hostname, port, username, password, exchangeName, exchangeType,
                                                       routingKey);//this routing key does not affect what is defined below.
    FmuContainer_LOG(fmi2OK, "logAll",
                     "rabbitmq publisher connecting to rabbitmq server at '%s:%d'", hostname.c_str(), port);
    try {
        if (!this->rabbitMqHandler->createConnection()) {
            FmuContainer_LOG(fmi2Fatal, "logAll",
                             "Connection failed to rabbitmq server. Please make sure that a rabbitmq server is running at '%s:%d'",
                             hostname.c_str(), port);
            return false;
        }
        this->rabbitMqHandler->createChannel(this->rabbitMqHandler->channelPub, this->rabbitMqHandler->rbmqExchange.first, this->rabbitMqHandler->rbmqExchangetype.first); //Channel where to publish data

        this->rabbitMqHandler->createChannel(this->rabbitMqHandler->channelSub, this->rabbitMqHandler->rbmqExchange.first, this->rabbitMqHandler->rbmqExchangetype.first); //Channel where to consume data

        FmuContainer_LOG(fmi2OK, "logAll",
                             "Routing key data: %s for pub and %s for sub",
                             this->rabbitMqHandler->routingKeyCD.c_str(), this->rabbitMqHandler->bindingKeyCD.c_str());
        //We bind only the queue from which we want to get the data.
        this->rabbitMqHandler->bind(this->rabbitMqHandler->channelSub, this->rabbitMqHandler->bindingKeyCD, this->rabbitMqHandler->rbmqExchange.first);


    } catch (RabbitMqHandlerException &ex) {
        FmuContainer_LOG(fmi2Fatal, "logAll",
                         "Connection failed to rabbitmq server at '%s:%d' with exception '%s'", hostname.c_str(), port,
                         ex.what());
        return false;
    }
    FmuContainer_LOG(fmi2OK, "logAll",
                     "Sending RabbitMQ ready message%s", "");

    this->rabbitMqHandler->publish(this->rabbitMqHandler->routingKeyCD,
                                   R"({"internal_status":"ready", "internal_message":"waiting for input data for simulation"})",this->rabbitMqHandler->channelPub, this->rabbitMqHandler->rbmqExchange.first);
    ////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    //create a separate connection that deals with publishing and consuming system health data, using a channel for eavh/////
    this->rabbitMqHandlerSystemHealth = createCommunicationHandler(hostname, port, username, password,  exchangeName,  exchangeType,
                                                       routingKey);
    FmuContainer_LOG(fmi2OK, "logAll",
                     "Another rabbitmq publisher connecting to rabbitmq server at '%s:%d'", hostname.c_str(), port);
    try {
        if (!this->rabbitMqHandlerSystemHealth->createConnection()) {
            FmuContainer_LOG(fmi2Fatal, "logAll",
                             "Connection failed to rabbitmq server. Please make sure that a rabbitmq server is running at '%s:%d'",
                             hostname.c_str(), port);
            return false;
        }
        FmuContainer_LOG(fmi2OK, "logAll",
                             "Routing key data: %s for pub and %s for sub",
                             this->rabbitMqHandlerSystemHealth->routingKeySH.c_str(), this->rabbitMqHandlerSystemHealth->bindingKeySH.c_str());
        //Create channel for handling the publishing
        this->rabbitMqHandlerSystemHealth->createChannel(this->rabbitMqHandlerSystemHealth->channelPub); //Channel where to publish system health data
        //Declare exchange
        this->rabbitMqHandlerSystemHealth->declareExchange(this->rabbitMqHandlerSystemHealth->channelPub, this->rabbitMqHandlerSystemHealth->rbmqExchange.second, this->rabbitMqHandlerSystemHealth->rbmqExchangetype.second);

        //Create channel for handling the consuming
        this->rabbitMqHandlerSystemHealth->createChannel(this->rabbitMqHandlerSystemHealth->channelSub); //Channel where to consume system health data 
        //we bind only the consume queue
        this->rabbitMqHandlerSystemHealth->bind(this->rabbitMqHandlerSystemHealth->channelSub, this->rabbitMqHandlerSystemHealth->bindingKeySH, this->rabbitMqHandlerSystemHealth->rbmqExchange.second);


    } catch (RabbitMqHandlerException &ex) {
        FmuContainer_LOG(fmi2Fatal, "logAll",
                         "Connection failed to rabbitmq server at '%s:%d' with exception '%s'", hostname.c_str(), port,
                         ex.what());
        return false;
    }
    ////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////

    //Initialise previousInputs and check whether the time_discrepancy is given
    for(auto it = this->nameToValueReference.cbegin(); it != this-> nameToValueReference.cend(); it++){
        if(it->second.input){
            //Init value 
            //cout << "Input type: " << it->second.type << endl;
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Real){
                this->previousInputs.doubleValues.insert(pair<unsigned int, double>(it->second.valueReference, it->second.d_value));
            }
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Boolean){
                this->previousInputs.booleanValues.insert(pair<unsigned int, bool>(it->second.valueReference, it->second.b_value));
            }
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Integer){
                this->previousInputs.integerValues.insert(pair<unsigned int, int>(it->second.valueReference, it->second.i_value));
            }
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::String){
                this->previousInputs.stringValues.insert(pair<unsigned int, string>(it->second.valueReference, it->second.s_value));
            }
        }
        else if(it->second.output && it->first.compare("time_discrepancy")==0){
            FmuContainer_LOG(fmi2OK, "logAll","time discrepancy present with vref: %d s",it->second.valueReference);
            this->timeOutputPresent = true;
            this->timeOutputVRef = it->second.valueReference;
        }
        else if(it->second.output && it->first.compare("simtime_discrepancy")==0){
            FmuContainer_LOG(fmi2OK, "logAll","simtime discrepancy present with vref: %d s",it->second.valueReference);
            this->simtimeOutputPresent = true;
            this->simtimeOutputVRef = it->second.valueReference;
        }
    }
    //Create container core
    //with core logging
    this->core = new FmuContainerCore(maxAge, calculateLookahead(lookaheadBound), this->m_functions, this->m_name.c_str());
    //without core logging
    //this->core = new FmuContainerCore(maxAge, calculateLookahead(lookaheadBound));

    //this->core->setVerbose(true);

    if (!initializeCoreState()) {
        FmuContainer_LOG(fmi2Fatal, "logError", "Initialization failed%s", "");
        return false;
    }

#ifdef USE_RBMQ_FMU_THREAD
    this->consumerThread = std::thread(&FmuContainer::consumerThreadFunc, this);
#endif //USE_RBMQ_FMU_THREAD

#ifdef USE_RBMQ_FMU_HEALTH_THREAD
    this->healthThread = std::thread(&FmuContainer::healthThreadFunc, this);
#endif //USE_RBMQ_FMU_HEALTH_THREAD

    std::stringstream startTimeStamp;
    startTimeStamp << this->core->getStartOffsetTime();

    FmuContainer_LOG(fmi2OK, "logAll",
                     "Initialization completed with: Hostname='%s', Port='%d', Username='%s', routingkey='%s', starttimestamp='%s', communication timeout %d s",
                     hostname.c_str(), port, username.c_str(), routingKey.c_str(), startTimeStamp.str().c_str(),
                     this->communicationTimeout);


    return true;
}

RabbitmqHandler *FmuContainer::createCommunicationHandler(const string &hostname, int port, const string &username,
                                                          const string &password, const string &exchange,const string &exchangetype,
                                                          const string &queueBindingKey) {
    return new RabbitmqHandler(hostname, port, username, password, exchange, exchangetype, queueBindingKey);
}


bool FmuContainer::initializeCoreState() {


    auto start = std::chrono::system_clock::now();
    try {

        string json;
        while (((std::chrono::duration<double>) (std::chrono::system_clock::now() - start)).count() <
               this->communicationTimeout) {

            if (this->rabbitMqHandler->consume(json)) {
                //data received
                DataPoint result;
                if (MessageParser::parse(&this->nameToValueReference, json.c_str(), &result)) {

                    std::stringstream startTimeStamp;
                    startTimeStamp << result.time;

                    /* FmuContainer_LOG(fmi2OK, "logOk", "Got data '%s', '%s'", startTimeStamp.str().c_str(), */
                    /*                  json.c_str()); */

                    //propagate new data to core
                    this->addToCore(result);

                    if (this->core->initialize()) {

                        FmuContainer_LOG(fmi2OK, "logOk", "Initialization OK%s", "");

//                        cout << "Initialized" << endl;
//                        cout << *this->core;

                        return true;
                    }
                } else {
                    FmuContainer_LOG(fmi2OK, "logWarn", "Got unknown json '%s'", json.c_str());
                }
            }

        }

    } catch (exception &e) {
        FmuContainer_LOG(fmi2Fatal, "logFatal", "Read message exception '%s'", e.what());
        throw e;
    }

    return false;

}


bool FmuContainer::terminate() { return true; }

#define secondsToMs(value) ((value)*1000.0)

std::chrono::milliseconds FmuContainer::messageTimeToSim(date::sys_time<std::chrono::milliseconds> messageTime) {
    return (messageTime - this->startOffsetTime);
}

fmi2ComponentEnvironment FmuContainer::getComponentEnvironment() { return (fmi2ComponentEnvironment) this; }

#ifdef USE_RBMQ_FMU_PROF
#define LOG_TIME_SIZE 6
std::chrono::high_resolution_clock::time_point log_time[LOG_TIME_SIZE];
std::chrono::high_resolution_clock::time_point log_time_last;
#define LOG_TIME(x) \
    log_time[x] = std::chrono::high_resolution_clock::now()
#define LOG_TIME_ELAPSED(x, y) \
    std::chrono::duration_cast<std::chrono::microseconds>(log_time[y] - log_time[x]).count()
#define LOG_TIME_TOTAL \
    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - log_time[0]).count() 
#define LOG_TIME_PRINT \
    FmuContainer_LOG(fmi2OK, "logAll",  "HE: 0:%lld, 1:+%lld, 2:+%lld, 3:+%lld, 4:+%lld, 5:+%lld\n", \
         std::chrono::duration_cast<std::chrono::microseconds>(log_time[0] - log_time_last).count(), \
         LOG_TIME_ELAPSED(0,1), \
         LOG_TIME_ELAPSED(1,2), \
         LOG_TIME_ELAPSED(2,3), \
         LOG_TIME_ELAPSED(3,4), \
         LOG_TIME_ELAPSED(4,5))
#else
#define LOG_TIME(x)
#define LOG_TIME_ELAPSED(x,y) 
#define LOG_TIME_TOTAL
#define LOG_TIME_PRINT
#endif //USE_RBMQ_FMU_PROF

bool FmuContainer::step(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    #ifdef USE_RBMQ_FMU_PROF
    log_time_last = log_time[0];
    #endif //USE_RBMQ_FMU_PROF
    LOG_TIME(0);

    auto simulationTime = secondsToMs(currentCommunicationPoint + communicationStepSize);

    FmuContainer_LOG(fmi2OK, "logAll", "************ Enter FmuContainer::step ***************%s", "");
    FmuContainer_LOG(fmi2OK, "logAll", "Step time %f s converted time %f ms", currentCommunicationPoint + communicationStepSize, simulationTime);

    simulationTime = std::round(simulationTime * precision) / precision;
    long long int milliSecondsSinceEpoch = this->core->simTimeToReal((long long) simulationTime).count(); // this is your starting point
    string cosim_time;
    this->core->convertTimeToString(milliSecondsSinceEpoch, cosim_time);
    cosim_time = R"({"simAtTime":")" + cosim_time + R"("})";
    FmuContainer_LOG(fmi2OK, "logAll", "Sending to rabbitmq: COSIM TIME: %s", cosim_time.c_str());


    //FmuContainer_LOG(fmi2OK, "logAll", "Real time in [ms] %.0f, and formatted %s", milliSecondsSinceEpoch, cosim_time.c_str());
    this->rabbitMqHandlerSystemHealth->publish(this->rabbitMqHandlerSystemHealth->routingKeySH, cosim_time, 
                                        this->rabbitMqHandlerSystemHealth->channelPub, this->rabbitMqHandlerSystemHealth->rbmqExchange.second);
//    cout << *this->core;
//    cout << "Step " << simulationTime << "\n";

//    std::ostringstream stream;
//    stream << *this->core;
//    std::string str = stream.str();
//    const char *chr = str.c_str();
//    FmuContainer_LOG(fmi2OK, "logAll", "Step reached target time %.0f [ms]: %s", simulationTime, chr);
//    cout << "Checking with existing messages\n";
#ifndef USE_RBMQ_FMU_THREAD
    if (this->core->process(simulationTime)) {
        FmuContainer_LOG(fmi2OK, "logAll", "Step reached target time %.0f [ms]", simulationTime);

        FmuContainer_LOG(fmi2OK, "logAll", "************ Exit 1 FmuContainer::step ***************%s", "");
        LOG_TIME(1); LOG_TIME(2); LOG_TIME(3); LOG_TIME(4); LOG_TIME(5); 
        //get time now here, and get difference between time now - log_time(0)
        LOG_TIME_PRINT;
#ifdef USE_RBMQ_FMU_PROF
        FmuContainer_LOG(fmi2OK, "logAll", "simtime_stepdur %.0f,%lld", simulationTime, LOG_TIME_TOTAL);
#endif

        return true;
    }
#endif //!USE_RBMQ_FMU_THREAD

    LOG_TIME(1);
    if (!this->rabbitMqHandler) {
        FmuContainer_LOG(fmi2Fatal, "logAll", "Rabbitmq handle not initialized%s", "");
        return false;
    }
    if(this->timeOutputPresent){
        this->previousTimeOutputVal = this->core->getTimeDiscrepancyOutput(this->timeOutputVRef);
    }
    if(this->simtimeOutputPresent){
        this->simpreviousTimeOutputVal = this->core->getTimeDiscrepancyOutput(this->simtimeOutputVRef);
    }
//    cout << "Checking with new messages\n";
    auto start = std::chrono::system_clock::now();
    try {

        string json;
        while (((std::chrono::duration<double>) (std::chrono::system_clock::now() - start)).count() <
               this->communicationTimeout) {
#ifndef USE_RBMQ_FMU_THREAD
            // Consume a message directly from socket and add to core
            bool msgAddSuccess = false;
            if (this->rabbitMqHandler->consume(json)) {
                //data received
                LOG_TIME(2);
                DataPoint result;
                if (MessageParser::parse(&this->nameToValueReference, json.c_str(), &result)) {
                    //std::stringstream startTimeStamp;
                    //startTimeStamp << result.time;
                    //FmuContainer_LOG(fmi2OK, "logOk", "Got data '%s', '%s'", startTimeStamp.str().c_str(), json.c_str());

                    //update core with the new data
                    this->addToCore(result);
                    msgAddSuccess = true;
                } else {
                    FmuContainer_LOG(fmi2OK, "logWarn", "Got unknown json '%s'", json.c_str());
                }
            } 
            // If failing consuming or parsing a message, then continue the while loop and try again
            if (!msgAddSuccess) {
                continue;
            }
#else
            // Wait for signal from consumer thread that core has messages
            std::unique_lock<std::mutex> lock(this->core->m);
            FmuContainer_LOG(fmi2OK, "logOk", "locked'%s'", "");
            cv.wait(lock, [this] {return this->core->hasUnprocessed();});
            lock.unlock();
#endif //!USE_RBMQ_FMU_THREAD
            LOG_TIME(2);

            //Check which of the inputs of the fmu has changed since the last step
            string message;
            this->checkInputs(message);

            //if anything to send, publish to rabbitmq
            if(!message.empty()){
                message = R"({)" + message + R"("timestep":")" + cosim_time + R"("})";
                this->rabbitMqHandler->publish(this->rabbitMqHandler->routingKeyCD, message, this->rabbitMqHandler->channelPub, this->rabbitMqHandler->rbmqExchange.first);
                FmuContainer_LOG(fmi2OK, "logAll", "This is the message sent to rabbitmq: %s", message.c_str());
            }

            LOG_TIME(3);

            string systemHealthData;
            bool tryAgain = true;
            bool validHealthData = false;
            double simTime_d, rTime_d;

#ifndef USE_RBMQ_FMU_HEALTH_THREAD
            if (this->rabbitMqHandlerSystemHealth->consume(systemHealthData)){
                FmuContainer_LOG(fmi2OK, "logAll", "At sim-time: %f [ms], received system health data: %s. \nIf output exists, it will be set.", simulationTime, systemHealthData.c_str());
                //Extract rtime value from message
                date::sys_time<std::chrono::milliseconds> simTime, rTime;
                if(MessageParser::parseSystemHealthMessage(simTime, rTime, systemHealthData.c_str())){
                    validHealthData = true;
                    rTime_d = this->core->message2SimTime(rTime).count();
                    simTime_d = this->core->message2SimTime(simTime).count();
                }
            }
#else
            FmuContainerCore::HealthData healthData;
            std::unique_lock<std::mutex> hlock(this->core->mHealth);
            if (this->core->hasUnprocessedHealth()) {
                validHealthData = true;
                healthData = this->core->incomingUnprocessedHealth.front();
                rTime_d = this->core->message2SimTime(healthData.first).count();
                simTime_d = this->core->message2SimTime(healthData.second).count();
                this->core->incomingUnprocessedHealth.pop_front();
            }
            hlock.unlock();
#endif //!USE_RBMQ_FMU_HEALTH_THREAD

            if (validHealthData) {
                FmuContainer_LOG(fmi2OK, "logAll", "NOTE: Difference in time between current simulation step, and received simulation step %.2f [ms]", abs(simulationTime-simTime_d));
                //If output time_discrepancy present, set its value
                if(rTime_d < simTime_d){
                    FmuContainer_LOG(fmi2OK, "logWarn", "Co-sim ahead in time by %f [ms]", simTime_d-rTime_d);
                }
                else if (rTime_d > simTime_d){
                    FmuContainer_LOG(fmi2OK, "logWarn", "Co-sim behind in time by %f [ms]", rTime_d-simTime_d);
                }
            } else {
                FmuContainer_LOG(fmi2OK, "logAll", "Ignoring (either bad json or own message): %s", systemHealthData.c_str());
            }

            LOG_TIME(4);
            if (this->core->process(simulationTime)) {    
                if(this->timeOutputPresent){     
                    this->core->setTimeDiscrepancyOutput(validHealthData, simTime_d-rTime_d, this->previousTimeOutputVal, this->timeOutputVRef);
                } 
                if(this->simtimeOutputPresent){     
                    this->core->setTimeDiscrepancyOutput(validHealthData, abs(simulationTime-simTime_d), this->simpreviousTimeOutputVal, this->simtimeOutputVRef);
                } 
                FmuContainer_LOG(fmi2OK, "logAll", "Step reached target time %.0f [ms]", simulationTime);
                FmuContainer_LOG(fmi2OK, "logAll", "Current data point seqno %d, %ld", this->core->getSeqNO(103), std::chrono::high_resolution_clock::now());

                LOG_TIME(5);
                LOG_TIME_PRINT;
#ifdef USE_RBMQ_FMU_PROF
                FmuContainer_LOG(fmi2OK, "logAll", "simtime_stepdur %.0f,%lld", simulationTime, LOG_TIME_TOTAL);
#endif
                FmuContainer_LOG(fmi2OK, "logAll", "************ Exit 2 FmuContainer::step ***************%s", "");
                return true;
            }
        }
    } catch (exception &e) {
        FmuContainer_LOG(fmi2Fatal, "logFatal", "Read message exception '%s'", e.what());
        return false;
    }
    FmuContainer_LOG(fmi2Fatal, "logError", "Did not get data to proceed to time '%f'", simulationTime);
    return false;
}

//Check if there is a a change of the inputs between two consequent timesteps
void FmuContainer::checkInputs(string &message){
    for(auto it = this->nameToValueReference.cbegin(); it != this-> nameToValueReference.cend(); it++){
        ostringstream val;
        if(it->second.input){
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Real){
                if(this->currentData.doubleValues[it->second.valueReference] != this->previousInputs.doubleValues[it->second.valueReference]){
                    double previous, current;
                    previous = this->previousInputs.doubleValues[it->second.valueReference];
                    current = this->currentData.doubleValues[it->second.valueReference];
                    FmuContainer_LOG(fmi2OK, "logAll", "INPUT has changed: current data: %f, previous data: %f", current, previous);

                    val << this->currentData.doubleValues[it->second.valueReference];
                    this->core->messageCompose(pair<string, string>(it->second.name, val.str()), message);
                    //Update previous to current value
                    this->previousInputs.doubleValues[it->second.valueReference] = this->currentData.doubleValues[it->second.valueReference];
                }
            }
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Boolean){
                if(this->currentData.booleanValues[it->second.valueReference] != this->previousInputs.booleanValues[it->second.valueReference]){
                    
                    bool previous, current;
                    previous = this->previousInputs.booleanValues[it->second.valueReference];
                    current = this->currentData.booleanValues[it->second.valueReference];
                    FmuContainer_LOG(fmi2OK, "logAll", "INPUT has changed: current data: %d, previous data: %d", current, previous);
                    val << this->currentData.booleanValues[it->second.valueReference];
                    this->core->messageCompose(pair<string, string>(it->second.name, val.str()), message);
                    //Update previous to current value
                    this->previousInputs.booleanValues[it->second.valueReference] = this->currentData.booleanValues[it->second.valueReference];
                }
            }
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Integer){
                if(this->currentData.integerValues[it->second.valueReference] != this->previousInputs.integerValues[it->second.valueReference]){
                    int previous, current;
                    previous = this->previousInputs.integerValues[it->second.valueReference];
                    current = this->currentData.integerValues[it->second.valueReference];
                    FmuContainer_LOG(fmi2OK, "logAll", "INPUT has changed: current data: %d, previous data: %d", current, previous);
                    val << this->currentData.integerValues[it->second.valueReference];
                    this->core->messageCompose(pair<string, string>(it->second.name, val.str()), message);
                    //Update previous to current value
                    this->previousInputs.integerValues[it->second.valueReference] = this->currentData.integerValues[it->second.valueReference];
                }
            }
            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::String){
                if(this->currentData.stringValues[it->second.valueReference] != this->previousInputs.stringValues[it->second.valueReference]){
                    string previous, current;
                    previous = this->previousInputs.stringValues[it->second.valueReference];
                    current = this->currentData.stringValues[it->second.valueReference];
                    FmuContainer_LOG(fmi2OK, "logAll", "INPUT has changed: current data: %s, previous data: %s", current.c_str(), previous.c_str());
                    string str = "\"";
                    str.append(this->currentData.stringValues[it->second.valueReference]);
                    str.append("\"");
                    this->core->messageCompose(pair<string, string>(it->second.name, str), message);
                    //Update previous to current value
                    this->previousInputs.stringValues[it->second.valueReference] = this->currentData.stringValues[it->second.valueReference];
                }
            }
        }
    }      
}

void FmuContainer::addToCore(DataPoint result){
    for (auto &pair: result.integerValues) {
        this->core->add(pair.first, std::make_pair(result.time, pair.second));
    }
    for (auto &pair: result.stringValues) {
        this->core->add(pair.first, std::make_pair(result.time, pair.second));
    }
    for (auto &pair: result.doubleValues) {
        this->core->add(pair.first, std::make_pair(result.time, pair.second));
    }
    for (auto &pair: result.booleanValues) {
        this->core->add(pair.first, std::make_pair(result.time, pair.second));
    }
}
/*####################################################
 *  Custom
 ###################################################*/

bool FmuContainer::fmi2GetMaxStepsize(fmi2Real *size) {
    if (!this->data.empty()) {

        auto f = messageTimeToSim(this->data.front().time);
        *size = f.count() / 1000.0;
        return true;
    }

    return false;
}

/*####################################################
 *  GET
 ###################################################*/

bool FmuContainer::getBoolean(const fmi2ValueReference *vr, size_t nvr, fmi2Boolean *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            value[i] = this->core->getData().at(vr[i]).second.b.b;
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "getBoolean Out of Range error: " << oor.what() << '\n';
        return false;
    }
}

bool FmuContainer::getInteger(const fmi2ValueReference *vr, size_t nvr, fmi2Integer *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            value[i] = this->core->getData().at(vr[i]).second.i.i;
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "getInteger Out of Range error: " << oor.what() << '\n';
        return false;
    }
}

bool FmuContainer::getReal(const fmi2ValueReference *vr, size_t nvr, fmi2Real *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            value[i] = this->core->getData().at(vr[i]).second.d.d;
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "getReal Out of Range error: " << oor.what() << '\n';
        return false;
    }
}

bool FmuContainer::getString(const fmi2ValueReference *vr, size_t nvr, fmi2String *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            value[i] = this->core->getData().at(vr[i]).second.s.s.c_str();
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "getString Out of Range error: " << oor.what() << '\n';
        return false;
    }
}


/*####################################################
 *  SET
 ###################################################*/

bool FmuContainer::setBoolean(const fmi2ValueReference *vr, size_t nvr, const fmi2Boolean *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            FmuContainer_LOG(fmi2OK, "logAll", "Setting boolean ref %d = %d", vr[i], value[i]);
            this->currentData.booleanValues[vr[i]] = value[i];
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        return false;
    }
}

bool FmuContainer::setInteger(const fmi2ValueReference *vr, size_t nvr, const fmi2Integer *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            FmuContainer_LOG(fmi2OK, "logAll", "Setting integer ref %d = %d", vr[i], value[i]);
            this->currentData.integerValues[vr[i]] = value[i];
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        return false;
    }
}

bool FmuContainer::setReal(const fmi2ValueReference *vr, size_t nvr, const fmi2Real *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            FmuContainer_LOG(fmi2OK, "logAll", "Setting real ref %d = %f", vr[i], value[i]);
            this->currentData.doubleValues[vr[i]] = value[i];
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        return false;
    }
}

bool FmuContainer::setString(const fmi2ValueReference *vr, size_t nvr, const fmi2String *value) {
    try {
        for (int i = 0; i < nvr; i++) {
            FmuContainer_LOG(fmi2OK, "logAll", "Setting string ref %d = %s", vr[i], value[i]);
            this->currentData.stringValues[vr[i]] = string(value[i]);
        }

        return true;
    } catch (const std::out_of_range &oor) {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        return false;
    }
}

int FmuContainer::coreIncomingSize(void){
    return this->core->incomingSize();
}

