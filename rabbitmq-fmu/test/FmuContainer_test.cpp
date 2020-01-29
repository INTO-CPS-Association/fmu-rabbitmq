//
// Created by Kenneth Guldbrandt Lausdahl on 21/01/2020.
//
#include "gtest/gtest.h"
#include "FmuContainer.h"
#include "DataPoint.h"

namespace {

    class TestRabbitMq : public RabbitmqHandler {
        list<string> messages;
    public :
        TestRabbitMq(list<string> messages) : RabbitmqHandler("", 0, "",
                                                              "",
                                                              "",
                                                              ""), messages(messages) {}


        bool open() override { return true; }

        void close() override {}

        void bind() override {}

        bool consume(string &json) override {

            if (messages.empty()) {
                return false;
            }

            json = messages.front();
            messages.pop_front();


            return true;

        }


    };


    class TestFmuContainer : public FmuContainer {
    public:
        TestFmuContainer(const fmi2CallbackFunctions *mFunctions,bool loggingOn, const char *mName,
                         map<string, ModelDescriptionParser::ScalarVariable> &nameToValueReference,
                         DataPoint &initialDataPoint) : FmuContainer(mFunctions,loggingOn, mName,
                                                                     nameToValueReference, initialDataPoint) {}

    private:
        RabbitmqHandler *createCommunicationHandler(const string &hostname, int port, const string &username,
                                                    const string &password, const string &exchange,
                                                    const string &queueBindingKey) {
            cout << "Create custom test communication handler" << endl;
            list<string> list;

//
//            4.4             1.014952800923189
//            4.5                    1.0048817851541365
//            4.6                   0.9958507989788068
//            4.699999999999999       0.9877524389752776

            list.push_back(R"({"time": "2019-01-04T16:00:04.400+02:00", "level": 1.014952800923189})");
            list.push_back(R"({"time": "2019-01-04T16:00:04.500+02:00", "level": 1.0048817851541365})");
            list.push_back(R"({"time": "2019-01-04T16:00:04.600+02:00", "level": 0.9958507989788068})");
            list.push_back(R"({"time": "2019-01-04T16:00:04.700+02:00", "level": 0.9877524389752776})");


//            list.push_back("{\"time\": \"2019-01-04T16:41:24+02:00\", \"level\": 0.0}");
//            list.push_back("{\"time\": \"2019-01-04T16:41:24.100000+02:00\", \"level\": 0.1099999999999999}");
            return new TestRabbitMq(list);
        }
    };


    TEST(FmuContainerTest, InitAndSim
    ) {

        map<string, ModelDescriptionParser::ScalarVariable> svNameRefMap;
        ModelDescriptionParser::ScalarVariable sv;
        sv.name = "level";
        sv.setReal(0);
        sv.valueReference = 0;

        svNameRefMap[sv.name] = sv;

        DataPoint dp;
        dp.stringValues[0] = "";
        dp.stringValues[2] = "";
        dp.stringValues[3] = "";
        dp.stringValues[4] = "";
        dp.integerValues[1] = 8;
        dp.integerValues[5] = 1;

        TestFmuContainer c(NULL, true,"m", svNameRefMap, dp);

        c.initialize();
        c.setup(0);
        double time, step = 0.1;

        double res[] = {1.014952800923189, 1.0048817851541365, 0.9958507989788068, 0.9877524389752776};


        fmi2ValueReference vrefs[1];
        vrefs[0] = sv.valueReference;


        fmi2Real values[1];
        c.getReal(vrefs, 1, values);
        fmi2Real level = values[0];
        ASSERT_DOUBLE_EQ(level, res[0]);

        for (int i = 0; i * step < 0.5; i++) {
            c.step(time, step);


            fmi2Real values[1];
            c.getReal(vrefs, 1, values);
            fmi2Real level = values[0];
            time += step;
            cout << "Level is " << level << " time " << time << endl;

            if (i + 1 < 4) {

                cout << "Comparing Expected: " << res[i + 1] << " = " << level << endl;
                ASSERT_DOUBLE_EQ(level, res[i + 1]);

            }

        }
    }

}