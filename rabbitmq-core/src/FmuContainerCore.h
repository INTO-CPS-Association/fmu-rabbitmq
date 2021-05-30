//
// Created by Kenneth Guldbrandt Lausdahl on 09/03/2020.
//

#ifndef RABBITMQFMUPROJECT_FMUCONTAINERCORE_H
#define RABBITMQFMUPROJECT_FMUCONTAINERCORE_H


#include <list>
#include <iterator>
#include "date/date.h"

#include <string>
#include <map>
#include <ctime>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>

#include "../../thirdparty/fmi/include/fmi2Functions.h"

using namespace std;

const int TU_STRING = 0;
const int TU_INT = 1;
const int TU_BOOL = 2;
const int TU_DOUBLE = 3;

union ScalarVariableBaseValue {


    struct i_type {
        int type;
        int i;
    } i;
    struct b_type {
        int type;
        bool b;
    } b;
    struct d_type {
        int type;
        double d;
    } d;
    struct s_type {
        int type;
        std::string s;
    } s;

    ScalarVariableBaseValue(int i) : i{TU_INT, i} {}

    ScalarVariableBaseValue(bool b) : b{TU_BOOL, b} {}

    ScalarVariableBaseValue(double d) : d{TU_DOUBLE, d} {}

    ScalarVariableBaseValue(std::string s) : s{TU_STRING, std::move(s)} {}

    ScalarVariableBaseValue(const ScalarVariableBaseValue &other) {
        // This is safe.
        switch (other.i.type) {
            case TU_INT:
                ::new(&i) auto(other.i);
                break;
            case TU_BOOL:
                ::new(&b) auto(other.b);
                break;
            case TU_DOUBLE:
                ::new(&d) auto(other.d);
                break;
            case TU_STRING:
                ::new(&s) auto(other.s);
                break;
        }
    }


    inline bool operator!=(const ScalarVariableBaseValue &rhs) const { return !(this == &rhs); }

    bool operator==(const ScalarVariableBaseValue &other) const {
        // This is safe.
        switch (other.i.type) {
            case TU_INT:
                return this->i.i == other.i.i;
            case TU_BOOL:
                return this->b.b == other.b.b;
            case TU_DOUBLE:
                return this->d.d == other.d.d;
            case TU_STRING:
                return this->s.s == other.s.s;
        }
        return false;
    }

    ScalarVariableBaseValue &operator=(const ScalarVariableBaseValue &other) // copy assignment
    {
        switch (other.i.type) {
            case TU_INT:
                this->i.i = other.i.i;
		break;
            case TU_BOOL:
                this->b.b = other.b.b;
		break;
            case TU_DOUBLE:
                this->d.d = other.d.d;
		break;
            case TU_STRING:
                this->s.s = std::move(other.s.s);
		break;
        }
        this->i.type = other.i.type;
        return *this;
    }

    ~ScalarVariableBaseValue() {
        // This is safe.
        if (TU_STRING == s.type) {
            s.~s_type();
        }
    }


    friend ostream &operator<<(ostream &os, const ScalarVariableBaseValue &c) {
        switch (c.i.type) {
            case TU_INT:
                os<<"I" << (int)(c.i.i);
                break;
            case TU_BOOL:
                os <<"B"<< (bool)(c.b.b);
                break;
            case TU_DOUBLE:
                os<<"D" << (double)(c.d.d);
                break;
            case TU_STRING:
                os<<"S" << c.s.s;
                break;
        }
        return os;
    }
};

class FmuContainerCore {


public:
    const fmi2CallbackFunctions *m_functions;
    const string m_name;

    typedef pair<date::sys_time<std::chrono::milliseconds>, ScalarVariableBaseValue> TimedScalarBasicValue;
    typedef unsigned int ScalarVariableId;


    FmuContainerCore(std::chrono::milliseconds maxAge,
                     std::map<ScalarVariableId, int> lookAhead);
    FmuContainerCore(std::chrono::milliseconds maxAge,
                     std::map<ScalarVariableId, int> lookAhead,
                     const fmi2CallbackFunctions *mFunctions,
                     const char *mName);

    void add(ScalarVariableId id, TimedScalarBasicValue value);

    bool process(double time);

    bool initialize();

    std::map<ScalarVariableId, TimedScalarBasicValue> getData();

    date::sys_time<std::chrono::milliseconds> getStartOffsetTime();

    void setVerbose(bool verbose);

    friend ostream &operator<<(ostream &os, const FmuContainerCore &c);

    void messageCompose(pair<string,string> input, string &message);

    std::chrono::milliseconds message2SimTime(date::sys_time<std::chrono::milliseconds> rTime);

    std::chrono::milliseconds simTimeToReal(long long simTime);
    
    void convertTimeToString(long long milliSecondsSinceEpoch, string &message);
    
    //void setTimeDiscrepancyOutput(double time, int vref);
    void setTimeDiscrepancyOutput(bool valid, double timeDiffNew, double timeDiffOld, int vref);
    double getTimeDiscrepancyOutput(int vref);
    int getSeqNO(int vref);
    std::chrono::milliseconds messageTimeToSim(date::sys_time<std::chrono::milliseconds> messageTime);

#ifdef USE_RBMQ_FMU_THREAD
    bool hasUnprocessed(void);
    std::mutex m;
#endif
    int incomingSize(void);

#ifdef USE_RBMQ_FMU_HEALTH_THREAD
    typedef pair<date::sys_time<std::chrono::milliseconds>, date::sys_time<std::chrono::milliseconds>> HealthData;
    bool hasUnprocessedHealth(void);
    std::mutex mHealth;
    std::list<HealthData> incomingUnprocessedHealth;
#endif

protected:

//TODO: these should be qualified by type because the svid is not globally unique
    class TimedScalarBasicValueCompare{
        public:
          bool operator()(const TimedScalarBasicValue &a, const TimedScalarBasicValue &b)
            {
                return a.first > b.first; 
            }
    };

    std::map<ScalarVariableId, priority_queue<TimedScalarBasicValue, vector<TimedScalarBasicValue>,
             TimedScalarBasicValueCompare>> incomingUnprocessed;
    std::map<ScalarVariableId, list<TimedScalarBasicValue>> incomingLookahead;

    std::map<ScalarVariableId, TimedScalarBasicValue> currentData;

    date::sys_time<std::chrono::milliseconds> startOffsetTime;


    std::map<ScalarVariableId, int> lookahead;
    std::chrono::milliseconds maxAge;
    std::string currentOutput; // current message without timestamp
    
private:

    bool verbose;


    bool check(double time);

    template<typename Predicate>
    void processIncoming(Predicate predicate);

    bool hasValueFor(std::map<ScalarVariableId, TimedScalarBasicValue> &currentData, list<ScalarVariableId> &knownIds);

    pair<bool, date::sys_time<std::chrono::milliseconds>> calculateStartTime();
};
#endif //RABBITMQFMUPROJECT_FMUCONTAINERCORE_H
