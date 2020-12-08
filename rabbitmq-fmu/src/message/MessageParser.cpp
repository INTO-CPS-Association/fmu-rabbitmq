//
// Created by Kenneth Guldbrandt Lausdahl on 12/12/2019.
//

#include "MessageParser.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include "Iso8601TimeParser.h"

using namespace std;
using namespace rapidjson;
using SvType = ModelDescriptionParser::ScalarVariable::SvType;


bool
MessageParser::parse(map<string, ModelDescriptionParser::ScalarVariable> *nameToValueReference, const char *json,
                     DataPoint *output) {

    DataPoint result;


    Document d;
    d.Parse(json);
    bool hasData = false;

    for (Value::ConstMemberIterator itr = d.MemberBegin();
         itr != d.MemberEnd(); ++itr) {
        auto memberName = itr->name.GetString();

        if(string(memberName).rfind("internal_",0)==0 || string(memberName).rfind("simAtTime",0)==0)
        {
            continue;
        }

        if (std::string("time") == memberName && d["time"].IsString()) {
            const char *timeString = d["time"].GetString();
            result.time = Iso8601::parseIso8601ToMilliseconds(std::string(timeString));

        } else {

            if ((*nameToValueReference).find(memberName) == (*nameToValueReference).end()) {
                // not found
                cout << "Input data contains unknown member " << memberName << endl;
            } else {
                hasData = true;
                auto sv = (*nameToValueReference)[memberName];

                cout << "Input data contains KNOWN member " << memberName << endl;
                switch (sv.type) {
                    case SvType::Integer:
                        if (d[memberName].IsInt()){
                            result.integerValues[sv.valueReference] = d[memberName].GetInt();
                        }
                        else if (d[memberName].IsDouble()){
                            result.integerValues[sv.valueReference] = (int) d[memberName].GetDouble();
                        }
                        else{
                            cout << "[ERROR]: Bad Type for "<< memberName << ". Expected int or double (to be cast as int)." << endl;
                        }
                        break;
                    case SvType::Real:
                        if (d[memberName].IsDouble() || d[memberName].IsInt()){
                            result.doubleValues[sv.valueReference] = d[memberName].GetDouble();
                        }
                        else{
                            cout << "[ERROR]: Bad Type for "<< memberName << ". Expected double or int to be cast as double." << endl;
                        }
//                        cout << "Got message with double name='"<<memberName<<"' ref="<<sv.valueReference<<"' value = "<<result.doubleValues[sv.valueReference]<<endl;
                        break;
                    case SvType::Boolean:
                        if (d[memberName].IsBool()){
                            result.booleanValues[sv.valueReference] = d[memberName].GetBool();
                        }
                        else{
                            cout << "[ERROR]: Bad Type for "<< memberName << ". Expected bool." << endl;
                        }
                        break;
                    case SvType::String:
                        if (d[memberName].IsString()){
                            result.stringValues[sv.valueReference] = string(d[memberName].GetString());
                        }
                        else{
                            cout << "[ERROR]: Bad Type for "<< memberName << ". Expected string." << endl;
                        }
                        break;
                }

            }

        }
    }
    //Add faux assignment for the time_discrepancy output
    if (! ((*nameToValueReference).find("time_discrepancy") == (*nameToValueReference).end()) ){
        auto sv = (*nameToValueReference)["time_discrepancy"];
        result.doubleValues[sv.valueReference] = 0.4242;
    }
    if (! ((*nameToValueReference).find("simtime_discrepancy") == (*nameToValueReference).end()) ){
        auto sv = (*nameToValueReference)["simtime_discrepancy"];
        result.doubleValues[sv.valueReference] = 0.4243;
    }
     *output = result;
   return hasData && d.HasMember("time");



}

bool
MessageParser::parseSystemHealthMessage(date::sys_time<std::chrono::milliseconds> &simTime, date::sys_time<std::chrono::milliseconds> &rTime, const char *json){
    Document d;
    d.Parse(json);
    //cout << "JSON to parse: " << json << endl;
    for (Value::ConstMemberIterator itr = d.MemberBegin();
         itr != d.MemberEnd(); ++itr) {
        auto memberName = itr->name.GetString();
            
        if (std::string("rtime") == memberName && d["rtime"].IsString()) {
            const char *timeString = d["rtime"].GetString();
            rTime = Iso8601::parseIso8601ToMilliseconds(std::string(timeString));

        } else if (std::string("cosimtime") == memberName && d["cosimtime"].IsString()) {
            const char *timeString = d["cosimtime"].GetString();
            simTime = Iso8601::parseIso8601ToMilliseconds(std::string(timeString));

        }
        else{
            // not known
            cout << "Input data contains unknown member " << memberName << endl;
            break;
        }
    }
    return d.HasMember("rtime") && d.HasMember("cosimtime");
}