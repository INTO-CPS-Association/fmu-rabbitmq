//
// Created by Kenneth Guldbrandt Lausdahl on 26/03/2020.
//
#include <iostream>
#include <vector>
#include <fstream>

#include <algorithm>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "FmuContainerCore.h"
#include "date/date.h"
#include <dirent.h>


using namespace std;
using namespace rapidjson;
using namespace date;

namespace tla {

    static bool verbose = false;
    static bool verbose2 = false;

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

        static void showValue(const char *prefix, FmuContainerCore::TimedScalarBasicValue val) {
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

        std::map<ScalarVariableId, list<TimedScalarBasicValue>> incommingLookaheadAndStateMap;

        const map<ScalarVariableId, list<TimedScalarBasicValue>> &getIncomingLookaheadAndState() {

            this->incommingLookaheadAndStateMap.clear();

            for (auto &pair: this->currentData) {
                auto id = pair.first;
                this->incommingLookaheadAndStateMap[id].push_back(pair.second);
            }


            for (auto &pair: this->incomingLookahead) {
                auto id = pair.first;

                this->incommingLookaheadAndStateMap.insert_or_assign(pair.first, pair.second);

                this->incommingLookaheadAndStateMap[id].sort(
                        [](const TimedScalarBasicValue &a, const TimedScalarBasicValue &b) {
                            return a.first < b.first;
                        });
            }


            return this->incommingLookaheadAndStateMap;

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

        static void showData(std::map<ScalarVariableId, TimedScalarBasicValue> currentData, const char *tag = "") {
            cout << "Data: " << tag << "                " << endl;
            for (auto &pair: currentData) {
                cout << "\tId: " << pair.first;

                showValue(" ", pair.second);


            }
            cout << endl;
        }

        void show(const char *tag) {
            date::sys_time<std::chrono::milliseconds> valueTimeZero;
            cout << "------------------------------ INFO " << tag << "------------------------------" << endl;
            cout << "Max age:              " << this->maxAge << endl;
            cout << "StartTime:            " << this->startOffsetTime << " ( "
                 << (this->startOffsetTime - valueTimeZero).count() << " )" << endl;
            cout << "Lookahead ids:        [ ";
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

            showData(currentData);
//            cout << "------------------------------------------------------------------" << endl;
        }
    };

    std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>>
    parseQueueValues(Value &doc, const char *queueName) {
        std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>> lookahead;
        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        if (verbose) {
            cout << queueName << endl;
        }

//        cout << "Parsing " << queueName << endl;
        if (doc.HasMember(queueName)) {
            auto &lookaheadVal = doc[queueName];

            for (Value::ConstMemberIterator itr = lookaheadVal.MemberBegin();
                 itr != lookaheadVal.MemberEnd(); ++itr) {

                auto keyStr = itr->name.GetString();

                stringstream keyStream(keyStr);

                unsigned int key = 0;
                keyStream >> key;
                if (verbose) {
                    cout << "Key " << key << endl;
                }

//                list<FmuContainerCore::TimedScalarBasicValue> list;

                for (auto &val: itr->value.GetArray()) {

                    if (val.IsArray() && val.GetInt() == 2) {

                        if (verbose) {
                            cout << "\t(" << val.GetArray()[0].GetInt() << " , " << val.GetArray()[1].GetInt() << " )"
                                 << endl;
                        }

                        lookahead[key].push_back(
                                std::make_pair(valueTimeZero + std::chrono::milliseconds(val.GetArray()[0].GetInt()),
                                               val.GetArray()[1].GetInt()));
                    }
                }

                //  lookahead[key] = list;
            }

        }
        return lookahead;
    }

    std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>>
    parseQueueValuesIncomming(Value &doc, const char *queueName) {
        std::map<FmuContainerCore::ScalarVariableId, list<FmuContainerCore::TimedScalarBasicValue>> lookahead;
        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        if (verbose) {
            cout << queueName << endl;
        }

//        cout << "Parsing " << queueName << endl;
        if (doc.HasMember(queueName)) {
            auto &lookaheadVal = doc[queueName];

            for (Value::ConstMemberIterator itr = lookaheadVal.MemberBegin();
                 itr != lookaheadVal.MemberEnd(); ++itr) {


                auto keyStr = itr->name.GetString();

                stringstream keyStream(keyStr);

                unsigned int key = 0;
                keyStream >> key;
                if (verbose) {
                    cout << "Key " << key << endl;
                }

                auto visiblen = itr->value["visiblen"].GetInt();

                int visibleNcount = 0;
                for (auto &val: itr->value["values"].GetArray()) {
                    if (visibleNcount >= visiblen) {
                        break;
                    }
                    visibleNcount++;

                    if (val.IsArray() && val.GetInt() == 2) {

                        if (verbose) {
                            cout << "\t(" << val.GetArray()[0].GetInt() << " , " << val.GetArray()[1].GetInt() << " )"
                                 << endl;
                        }

                        lookahead[key].push_back(
                                std::make_pair(valueTimeZero + std::chrono::milliseconds(val.GetArray()[0].GetInt()),
                                               val.GetArray()[1].GetInt()));
                    }
                }

                //  lookahead[key] = list;
            }

        }
        return lookahead;
    }

    std::map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue>
    parseCurrentValues(Value &doc, const char *queueName) {
        std::map<FmuContainerCore::ScalarVariableId, FmuContainerCore::TimedScalarBasicValue> currenntData;
        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        if (verbose) {
            cout << queueName << endl;
        }

//        cout << "Parsing " << queueName << endl;
        if (doc.HasMember(queueName)) {
            auto &lookaheadVal = doc[queueName];

            for (Value::ConstMemberIterator itr = lookaheadVal.MemberBegin();
                 itr != lookaheadVal.MemberEnd(); ++itr) {

                auto keyStr = itr->name.GetString();

                stringstream keyStream(keyStr);

                unsigned int key = 0;
                keyStream >> key;
                if (verbose) {
                    cout << "Key " << key << endl;
                }
//                list<FmuContainerCore::TimedScalarBasicValue> list;
                auto &val = itr->value;


                if (val.IsArray() && val.GetInt() == 2) {

                    if (verbose) {
                        cout << "\t(" << val.GetArray()[0].GetInt() << " , " << val.GetArray()[1].GetInt() << " )"
                             << endl;
                    }

                    //no signal value. Special case from the model to signal that there could be a value
                    if (val.GetArray()[0].GetInt() == 0 && val.GetArray()[1].GetInt() == 0) {
                        continue;
                    }


                    auto f = std::make_pair(valueTimeZero + std::chrono::milliseconds(val.GetArray()[0].GetInt()),
                                            ScalarVariableBaseValue((int) val.GetArray()[1].GetInt()));
//                    currenntData.
//                    currenntData.at(key).first = f.first;



                    currenntData.insert(std::make_pair(key, f));
                }


                //  lookahead[key] = list;
            }

        }
        return currenntData;
    }

    FmuContainerCoreTestProxy::State createState(Value &doc) {

        int globalLookahead = 0;
        if (doc.HasMember("lookahead")) {
            auto &lookaheadVal = doc["lookahead"];
            globalLookahead = lookaheadVal.GetInt();
        }


        date::sys_time<std::chrono::milliseconds> valueTimeZero;

        auto incomingUnprocessed = parseQueueValuesIncomming(doc, "incomingUnprocessed");

        std::map<FmuContainerCore::ScalarVariableId, int> lookahead;


        for (auto &pair: incomingUnprocessed) {
            lookahead[pair.first] = globalLookahead;
        }

        FmuContainerCoreTestProxy::State pre = {
                .maxAge=std::chrono::milliseconds(doc["maxAge"].GetInt()),
                .lookahead=lookahead,
                .incomingUnprocessed=incomingUnprocessed,
                .incomingLookahead=parseQueueValues(doc, "incomingLookahead"),
                .currentData=parseCurrentValues(doc, "currentData"),
                .startOffsetTime =valueTimeZero + std::chrono::milliseconds(doc["startOffsetTime"].GetInt())
        };

        return pre;
    }

    bool check(FmuContainerCoreTestProxy &fcc, FmuContainerCoreTestProxy::State &expectedState, bool blocked) {
        if (!blocked && expectedState.currentData != fcc.getData()) {

            if (verbose) {
                cerr << "Current state does not match" << endl;

                FmuContainerCoreTestProxy::showData(fcc.getData(), "Actual");
                FmuContainerCoreTestProxy::showData(expectedState.currentData, "Expected");
            }
            return false;
        }/* else if (post.lookahead != fcc.getLookahead()) {
            cerr << "lookahead does not match" << endl;
            return false;
        }*//* else if (!blocked && post.incomingLookahead != fcc.getIncomingLookahead()) {
            cerr << "incomingLookahead does not match" << endl;
            return false;
        }*/

        if (expectedState.incomingLookahead != fcc.getIncomingLookaheadAndState()) {
            if (verbose) {
                cerr << "incomingLookahead does not match" << endl;
            }
            return false;
        } else if (expectedState.incomingUnprocessed != fcc.getIncomingUnprocessed()) {
            if (verbose) {
                cerr << "incomingUnprocessed does not match" << endl;
            }
            return false;
        }
        return true;
    }


    bool processTest(Document &doc) {

        if (!doc.IsObject() || !doc.HasMember("meta") || !doc.HasMember("action") || !doc.HasMember("preState") ||
            !doc.HasMember("postState")) {
            cout << "\t##  " << "Parse failed" << endl;
            return false;
        }


        Value &meta = doc["meta"];
        Value &action = doc["action"];
        Value &pre = doc["preState"];
        Value &post = doc["postState"];

        if (verbose) {
            cout << "\t##  Meta:" << meta.GetString() << endl;
            cout << "\t##  Action: " << action.GetString() << endl;
        }

        auto preState = createState(pre);
        auto postState = createState(post);

        FmuContainerCoreTestProxy fcc(preState);
        fcc.setVerbose(verbose2);
//        cout << "######################## PRE  ########################" << endl;
        if (verbose) {
            fcc.show("PRE ");
        }

        cout << ">> " << action.GetString() << endl;

        auto res = false;

        if (string("dostep") == action.GetString()) {

            auto targetTime =pre["cstime"].GetInt()+ pre["H"].GetInt();

            if (verbose) {
                cout << "STEP to time " << targetTime << endl;
            }

            res = fcc.process(targetTime);
            if (verbose) {
                cout << "DoStep Result = " << res << endl;

            }
            res = !post["blocked"].GetBool() == res;
        } else if (string("initialize").compare(action.GetString()) == 0) {
            res = fcc.initialize();
            if (verbose) {
                cout << "Initiialized: " << res << endl;
            }
            //!blocked &&initialized

            res = (!post["blocked"].GetBool() && post["initialized"].GetBool()) == res;
        }

        if (verbose) {
            fcc.show("POST ");
        }
        return check(fcc, postState, post["blocked"].GetBool()) && res;

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
        cout << " -v     verbose" << endl;
        cout << " -vv    more verbose" << endl;
        return 0;
    }

    verbose = input.cmdOptionExists("-v");
    verbose2 = input.cmdOptionExists("-vv");
    verbose = verbose || verbose2;

    const std::string &filename = input.getCmdOption("-t");
    if (!filename.empty()) {
        if (verbose) {
            cout << "Adding test: " << filename << endl;
        }
        testFiles.push_back(filename);
    }

    const std::string &searchPath = input.getCmdOption("-s");
    if (!searchPath.empty()) {
        if (verbose) {
            cout << "Searching for tests in path: " << searchPath << endl;
        }

        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(searchPath.c_str())) != NULL) {
            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != NULL) {
//                printf ("%s\n", ent->d_name);
                testFiles.push_back(ent->d_name);
            }
            closedir(dir);
        } else {
            /* could not open directory */
            perror("");
            return EXIT_FAILURE;
        }

        /*  for (const auto &entry : fs::directory_iterator(searchPath)) {
              auto fn = entry.path().generic_string();
              if (fn.substr(fn.find_last_of(".") + 1) == "json") {
                  if (verbose) {
                      cout << "Adding test: " << fn << endl;
                  }
                  testFiles.push_back(fn);

              }
          }*/
    }
    if (verbose) {
        cout << "Testing..." << endl;
    }
    int failures = 0;


    for (auto &path:testFiles) {

        //cout << "############################## " << path << " ##############################" << endl;
        if (verbose) {
            cout << "\t## Test: " << path << endl;
        }
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
        auto verdict = processTest(d);


        if (!verbose) {
            cout << path << " ->";
        } else {
            cout << "\t##  ";
        }

        if (verdict) {
            cout << " PASSED" << endl;
        } else {
            cout << " FAIL" << endl;

            failures++;
        }

    }

    cout << "Summery Tests: " << testFiles.size() << " Failures: " << failures << endl;

    return 0;
}