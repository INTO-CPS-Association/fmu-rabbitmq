#include "gtest/gtest.h"

#include <string>
#include <iostream>
#include <locale>
#include "Iso8601Time.h"
#include "date/date.h"
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
using namespace std;
using namespace Iso8601;

namespace {
    TEST(Timestamp, External) {
        using namespace date;
        cout << parseIso8601String("2014-11-12T19:12:14.505Z") << '\n';
        cout << parseIso8601String("2014-11-12T12:12:14.505-05:00") << '\n';

        cout << "This one is not working " << parseIso8601String("2019-01-04T16:41:24.100000+05:00") << '\n';

        auto diff = parseIso8601String("2014-11-12T19:12:14.505Z") - parseIso8601String("2014-11-12T19:12:13.505Z");

        using namespace date;
        using namespace std;
        using namespace std::chrono;
        using dsecs = sys_time<duration<double> >;
        cout << "My test of 1 s " << floor<milliseconds>(diff) << endl;
    }


    TEST(Timestamp, checksConvertTimeToString) {
        //GTEST_SKIP();
        cout << "Testing: FmuContainerCore::convertTimeToString " << endl;
        std::chrono::milliseconds maxAge(1000);
        // std::map<FmuContainerCore::ScalarVariableId, int> lookAhead;
        // FmuContainerCore test = FmuContainerCore(maxAge, lookAhead);

        long long milliSecondsSinceEpoch[] = {
            (long long) 100.0, (long long) 200.0, (long long) 300.0, (long long) 400.0, (long long) 500.0,
            (long long) 600.0, (long long) 700.0, (long long) 800.0, (long long) 900.0, (long long) 1000.0
        };
        string message[] = {
            "1970-01-01T00:00:00.100000Z", "1970-01-01T00:00:00.200000Z", "1970-01-01T00:00:00.300000Z",
            "1970-01-01T00:00:00.400000Z", "1970-01-01T00:00:00.500000Z", "1970-01-01T00:00:00.600000Z",
            "1970-01-01T00:00:00.700000Z", "1970-01-01T00:00:00.800000Z", "1970-01-01T00:00:00.900000Z",
            "1970-01-01T00:00:01.000000Z"
        };

        for (int i = 0; i < (sizeof(milliSecondsSinceEpoch) / sizeof(*milliSecondsSinceEpoch)); i++) {
            string out;


            auto tp = system_clock::time_point{milliseconds(milliSecondsSinceEpoch[i])};
            out = toIso8601ToString(tp);
            cout << "EPOC " << milliSecondsSinceEpoch[i] << endl;
            cout << "Calculated : " << out << endl << "Expected:    " << message[i] << endl << endl;
            cout << "milis exp:   " << parseIso8601String(message[i]).time_since_epoch() << endl;
            cout << "milis:       " << parseIso8601String(out).time_since_epoch() << endl;


            ASSERT_STREQ(out.c_str(), message[i].c_str());
        }
    }
}
