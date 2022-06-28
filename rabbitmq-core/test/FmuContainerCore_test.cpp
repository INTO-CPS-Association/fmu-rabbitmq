#include "gtest/gtest.h"

#include "FmuContainerCore.h"


#include <iostream>

#include "date/date.h"

using namespace std;

using namespace date;

//IMPORTANT: setVerbose(true) should not be used, as the fmucontainercore_log would not be properly initialised
//from the core constructor.
class FmuContainerCoreTestProxy : public FmuContainerCore {
public:

    struct State {
        std::chrono::milliseconds maxAge;
        std::map<FmuContainerCore::ScalarVariableId, int> lookahead;
        std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>> incomingUnprocessed;
        std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>> incomingLookahead;

        std::map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue> currentData;

        date::sys_time<std::chrono::milliseconds> startOffsetTime;
    };


    FmuContainerCoreTestProxy(const chrono::milliseconds &maxAge, std::map<ScalarVariableId, int> lookAhead)
            : FmuContainerCore(maxAge, lookAhead) {}

    FmuContainerCoreTestProxy(const chrono::milliseconds &maxAge, std::map<ScalarVariableId, int> lookAhead,
                              std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>> incomingUnprocessed,
                              std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>> incomingLookahead,

                              std::map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue> currentData,

                              date::sys_time<std::chrono::milliseconds> startOffsetTime)
            : FmuContainerCore(maxAge, lookAhead) {
        this->incomingUnprocessed.insert(incomingUnprocessed.begin(), incomingUnprocessed.end());
        this->incomingLookahead.insert(incomingLookahead.begin(), incomingLookahead.end());
        this->currentData.insert(currentData.begin(), currentData.end());
        this->startOffsetTime = startOffsetTime;
    }


    FmuContainerCoreTestProxy(const State &s)
            : FmuContainerCoreTestProxy(s.maxAge, s.lookahead, s.incomingUnprocessed, s.incomingLookahead,
                                        s.currentData, s.startOffsetTime) {
    }


    const map<ScalarVariableId, list<TimedScalarBasicValue>> &getIncomingUnprocessed() const {
        return incomingUnprocessed;
    }

    const map<ScalarVariableId, list<TimedScalarBasicValue>> &getIncomingLookahead() const {
        return incomingLookahead;
    }

    const map<ScalarVariableId, TimedScalarBasicValue> &getCurrentData() const {
        return currentData;
    }

    const sys_time<chrono::milliseconds> &getStartOffsetTime() const {
        return startOffsetTime;
    }

    const map<ScalarVariableId, int> &getLookahead() const {
        return lookahead;
    }

    const chrono::milliseconds &getMaxAge() const {
        return maxAge;
    }

    void show() {
        cout << "------------------------------ INFO ------------------------------" << endl;
        cout << "Max age: " << this->maxAge << endl;
        cout << "StartTime: " << this->startOffsetTime << endl;
        cout << "Lookahead ids: [";
        for (auto &id:lookahead) {
            cout << id.first << "->" << id.second << " ";
        }
        cout << "]" << endl;
        cout << "Incoming" << endl;
        for (auto &pair: incomingUnprocessed) {
            cout << "\tId: " << pair.first << endl;
            for (auto &val: pair.second) {
                showValue("\t\t", val);

            }
        }

        cout << "Lookahead" << endl;
        for (auto &pair: incomingLookahead) {
            cout << "\tId: " << pair.first << endl;
            for (auto &val: pair.second) {
                showValue("\t\t", val);

            }
        }

        cout << "Data" << endl;
        for (auto &pair: currentData) {
            cout << "\tId: " << pair.first;

            showValue(" ", pair.second);


        }
        cout << endl;
        cout << "------------------------------------------------------------------" << endl;
    }

private:
    void showValue(const char *prefix, FmuContainerCore::TimedScalarBasicValue val) {
        cout << prefix << "Time: " << val.first.time_since_epoch().count() << " Value: " << val.second << endl;
    }

};
namespace {


    bool eq(map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue> a,
            map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue> b) {
        if (a.size() != b.size()) {
            printf("wrong size\n");
            return false;
        }

        for (auto &pair :a) {
            if (b.find(pair.first) == b.end()) {
                printf("wrong size 1\n");
                return false;
            }

            if (pair.second.first != b.at(pair.first).first || pair.second.second != b.at(pair.first).second) {
                printf("wrong size 2\n");
                return false;
            }
        }
        return true;
    }

    void
    checkInitialize(FmuContainerCoreTestProxy::State pre, FmuContainerCoreTestProxy::State post, bool expectInitOk) {

        FmuContainerCoreTestProxy c(pre);
        //c.setVerbose(true);
        cout << "\n######################## PRE  ########################" << endl;
        c.show();

        bool initSuccess = c.initialize();

        cout << "######################## POST ########################" << endl;
        c.show();

        //if (post.currentData != c.getCurrentData() || post.incomingUnprocessed != c.getIncomingUnprocessed()) {
        cout << "######################## Expected POST ########################" << endl;
        FmuContainerCoreTestProxy tmp(post);
        tmp.show();
        //}

        ASSERT_EQ(initSuccess, expectInitOk) << "Initialisation error";

        ASSERT_TRUE(post.maxAge == c.getMaxAge()) << "Max age must match";


        ASSERT_TRUE(post.currentData == c.getCurrentData()) << "State must match";
        ASSERT_TRUE(post.incomingUnprocessed == c.getIncomingUnprocessed()) << "Incoming unprocessed  must match";
        ASSERT_TRUE(post.startOffsetTime == c.getStartOffsetTime()) << "Start time offset  must match";


    }

    void
    checkStep(FmuContainerCoreTestProxy::State pre, FmuContainerCoreTestProxy::State post, double time,
              bool expectStepOk) {

        FmuContainerCoreTestProxy c(pre);
        //c.setVerbose(true);
        cout << "\n######################## PRE  ########################" << endl;
        c.show();

        bool initSuccess = c.process(time);

        cout << "######################## POST ########################" << endl;
        c.show();

        //if (post.currentData != c.getCurrentData() || post.incomingUnprocessed != c.getIncomingUnprocessed()) {
        cout << "######################## Expected POST ########################" << endl;
        FmuContainerCoreTestProxy tmp(post);
        tmp.show();
        //}

        ASSERT_EQ(initSuccess, expectStepOk) << "dostep error";

        ASSERT_TRUE(post.maxAge == c.getMaxAge()) << "Max age must match";


        ASSERT_TRUE(post.currentData == c.getCurrentData()) << "State must match";
        ASSERT_TRUE(post.incomingUnprocessed == c.getIncomingUnprocessed()) << "Incoming unprocessed  must match";
        ASSERT_TRUE(post.startOffsetTime == c.getStartOffsetTime()) << "Start time offset  must match";


    }


    TEST(FmuContainerCore, Initialize1
    ) {
        int sv1 = 1;

        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        FmuContainerCoreTestProxy::State pre = {
                .maxAge=std::chrono::milliseconds(0),
                .lookahead={{sv1, 1}},
                .incomingUnprocessed={{sv1, {std::make_pair(valueTimeZero + std::chrono::milliseconds(0), 1)}}}


        };

        FmuContainerCoreTestProxy::State post = {
                .maxAge=std::chrono::milliseconds(0),
                .lookahead={{sv1, 1}},
                .incomingUnprocessed={},
                .incomingLookahead={},
                .currentData = {{sv1, std::make_pair(valueTimeZero + std::chrono::milliseconds(0), 1)}}


        };


        checkInitialize(pre, post, true);
    }

    TEST(FmuContainerCore, BasicOk
    ) {

        FmuContainerCore::ScalarVariableId svId1 = 1;

        std::map<FmuContainerCore::ScalarVariableId, int> lookahead = {{svId1, 1}};

        std::chrono::milliseconds maxAge = std::chrono::milliseconds(0);


        FmuContainerCoreTestProxy c(maxAge, lookahead);


        ASSERT_FALSE(c.initialize()) << "Initialization Should fail";

        date::sys_time<std::chrono::milliseconds> startTime;

        std::stringstream startTimeStamp;
        startTimeStamp << startTime;
        c.add(svId1, std::make_pair(startTime, ScalarVariableBaseValue(1)));

        ASSERT_TRUE(c.initialize()) << "Initialization Should NOT fail";

        date::sys_time<std::chrono::milliseconds> t;
        for (int i = 0; i < 5; i++) {

            t += std::chrono::milliseconds(1);
            c.add(svId1, std::make_pair(t, ScalarVariableBaseValue(i + 1)));
        }
        c.show();
        bool ok = c.process(2);
        c.show();

        cout << "OK " << ok << endl;
    }


    TEST(FmuContainerCoreTla, InitOk
    ) {
        int sv1 = 1;
        int sv0 = 0;

        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        FmuContainerCoreTestProxy::State pre = {
                .maxAge=std::chrono::milliseconds(0),
                .lookahead={{sv0, 1},
                            {sv1, 1}},
                .incomingUnprocessed={{sv0, {std::make_pair(valueTimeZero + std::chrono::milliseconds(2), 2)}},
                                      {sv1, {std::make_pair(valueTimeZero + std::chrono::milliseconds(2),
                                                            2), std::make_pair(
                                              valueTimeZero + std::chrono::milliseconds(1), 1)}}}


        };

        FmuContainerCoreTestProxy::State post = {
                .maxAge=std::chrono::milliseconds(0),
                .lookahead={{sv0, 1},
                            {sv1, 1}},
                .incomingUnprocessed={
                        {sv1, {std::make_pair(
                                valueTimeZero + std::chrono::milliseconds(1), 1)}}},
                .incomingLookahead={{sv0, {std::make_pair(valueTimeZero + std::chrono::milliseconds(2), 2)}},
                                    {sv1, {std::make_pair(valueTimeZero + std::chrono::milliseconds(2),
                                                          2)}}},
                .currentData = {{sv0, std::make_pair(valueTimeZero + std::chrono::milliseconds(2), 2)},
                                {sv1, std::make_pair(valueTimeZero + std::chrono::milliseconds(2), 2)}},
                .startOffsetTime=valueTimeZero + std::chrono::milliseconds(2)


        };

        checkInitialize(pre, post, true);

    }

    FmuContainerCore::TimedScalarBasicValue V(int time, int val) {
        date::sys_time<std::chrono::milliseconds> valueTimeZero;
        return std::make_pair(valueTimeZero + std::chrono::milliseconds(time), val);
    }

    TEST(FmuContainerCoreTla, InitOkDoStepOk
    ) {
        int sv1 = 1;
        int sv0 = 0;

        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        FmuContainerCoreTestProxy::State pre = {
                .maxAge=std::chrono::milliseconds(1),
                .lookahead={{sv0, 1},
                            {sv1, 2}},
                .incomingUnprocessed={{sv0, {V(1, 1), V(2, 2)}},
                                      {sv1, {V(1, 1), V(2, 2)}}}


        };

        FmuContainerCoreTestProxy::State post = {
                .maxAge=std::chrono::milliseconds(1),
                .lookahead={{sv0, 1},
                            {sv1, 2}},
                .incomingUnprocessed={{sv0, {V(2, 2)}}},
                .incomingLookahead={},
//                .incomingLookahead={
//                    { sv0, {V(1, 1)}},
//                    { sv1, {V(2, 2)}}
//                },
                .currentData = {
                        {sv0, V(1, 1)},
                        {sv1, V(2, 2)}
                },
                .startOffsetTime=valueTimeZero + std::chrono::milliseconds(2)


        };

        checkInitialize(pre, post, true);

        FmuContainerCoreTestProxy::State postDoStep = {
                .maxAge=std::chrono::milliseconds(1),
                .lookahead={{sv0, 1},
                            {sv1, 2}},
                .incomingUnprocessed={},
                .incomingLookahead={},
//                .incomingUnprocessed={{sv0, {V(2, 2)}}},
//                .incomingLookahead={
//                    { sv0, {V(1, 1)}},
//                    { sv1, {V(2, 2)}}
//                },
                .currentData = {
                        {sv0, V(2, 2)},
                        {sv1, V(2, 2)}
                },
                .startOffsetTime=valueTimeZero + std::chrono::milliseconds(2)


        };


        checkStep(post, postDoStep, 1, true);
    }

}