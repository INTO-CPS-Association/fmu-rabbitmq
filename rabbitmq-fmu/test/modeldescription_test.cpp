#include "gtest/gtest.h"
#include "modeldescription/ModelDescriptionParser.h"

using namespace std;
using SvType = ModelDescriptionParser::ScalarVariable::SvType;
namespace {
    TEST(ModelDescriptionParser, Basic
    ) {
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
}