//
// Created by Kenneth Guldbrandt Lausdahl on 07/01/2020.
//

#include "Iso8601Time.h"
#include <iomanip>
#include <iostream>

#if defined(_WIN32)
#include <time.h>
#define timegm _mkgmtime
#endif

namespace Iso8601 {
    std::chrono::system_clock::time_point parseIso8601String(const std::string &isoStr) {
        std::tm tm = {};
        int microseconds = 0;
        int tz_hour = 0, tz_minute = 0;
        char tz_sign = '+';

        size_t pos = 0;

        if (isoStr.size() < 19)
            throw std::runtime_error("Invalid ISO8601 format");

        // Parse date and time
        std::istringstream ss(isoStr);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail())
            throw std::runtime_error("Failed to parse date and time");

        pos = ss.tellg();

        // Check for fractional seconds
        if (pos < isoStr.size() && isoStr[pos] == '.') {
            ++pos;
            std::string micros;
            while (pos < isoStr.size() && std::isdigit(isoStr[pos]) && micros.size() < 6) {
                micros += isoStr[pos++];
            }
            while (micros.size() < 6) micros += '0';
            microseconds = std::stoi(micros);
        }

        // Time zone: 'Z' or ±hh:mm
        if (pos >= isoStr.size())
            throw std::runtime_error("Missing timezone specifier");

        char tz_char = isoStr[pos];
        if (tz_char == 'Z') {
            // UTC
            ++pos;
        } else if (tz_char == '+' || tz_char == '-') {
            tz_sign = tz_char;
            if (pos + 6 > isoStr.size())
                throw std::runtime_error("Invalid timezone format");

            tz_hour = std::stoi(isoStr.substr(pos + 1, 2));
            tz_minute = std::stoi(isoStr.substr(pos + 4, 2));

            if (isoStr[pos + 3] != ':')
                throw std::runtime_error("Expected ':' in timezone offset");

            pos += 6;
        } else {
            throw std::runtime_error("Unexpected timezone format");
        }

        // Convert to UTC time_point
        time_t t = timegm(&tm); // interpreted as UTC
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(t);
        tp += std::chrono::microseconds(microseconds);

        if (tz_char != 'Z') {
            int offset = (tz_hour * 60 + tz_minute) * 60;
            if (tz_sign == '+') tp -= std::chrono::seconds(offset);
            else                tp += std::chrono::seconds(offset);
        }

        return  tp;
    }


    string toIso8601ToString(system_clock::time_point timePoint) {
        auto duration = timePoint.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto micros = std::chrono::duration_cast<microseconds>(duration - seconds);

        std::time_t time = system_clock::to_time_t(timePoint);
#if defined(_WIN32)
        std::tm tm;
        gmtime_s(&tm, &time);
#else
        std::tm tm;
        gmtime_r(&time, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        oss << "." << std::setw(6) << std::setfill('0') << micros.count() << "Z";
       return oss.str();
    }
}
