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


FmuContainer::FmuContainer(const fmi2CallbackFunctions *mFunctions, const char *mName,
                           map<string, ModelDescriptionParser::ScalarVariable> nameToValueReference,
                           DataPoint initialDataPoint)
        : m_functions(mFunctions), m_name(mName), nameToValueReference(std::move(nameToValueReference)),
          currentData(std::move(initialDataPoint)), rabbitMqHandler(NULL), startOffsetTime(0),
          communicationTimeout(30) {
}

FmuContainer::~FmuContainer() {
    if (this->rabbitMqHandler) {
        this->rabbitMqHandler->close();
        delete this->rabbitMqHandler;
    }
}

/*####################################################
 *  Other
 ###################################################*/

bool FmuContainer::setup(fmi2Real startTime) {
    this->time = startTime;
    return true;
}

bool FmuContainer::initialize() {
    FmuContainer_LOG(fmi2OK, "logAll", "Preparing initialization. Looking Up configuration parameters%s", "");

    auto stringMap = this->currentData.stringValues;

    auto stringConfigs = {std::make_pair(RABBITMQ_FMU_HOSTNAME_ID, "hostname"),
                          std::make_pair(RABBITMQ_FMU_USER, "username"),
                          std::make_pair(RABBITMQ_FMU_PWD, "password"),
                          std::make_pair(RABBITMQ_FMU_ROUTING_KEY, "routing key"),
                          std::make_pair(RABBITMQ_FMU_START_TIMESTAMP, "starttimestamp")};


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
    auto startTimeStamp = stringMap[RABBITMQ_FMU_START_TIMESTAMP];

    this->startOffsetTime = Iso8601::parseIso8601ToMilliseconds(startTimeStamp);

    if (this->startOffsetTime == 0) {
        FmuContainer_LOG(fmi2Error, "logError",
                         "Start offset time at value ref %d may not be in format ISO 8601. Either is is the UTC zero time or a parse error occurred",
                         RABBITMQ_FMU_START_TIMESTAMP);
    }


    auto port = this->currentData.integerValues[RABBITMQ_FMU_PORT];
    this->communicationTimeout = this->currentData.integerValues[RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT];

    FmuContainer_LOG(fmi2OK, "logAll",
                     "Preparing initialization. Hostname='%s', Port='%d', Username='%s', routingkey='%s', starttimestamp='%s', communication timeout %d s",
                     hostname.c_str(), port, username.c_str(), routingKey.c_str(), startTimeStamp.c_str(),
                     this->communicationTimeout);

    this->rabbitMqHandler = new RabbitmqHandler(hostname, port, username, password, "fmi_digiital_twin", routingKey);

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

    return true;
}

bool FmuContainer::terminate() { return true; }

#define secondsToMs(value) (value*1000)

fmi2ComponentEnvironment FmuContainer::getComponentEnvironment() { return (fmi2ComponentEnvironment) this; }

bool FmuContainer::step(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {

    std::time_t simulationTime = secondsToMs(currentCommunicationPoint + communicationStepSize);

    if (!this->rabbitMqHandler) {
        FmuContainer_LOG(fmi2Fatal, "logAll", "Rabbitmq handle not initialized%s", "");
        return false;
    }

    string json;
    auto start = std::chrono::system_clock::now();
    while (this->data.empty() || this->data.back().time - this->startOffsetTime < simulationTime) {

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
//        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        if (elapsed_seconds.count() >= this->communicationTimeout) {
            FmuContainer_LOG(fmi2Fatal, "logError",
                             "Rabbitmq communication timeout requiring another message with later timestamp%s",
                             "");
            return false;
        }

        if (this->rabbitMqHandler->consume(json)) {
            //data received
            cout << "Got message from server" << endl;
            auto result = MessageParser::parse(&this->nameToValueReference, json.c_str());

            if (result.time == 0) {
                FmuContainer_LOG(fmi2Warning, "logError",
                                 "Rabbitmq message time may not be in format ISO 8601, or is is UTC zero. Possible parse error%s",
                                 "");
            }

            cout << "Decoded message with to relative  time " << result.time - this->startOffsetTime
                 << " continuing until time is > " << simulationTime << endl;

            if (result.time < this->startOffsetTime) {
                FmuContainer_LOG(fmi2Warning, "logError",
                                 "Discarting rabbitmq message since time is before thest time%s",
                                 "");
                continue;
            }
            this->data.push_back(result);
        } else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        }

    }

    cout << "sufficient messages received" << endl;

    //forward merge. Take next message until the simulation time is reached
    while (!this->data.empty() && this->data.front().time - this->startOffsetTime <= simulationTime) {

        cout << "Merging message with relative time " << this->data.front().time - this->startOffsetTime
             << " Target time is " << simulationTime << endl;
        this->currentData.merge(this->data.front());
        this->data.pop_front();

    }

    /*the current state is only valid if we have a next message with a timestamp that is after simulationTime.
     * Then we know that the current values are valid until after the simulation time and we can safely use these*/
    return !this->data.empty() && this->data.front().time - this->startOffsetTime > simulationTime;
}

/*####################################################
 *  Custom
 ###################################################*/

bool FmuContainer::fmi2GetMaxStepsize(fmi2Real *size) {
    if (!this->data.empty()) {
        *size = this->data.back().time;
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



