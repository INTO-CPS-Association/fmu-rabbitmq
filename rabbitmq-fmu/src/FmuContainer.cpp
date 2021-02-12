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
          routingKey(), routingKeySystemHealth(), timeOutputPresent(false), timeOutputVRef(-1), previousTimeOutputVal(0.42), 
          simtimeOutputPresent(false), simtimeOutputVRef(-1), simpreviousTimeOutputVal(0.43), channelPub(1), channelSub(2){

    auto intConfigs = {std::make_pair(RABBITMQ_FMU_MAX_AGE, "max_age"),
                       std::make_pair(RABBITMQ_FMU_LOOKAHEAD, "lookahead")};


    int maxAgeMs = 0;
    int lookaheadBound = 1;

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

bool FmuContainer::initialize() {
    FmuContainer_LOG(fmi2OK, "logAll", "Preparing initialization. Looking Up configuration parameters%s", "");

    auto stringMap = this->currentData.stringValues;

    auto stringConfigs = {std::make_pair(RABBITMQ_FMU_HOSTNAME_ID, "hostname"),
                          std::make_pair(RABBITMQ_FMU_USER, "username"),
                          std::make_pair(RABBITMQ_FMU_PWD, "password"),
                          std::make_pair(RABBITMQ_FMU_ROUTING_KEY, "routing key")};


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

    /*
    this->rabbitMqHandler = createCommunicationHandler(hostname, port, username, password, "fmi_digital_twin",
                                                       routingKey);

    FmuContainer_LOG(fmi2OK, "logAll",
                     "Connecting to rabbitmq server at '%s:%d'", hostname.c_str(), port);

    try {
        if (!this->rabbitMqHandler->open()) {
            FmuContainer_LOG(fmi2Fatal, "logAll",
                             "Connection failed to rabbitmq server. Please make sure that a rabbitmq server is running at '%s:%d'",
                             hostname.c_str(), port);
            return false;
        }

        this->rabbitMqHandler->bind();

    } catch (RabbitMqHandlerException &ex) {
        FmuContainer_LOG(fmi2Fatal, "logAll",
                         "Connection failed to rabbitmq server at '%s:%d' with exception '%s'", hostname.c_str(), port,
                         ex.what());
        return false;
    }

    FmuContainer_LOG(fmi2OK, "logAll",
                     "Sending RabbitMQ ready message%s", "");
    this->rabbitMqHandler->publish(routingKey,
                                   R"({"internal_status":"ready", "internal_message":"waiting for input data for simulation"})");
    */
    /////////////////////////////////////////////////////////////////////////////////////
    //create a separate connection that deals with publishing to the rabbitmq server/////
    this->rabbitMqHandler = createCommunicationHandler(hostname, port, username, password, "fmi_digital_twin",
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
        
        this->exchange.first = "fmi_digital_twin_cd";
        this->exchangetype.first = "direct";
        this->rabbitMqHandler->createChannel(this->channelPub, this->exchange.first, this->exchangetype.first); //Channel where to publish data
        this->rabbitMqHandler->createChannel(this->channelSub, this->exchange.first, this->exchangetype.first); //Channel where to consume data
        this->routingKey.first = routingKey;
        this->routingKey.first.append(".data.from_cosim");
        this->routingKey.second = routingKey;
        this->routingKey.second.append(".data.to_cosim");
        cout << "Routing key data: " << this->routingKey.first << " and " << this->routingKey.second << endl;
        this->rabbitMqHandler->bind(this->channelPub, this->routingKey.first, this->queuenameContentData.first, this->exchange.first);
        this->rabbitMqHandler->bind(this->channelSub, this->routingKey.second, this->queuenameContentData.second, this->exchange.first);
        cout << "Queuenames: " << std::string(reinterpret_cast< char const * >(this->queuenameContentData.first.bytes), this->queuenameContentData.first.len).c_str() << " and " << std::string(reinterpret_cast< char const * >(this->queuenameContentData.second.bytes), this->queuenameContentData.second.len).c_str() << endl;


    } catch (RabbitMqHandlerException &ex) {
        FmuContainer_LOG(fmi2Fatal, "logAll",
                         "Connection failed to rabbitmq server at '%s:%d' with exception '%s'", hostname.c_str(), port,
                         ex.what());
        return false;
    }
    FmuContainer_LOG(fmi2OK, "logAll",
                     "Sending RabbitMQ ready message%s", "");
    this->rabbitMqHandler->publish(this->routingKey.second,
                                   R"({"internal_status":"ready", "internal_message":"waiting for input data for simulation"})",this->channelSub, this->exchange.first);
    ////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    //create a separate connection that deals with publishing and consuming system health data, using a channel for eavh/////
    this->rabbitMqHandlerSystemHealth = createCommunicationHandler(hostname, port, username, password, "fmi_digital_twin",
                                                       "system_health");//this binding key is not really used in this context
    FmuContainer_LOG(fmi2OK, "logAll",
                     "Another rabbitmq publisher connecting to rabbitmq server at '%s:%d'", hostname.c_str(), port);
    try {
        if (!this->rabbitMqHandlerSystemHealth->createConnection()) {
            FmuContainer_LOG(fmi2Fatal, "logAll",
                             "Connection failed to rabbitmq server. Please make sure that a rabbitmq server is running at '%s:%d'",
                             hostname.c_str(), port);
            return false;
        }
        //Create channel for handling the publishing
        this->rabbitMqHandlerSystemHealth->createChannel(this->rabbitMqHandlerSystemHealth->channelPub); //Channel where to publish system health data
        //Declare exchange
        this->rabbitMqHandlerSystemHealth->declareExchange(this->rabbitMqHandlerSystemHealth->channelPub, this->rabbitMqHandlerSystemHealth->exchangeSH, this->rabbitMqHandlerSystemHealth->exchangetypeSH);
        this->rabbitMqHandlerSystemHealth->routingKeySH = routingKey;
        this->rabbitMqHandlerSystemHealth->routingKeySH.append(".system_health.from_cosim");
        this->rabbitMqHandlerSystemHealth->bindingKeySH = routingKey;
        this->rabbitMqHandlerSystemHealth->bindingKeySH.append(".system_health.to_cosim");

        //Create channel for handling the consuming
        this->rabbitMqHandlerSystemHealth->createChannel(this->rabbitMqHandlerSystemHealth->channelSub); //Channel where to consume system health data 
        //Declare queue from which to consume
        //this->rabbitMqHandlerSystemHealth->queue_declare(this->rabbitMqHandlerSystemHealth->channelSub, this->rabbitMqHandlerSystemHealth->queuenameSH.c_str());
        //this->rabbitMqHandlerSystemHealth->bind(this->rabbitMqHandlerSystemHealth->channelPub, this->rabbitMqHandlerSystemHealth->routingKeySH, this->rabbitMqHandlerSystemHealth->exchangeSH);

        this->rabbitMqHandlerSystemHealth->bind(this->rabbitMqHandlerSystemHealth->channelSub, this->rabbitMqHandlerSystemHealth->bindingKeySH, this->rabbitMqHandlerSystemHealth->exchangeSH);


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
            cout << "time discrepancy presence: " <<  it->first << " with vref: " << it->second.valueReference << endl;
            FmuContainer_LOG(fmi2OK, "logAll","time discrepancy present with vref: %d s",it->second.valueReference);
            this->timeOutputPresent = true;
            this->timeOutputVRef = it->second.valueReference;
        }
        else if(it->second.output && it->first.compare("simtime_discrepancy")==0){
            cout << "simtime discrepancy presence: " <<  it->first << " with vref: " << it->second.valueReference << endl;
            FmuContainer_LOG(fmi2OK, "logAll","simtime discrepancy present with vref: %d s",it->second.valueReference);
            this->simtimeOutputPresent = true;
            this->simtimeOutputVRef = it->second.valueReference;
        }
    }
    //Create container core
    this->core = new FmuContainerCore(maxAge, calculateLookahead(lookaheadBound));
    //this->core->setVerbose(true);

    if (!initializeCoreState()) {
        FmuContainer_LOG(fmi2Fatal, "logError", "Initialization failed%s", "");
        return false;
    }

    std::stringstream startTimeStamp;
    startTimeStamp << this->core->getStartOffsetTime();

    FmuContainer_LOG(fmi2OK, "logAll",
                     "Initialization completed with: Hostname='%s', Port='%d', Username='%s', routingkey='%s', starttimestamp='%s', communication timeout %d s",
                     hostname.c_str(), port, username.c_str(), routingKey.c_str(), startTimeStamp.str().c_str(),
                     this->communicationTimeout);


    return true;
}

RabbitmqHandler *FmuContainer::createCommunicationHandler(const string &hostname, int port, const string &username,
                                                          const string &password, const string &exchange,
                                                          const string &queueBindingKey) {
    return new RabbitmqHandler(hostname, port, username, password, exchange, queueBindingKey);
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

                    FmuContainer_LOG(fmi2OK, "logOk", "Got data '%s', '%s'", startTimeStamp.str().c_str(),
                                     json.c_str());

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


bool FmuContainer::step(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    auto simulationTime = secondsToMs(currentCommunicationPoint + communicationStepSize);
//    cout << "Step time " << currentCommunicationPoint + communicationStepSize << " s converted time " << simulationTime
//         << " ms" << endl;

    simulationTime = std::round(simulationTime * precision) / precision;
    long long int milliSecondsSinceEpoch = this->core->simTimeToReal((long long) simulationTime).count(); // this is your starting point
    string cosim_time;
    this->core->convertTimeToString(milliSecondsSinceEpoch, cosim_time);
    cosim_time = R"({"simAtTime":")" + cosim_time + R"("})";
    cout << "Sending to rabbitmq: COSIM TIME:\n" << cosim_time << endl;

    //FmuContainer_LOG(fmi2OK, "logAll", "Real time in [ms] %.0f, and formatted %s", milliSecondsSinceEpoch, cosim_time.c_str());
    this->rabbitMqHandlerSystemHealth->publish(this->rabbitMqHandlerSystemHealth->routingKeySH, cosim_time, 
                                        this->rabbitMqHandlerSystemHealth->channelPub, this->rabbitMqHandlerSystemHealth->exchangeSH);
//    cout << *this->core;
//    cout << "Step " << simulationTime << "\n";

//    std::ostringstream stream;
//    stream << *this->core;
//    std::string str = stream.str();
//    const char *chr = str.c_str();
//    FmuContainer_LOG(fmi2OK, "logAll", "Step reached target time %.0f [ms]: %s", simulationTime, chr);
//    cout << "Checking with existing messages\n";
    if (this->core->process(simulationTime)) {
        FmuContainer_LOG(fmi2OK, "logAll", "Step reached target time %.0f [ms]", simulationTime);
        return true;
    }

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
            //Note that consume, consumes from all channels.
            if (this->rabbitMqHandler->consume(json)) {
                //data received

                DataPoint result;
                if (MessageParser::parse(&this->nameToValueReference, json.c_str(), &result)) {

                    std::stringstream startTimeStamp;
                    startTimeStamp << result.time;

                    FmuContainer_LOG(fmi2OK, "logOk", "Got data '%s', '%s'", startTimeStamp.str().c_str(),
                                     json.c_str());

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

                    //Check which of the inputs of the fmu has changed since the last step
                    string message;
                    for(auto it = this->nameToValueReference.cbegin(); it != this-> nameToValueReference.cend(); it++){
                        ostringstream val;
                        if(it->second.input){
                            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Real){
                                cout << "CURRENT DATA: " << this->currentData.doubleValues[it->second.valueReference] << ", previous DATA: " <<this->previousInputs.doubleValues[it->second.valueReference] << endl;
                                if(this->currentData.doubleValues[it->second.valueReference] != this->previousInputs.doubleValues[it->second.valueReference]){
                                    cout << "INPUT has changed" << endl;
                                    val << this->currentData.doubleValues[it->second.valueReference];
                                    this->core->messageCompose(pair<string, string>(it->second.name, val.str()), message);
                                    //Update previous to current value
                                    this->previousInputs.doubleValues[it->second.valueReference] = this->currentData.doubleValues[it->second.valueReference];
                                }
                            }
                            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Boolean){
                                if(this->currentData.booleanValues[it->second.valueReference] != this->previousInputs.booleanValues[it->second.valueReference]){
                                    cout << "INPUT has changed" << endl;
                                    val << this->currentData.booleanValues[it->second.valueReference];
                                    this->core->messageCompose(pair<string, string>(it->second.name, val.str()), message);
                                    //Update previous to current value
                                    this->previousInputs.booleanValues[it->second.valueReference] = this->currentData.booleanValues[it->second.valueReference];
                                }
                            }
                            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::Integer){
                                if(this->currentData.integerValues[it->second.valueReference] != this->previousInputs.integerValues[it->second.valueReference]){
                                    cout << "INPUT has changed" << endl;
                                    val << this->currentData.integerValues[it->second.valueReference];
                                    this->core->messageCompose(pair<string, string>(it->second.name, val.str()), message);
                                    //Update previous to current value
                                    this->previousInputs.integerValues[it->second.valueReference] = this->currentData.integerValues[it->second.valueReference];
                                }
                            }
                            if(it->second.type == ModelDescriptionParser::ScalarVariable::SvType::String){
                                if(this->currentData.stringValues[it->second.valueReference] != this->previousInputs.stringValues[it->second.valueReference]){
                                    cout << "INPUT has changed" << endl;
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

                    //if anything to send, publish to rabbitmq
                    if(!message.empty()){
                        message = R"({)" + message + R"("timestep":")" + startTimeStamp.str() + R"("})";
                        cout << "This is the message sent to rabbitmq: " << message << endl;
                        this->rabbitMqHandler->publish(this->routingKey.first, message, this->channelPub, this->exchange.first);
                        cout << "Where does it segment fault"  << endl;
                        //Reset Inputs to what they were before send.
                    }
                    
                    //Check if there is info on the system real time
                   
                    string systemHealthData;
                    bool tryAgain = true;
                    bool validData = false;
                    double simTime_d, rTime_d;

                    //if (this->rabbitMqHandlerSystemHealth->getFromChannel(systemHealthData, this->rabbitMqHandlerSystemHealth->channelSub, this->rabbitMqHandlerSystemHealth->queuenameSH.c_str())){
                        if (this->rabbitMqHandlerSystemHealth->consume(systemHealthData)){

                        cout << "New message: " << systemHealthData << ", current simulation time: " << simulationTime << endl;
                        FmuContainer_LOG(fmi2OK, "logAll", "At sim-time: %f [ms], received system health data: %s. \nIf output exists, it will be set.", simulationTime, systemHealthData.c_str());

                        //Extract rtime value from message
                        date::sys_time<std::chrono::milliseconds> simTime, rTime;
                        if(MessageParser::parseSystemHealthMessage(simTime, rTime, systemHealthData.c_str())){
                            validData = true;
                            rTime_d = this->core->message2SimTime(rTime).count();
                            simTime_d = this->core->message2SimTime(simTime).count();

                            cout << "New info on real-time of the system: " << rTime_d  << ", associated simulation time: " << simTime_d << ". Current simulation time: " << simulationTime << endl;
                                
                            FmuContainer_LOG(fmi2OK, "logAll", "NOTE: Difference in time between current simulation step, and received simulation step %.2f [ms]", abs(simulationTime-simTime_d));

                            //If output time_discrepancy present, set its value

                            if(rTime_d < simTime_d){
                                FmuContainer_LOG(fmi2OK, "logWarn", "Co-sim ahead in time by %f [ms]", simTime_d-rTime_d);
                            }
                            else  if (rTime_d > simTime_d){
                                FmuContainer_LOG(fmi2OK, "logWarn", "Co-sim behind in time by %f [ms]", rTime_d-simTime_d);
                            }

                        }
                        else{
                            cout << "Ignoring (either bad json or own message): " << systemHealthData.c_str() << endl << "Will try consume once more" << endl;
                            }
                    }
                    
                    if (this->core->process(simulationTime)) {    
                        if(this->timeOutputPresent){                   
                            if(validData){
                                cout << "It should be here setting the time_discreapncy output" << endl;
                                FmuContainer_LOG(fmi2OK, "logAll", "Setting the time discrepancy output %.2f [ms]", simTime_d-rTime_d);

                                this->core->setTimeDiscrepancyOutput(simTime_d-rTime_d, this->timeOutputVRef);
                            }
                            else{
                                this->core->setTimeDiscrepancyOutput(this->previousTimeOutputVal, this->timeOutputVRef);
                                FmuContainer_LOG(fmi2OK, "logWarn", "There is no valid data for the calculation of the %s output, keeping previous value", "time_discrepancy");
                            }
                        } 
                        if(this->simtimeOutputPresent){                   
                            if(validData){
                                cout << "It should be here setting the time_discrepancy output" << endl;
                                FmuContainer_LOG(fmi2OK, "logAll", "Setting the simtime discrepancy output %.2f [ms]", abs(simulationTime-simTime_d));

                                this->core->setTimeDiscrepancyOutput(abs(simulationTime-simTime_d), this->simtimeOutputVRef);
                            }
                            else{
                                this->core->setTimeDiscrepancyOutput(this->simpreviousTimeOutputVal, this->simtimeOutputVRef);
                                FmuContainer_LOG(fmi2OK, "logWarn", "There is no valid data for the calculation of the %s output, keeping previous value", "time_discrepancy");
                            }
                        } 
                        FmuContainer_LOG(fmi2OK, "logAll", "Step reached target time %.0f [ms]", simulationTime);

                        return true;
                    }

                } else {
                    FmuContainer_LOG(fmi2OK, "logWarn", "Got unknown json '%s'", json.c_str());
                }
            }
        }

    } catch (exception &e) {
        FmuContainer_LOG(fmi2Fatal, "logFatal", "Read message exception '%s'", e.what());
        return false;
    }
    FmuContainer_LOG(fmi2Fatal, "logError", "Did not get data to proceed to time '%f'", simulationTime);

    return false;

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



