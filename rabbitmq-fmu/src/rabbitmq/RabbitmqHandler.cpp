//
// Created by Kenneth Guldbrandt Lausdahl on 03/01/2020.
//

#include "RabbitmqHandler.h"
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr

static std::string string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int) fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while (1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

void RabbitmqHandler::throw_on_error(int x, char const *context) {
    if (x < 0) {

        throw RabbitMqHandlerException(string_format("%s: %s\n", context, amqp_error_string2(x)));
    }
}


RabbitmqHandler::RabbitmqHandler(const string &hostname, int port, const string &username, const string &password,
                                 const string &exchange,
                                 const string &queueBindingKey) {
    this->hostname = hostname;
    this->port = port;
    this->username = username;
    this->password = password;
    this->connected = false;
    this->exchange = exchange;
    this->exchangetype = "direct";
    this->queueBindinngKey = queueBindingKey;

    this->timeout.tv_sec = 1;
    this->timeout.tv_usec = 0;
}

RabbitmqHandler::~RabbitmqHandler() {
    close();
}


void RabbitmqHandler::throw_on_amqp_error(amqp_rpc_reply_t x, char const *context) {
    switch (x.reply_type) {
        case AMQP_RESPONSE_NORMAL:
            break;

        case AMQP_RESPONSE_NONE:
            throw RabbitMqHandlerException(string_format("%s: missing RPC reply type!\n", context));

        case AMQP_RESPONSE_LIBRARY_EXCEPTION:
            throw RabbitMqHandlerException(string_format("%s: %s\n", context, amqp_error_string2(x.library_error)));

        case AMQP_RESPONSE_SERVER_EXCEPTION:
            switch (x.reply.id) {
                case AMQP_CONNECTION_CLOSE_METHOD: {
                    amqp_connection_close_t *m =
                            (amqp_connection_close_t *) x.reply.decoded;
                    throw RabbitMqHandlerException(string_format("%s: server connection error %uh, message: %.*s\n",
                                                                 context, m->reply_code, (int) m->reply_text.len,
                                                                 (char *) m->reply_text.bytes));
                }
                case AMQP_CHANNEL_CLOSE_METHOD: {
                    amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
                    throw RabbitMqHandlerException(string_format("%s: server channel error %uh, message: %.*s\n",
                                                                 context, m->reply_code, (int) m->reply_text.len,
                                                                 (char *) m->reply_text.bytes));
                }
                default:
                    throw RabbitMqHandlerException(string_format("%s: unknown server error, method id 0x%08X\n",
                                                                 context, x.reply.id));
            }
        default:
            throw RabbitMqHandlerException(string_format("%s: unknown error, method id 0x%08X\n",
                                                         context, x.reply.id));
    }
}

bool RabbitmqHandler::open() {

    if (this->connected) {
        return this->connected;
    }

    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        throw RabbitMqHandlerException("creating TCP socket");
    }

    auto status = amqp_socket_open(this->socket, this->hostname.c_str(), this->port);
    if (status) {
        throw RabbitMqHandlerException("opening TCP socket");
    }

    throw_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                                   this->username.c_str(), this->password.c_str()),
                        "Logging in");
    amqp_channel_open(conn, 1);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
    this->connected = true;

    declareExchange();

    return this->connected;
}

void RabbitmqHandler::close() {
    if (this->connected) {
        throw_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                            "Closing channel");
        throw_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                            "Closing connection");

        throw_on_error(amqp_destroy_connection(conn), "Ending connection");
        this->connected = false;
    }
}

void RabbitmqHandler::declareExchange() {
    amqp_exchange_declare(conn, 1, amqp_cstring_bytes(this->exchange.c_str()),
                          amqp_cstring_bytes(exchangetype.c_str()), 0, 0, 0, 0,
                          amqp_empty_table);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange");
}


void RabbitmqHandler::bind() {

    amqp_queue_declare_ok_t *r = amqp_queue_declare(
            conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
        fprintf(stderr, "Out of memory while copying queue name");
        return;
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange.c_str()),
                    amqp_cstring_bytes(this->queueBindinngKey.c_str()), amqp_empty_table);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

    amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 0, 1,
                       amqp_empty_table);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    bound = true;
}


bool RabbitmqHandler::consume(string &payload) {

    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;

    amqp_maybe_release_buffers(conn);


    res = amqp_consume_message(conn, &envelope, &this->timeout, 0);

    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        return false;
    }

//    printf("Delivery %u, exchange %.*s routingkey %.*s\n",
//           (unsigned)envelope.delivery_tag, (int)envelope.exchange.len,
//           (char *)envelope.exchange.bytes, (int)envelope.routing_key.len,
//           (char *)envelope.routing_key.bytes);
//
//    if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
//        printf("Content-type: %.*s\n",
//               (int)envelope.message.properties.content_type.len,
//               (char *)envelope.message.properties.content_type.bytes);
//    }
//    printf("----\n");

    // amqp_dump(envelope.message.body.bytes, envelope.message.body.len);


    payload = std::string(reinterpret_cast< char const * >(envelope.message.body.bytes), envelope.message.body.len);

    amqp_destroy_envelope(&envelope);

    return true;

}

void RabbitmqHandler::publish(const string &routingkey, const string &messagebody) {
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */
    throw_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange.c_str()),
                                      amqp_cstring_bytes(routingkey.c_str()), 0, 0,
                                      &props, amqp_cstring_bytes(messagebody.c_str())),
                   "Publishing");
}

//Below methods that detach the creation of connections, channels, and exchanges.
bool RabbitmqHandler::createConnection(){
    if (this->connected) {
        return this->connected;
    }
    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        throw RabbitMqHandlerException("creating TCP socket");
    }

    auto status = amqp_socket_open(this->socket, this->hostname.c_str(), this->port);
    if (status) {
        throw RabbitMqHandlerException("opening TCP socket");
    }

    throw_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                                   this->username.c_str(), this->password.c_str()),
                        "Logging in");

    this->connected = true;
    return this->connected;
}

bool RabbitmqHandler::createChannel(amqp_channel_t channelID, string exchange, string exchangetype){
    amqp_channel_open(conn, channelID);
    
    string printText = "Opening channel with ID: " + to_string(channelID);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), printText.c_str());
    declareExchange(channelID, exchange, exchangetype);
    return true;
}

void RabbitmqHandler::declareExchange(amqp_channel_t channelID, string exchange, string exchangetype) {
    amqp_exchange_declare(conn, channelID, amqp_cstring_bytes(exchange.c_str()),
                          amqp_cstring_bytes(exchangetype.c_str()), 0, 0, 0, 0,
                          amqp_empty_table);
    string printText = "Declaring exchange on channel with ID: " + to_string(channelID) + "\nExchange name: " + exchange.c_str() + " exchange type: " + exchangetype.c_str();
    throw_on_amqp_error(amqp_get_rpc_reply(conn), printText.c_str());
}

void RabbitmqHandler::bind(amqp_channel_t channelID, const string &queueBindingKey, amqp_bytes_t &queue, string exchange) {

    amqp_queue_declare_ok_t *r = amqp_queue_declare(
            conn, channelID, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    queue = amqp_bytes_malloc_dup(r->queue);
    printf("QUEUENAME %s\n", std::string(reinterpret_cast< char const * >(queue.bytes), queue.len).c_str());
    if (queue.bytes == NULL) {
        fprintf(stderr, "Out of memory while copying queue name");
        return;
    }

    amqp_queue_bind(conn, channelID, queue, amqp_cstring_bytes(exchange.c_str()),
                    amqp_cstring_bytes(queueBindingKey.c_str()), amqp_empty_table);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

    amqp_basic_consume(conn, channelID, queue, amqp_empty_bytes, 0, 0, 1,
                       amqp_empty_table);
    throw_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    bound = true;
}
                   
void RabbitmqHandler::publish(const string &routingkey, const string &messagebody, amqp_channel_t channelID, string exchange) {
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */
    throw_on_error(amqp_basic_publish(conn, channelID, amqp_cstring_bytes(exchange.c_str()),
                                      amqp_cstring_bytes(routingkey.c_str()), 0, 0,
                                      &props, amqp_cstring_bytes(messagebody.c_str())),
                   "Publishing");

    printf("after it published \n");
}

bool RabbitmqHandler::getFromChannel(string &payload, amqp_channel_t channelID, amqp_bytes_t queue) {

    amqp_rpc_reply_t res, res2;
    amqp_message_t message;
    amqp_boolean_t no_ack = false;


    //amqp_maybe_release_buffers(conn);
printf("were here, with queue name %s\n", queue);
    //res = amqp_basic_get(conn, channelID, amqp_empty_bytes, no_ack);
    res = amqp_basic_get(conn, channelID, queue, no_ack);
    printf("1:reply type %d\n", res.reply_type);


    printf("QUEUENAME %s\n", std::string(reinterpret_cast< char const * >(queuename.bytes), queuename.len).c_str());
    printf("2:reply type %d\n", res.reply_type);
    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        printf("6:reply type %d\n", res.reply_type);
        return false;
    }

    printf("4:reply type %d\n", res.reply_type);

    res2 = amqp_read_message(conn,channelID,&message,0);
    printf("5:reply type %d\n", res2.reply_type);

    if (AMQP_RESPONSE_NORMAL != res2.reply_type) {
        printf("6:reply type %d\n", res2.reply_type);
        return false;
    }

printf("then were here\n");
    payload = std::string(reinterpret_cast< char const * >(message.body.bytes), message.body.len);

printf("then were here\n %s", payload.c_str());
    amqp_destroy_message(&message);
    return true;

}
