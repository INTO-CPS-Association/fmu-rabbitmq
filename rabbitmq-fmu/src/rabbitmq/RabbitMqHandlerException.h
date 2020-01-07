//
// Created by Kenneth Guldbrandt Lausdahl on 03/01/2020.
//

#ifndef RABBITMQFMUPROJECT_RABBITMQHANDLEREXCEPTION_H
#define RABBITMQFMUPROJECT_RABBITMQHANDLEREXCEPTION_H

#include <string>

using namespace std;

class RabbitMqHandlerException : public std::exception {
public:
    RabbitMqHandlerException(const string &message);

    RabbitMqHandlerException(const char *message);

    virtual const char *what() const throw() {
        return this->message.c_str();
    }

private:
    string message;
};


#endif //RABBITMQFMUPROJECT_RABBITMQHANDLEREXCEPTION_H
