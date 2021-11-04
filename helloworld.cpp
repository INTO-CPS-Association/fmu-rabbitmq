#include <iostream>

#include <string>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <cmath>
#include <Windows.h>

void convertTimeToString(long long milliSecondsSinceEpoch, std::string &message){                
    const auto durationSinceEpoch = std::chrono::milliseconds(milliSecondsSinceEpoch);
    const std::chrono::time_point<std::chrono::system_clock> tp_after_duration(durationSinceEpoch);
    time_t time_after_duration = std::chrono::system_clock::to_time_t(tp_after_duration);
    std::tm* formattedTime = std::gmtime(&time_after_duration);
    long long int milliseconds_remainder = milliSecondsSinceEpoch % 1000;
    std::stringstream transTime, formatString;
    int no_digits = 0;
    std::string appendZeros = "";
    if(milliseconds_remainder>0){
        no_digits = floor(log10(milliseconds_remainder))+1;
        std::cout << "Number of digits: " << no_digits << std::endl;
        if(no_digits==1)appendZeros.append("00");
        if(no_digits==2)appendZeros.append("0");
    }
    //transTime << std::put_time(std::localtime(&time_after_duration), "%Y-%m-%dT%H:%M:%S.")<< appendZeros.c_str() << milliseconds_remainder << "+01:00";
        //std::cout << "milliseconds remainder: " << milliseconds_remainder << std::endl;
    std::cout << "transtime: " << transTime.str().c_str() << std::endl;
    
    /*
    formatString << "%Y-%m-%dT%H:%M:%S."<< appendZeros.c_str() << milliseconds_remainder <<"%Z" ;

    std::cout << "Format string: " << formatString.str().c_str() << std::endl;
    transTime << std::put_time(std::localtime(&time_after_duration), formatString.str().c_str());
    message = transTime.str().insert(transTime.str().length()-2, ":");
    std::cout << "transtime: " << transTime.str().c_str() << std::endl;
    */
    TIME_ZONE_INFORMATION time_zone;
    GetTimeZoneInformation(&time_zone);
    //UTC = localtime + bias; bias is in minutes
    int utc_offset_hours = time_zone.Bias / 60;
    int utc_offset_minutes = std::abs(time_zone.Bias - (utc_offset_hours * 60));
    char offset_sign = time_zone.Bias > 0 ? '-' : '+';
    formatString << std::setfill('0') << "%Y-%m-%dT%H:%M:%S."<< appendZeros.c_str() << milliseconds_remainder << offset_sign << std::setw(2) << std::abs(utc_offset_hours) << ":" << utc_offset_minutes<< utc_offset_minutes ;

    transTime << std::put_time(std::localtime(&time_after_duration), formatString.str().c_str());
    std::cout << "transtime: " << transTime.str().c_str() << std::endl;
}

void format_system_time(const SYSTEMTIME& sys_time, const TIME_ZONE_INFORMATION& time_zone)
{
    std::ostringstream formatted_date_time;
    formatted_date_time << std::setfill('0');
    formatted_date_time << sys_time.wYear <<  "-" << std::setw(2) << sys_time.wMonth << "-" <<
        std::setw(2) << sys_time.wDay << "T" << std::setw(2) << sys_time.wHour << ":" <<
        std::setw(2) << sys_time.wMinute << ":" << std::setw(2) << sys_time.wSecond << "." <<
        std::setw(3) << sys_time.wMilliseconds;

    //UTC = localtime + bias; bias is in minutes
    int utc_offset_hours = time_zone.Bias / 60;
    int utc_offset_minutes = std::abs(time_zone.Bias - (utc_offset_hours * 60));
    char offset_sign = time_zone.Bias > 0 ? '-' : '+';
    formatted_date_time << offset_sign << std::setw(2) << std::abs(utc_offset_hours) << ":" << utc_offset_minutes<< utc_offset_minutes;

std::cout << formatted_date_time.str().c_str() << std::endl;
}

int main(int argc, char **argv){ 
        std::cout << "call function" << std::endl;
        long long val;

        if (argc >= 2)
        {
            std::istringstream iss( argv[1] );

            if (iss >> val)
            {
                // Conversion successful
            }
        }
        else val = 205;

        std::string output;
        convertTimeToString(val, output);

    SYSTEMTIME date_and_time;
    GetLocalTime(&date_and_time);

    TIME_ZONE_INFORMATION time_zone;
    GetTimeZoneInformation(&time_zone);

   format_system_time(date_and_time, time_zone);

        return 0;
    }