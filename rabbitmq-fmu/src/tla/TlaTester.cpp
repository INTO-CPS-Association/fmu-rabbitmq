//
// Created by Kenneth Guldbrandt Lausdahl on 26/03/2020.
//
#include <iostream>
#include <vector>
#include <fstream>


#include <filesystem>


#include <algorithm>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <iostream>

#include "FmuContainerCore.h"


#include <iostream>

#include "date/date.h"


namespace fs = std::filesystem;

using namespace std;
using namespace rapidjson;
using namespace date;

namespace tla {

//    static bool operator==(const pair<date::sys_time<std::chrono::milliseconds>, ScalarVariableBaseValue>& a1, const pair<date::sys_time<std::chrono::milliseconds>, ScalarVariableBaseValue>& a2) {
//        return false;
//    }

    class InputParser {
    public:
        InputParser(int &argc, char **argv) {
            for (int i = 1; i < argc; ++i)
                this->tokens.push_back(std::string(argv[i]));
        }

        /// @author iain
        const std::string &getCmdOption(const std::string &option) const {
            std::vector<std::string>::const_iterator itr;
            itr = std::find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()) {
                return *itr;
            }
            static const std::string empty_string("");
            return empty_string;
        }

        /// @author iain
        bool cmdOptionExists(const std::string &option) const {
            return std::find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }

    private:
        std::vector<std::string> tokens;
    };


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

        void showValue(const char *prefix, FmuContainerCore::TimedScalarBasicValue val) {
            cout << prefix << "Time: " << val.first.time_since_epoch().count() << " Value: ";

            switch (val.second.b.type) {
                case TU_STRING:
                    cout << val.second.s.s;
                    break;
                case TU_INT:
                    cout << val.second.i.i;
                    break;
                case TU_BOOL:
                    cout << val.second.b.b;
                    break;
                case TU_DOUBLE:
                    cout << val.second.d.d;
                    break;

            }

            cout << endl;
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


        void show(const char *tag) {
            cout << "------------------------------ INFO " << tag << "------------------------------" << endl;
            cout << "Max age:              " << this->maxAge << endl;
            cout << "StartTime:            " << this->startOffsetTime << endl;
            cout << "Lookahead ids:        [";
            for (auto &id:lookahead) {
                cout << id.first << "->" << id.second << " ";
            }
            cout << "]" << endl;
            cout << "Incoming Unprocessed:  " << endl;
            for (auto &pair: incomingUnprocessed) {
                cout << "\tId: " << pair.first << endl;
                for (auto &val: pair.second) {
                    showValue("\t\t", val);

                }
            }

            cout << "IncomingLookahead:    " << endl;
            for (auto &pair: incomingLookahead) {
                cout << "\tId: " << pair.first << endl;
                for (auto &val: pair.second) {
                    showValue("\t\t", val);

                }
            }

            cout << "Data:                 " << endl;
            for (auto &pair: currentData) {
                cout << "\tId: " << pair.first;

                showValue(" ", pair.second);


            }
            cout << endl;
//            cout << "------------------------------------------------------------------" << endl;
        }
    };

    std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>>
    parseQueueValues(Value &doc, const char *queueName) {
        std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>> lookahead;
        date::sys_time<std::chrono::milliseconds> valueTimeZero;

//        cout << "Parsing " << queueName << endl;
        if (doc.HasMember(queueName)) {
            auto &lookaheadVal = doc[queueName];

            for (Value::ConstMemberIterator itr = lookaheadVal.MemberBegin();
                 itr != lookaheadVal.MemberEnd(); ++itr) {

                auto keyStr = itr->name.GetString();

                stringstream keyStream(keyStr);

                unsigned int key = 0;
                keyStream >> key;

                cout << key << endl;

//                list<FmuContainerCore::TimedScalarBasicValue> list;
                for (auto &val: itr->value.GetArray()) {
                    lookahead[key].push_front(
                            std::make_pair(valueTimeZero + std::chrono::milliseconds(val.GetInt()), val.GetInt()));
                }

                //  lookahead[key] = list;
            }

        }
        return lookahead;
    }

    FmuContainerCoreTestProxy::State createState(Value &doc) {

        std::map<FmuContainerCore::ScalarVariableId, int> lookahead;
        if (doc.HasMember("lookahead")) {
            auto &lookaheadVal = doc["lookahead"];

            for (Value::ConstMemberIterator itr = lookaheadVal.MemberBegin();
                 itr != lookaheadVal.MemberEnd(); ++itr) {

                auto keyStr = itr->name.GetString();

                stringstream keyStream(keyStr);

                int key = 0;
                keyStream >> key;


                auto val = itr->value.GetInt();

                lookahead[key] = val;
            }

        }


        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        FmuContainerCoreTestProxy::State pre = {
                .maxAge=std::chrono::milliseconds(doc["maxAge"].GetInt()),
                .lookahead=lookahead,
                .incomingUnprocessed=parseQueueValues(doc, "incomingUnprocessed"),
                .incomingLookahead=parseQueueValues(doc, "incomingLookahead")

        };

        return pre;
    }

    bool check(FmuContainerCoreTestProxy &fcc, FmuContainerCoreTestProxy::State &post) {
        if (post.currentData != fcc.getData()) {
            cout << "Current state does not match" << endl;
            return false;
        } else if (post.lookahead != fcc.getLookahead()) {
            cout << "lookahead does not match" << endl;
            return false;
        } else if (post.incomingLookahead != fcc.getIncomingLookahead()) {
            cout << "incomingLookahead does not match" << endl;
            return false;
        } else if (post.incomingUnprocessed != fcc.getIncomingUnprocessed()) {
            cout << "incomingUnprocessed does not match" << endl;
            return false;
        }
        return true;
    }


    bool processTest(Document &doc) {

        if (!doc.IsObject() || !doc.HasMember("meta") || !doc.HasMember("action") || !doc.HasMember("pre") ||
            !doc.HasMember("post")) {
            cout << "\t##  " << "Parse failed" << endl;
            return false;
        }


        Value &meta = doc["meta"];
        Value &action = doc["action"];
        Value &pre = doc["pre"];
        Value &post = doc["post"];

        cout << "\t##  Meta:" << meta.GetString() << endl;
        cout << "\t##  Action: " << action.GetString() << endl;

        auto preState = createState(pre);
        auto postState = createState(post);

        FmuContainerCoreTestProxy fcc(preState);
//        cout << "######################## PRE  ########################" << endl;
        fcc.show("PRE ");

        cout << ">> " << action.GetString() << endl;

        auto res = false;

        if (string("dostep") == action.GetString()) {

            //FIXME what time?
            res = fcc.process(0);
        } else if (string("initialize").compare(action.GetString()) == 0) {
            res = fcc.initialize();
        }


        fcc.show("POST ");

        return post["fmuInitialized"].GetBool() == res;

        return check(fcc, postState);


    }


}

using namespace tla;

int main(int argc, char **argv) {
    cout << "tla" << endl;
    std::vector<std::string> testFiles;

    InputParser input(argc, argv);
    if (input.cmdOptionExists("-h")) {
        // Do stuff
        cout << "usage rabbitmq-tla [args]" << endl;
        cout << " -h     help" << endl;
        cout << " -s     path to a folder containing *json test files" << endl;
        cout << " -t     path to a single test *.json file" << endl;
        return 0;
    }
    const std::string &filename = input.getCmdOption("-t");
    if (!filename.empty()) {
        cout << "Adding test: " << filename << endl;
        testFiles.push_back(filename);
    }

    const std::string &searchPath = input.getCmdOption("-s");
    if (!searchPath.empty()) {
        cout << "Searching for tests in path: " << searchPath << endl;
        for (const auto &entry : fs::directory_iterator(searchPath)) {
            auto fn = entry.path().generic_string();
            if (fn.substr(fn.find_last_of(".") + 1) == "json") {
                cout << "Adding test: " << fn << endl;
                testFiles.push_back(fn);

            }
        }
    }

    cout << "Testing..." << endl;

    for (auto &path:testFiles) {

        cout << "############################## " << path << " ##############################" << endl;
        cout << "\t## Test" << path << endl;

        ifstream f(path.c_str());
        if (!f.good()) {
            cout << "\t##  " << " File does not exist" << endl;
            cout << "\t##  " << " FAIL" << endl;
            continue;
        }

        std::ifstream t(path.c_str());
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());

        Document d;
        d.Parse(str.c_str());
        if (processTest(d)) {
            cout << "\t##  " << " PASSED" << endl;
        } else {
            cout << "\t##  " << " FAIL" << endl;
        }
    }

    return 0;
}