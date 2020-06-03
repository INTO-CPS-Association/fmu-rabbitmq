#include "gtest/gtest.h"
#include "message/MessageParser.h"
#include "modeldescription/ModelDescriptionParser.h"
#include <iostream>

#include <fstream>

using namespace std;

namespace {


    void printDataPoint(DataPoint dp) {
        cout << "###################################" << endl << "Data point info" << endl;


        for (auto &it : dp.doubleValues) {
            cout << "Double " << it.first << " = " << it.second << endl;
        }
        for (auto &it : dp.integerValues) {
            cout << "Int     " << it.first << " = " << it.second << endl;
        }
        for (auto &it : dp.booleanValues) {
            cout << "Bool   " << it.first << " = " << it.second << endl;
        }
        for (auto &it : dp.stringValues) {
            cout << "String " << it.first << " = " << it.second << endl;
        }

        cout << "###################################" << endl;
    }

    TEST(MessageParser, ParseTest
    ) {
        ModelDescriptionParser mdParser;
        auto svs = mdParser.parse(string("modelDescription.xml"));


        MessageParser messageParser;


        std::ifstream t("data_modeldescription.json");
        std::stringstream buffer;
        buffer << t.rdbuf();
        std::cout << buffer.str() << std::endl;

        auto json = buffer.str().c_str();

        DataPoint dp;
        auto success = messageParser.parse(&svs, json, &dp);

        printDataPoint(dp);

    }
}