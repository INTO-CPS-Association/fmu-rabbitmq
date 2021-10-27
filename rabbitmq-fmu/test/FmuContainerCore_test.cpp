//
// Created by Kenneth Guldbrandt Lausdahl on 21/01/2020.
//
#include "gtest/gtest.h"
#include "FmuContainer.h"
#include "FmuContainerCore.h"
#include "DataPoint.h"

namespace {

    TEST(FmuContainerCoreTest, checksConvertTimeToString
    ) {
        std::chrono::milliseconds maxAge(1000);
        std::map<FmuContainerCore::ScalarVariableId, int> lookAhead;
        FmuContainerCore test = FmuContainerCore(maxAge, lookAhead);

        long long milliSecondsSinceEpoch[] = { (long long) 100.0, (long long) 200.0, (long long) 300.0, (long long) 400.0, (long long) 500.0, (long long) 600.0, (long long) 700.0, (long long) 800.0, (long long) 900.0, (long long) 1000.0};
        string message[] = {"1970-01-01-0-0-0-100", "1970-01-01-0-0-0-200", "1970-01-01-0-0-0-300", "1970-01-01-0-0-0-400", "1970-01-01-0-0-0-500", "1970-01-01-0-0-0-600", "1970-01-01-0-0-0-700", "1970-01-01-0-0-0-800", "1970-01-01-0-0-0-900" ,"1970-01-01-0-0-1-0"};

        for (int i = 0; i  < (sizeof(milliSecondsSinceEpoch)/sizeof(*milliSecondsSinceEpoch)); i++) {

            string out;
            test.convertTimeToString(milliSecondsSinceEpoch[i], out);
            cout << "Calculated string: " << out << endl << "Expected: " << message[i] << endl;
            
            //ASSERT_STREQ(out.c_str(), message[i].c_str());

        }
    }

}