//
// Created by Kenneth Guldbrandt Lausdahl on 11/12/2019.
//

#ifndef RABBITMQ_FMU_MODELDESCRIPTIONPARSER_H
#define RABBITMQ_FMU_MODELDESCRIPTIONPARSER_H

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <utility>
#include "DataPoint.h"
#include <memory>

using namespace std;

class ModelDescriptionParser {

public:

    struct ScalarVariable {
        string name;
        unsigned int valueReference;
        enum SvType{Real,Integer,Boolean,String};
        bool output;
        bool input;
        SvType type;
        union {
            double d_value;
            int i_value;
            bool b_value;

        };
               string s_value;
        bool hasStartValue = false;

        void setReal(double val){
            this->type = SvType::Real;
            this->d_value=val;
            this->hasStartValue = true;
        }
        void setInt(int val){
            this->type = SvType::Integer;
            this->i_value=val;
            this->hasStartValue = true;
        }
        void setBool(bool val){
            this->type = SvType::Boolean;
            this->b_value=val;
            this->hasStartValue = true;
        }
        void setString(string val){
            this->type = SvType::String;
            this->s_value=val;

            this->hasStartValue = true;
        }

    };

   static map<string, ScalarVariable> parse(std::string path);
   static DataPoint extractDataPoint(map<string, ScalarVariable> svs);
   static string extractToolVersion(std::string path);


};


#endif //RABBITMQ_FMU_MODELDESCRIPTIONPARSER_H
