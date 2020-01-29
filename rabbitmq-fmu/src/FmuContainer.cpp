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


FmuContainer::FmuContainer(const fmi2CallbackFunctions *mFunctions, bool logginOn, const char *mName,
                           map<string, ModelDescriptionParser::ScalarVariable> nameToValueReference,
                           DataPoint initialDataPoint)
        : m_functions(mFunctions), m_name(mName), nameToValueReference(std::move(nameToValueReference)),
          currentData(std::move(initialDataPoint)), rabbitMqHandler(NULL),
          startOffsetTime(floor<milliseconds>(std::chrono::system_clock::now())),
          communicationTimeout(30), loggingOn(logginOn) {
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
                       std::make_pair(RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT, "communicationtimeout")};

    for (auto const &value: intConfigs) {
        auto vRef = value.first;
        auto description = value.second;

        if (this->currentData.integerValues.find(vRef) == this->currentData.integerValues.end()) {
            FmuContainer_LOG(fmi2Fatal, "logError", "Missing parameter. Value reference '%d', Description '%s' ", vRef,
                             description);
            allParametersPresent = false;
        }
    }


    if (!allParametersPresent) {
        return false;
    }


    auto hostname = stringMap[RABBITMQ_FMU_HOSTNAME_ID];
    auto username = stringMap[RABBITMQ_FMU_USER];
    auto password = stringMap[RABBITMQ_FMU_PWD];
    auto routingKey = stringMap[RABBITMQ_FMU_ROUTING_KEY];

    auto port = this->currentData.integerValues[RABBITMQ_FMU_PORT];
    this->communicationTimeout = this->currentData.integerValues[RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT];

    FmuContainer_LOG(fmi2OK, "logAll",
                     "Preparing initialization. Hostname='%s', Port='%d', Username='%s', routingkey='%s', communication timeout %d s",
                     hostname.c_str(), port, username.c_str(), routingKey.c_str(),
                     this->communicationTimeout);

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

    DataPoint zeroTimeDt;
    bool timeoutOccurred;
    if (!readMessage(&zeroTimeDt, this->communicationTimeout, &timeoutOccurred)) {
        FmuContainer_LOG(fmi2Fatal, "logAll",
                         "Did not receive initial message withing %d seconds", this->communicationTimeout);
        return false;
    }

    std::stringstream startTimeStamp;
    startTimeStamp << zeroTimeDt.time;

    FmuContainer_LOG(fmi2OK, "logAll",
                     "Received initial data message with time '%s' which will be simulation time zero '0'",
                     startTimeStamp.str().c_str());
    this->currentData.merge(zeroTimeDt);
    this->startOffsetTime = zeroTimeDt.time;


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


bool FmuContainer::terminate() { return true; }

#define secondsToMs(value) ((value)*1000.0)

std::chrono::milliseconds FmuContainer::messageTimeToSim(date::sys_time<std::chrono::milliseconds> messageTime) {
    return (messageTime - this->startOffsetTime);
}

fmi2ComponentEnvironment FmuContainer::getComponentEnvironment() { return (fmi2ComponentEnvironment) this; }

bool FmuContainer::readMessage(DataPoint *dataPoint, int timeout, bool *timeoutOccurred) {
    auto start = std::chrono::system_clock::now();
    try {

        string json;
        while (((std::chrono::duration<double>) (std::chrono::system_clock::now() - start)).count() < timeout) {

            if (this->rabbitMqHandler->consume(json)) {
                //data received

                auto result = MessageParser::parse(&this->nameToValueReference, json.c_str());

                // cout << "parse completed" << endl;
                std::stringstream startTimeStamp;
                startTimeStamp << result.time;


                FmuContainer_LOG(fmi2OK, "logOk", "Got data '%s', '%s'", startTimeStamp.str().c_str(), json.c_str());


//                cout << "Got message from server:" << json << " Decoded time: " << result.time << endl;
//                cout << flush;

                *dataPoint = result;
                *timeoutOccurred = false;
                return true;
            }

        }

    } catch (exception &e) {
        cout << "Standard exception: " << e.what() << endl;
        throw e;
    }

    *timeoutOccurred = true;
    return false;

}

bool FmuContainer::step(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {

    auto simulationTime = secondsToMs(currentCommunicationPoint + communicationStepSize);
    cout << "Step time " << currentCommunicationPoint + communicationStepSize << " s converted time " << simulationTime
         << " ms" << endl;


    if (!this->rabbitMqHandler) {
        FmuContainer_LOG(fmi2Fatal, "logAll", "Rabbitmq handle not initialized%s", "");
        return false;
    }

    if (messageTimeToSim(this->currentData.time).count() == simulationTime) {
        return true;
    }

    if (!data.empty()) {
        cout << "Queue " << data.size() << " ";
        for (auto const &pair : data) {
            cout << "'" << pair.time << "==>" << messageTimeToSim(this->data.front().time).count() << " <= "
                 << simulationTime << "', ";
        }
        cout << " done" << endl << flush;
    }

    while (!this->data.empty() && messageTimeToSim(this->data.front().time).count() <= simulationTime) {

        cout << "Merging previously received message with relative time " << messageTimeToSim(this->data.front().time)
             << " Target time is " << simulationTime << endl;
        this->currentData.merge(this->data.front());
        this->data.pop_front();

    }


    DataPoint newMessage;
    bool timeoutOccurred = false;

    while (readMessage(&newMessage, this->communicationTimeout, &timeoutOccurred)) {
        auto msgSimTime = messageTimeToSim(newMessage.time).count();

        cout << "Read message time is " << msgSimTime << "ms in simulation time. Simulation time is "
             << simulationTime << " ms" << endl;
        if (msgSimTime > simulationTime) {

            //ok this message is for the future.
            //queue current read message for newt step
            DataPoint tmp;

            tmp = newMessage;
            this->data.push_back(tmp);
            //Stop reading messages and leave them queue on the queue outside this program
            break;
        } else {
            //ok the message defined the values for this step so merge it
            cout << "Merging" << endl;
            this->currentData.merge(newMessage);
        }
    }


    if (!data.empty()) {
        cout << "After Queue";
        for (auto const &pair : data) {
            cout << pair.time << ", ";
        }
        cout << " done" << endl << flush;
    }


    FmuContainer_LOG(fmi2OK, "logAll", "Step time %.0f [ms] data time %lld [ms]", simulationTime,
                     messageTimeToSim(this->currentData.time).count());

    /*the current state is only valid if we have a next message with a timestamp that is after simulationTime.
     * Then we know that the current values are valid until after the simulation time and we can safely use these*/
    return !timeoutOccurred && (messageTimeToSim(this->currentData.time).count() == simulationTime ||
                                (messageTimeToSim(currentData.time).count() < simulationTime && !this->data.empty() &&
                                 messageTimeToSim(this->data.front().time).count() > simulationTime));
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
            value[i] = this->currentData.booleanValues.at(vr[i]);
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
            value[i] = this->currentData.integerValues.at(vr[i]);
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
            value[i] = this->currentData.doubleValues.at(vr[i]);
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
            value[i] = this->currentData.stringValues.at(vr[i]).c_str();
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



