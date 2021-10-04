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
}