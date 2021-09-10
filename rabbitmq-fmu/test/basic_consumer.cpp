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
#include "gtest/gtest.h"

using namespace std;


static const int fixed_channel_id = 1;
static const char test_queue_name[] = "boasorte";

bool getFromChannel(amqp_connection_state_t conn, string &payload, amqp_channel_t channelID, const char* queueName) {

    amqp_rpc_reply_t res, res2;
    amqp_message_t message;
    amqp_boolean_t no_ack = false;

    amqp_maybe_release_buffers(conn);
printf("were here, with queue name %s, on channel %d\n", queueName, channelID);

    amqp_time_t deadline;
    struct timeval timeout = { 1 , 0 };//same timeout used in consume(json)
    int time_rc = amqp_time_from_now(&deadline, &timeout);
    assert(time_rc == AMQP_STATUS_OK);

    do {
        res = amqp_basic_get(conn, channelID, amqp_cstring_bytes(queueName), no_ack);
    } while (res.reply_type == AMQP_RESPONSE_NORMAL &&
            res.reply.id == AMQP_BASIC_GET_EMPTY_METHOD 
            && amqp_time_has_past(deadline) == AMQP_STATUS_OK);

    if (AMQP_RESPONSE_NORMAL != res.reply_type || AMQP_BASIC_GET_OK_METHOD != res.reply.id) {
        printf("amqp_basic_get error codes amqp_response_normal %d, amqp_basic_get_ok_method %d\n", res.reply_type, res.reply.id);
        return false;
    }

    printf("amqp_basic_get outcome %d\n", res.reply_type);
    printf("amqp_basic_get outcome %d\n", res.reply.id);

    res2 = amqp_read_message(conn,channelID,&message,0);
    printf("error %s\n", amqp_error_string2(res2.library_error));
    printf("5:reply type %d\n", res2.reply_type);

    if (AMQP_RESPONSE_NORMAL != res2.reply_type) {
        printf("6:reply type %d\n", res2.reply_type);
        amqp_frame_t decoded_frame;
        amqp_simple_wait_frame_noblock(conn, &decoded_frame, &timeout);
        return false;
    }

    payload = std::string(reinterpret_cast< char const * >(message.body.bytes), message.body.len);

printf("then were here\n %s", payload.c_str());
    amqp_destroy_message(&message);
    return true;

}

void queue_declare(amqp_connection_state_t connection_state_,
                   const char *queue_name_) {
  amqp_queue_declare_ok_t *res = amqp_queue_declare(
      connection_state_, fixed_channel_id, amqp_cstring_bytes(queue_name_),
      /*passive*/ 0,
      /*durable*/ 0,
      /*exclusive*/ 0,
      /*auto_delete*/ 1, amqp_empty_table);
  assert(res != NULL);
  printf("Declaring the queue: %s\n", queue_name_);
}

amqp_connection_state_t setup_connection_and_channel(void) {
  amqp_connection_state_t connection_state_ = amqp_new_connection();

  amqp_socket_t *socket = amqp_tcp_socket_new(connection_state_);
  assert(socket);

  int rc = amqp_socket_open(socket, "localhost", AMQP_PROTOCOL_PORT);
  assert(rc == AMQP_STATUS_OK);

  amqp_rpc_reply_t rpc_reply = amqp_login(
      connection_state_, "/", 1, AMQP_DEFAULT_FRAME_SIZE,
      AMQP_DEFAULT_HEARTBEAT, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

  amqp_channel_open_ok_t *res =
      amqp_channel_open(connection_state_, fixed_channel_id);
  assert(res != NULL);
 
  
  printf("Setting up the connection\n");

  return connection_state_;
}

void close_and_destroy_connection(amqp_connection_state_t connection_state_) {
  amqp_rpc_reply_t rpc_reply =
      amqp_connection_close(connection_state_, AMQP_REPLY_SUCCESS);
  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

  int rc = amqp_destroy_connection(connection_state_);
  assert(rc == AMQP_STATUS_OK);

  printf("Closing the connection\n");
}

void basic_publish(amqp_connection_state_t connectionState_,
                   const char *message_) {
  amqp_bytes_t message_bytes = amqp_cstring_bytes(message_);

  amqp_basic_properties_t properties;
  properties._flags = 0;

  properties._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
  properties.delivery_mode = AMQP_DELIVERY_NONPERSISTENT;

  int retval = amqp_basic_publish(
      connectionState_, fixed_channel_id, amqp_cstring_bytes(""),
      amqp_cstring_bytes(test_queue_name),
      /* mandatory=*/1,
      /* immediate=*/0, /* RabbitMQ 3.x does not support the "immediate" flag
                          according to
                          https://www.rabbitmq.com/specification.html */
      &properties, message_bytes);

  assert(retval == 0);

  printf("Publishing message: %s\n", message_);
}

char *basic_get(amqp_connection_state_t connection_state_,
                const char *queue_name_, uint64_t *out_body_size_) {
  amqp_rpc_reply_t rpc_reply;
  amqp_time_t deadline;
  struct timeval timeout = {5, 0};
  int time_rc = amqp_time_from_now(&deadline, &timeout);
  assert(time_rc == AMQP_STATUS_OK);

  printf("basig_get from: %s\n", queue_name_);

  do {
    rpc_reply = amqp_basic_get(connection_state_, fixed_channel_id,
                               amqp_cstring_bytes(queue_name_), /*no_ack*/ 1);
  } while (rpc_reply.reply_type == AMQP_RESPONSE_NORMAL &&
           rpc_reply.reply.id == AMQP_BASIC_GET_EMPTY_METHOD &&
           amqp_time_has_past(deadline) == AMQP_STATUS_OK);

  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);
  assert(rpc_reply.reply.id == AMQP_BASIC_GET_OK_METHOD);

  amqp_message_t message;
  rpc_reply =
      amqp_read_message(connection_state_, fixed_channel_id, &message, 0);
  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

  char *body = (char*)malloc(message.body.len);
  memcpy(body, message.body.bytes, message.body.len);
  *out_body_size_ = message.body.len;
  amqp_destroy_message(&message);


  printf("Received message: %s\n", body);

  return body;
}

void publish_and_basic_get_message(const char *msg_to_publish) {
  amqp_connection_state_t connection_state = setup_connection_and_channel();

  queue_declare(connection_state, test_queue_name);
  //basic_publish(connection_state, msg_to_publish);

  uint64_t body_size;
  char *msg = basic_get(connection_state, test_queue_name, &body_size);

  assert(body_size == strlen(msg_to_publish));
  assert(strncmp(msg_to_publish, msg, body_size) == 0);
  free(msg);

  close_and_destroy_connection(connection_state);
}

namespace{
    TEST(Rbmq, basicConsumer){
        GTEST_SKIP();
        amqp_connection_state_t conn = amqp_new_connection();

        amqp_socket_t* socket = amqp_tcp_socket_new(conn);
        if (!socket) {
            printf("\ncreating TCP socket");
        }

        auto status = amqp_socket_open(socket, "localhost", 5672);
        if (status) {
            printf("\nopening TCP socket");
        }

        amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,"guest", "guest");

        int channelPub = 1;
        amqp_channel_open(conn, channelPub);
    
        std::string printText = "Opening channel with ID: " + std::to_string(channelPub);
        printf("\n%s", printText.c_str());
        amqp_get_rpc_reply(conn), printText.c_str();

        std::string exchange = "fmi_digital_twin_sh";
        std::string exchangetype = "direct";
        amqp_exchange_declare(conn, channelPub, amqp_cstring_bytes(exchange.c_str()),
                          amqp_cstring_bytes(exchangetype.c_str()), 0, 0, 0, 0,
                          amqp_empty_table);
        printText = "Declaring exchange on channel with ID: " + std::to_string(channelPub) + 
        "\nExchange name: " + exchange.c_str() + " exchange type: " + exchangetype.c_str();
        printf("\n%s\n", printText.c_str());
        amqp_get_rpc_reply(conn), printText.c_str();

        int count = 0;

        amqp_basic_properties_t props;
        std::string routingkey = "linefollower.system_health.from_cosim";

        ////// create consumer on same connection different channel
        int channelSub = 2;
        amqp_channel_open(conn, channelSub);
        printText = "Opening channel with ID: " + std::to_string(channelSub);

        const char* qnametest = "boasorte";
        const char* queueBindingKey = "linefollower.system_health.to_cosim";
        printText = "Declaring queue on channel with ID: " + std::to_string(channelSub);
        printf("\n%s\n", printText.c_str());
        amqp_queue_declare_ok_t *res = amqp_queue_declare(
        conn, channelSub, amqp_cstring_bytes(qnametest),
        /*passive*/ 0,
        /*durable*/ 0,
        /*exclusive*/ 0,
        /*auto_delete*/ 0, amqp_empty_table);
        assert(res != NULL);
        amqp_queue_bind(conn, channelSub, amqp_cstring_bytes(qnametest), amqp_cstring_bytes(exchange.c_str()),
                    amqp_cstring_bytes(queueBindingKey), amqp_empty_table);
        amqp_get_rpc_reply(conn);

        amqp_basic_consume(conn, channelSub,  amqp_cstring_bytes(qnametest), amqp_empty_bytes, 0, 0, 1,
                       amqp_empty_table);
        amqp_get_rpc_reply(conn);



        while(count < 10){
            props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
            props.content_type = amqp_cstring_bytes("text/plain");
            props.delivery_mode = 2; /* persistent delivery mode */
            std::string messagebody = "Ping count: " + std::to_string(count);
            amqp_basic_publish(conn, channelPub, amqp_cstring_bytes(exchange.c_str()),
                                            amqp_cstring_bytes(routingkey.c_str()), 0, 0,
                                            &props, amqp_cstring_bytes(messagebody.c_str()));
            printf("\nMessage sent: %s\n", messagebody.c_str());
            count++;

            std::string payload;
            getFromChannel(conn, payload, channelSub, qnametest);
        }
    }

    TEST(Rbmq, exampleConsumer){
        GTEST_SKIP();
        int count = 0;
        while(count < 10){
            publish_and_basic_get_message("TEST");
            count++;
        }

    }
}