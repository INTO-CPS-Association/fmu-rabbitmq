#include <iostream>

#include <string>
#include <iomanip>
#include <chrono>

#include <cmath>

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
    transTime << std::put_time(std::localtime(&time_after_duration), "%Y-%m-%dT%H:%M:%S.")<< appendZeros.c_str() << milliseconds_remainder << "+01:00";
        //std::cout << "milliseconds remainder: " << milliseconds_remainder << std::endl;
    std::cout << "transtime: " << transTime.str().c_str() << std::endl;
    
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
        else val = 100;

        std::string output;
        convertTimeToString(val, output);
        return 0;
    }