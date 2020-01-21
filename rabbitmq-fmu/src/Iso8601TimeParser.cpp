//
// Created by Kenneth Guldbrandt Lausdahl on 07/01/2020.
//

#include "Iso8601TimeParser.h"

#include <iostream>
#include <sstream>

namespace Iso8601 {


    date::sys_time<std::chrono::milliseconds> parseIso8601ToMilliseconds(const std::string input) {
//        std::string save;
//        input >> save;
        std::istringstream in{input};
        date::sys_time<std::chrono::milliseconds> tp;
        in >> date::parse("%FT%T%Z", tp);
        if (in.fail()) {
            cout << "fail on: "<<input<<endl;
            in.clear();
            in.exceptions(std::ios::failbit);
            in.str(input);
            in >> date::parse("%FT%T%Ez", tp);

            if (in.fail())
            {
                cout << "fail2 on: "<<input<<endl;
            }
        }
        return tp;
    }
}