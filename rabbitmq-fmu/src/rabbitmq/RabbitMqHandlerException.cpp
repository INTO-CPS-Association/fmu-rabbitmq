//
// Created by Kenneth Guldbrandt Lausdahl on 03/01/2020.
//

#include "RabbitMqHandlerException.h"

RabbitMqHandlerException::RabbitMqHandlerException(const string &message) { this->message = message; }

RabbitMqHandlerException::RabbitMqHandlerException(const char* message) { this->message =string( message); }
