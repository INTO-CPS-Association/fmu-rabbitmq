#include "gtest/gtest.h"

#include "fmi2Functions.h"

#include "FmuContainerCore.h"

#include <stdio.h>  /* defines FILENAME_MAX */

#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else

#include <unistd.h>

#define GetCurrentDir getcwd
#endif


using namespace std;

void showStatus(const char *what, fmi2Status status) {
    const char **statuses = new const char *[6]{"ok", "warning", "discard", "error", "fatal", "pending"};
    cout << "Executed '" << what << "' with status '" << statuses[status] << "'" << endl;

    if (status != fmi2OK) {
        throw status;
    }
}

namespace {
    TEST(FmuTest, BasicFlow
    ) {

        GTEST_SKIP();
        cout << " Simulation test for FMI " << fmi2GetVersion() << endl;


        char cCurrentPath[FILENAME_MAX];

        if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))) {
            return;
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


        cout << "xml path for test " << currentUri << endl;


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

            showStatus("fmi2EnterInitializationMode", fmi2EnterInitializationMode(c));
            showStatus("fmi2ExitInitializationMode", fmi2ExitInitializationMode(c));

            size_t nvr = 1;
            const fmi2ValueReference *vr = new fmi2ValueReference[nvr]{20};
            fmi2Real *value = new fmi2Real[nvr];

            showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
            for (int i = 0; i < nvr; i++) {
                cout << "Ref: '" << vr[i] << "' Value '" << value[i] << "'" << endl;
            }


            fmi2Real currentCommunicationPoint = 0;
            fmi2Real communicationStepSize = 0.1;
            fmi2Boolean noSetFMUStatePriorToCurrentPoint = false;


            showStatus("fmi2DoStep", fmi2DoStep(c, currentCommunicationPoint, communicationStepSize,
                                                noSetFMUStatePriorToCurrentPoint));

            showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
            for (int i = 0; i < nvr; i++) {
                cout << "Ref: '" << vr[i] << "' Value '" << value[i] << "'" << endl;
            }


//        fmi2Terminate(fmi2Component c)
        } catch (const char *status) {
            cout << "Error " << status << endl;
        }
        fmi2FreeInstance(c);

    }

    TEST(FmuTest, SetsLookaheadAndMaxAge){

        GTEST_SKIP();
        cout << " Simulation test that includes setting lookahead and max ageissue " << fmi2GetVersion() << endl;


        char cCurrentPath[FILENAME_MAX];

        if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))) {
            return;
        }

        cCurrentPath[sizeof(cCurrentPath) - 1] = '\0'; /* not really required */


        cout << "Working directory is " << cCurrentPath << endl;

        fmi2String instanceName = "rabbitmq";
        fmi2Type fmuType = fmi2CoSimulation;
        fmi2String fmuGUID = "63ba49fe-07d3-402c-b9db-2df495167424";
        string currentUri = (string("file://") + string(cCurrentPath) + string("/rabbitmq-fmu/xmls-for-tests"));
        fmi2String fmuResourceLocation = currentUri.c_str();
        const fmi2CallbackFunctions *functions = nullptr;
        fmi2Boolean visible = true;
        fmi2Boolean loggingOn = true;


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


            size_t nvr = 1;
            // Test for setting max age
            const fmi2ValueReference *vrMaxAge = new fmi2ValueReference[nvr]{7};
            const fmi2Integer *valueMaxAge = new fmi2Integer[nvr]{100};
            showStatus("fmi2SetSetInteger", fmi2SetInteger(c, vrMaxAge, nvr, valueMaxAge));

            // Test for setting lookahead
            const fmi2ValueReference *vrLookahead = new fmi2ValueReference[nvr]{8};
            const fmi2Integer *valueLookahead = new fmi2Integer[nvr]{10};
            showStatus("fmi2SetSetInteger", fmi2SetInteger(c, vrLookahead, nvr, valueLookahead));

            showStatus("fmi2SetupExperiment", fmi2SetupExperiment(
                    c, toleranceDefined, tolerance,
                    startTime, stopTimeDefined, stopTime));



            showStatus("fmi2EnterInitializationMode", fmi2EnterInitializationMode(c));
            showStatus("fmi2ExitInitializationMode", fmi2ExitInitializationMode(c));

            const fmi2ValueReference *vr = new fmi2ValueReference[nvr]{20};
            fmi2Real *value = new fmi2Real[nvr];

            showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
            for (int i = 0; i < nvr; i++) {
                cout << "Ref: '" << vr[i] << "' Value '" << value[i] << "'" << endl;
            }


            fmi2Real currentCommunicationPoint = 0;
            fmi2Real communicationStepSize = 0.1;
            fmi2Boolean noSetFMUStatePriorToCurrentPoint = false;


            showStatus("fmi2DoStep", fmi2DoStep(c, currentCommunicationPoint, communicationStepSize,
                                                noSetFMUStatePriorToCurrentPoint));

            showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
            for (int i = 0; i < nvr; i++) {
                cout << "Ref: '" << vr[i] << "' Value '" << value[i] << "'" << endl;
            }

            currentCommunicationPoint = currentCommunicationPoint + communicationStepSize;

            showStatus("fmi2DoStep", fmi2DoStep(c, currentCommunicationPoint, communicationStepSize,
                                                noSetFMUStatePriorToCurrentPoint));
            currentCommunicationPoint = currentCommunicationPoint + communicationStepSize;
            showStatus("fmi2DoStep", fmi2DoStep(c, currentCommunicationPoint, communicationStepSize,
                                                noSetFMUStatePriorToCurrentPoint));


//        fmi2Terminate(fmi2Component c)
        } catch (const char *status) {
            cout << "Error " << status << endl;
        }
        fmi2FreeInstance(c);
    }

    TEST(FmuContainerCoreTest, checksConvertTimeToString)
    {
        GTEST_SKIP();
        cout << "Testing: FmuContainerCore::convertTimeToString " << endl;
        std::chrono::milliseconds maxAge(1000);
        std::map<FmuContainerCore::ScalarVariableId, int> lookAhead;
        FmuContainerCore test = FmuContainerCore(maxAge, lookAhead);

        long long milliSecondsSinceEpoch[] = { (long long) 100.0, (long long) 200.0, (long long) 300.0, (long long) 400.0, (long long) 500.0, (long long) 600.0, (long long) 700.0, (long long) 800.0, (long long) 900.0, (long long) 1000.0};
        string message[] = {"1970-01-01T00:00:00.100+01:00", "1970-01-01T00:00:00.200+01:00", "1970-01-01T00:00:00.300+01:00", "1970-01-01T00:00:00.400+01:00", "1970-01-01T00:00:00.500+01:00", "1970-01-01T00:00:00.600+01:00", "1970-01-01T00:00:00.700+01:00", "1970-01-01T00:00:00.800+01:00", "1970-01-01T00:00:00.900+01:00" ,"1970-01-01T00:00:01.0+01:00"};

        for (int i = 0; i  < (sizeof(milliSecondsSinceEpoch)/sizeof(*milliSecondsSinceEpoch)); i++) {

            string out;
            test.convertTimeToString(milliSecondsSinceEpoch[i], out);
            cout << "Calculated string: " << out << endl << "Expected: " << message[i] << endl;

            ASSERT_STREQ(out.c_str(), message[i].c_str());

        }
    }

    TEST(FmuSendTest, EnableSend){
        cout << " Simulation test for FMI " << fmi2GetVersion() << endl;


        char cCurrentPath[FILENAME_MAX];

        if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))) {
            return;
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
#define RABBITMQ_FMU_MAXAGE 7
#define RABBITMQ_FMU_SEND_ENABLE 15
#define RABBITMQ_FMU_VHOST 18
#define RABBITMQ_FMU_HOW_TO_SEND 19

#define RABBITMQ_FMU_XPOS 20
#define RABBITMQ_FMU_YPOS 21
#define RABBITMQ_FMU_COMMAND_STOP 22

            fmi2ValueReference vrefs[] = {RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT, RABBITMQ_FMU_PRECISION,
                                          RABBITMQ_FMU_PORT, RABBITMQ_FMU_MAXAGE };
            int intVals[] = {60, 10, 5672, 1000};
            fmi2SetInteger(c, vrefs, 4, intVals);


            fmi2ValueReference vrefsString[] = {RABBITMQ_FMU_HOSTNAME_ID, RABBITMQ_FMU_USER, RABBITMQ_FMU_PWD,
                                                RABBITMQ_FMU_ROUTING_KEY, RABBITMQ_FMU_VHOST};
            const char *stringVals[] = {"localhost", "guest", "guest", "linefollower.data.to_cosim", "/"};
            fmi2SetString(c, vrefsString, 5, stringVals);

            fmi2Boolean boolVals[] = {true};
            fmi2ValueReference vrefsBoolean[] = {RABBITMQ_FMU_HOW_TO_SEND};
            fmi2SetBoolean(c, vrefsBoolean, 1, boolVals);

            showStatus("fmi2EnterInitializationMode", fmi2EnterInitializationMode(c));
            showStatus("fmi2ExitInitializationMode", fmi2ExitInitializationMode(c));

            cout << "Initialization one"<<endl;

            size_t nvr = 2;
            const fmi2ValueReference *vr = new fmi2ValueReference[nvr]{RABBITMQ_FMU_XPOS,RABBITMQ_FMU_YPOS};
            fmi2Real *value = new fmi2Real[nvr];

            showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
            for (int i = 0; i < nvr; i++) {
                cout << "Ref: '" << vr[i] << "' Value '" << value[i] << "'" << endl;
            }


            fmi2Real currentCommunicationPoint = 0;
            fmi2Real communicationStepSize = 0.1;
            /* fmi2Real communicationStepSize = 0.002; */
            fmi2Boolean noSetFMUStatePriorToCurrentPoint = false;

            fmi2Real simDuration = 3.0;
            /* fmi2Real simDuration = 10*4; */
            /* fmi2Real simDuration = 10*20*50; */
            bool changeInput = true;
            fmi2ValueReference vrefsBool[] = {RABBITMQ_FMU_COMMAND_STOP, RABBITMQ_FMU_SEND_ENABLE };
            fmi2Real reals[] = {3.5};
            fmi2Integer ints[] = {5};
            fmi2Boolean enable = true;
            fmi2Boolean bools[] = {changeInput, enable};
            fmi2String strs[] = {"hejsan"};

            showStatus("fmi2SetBoolean", fmi2SetBoolean(c, vrefsBool, 2, bools));

            for(int i = 0; i <= simDuration; i++){
                cout << "ENTER STEP" << endl;
                fmi2Real maxStepSize = 0;
                showStatus("fmi2DoStep", fmi2DoStep(c, currentCommunicationPoint, communicationStepSize,
                                                    noSetFMUStatePriorToCurrentPoint));
                currentCommunicationPoint = currentCommunicationPoint  + communicationStepSize;
                cout << "EXIT STEP" << endl;
                showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
                for (int i = 0; i < nvr; i++) {
                    cout << "Ref: '" << vr[i] << "' Value '" << setprecision(10) << value[i] << "'" << endl;
                }

                enable = !enable;
                if(enable){
                    changeInput = !changeInput;

                }
                fmi2Boolean t[] = {changeInput, enable};
                showStatus("fmi2SetBoolean", fmi2SetBoolean(c, vrefsBool, 2, t));
                cout << "SHOULD have updated" << endl;

            }

//        fmi2Terminate(fmi2Component c)
        } catch (const char *status) {
            cout << "Error " << status << endl;
        }
        fmi2FreeInstance(c);
    }

    TEST(FmuSendTest, BufferLimit){
    cout << " Simulation test for FMI " << fmi2GetVersion() << endl;


    char cCurrentPath[FILENAME_MAX];

    if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))) {
        return;
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
#define RABBITMQ_FMU_MAXAGE 7
#define RABBITMQ_FMU_SEND_ENABLE 15
#define RABBITMQ_FMU_USE_SSL 16
#define RABBITMQ_FMU_QUEUE_UPPER_BOUND 17
#define RABBITMQ_FMU_VHOST 18
#define RABBITMQ_FMU_HOW_TO_SEND 19

#define RABBITMQ_FMU_XPOS 20
#define RABBITMQ_FMU_YPOS 21
#define RABBITMQ_FMU_COMMAND_STOP 22

        fmi2ValueReference vrefs[] = {RABBITMQ_FMU_COMMUNICATION_READ_TIMEOUT, RABBITMQ_FMU_PRECISION,
                                        RABBITMQ_FMU_PORT, RABBITMQ_FMU_MAXAGE, RABBITMQ_FMU_QUEUE_UPPER_BOUND };
        int intVals[] = {60, 10, 5672, 150, 2};
        fmi2SetInteger(c, vrefs, 5, intVals);


        fmi2ValueReference vrefsString[] = {RABBITMQ_FMU_HOSTNAME_ID, RABBITMQ_FMU_USER, RABBITMQ_FMU_PWD,
                                            RABBITMQ_FMU_ROUTING_KEY, RABBITMQ_FMU_VHOST};
        const char *stringVals[] = {"localhost", "guest", "guest", "linefollower.data.to_cosim", "/"};
        fmi2SetString(c, vrefsString, 5, stringVals);

        //fmi2SetBoolean(c, vrefsBoolean, sizeof(boolVals)/sizeof(*boolVals), boolVals);
        fmi2Boolean boolVals[] = {true};
        fmi2ValueReference vrefsBoolean[] = {RABBITMQ_FMU_HOW_TO_SEND};
        fmi2SetBoolean(c, vrefsBoolean, 1, boolVals);

        showStatus("fmi2EnterInitializationMode", fmi2EnterInitializationMode(c));
        showStatus("fmi2ExitInitializationMode", fmi2ExitInitializationMode(c));

        cout << "Initialization one"<<endl;

        size_t nvr = 2;
        const fmi2ValueReference *vr = new fmi2ValueReference[nvr]{RABBITMQ_FMU_XPOS,RABBITMQ_FMU_YPOS};
        fmi2Real *value = new fmi2Real[nvr];

        showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
        for (int i = 0; i < nvr; i++) {
            cout << "Ref: '" << vr[i] << "' Value '" << value[i] << "'" << endl;
        }


        fmi2Real currentCommunicationPoint = 0;
        fmi2Real communicationStepSize = 0.1;
        /* fmi2Real communicationStepSize = 0.002; */
        fmi2Boolean noSetFMUStatePriorToCurrentPoint = false;

        fmi2Real simDuration = 3.0;
        /* fmi2Real simDuration = 10*4; */
        /* fmi2Real simDuration = 10*20*50; */
        bool changeInput = true;
        fmi2ValueReference vrefsBool[] = {RABBITMQ_FMU_COMMAND_STOP, RABBITMQ_FMU_SEND_ENABLE };
        fmi2Real reals[] = {3.5};
        fmi2Integer ints[] = {5};
        fmi2Boolean enable = true;
        fmi2Boolean bools[] = {changeInput, enable};
        fmi2String strs[] = {"hejsan"};

        showStatus("fmi2SetBoolean", fmi2SetBoolean(c, vrefsBool, 2, bools));

        for(int i = 0; i <= simDuration; i++){
            cout << "ENTER STEP" << endl;
            fmi2Real maxStepSize = 0;
            showStatus("fmi2DoStep", fmi2DoStep(c, currentCommunicationPoint, communicationStepSize,
                                                noSetFMUStatePriorToCurrentPoint));
            currentCommunicationPoint = currentCommunicationPoint  + communicationStepSize;
            cout << "EXIT STEP" << endl;
            showStatus("fmi2GetReal", fmi2GetReal(c, vr, nvr, value));
            for (int i = 0; i < nvr; i++) {
                cout << "Ref: '" << vr[i] << "' Value '" << setprecision(10) << value[i] << "'" << endl;
            }

            enable = !enable;
            if(enable){
                changeInput = !changeInput;

            }
            fmi2Boolean t[] = {changeInput, enable};
            showStatus("fmi2SetBoolean", fmi2SetBoolean(c, vrefsBool, 2, t));
            cout << "SHOULD have updated" << endl;

        }

//        fmi2Terminate(fmi2Component c)
    } catch (const char *status) {
        cout << "Error " << status << endl;
    }
    fmi2FreeInstance(c);
}
}
