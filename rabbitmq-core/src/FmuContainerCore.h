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
        printf("Assign\n");
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
    typedef pair<date::sys_time<std::chrono::milliseconds>, ScalarVariableBaseValue> TimedScalarBasicValue;
    typedef unsigned int ScalarVariableId;

    FmuContainerCore(std::chrono::milliseconds maxAge,
                     std::map<ScalarVariableId, int> lookAhead);

    void add(ScalarVariableId id, TimedScalarBasicValue value);

    bool process(double time);

    bool initialize();

    std::map<ScalarVariableId, TimedScalarBasicValue> getData();

    date::sys_time<std::chrono::milliseconds> getStartOffsetTime();

    void setVerbose(bool verbose);

    friend ostream &operator<<(ostream &os, const FmuContainerCore &c);

    void add_flag(int flagVRef, pair<string, bool> nameVal);

    void update_flag(int flagVRef, pair<string, bool> nameVal);

    void add_input_val(int inputVRef, pair<string, string> nameVal);

    void update_input_val(int inputVRef, pair<string, string> nameVal);

    void sendCheckCompose(string &message);
    
    void printFlagsInputs();

protected:

//TODO: these should be qualified by type because the svid is not globally unique
    std::map<ScalarVariableId, list<TimedScalarBasicValue>> incomingUnprocessed;
    std::map<ScalarVariableId, list<TimedScalarBasicValue>> incomingLookahead;

    std::map<ScalarVariableId, TimedScalarBasicValue> currentData;

    date::sys_time<std::chrono::milliseconds> startOffsetTime;


    std::map<ScalarVariableId, int> lookahead;
    std::chrono::milliseconds maxAge;


    std::map<int, std::pair<string, bool>> inputFlags; //<vref, <name, value>>
    std::map<int, std::pair<string, string>> inputVals; //<vref, <name, value>>
    
private:

    bool verbose;

    std::chrono::milliseconds messageTimeToSim(date::sys_time<std::chrono::milliseconds> messageTime);

    bool check(double time);

    void processIncoming();

    bool hasValueFor(std::map<ScalarVariableId, TimedScalarBasicValue> &currentData, list<ScalarVariableId> &knownIds);

    pair<bool, date::sys_time<std::chrono::milliseconds>> calculateStartTime();

    template<typename Predicate>
    void processLookahead(Predicate predicate);
};


#endif //RABBITMQFMUPROJECT_FMUCONTAINERCORE_H
