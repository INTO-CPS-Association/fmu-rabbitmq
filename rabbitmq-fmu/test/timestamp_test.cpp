#include "gtest/gtest.h"

#include <string>
#include <iostream>
#include <locale>
#include "Iso8601TimeParser.h"
#include "date/date.h"

using namespace std;
using namespace Iso8601;
namespace {

    TEST(Timestamp, External
    ) {
        using namespace date;
        cout << parseIso8601ToMilliseconds("2014-11-12T19:12:14.505Z") << '\n';
        cout << parseIso8601ToMilliseconds("2014-11-12T12:12:14.505-5:00") << '\n';

        cout << "This one is not working " << parseIso8601ToMilliseconds("2019-01-04T16:41:24.100000+05:00") << '\n';

        auto diff = parseIso8601ToMilliseconds("2014-11-12T19:12:14.505Z") -
                    parseIso8601ToMilliseconds("2014-11-12T19:12:13.505Z");

        using namespace date;
        using namespace std;
        using namespace std::chrono;
        using dsecs = sys_time<duration<double>>;
        cout << "My test of 1 s " << floor<milliseconds>(diff) << endl;


    }


}