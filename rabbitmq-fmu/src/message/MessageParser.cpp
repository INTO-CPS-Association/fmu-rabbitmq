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

        if(string(memberName).rfind("internal_",0)==0)
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

                switch (sv.type) {
                    case SvType::Integer:
                        result.integerValues[sv.valueReference] = d[memberName].GetInt();
                        break;
                    case SvType::Real:
                        result.doubleValues[sv.valueReference] = d[memberName].GetDouble();
//                        cout << "Got message with double name='"<<memberName<<"' ref="<<sv.valueReference<<"' value = "<<result.doubleValues[sv.valueReference]<<endl;
                        break;
                    case SvType::Boolean:
                        result.booleanValues[sv.valueReference] = d[memberName].GetBool();
                        break;
                    case SvType::String:
                        result.stringValues[sv.valueReference] = string(d[memberName].GetString());
                        break;
                }

            }

        }
    }
     *output = result;

   return hasData && d.HasMember("time");



}