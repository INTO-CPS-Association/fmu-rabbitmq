//
// Created by Kenneth Guldbrandt Lausdahl on 03/01/2020.
//

#ifndef RABBITMQFMUPROJECT_RABBITMQHANDLER_H
#define RABBITMQFMUPROJECT_RABBITMQHANDLER_H

#include <string>

#ifdef _WIN32
#include <time.h>
#include <sys/time.h>
#endif
#include <amqp.h>
extern "C"
{
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <amqp_tcp_socket.h>
#include <amqp_time.h>
}

#include "RabbitMqHandlerException.h"

using namespace std;

class RabbitmqHandler {

public :
    RabbitmqHandler(const string &hostname, int port, const string& username, const string &password,const string &exchange,const string &queueBindingKey);

    ~RabbitmqHandler();

    virtual bool open();

    virtual void close();

    virtual void bind();

    virtual bool consume(string & json);
    void publish(const string & routingkey, const string &message);


    virtual bool createConnection();
    virtual bool createChannel(amqp_channel_t channelID, string exchange, string exchangetype);
    virtual void bind(amqp_channel_t channelID, const string &routingkey, amqp_bytes_t &queue, string exchange);
    virtual void bind(amqp_channel_t channelID, const string &routingkey, string exchange);
    void publish(const string &routingkey, const string &message, amqp_channel_t channelID, string exchange);

    virtual bool createChannel(amqp_channel_t channelID);
    virtual void queue_declare(amqp_channel_t channelID, const char *queue_name_);
    virtual bool getFromChannel(string &payload, amqp_channel_t channelID, const char*  queueName);
    void declareExchange(amqp_channel_t channelID, string exchange, string exchangetype);
    void bindExchange(amqp_channel_t channelID, string exchange, string exchangetype); 


    string exchangeCD, exchangetypeCD;
    string exchangeSH, exchangetypeSH;
    string queuenameCD, queuenameSH;
    string routingKeySH;
    string bindingKeySH;

    int channelPub, channelSub;

private:

    string hostname;
    int port;
    string username;
    string password;
    string queueBindinngKey;
    string exchange;
    string exchangetype;

    bool connected;
    bool bound;

    amqp_socket_t *socket = nullptr;
    amqp_connection_state_t conn;
    amqp_bytes_t queuename;//relevant to RabbitmqHandler::bind()
    struct timeval timeout;

    void throw_on_amqp_error(amqp_rpc_reply_t x, char const *context);
    void declareExchange();
    void throw_on_error(int x, char const *context);

};


#endif //RABBITMQFMUPROJECT_RABBITMQHANDLER_H
