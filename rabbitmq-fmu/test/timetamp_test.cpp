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


#include "Iso8601TimeParser.h"


#include "date/date.h"

using namespace std;

namespace {


    date::sys_time<std::chrono::milliseconds>
    parse8601(std::istream &&is) {
        std::string save;
        is >> save;
        std::istringstream in{save};
        date::sys_time<std::chrono::milliseconds> tp;
        in >> date::parse("%FT%T%Z", tp);
        if (in.fail()) {
            in.clear();
            in.exceptions(std::ios::failbit);
            in.str(save);
            in >> date::parse("%FT%T%Ez", tp);
        }
        return tp;
    }


    TEST(Timestamp, External
    ) {
        using namespace date;
        cout << parse8601(istringstream{"2014-11-12T19:12:14.505Z"}) << '\n';
        cout << parse8601(istringstream{"2014-11-12T12:12:14.505-5:00"}) << '\n';



//        std::istringstream in{"30/03/09 16:31:32.121\n"
//                              "30/03/09 16:31:32.1214"};
//        std::chrono::system_clock::time_point tp;
//        in >> date::parse("%d/%m/%y %T", tp);
//        using namespace date;
//        std::cout << tp << '\n';
//        in >> date::parse(" %d/%m/%y %T", tp);
//        std::cout << tp << '\n';



        cout <<"This one is not working " << parse8601(istringstream{"2019-01-04T16:41:24.100000+05:00"}) << '\n';



        auto diff = parse8601(istringstream{"2014-11-12T19:12:14.505Z"}) -
                    parse8601(istringstream{"2014-11-12T19:12:13.505Z"});
        using namespace date;
        using namespace std;
        using namespace std::chrono;
        using dsecs = sys_time<duration<double>>;
        cout << "My test of 1 s " << floor<milliseconds>(diff) << endl;


    }


}