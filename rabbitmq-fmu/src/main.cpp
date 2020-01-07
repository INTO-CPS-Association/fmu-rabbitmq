#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

#include <fstream>
#include <sstream>

#include <cstdlib>
#include <ctime>
#include <string>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

#include "modeldescription/ModelDescriptionParser.h"

using namespace std;
using SvType = ModelDescriptionParser::ScalarVariable::SvType;
using namespace rapidjson;

inline int ParseInt(const char *value) {
    return std::strtol(value, nullptr, 10);
}

std::time_t ParseISO8601(const std::string &input) {
    constexpr const size_t expectedLength = sizeof("1234-12-12T12:12:12Z") - 1;
    static_assert(expectedLength == 20, "Unexpected ISO 8601 date/time length");

    if (input.length() < expectedLength) {
        return 0;
    }

    std::tm time = {0};
    time.tm_year = ParseInt(&input[0]) - 1900;
    time.tm_mon = ParseInt(&input[5]) - 1;
    time.tm_mday = ParseInt(&input[8]);
    time.tm_hour = ParseInt(&input[11]);
    time.tm_min = ParseInt(&input[14]);
    time.tm_sec = ParseInt(&input[17]);
    time.tm_isdst = 0;
    const int millis = input.length() > 20 ? ParseInt(&input[20]) : 0;
    return timegm(&time) * 1000 + millis;
}

void testMd()
{
    ModelDescriptionParser parser;
    auto map = parser.parse(string("modelDescription.xml"));

    for (auto &it : map) {
        auto sv = it.second;
        cout << it.first << " => " << "ref " << it.second.valueReference << " start value '";
        switch (sv.type) {
            case SvType::Integer:
                cout << sv.i_value;
                break;
            case SvType::Real:
                cout << sv.d_value;
                break;
            case SvType::Boolean:
                cout << sv.b_value;
                break;
            case SvType::String:
                cout << sv.s_value;
                break;
        }

        cout << "'\n";
    }
}

int main() {
    {

     testMd();
     return 0;

        // 1. Parse a JSON string into DOM.
        const char *json = "{\"project\":\"rapidjson\",\"stars\":10}";
        Document d;
        d.Parse(json);

        // 2. Modify it by DOM.
        Value &s = d["stars"];
        s.SetInt(s.GetInt() + 13);

        int k = s.GetInt();

        // 3. Stringify the DOM
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        d.Accept(writer);

        // Output {"project":"rapidjson","stars":11}
        std::cout << buffer.GetString() << std::endl;
    }

    std::ifstream t("data.json");
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::cout << buffer.str() << std::endl;

    Document d;
    d.Parse(buffer.str().c_str());

    if (d.HasMember("time") && d["time"].IsString()) {
        const char *timeString = d["time"].GetString();
        std::cout << "Time is: " << timeString << std::endl;

        std::time_t t = ParseISO8601(std::string(timeString));

        std::tm *ptm = std::localtime(&t);
        char buffer2[32];
// Format: Mo, 15.06.2009 20:20:00
        std::strftime(buffer2, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);

        std::cout << "Time is: " << buffer2 << std::endl;
    }

    return 0;
}
