#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

#include <fstream>
#include <sstream>

#include <cstdlib>
#include <ctime>
#include <string>

#include <iomanip>
#ifdef _WIN32
#define timegm _mkgmtime
#endif

#include "modeldescription/ModelDescriptionParser.h"

#include "fmi2Functions.h"


#include <stdio.h>  /* defines FILENAME_MAX */

#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else

#include <unistd.h>

#define GetCurrentDir getcwd
#endif

using namespace std;
using SvType = ModelDescriptionParser::ScalarVariable::SvType;
using namespace rapidjson;

inline int ParseInt(const char *value) {
    return std::strtol(value, nullptr, 10);
}

std::time_t ParseISO8601(const std::string &input) {
    constexpr const size_t expectedLength = sizeof("1234-12-12T12:12:12Z") - 1;
    static_assert(expectedLength == 20, "Unexpected ISO 8601 date/time length");

    if (input.length() < expectedLength) {
        return 0;
    }

    std::tm time = {0};
    time.tm_year = ParseInt(&input[0]) - 1900;
    time.tm_mon = ParseInt(&input[5]) - 1;
    time.tm_mday = ParseInt(&input[8]);
    time.tm_hour = ParseInt(&input[11]);
    time.tm_min = ParseInt(&input[14]);
    time.tm_sec = ParseInt(&input[17]);
    time.tm_isdst = 0;
    const int millis = input.length() > 20 ? ParseInt(&input[20]) : 0;
    return timegm(&time) * 1000 + millis;
}

void testMd() {
    ModelDescriptionParser parser;
    auto map = parser.parse(string("modelDescription.xml"));

    for (auto &it : map) {
        auto sv = it.second;
        cout << it.first << " => " << "ref " << it.second.valueReference << " start value '";
        switch (sv.type) {
            case SvType::Integer:
                cout << sv.i_value;
                break;
            case SvType::Real:
                cout << sv.d_value;
                break;
            case SvType::Boolean:
                cout << sv.b_value;
                break;
            case SvType::String:
                cout << sv.s_value;
                break;
        }

        cout << "'\n";
    }
}

void showStatus(const char *what, fmi2Status status) {
    const char **statuses = new const char *[6]{"ok", "warning", "discard", "error", "fatal", "pending"};
    cout << "Executed '" << what << "' with status '" << statuses[status] << "'" << endl;

    if (status != fmi2OK) {
        throw status;
    }
}

int main() {
    {
        cout << " Simulation test for FMI " << fmi2GetVersion() << endl;


        char cCurrentPath[FILENAME_MAX];

        if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))) {
            return 1;
        }

        cCurrentPath[sizeof(cCurrentPath) - 1] = '\0'; /* not really required */


        cout << "Working directory is " << cCurrentPath << endl;

        fmi2String instanceName = "rabbitmq";
        fmi2Type fmuType = fmi2CoSimulation;
        fmi2String fmuGUID = "63ba49fe-07d3-402c-b9db-2df495167424";
        string currentUri = (string("file://") + string(cCurrentPath));
        fmi2String fmuResourceLocation = currentUri.c_str();
        const fmi2CallbackFunctions *functions = nullptr;
        fmi2Boolean visible = false;
        fmi2Boolean loggingOn = false;


        auto c = fmi2Instantiate(
                instanceName,
                fmuType, fmuGUID,
                fmuResourceLocation,
                functions,
                visible,
                loggingOn);

        try {
            fmi2Boolean toleranceDefined = false;
            fmi2Real tolerance = 0;
            fmi2Real startTime = 0;
            fmi2Boolean stopTimeDefined = true;
            fmi2Real stopTime = true;

            showStatus("fmi2SetupExperiment", fmi2SetupExperiment(
                    c, toleranceDefined, tolerance,
                    startTime, stopTimeDefined, stopTime));

#define RABBITMQ_FMU_HOSTNAME_ID 0
#define RABBITMQ_FMU_PORT 1
#define RABBITMQ_FMU_USER 2
#define RABBITMQ_FMU_PWD 3
#define RABBITMQ_FMU_ROUTING_KEY 4
#define RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT 5
#define RABBITMQ_FMU_PRECISION 6

#define RABBITMQ_FMU_SEND_FLAG_CSTOP 21
#define RABBITMQ_FMU_COMMAND_STOP 23
#define RABBITMQ_FMU_COMMAND_INT 24

            fmi2ValueReference vrefs[] = {RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT, RABBITMQ_FMU_PRECISION,
                                          RABBITMQ_FMU_PORT};
            int intVals[] = {60, 10, 5672};
            fmi2SetInteger(c, vrefs, 3, intVals);


            fmi2ValueReference vrefsString[] = {RABBITMQ_FMU_HOSTNAME_ID, RABBITMQ_FMU_USER, RABBITMQ_FMU_PWD,
                                                RABBITMQ_FMU_ROUTING_KEY};
            const char *stringVals[] = {"localhost", "guest", "guest", "linefollower"};
            fmi2SetString(c, vrefsString, 4, stringVals);

            //fmi2SetBoolean(c, vrefsBoolean, sizeof(boolVals)/sizeof(*boolVals), boolVals);

            showStatus("fmi2EnterInitializationMode", fmi2EnterInitializationMode(c));
            showStatus("fmi2ExitInitializationMode", fmi2ExitInitializationMode(c));

            cout << "Initialization one"<<endl;

#define RABBITMQ_FMU_LEVEL 20

            size_t nvr = 2;
            const fmi2ValueReference *vr = new fmi2ValueReference[nvr]{RABBITMQ_FMU_LEVEL,22};
            fmi2Real *value = new fmi2Real[nvr];

            showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
            for (int i = 0; i < nvr; i++) {
                cout << "Ref: '" << vr[i] << "' Value '" << value[i] << "'" << endl;
            }


            fmi2Real currentCommunicationPoint = 0;
            fmi2Real communicationStepSize = 0.1;
            fmi2Boolean noSetFMUStatePriorToCurrentPoint = false;

            fmi2Real simDuration = 10;
            bool changeInput = false;
            fmi2ValueReference vrefsReals[] = {RABBITMQ_FMU_COMMAND_STOP};
            fmi2ValueReference vrefsInt[] = {RABBITMQ_FMU_COMMAND_INT};
            fmi2ValueReference vrefsBool[] = {26};
            fmi2ValueReference vrefsStrs[] = {25};
            fmi2Real reals[] = {3.5};
            fmi2Integer ints[] = {5};
            fmi2Boolean bools[] = {true};
            fmi2String strs[] = {"hejsan"};
            for(int i = 0; i <= simDuration; i++){
                showStatus("fmi2DoStep", fmi2DoStep(c, currentCommunicationPoint, communicationStepSize,
                                                    noSetFMUStatePriorToCurrentPoint));

                showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
                for (int i = 0; i < nvr; i++) {
                    cout << "Ref: '" << vr[i] << "' Value '" << setprecision(10) << value[i] << "'" << endl;
                }
                currentCommunicationPoint = currentCommunicationPoint + communicationStepSize;

                if(changeInput){
                    showStatus("fmi2SetReal", fmi2SetReal(c, vrefsReals, 1, reals));
                    showStatus("fmi2SetString", fmi2SetString(c, vrefsStrs, 1, strs));
                    cout << "SHOULD have updated" << endl;
                }
                else{

                    showStatus("fmi2SetInteger", fmi2SetInteger(c, vrefsInt, 1, ints));
                    showStatus("fmi2SetBoolean", fmi2SetBoolean(c, vrefsBool, 1, bools));
                }

                changeInput=!changeInput;
            }

//        fmi2Terminate(fmi2Component c)
        } catch (const char *status) {
            cout << "Error " << status << endl;
        }
        fmi2FreeInstance(c);
//
//     testMd();
//     return 0;

        // 1. Parse a JSON string into DOM.
        const char *json = "{\"project\":\"rapidjson\",\"stars\":10}";
        Document d;
        d.Parse(json);

        // 2. Modify it by DOM.
        Value &s = d["stars"];
        s.SetInt(s.GetInt() + 13);

        int k = s.GetInt();

        // 3. Stringify the DOM
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        d.Accept(writer);

        // Output {"project":"rapidjson","stars":11}
        std::cout << buffer.GetString() << std::endl;
    }

    std::ifstream t("data.json");
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::cout << buffer.str() << std::endl;

    Document d;
    d.Parse(buffer.str().c_str());

    if (d.HasMember("time") && d["time"].IsString()) {
        const char *timeString = d["time"].GetString();
        std::cout << "Time is: " << timeString << std::endl;

        std::time_t t = ParseISO8601(std::string(timeString));

        std::tm *ptm = std::localtime(&t);
        char buffer2[32];
// Format: Mo, 15.06.2009 20:20:00
        std::strftime(buffer2, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);

        std::cout << "Time is: " << buffer2 << std::endl;
    }

    return 0;
}
