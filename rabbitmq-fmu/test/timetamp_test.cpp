#include "gtest/gtest.h"

#include <sstream>

#include <cstdlib>
#include <ctime>
#include <string>
#include <clocale>
#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>
#ifdef _WIN32
#define timegm _mkgmtime
#endif

#include "Iso8601TimeParser.h"

using namespace std;

namespace {
//    inline int ParseInt(const char *value) {
//        return std::strtol(value, nullptr, 10);
//    }
//
//    std::time_t ParseISO8601(const std::string &input) {
//        constexpr const size_t expectedLength = sizeof("1234-12-12T12:12:12Z") - 1;
//        static_assert(expectedLength == 20, "Unexpected ISO 8601 date/time length");
//
//        if (input.length() < expectedLength) {
//            return 0;
//        }
//
//        std::tm time = {0};
//        time.tm_year = ParseInt(&input[0]) - 1900;
//        time.tm_mon = ParseInt(&input[5]) - 1;
//        time.tm_mday = ParseInt(&input[8]);
//        time.tm_hour = ParseInt(&input[11]);
//        time.tm_min = ParseInt(&input[14]);
//        time.tm_sec = ParseInt(&input[17]);
//        time.tm_isdst = 0;
//        const int millis = input.length() > 20 ? ParseInt(&input[20]) : 0;
//        return timegm(&time) * 1000 + millis;
//    }



    TEST(Timestamp, Basic
    ) {
        const char *timeString = "2019-12-10T19:01:41+00:00";
        std::cout << "Time is: " << timeString << std::endl;

        std::time_t t = Iso8601::parseIso8601(std::string(timeString));

        std::tm *ptm = std::localtime(&t);
        char buffer2[32];
// Format: Mo, 15.06.2009 20:20:00
        std::strftime(buffer2, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);

        std::cout << "Time is: " << buffer2 << std::endl;
        ASSERT_STREQ("Sun, 06.08.51911 22:03:20",buffer2);
    }


    TEST(Timestamp, other)
    {
//        {
//            // date and time formatting will be English
//            std::setlocale(LC_TIME, "en_US.UTF-8");
//
//            char buf[100];
//            time_t t;
//            struct tm *timeptr, result;
//
//
//            t = time(NULL);
//            timeptr = localtime(&t);
//
//
//            strftime(buf, sizeof(buf), "%a, %d.%m.%Y %H:%M:%S", timeptr);
//
//            cout << buf << endl;
//
//            if (strptime("Sun, 06.08.51911 22:03:20", "%a, %d.%m.%Y %H:%M:%S", &result) == NULL)
//                printf("\nstrptime failed\n");
//            else {
//                printf("tm_hour:  %d\n", result.tm_hour);
//                printf("tm_min:  %d\n", result.tm_min);
//                printf("tm_sec:  %d\n", result.tm_sec);
//                printf("tm_mon:  %d\n", result.tm_mon);
//                printf("tm_mday:  %d\n", result.tm_mday);
//                printf("tm_year:  %d\n", result.tm_year);
//                printf("tm_yday:  %d\n", result.tm_yday);
//                printf("tm_wday:  %d\n", result.tm_wday);
//
//                // strftime(buf,sizeof(buf), "%a, %d.%m.%Y %H:%M:%S", result.);
//
//                cout << buf << endl;
//
//            }
//        }

        std::tm t = {};
        std::istringstream ss("Sun, 06.08.2011 22:03:20");
        ss.imbue(std::locale("en_US.UTF-8"));
        ss >> std::get_time(&t, "%a, %d.%m.%Y %H:%M:%S");
        if (ss.fail()) {
            std::cout << "Parse failed\n";
        } else {
            std::cout << std::put_time(&t, "%c") << '\n';


            std::time_t end_time = std::mktime(&t);
            cout << "Time in epic "<< end_time<<endl;
        }


//        auto start = std::chrono::system_clock::now();
//      //  std::cout << "f(42) = " << fibonacci(42) << '\n';
//        auto end = std::chrono::system_clock::now();
//
//        std::chrono::duration<double> elapsed_seconds = end-start;
//        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
//
//        std::cout << "finished computation at " << std::ctime(&end_time)
//                  << "elapsed time: " << elapsed_seconds.count() << "s\n";
    }
}