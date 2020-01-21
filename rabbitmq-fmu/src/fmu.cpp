/*
 * fmi.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: parallels
 */

#include <stdio.h>

/*
 * fmi2Functions.c
 *
 *  Created on: May 22, 2015
 *      Author: kel
 */

#include "fmi2Functions.h"
#include "FmuContainer.h"
#include "uri.h"
//#define UUID4_LEN 37
#include <string.h>


#ifdef _WIN32
#include <io.h>
#define access _access_s
#else



#endif

#include <unistd.h>
#include <string>
#include <vector>

#include "modeldescription/ModelDescriptionParser.h"

bool FileExists(const std::string &Filename) {
    return access(Filename.c_str(), 0) == 0;
}

//static int currentId = 0;

std::vector<FmuContainer *> g_clients;

static FmuContainer *getFmuContainer(fmi2Component c) {
    for (auto &container : g_clients) {
        if (container == c) {
            return (FmuContainer *) c;
        }
    }
    return nullptr;
}


static void notimplemented(fmi2Component c, fmi2String message) {
    auto *fmu = (FmuContainer *) c;

    if (fmu != nullptr && fmu->m_functions != nullptr &&
        fmu->m_functions->logger != nullptr) {
        std::string base("Not implemented: ");
        std::string m(message);
        fmu->m_functions->logger(fmu->getComponentEnvironment(), fmu->m_name.c_str(), fmi2Error,
                                 "error", (base + m).c_str());
    }
}





#define LOG(functions, name, status, category, message, args...)       \
  if (functions != NULL) {                                             \
    if (functions->logger != NULL) {                                   \
      functions->logger(functions->componentEnvironment, name, status, \
                        category, message, args);                      \
    }                                                                  \
  } else {                                                             \
    fprintf(stderr, "Name '%s', Category: '%s'\n", name, category);    \
    fprintf(stderr, message, args);                                    \
    fprintf(stderr, "\n");                                             \
  }


// ---------------------------------------------------------------------------
// FMI functions
// ---------------------------------------------------------------------------
extern "C" fmi2Component fmi2Instantiate(fmi2String instanceName,
                                         fmi2Type fmuType, fmi2String fmuGUID,
                                         fmi2String fmuResourceLocation,
                                         const fmi2CallbackFunctions *functions,
                                         fmi2Boolean visible,
                                         fmi2Boolean loggingOn) {

    if (loggingOn) {
        LOG(functions, instanceName, fmi2OK, "logFmiCall",
            "FMU: Called instantiate with instance '%s' and GUID '%s'", instanceName,
            fmuGUID);
    }

    std::string resourceLocationStr(URIToNativePath(fmuResourceLocation));
    std::string modelDescriptionFile = resourceLocationStr + std::string("modelDescription.xml");

    if (!FileExists(modelDescriptionFile)) {
        LOG(functions, instanceName, fmi2Error, "internal",
            "FMU: Model description in resources missing '%s'. Cannot detect bound scalar variables",
            modelDescriptionFile.c_str());
        return nullptr;
    }

    cout << "Model description path: "<<modelDescriptionFile.c_str() <<endl <<flush;

    if(loggingOn)
    {
        LOG(functions, instanceName, fmi2OK, "logAll",
            "FMU: Resource model description file location is '%s'",
            modelDescriptionFile.c_str());
    }

    auto svs = ModelDescriptionParser::parse(modelDescriptionFile);

    if(loggingOn)
    {
        LOG(functions, instanceName, fmi2OK, "logAll",
            "Parsed model description to obtain name, ref, type triples '%s'",
            modelDescriptionFile.c_str());
    }

    DataPoint dp = ModelDescriptionParser::extractDataPoint(svs);

    if(loggingOn)
    {
        LOG(functions, instanceName, fmi2OK, "logAll",
            "Extracted start values from model description '%s'",
            modelDescriptionFile.c_str());
    }

    auto *container =
            new FmuContainer(functions, instanceName, svs, dp);

    g_clients.push_back(container);

    if(loggingOn)
    {
        LOG(functions, instanceName, fmi2OK, "logAll",
            "Initialization donw '%s'",instanceName);
    }

    return (void *) container;
}

extern "C" fmi2Status fmi2SetupExperiment(
        fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
        fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->setup(startTime)) {
        return fmi2OK;
    }

    return fmi2OK;
}

extern "C" fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->initialize()) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    return fmi2OK;
}

extern "C" fmi2Status fmi2Terminate(fmi2Component c) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->terminate()) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2Reset(fmi2Component c) {
    return fmi2OK;
}

extern "C" void fmi2FreeInstance(fmi2Component c) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != NULL) {
        delete fmu;
    }
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

extern "C" const char *fmi2GetVersion() { return fmi2Version; }

extern "C" const char *fmi2GetTypesPlatform() { return fmi2TypesPlatform; }

// ---------------------------------------------------------------------------
// FMI functions: logging control, setters and getters for Real, Integer,
// Boolean, String
// ---------------------------------------------------------------------------

extern "C" fmi2Status fmi2SetDebugLogging(fmi2Component c,
                                          fmi2Boolean loggingOn,
                                          size_t nCategories,
                                          const fmi2String categories[]) {
    return fmi2OK;
}

extern "C" fmi2Status fmi2GetReal(fmi2Component c,
                                  const fmi2ValueReference vr[], size_t nvr,
                                  fmi2Real value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->getReal(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetInteger(fmi2Component c,
                                     const fmi2ValueReference vr[], size_t nvr,
                                     fmi2Integer value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->getInteger(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetBoolean(fmi2Component c,
                                     const fmi2ValueReference vr[], size_t nvr,
                                     fmi2Boolean value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->getBoolean(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetString(fmi2Component c,
                                    const fmi2ValueReference vr[], size_t nvr,
                                    fmi2String value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->getString(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2SetReal(fmi2Component c,
                                  const fmi2ValueReference vr[], size_t nvr,
                                  const fmi2Real value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->setReal(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2SetInteger(fmi2Component c,
                                     const fmi2ValueReference vr[], size_t nvr,
                                     const fmi2Integer value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->setInteger(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2SetBoolean(fmi2Component c,
                                     const fmi2ValueReference vr[], size_t nvr,
                                     const fmi2Boolean value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->setBoolean(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2SetString(fmi2Component c,
                                    const fmi2ValueReference vr[], size_t nvr,
                                    const fmi2String value[]) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->setString(vr, nvr, value)) {
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate *FMUstate) {
    notimplemented(c, "fmi2GetFMUstate");
    return fmi2Error;
}
extern "C" fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate FMUstate) {
    notimplemented(c, "fmi2SetFMUstate");
    return fmi2Error;
}
extern "C" fmi2Status fmi2FreeFMUstate(fmi2Component c,
                                       fmi2FMUstate *FMUstate) {
    notimplemented(c, "fmi2FreeFMUstate");
    return fmi2Error;
}
extern "C" fmi2Status fmi2SerializedFMUstateSize(fmi2Component c,
                                                 fmi2FMUstate FMUstate,
                                                 size_t *size) {
    notimplemented(c, "fmi2SerializedFMUstateSize");
    return fmi2Error;
}
extern "C" fmi2Status fmi2SerializeFMUstate(fmi2Component c,
                                            fmi2FMUstate FMUstate,
                                            fmi2Byte serializedState[],
                                            size_t size) {
    notimplemented(c, "fmi2SerializeFMUstate");
    return fmi2Error;
}
extern "C" fmi2Status fmi2DeSerializeFMUstate(fmi2Component c,
                                              const fmi2Byte serializedState[],
                                              size_t size,
                                              fmi2FMUstate *FMUstate) {
    notimplemented(c, "fmi2DeSerializeFMUstate");
    return fmi2Error;
}

extern "C" fmi2Status fmi2GetDirectionalDerivative(
        fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
        const fmi2ValueReference vKnown_ref[], size_t nKnown,
        const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {
    notimplemented(c, "fmi2GetDirectionalDerivative");
    return fmi2Error;
}

// ---------------------------------------------------------------------------
// Functions for FMI for Co-Simulation
// ---------------------------------------------------------------------------
#ifdef FMI_COSIMULATION
/* Simulating the slave */
extern "C" fmi2Status fmi2SetRealInputDerivatives(fmi2Component c,
                                                  const fmi2ValueReference vr[],
                                                  size_t nvr,
                                                  const fmi2Integer order[],
                                                  const fmi2Real value[]) {
    notimplemented(c, "fmi2SetRealInputDerivatives");
    return fmi2Error;
}

extern "C" fmi2Status fmi2GetRealOutputDerivatives(
        fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
        const fmi2Integer order[], fmi2Real value[]) {
    notimplemented(c, "fmi2GetRealOutputDerivatives");
    return fmi2Error;
}

extern "C" fmi2Status fmi2CancelStep(fmi2Component c) {
    notimplemented(c, "fmi2CancelStep");
    return fmi2Error;
}

extern "C" fmi2Status fmi2DoStep(fmi2Component c,
                                 fmi2Real currentCommunicationPoint,
                                 fmi2Real communicationStepSize,
                                 fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->step(currentCommunicationPoint, communicationStepSize)) {
        return fmi2OK;
    }

    return fmi2Discard;
}

extern "C" fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s,
                                    fmi2Status *value) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr) {
        *value = fmi2OK;
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s,
                                        fmi2Real *value) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr) {
        *value = fmi2OK;
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetIntegerStatus(fmi2Component c,
                                           const fmi2StatusKind s,
                                           fmi2Integer *value) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr) {
        *value = fmi2OK;
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetBooleanStatus(fmi2Component c,
                                           const fmi2StatusKind s,
                                           fmi2Boolean *value) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr) {
        *value = fmi2OK;
        return fmi2OK;
    }
    return fmi2Fatal;
}

extern "C" fmi2Status fmi2GetStringStatus(fmi2Component c,
                                          const fmi2StatusKind s,
                                          fmi2String *value) {
    return fmi2Fatal;
}

/* INTO cps specific*/
extern "C" fmi2Status fmi2GetMaxStepsize(fmi2Component c, fmi2Real *size) {
    FmuContainer *fmu = getFmuContainer(c);

    if (fmu != nullptr && fmu->fmi2GetMaxStepsize(size)) {
        return fmi2OK;
    }

    return fmi2Fatal;
}

// ---------------------------------------------------------------------------
// Functions for FMI2 for Model Exchange
// ---------------------------------------------------------------------------
#else

/* Enter and exit the different modes */
fmi2Status fmi2EnterEventMode(fmi2Component c) { return fmi2Error; }

fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo *eventInfo) {
    return fmi2Error;
}

fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) { return fmi2Error; }

fmi2Status fmi2CompletedIntegratorStep(
        fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint,
        fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation) {
    return fmi2Error;
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) { return fmi2Error; }

fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[],
                                   size_t nx) {
    return fmi2Error;
}

/* Evaluation of the model equations */
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[],
                              size_t nx) {
    return fmi2Error;
}

fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[],
                                  size_t ni) {
    return fmi2Error;
}

fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real states[],
                                   size_t nx) {
    return fmi2Error;
}

fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c,
                                             fmi2Real x_nominal[], size_t nx) {
    return fmi2Error;
}

#endif  // Model Exchange