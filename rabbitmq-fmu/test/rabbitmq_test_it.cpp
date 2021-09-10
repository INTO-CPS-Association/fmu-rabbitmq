#include "gtest/gtest.h"

extern "C"
{
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp.h>
#include <amqp_time.h>
#include <amqp_tcp_socket.h>
}

#include <stdarg.h>
namespace {
    void die(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n");
        exit(1);
    }

    void die_on_error(int x, char const *context) {
        if (x < 0) {
            fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x));
            exit(1);
        }
    }

    void die_on_amqp_error(amqp_rpc_reply_t x, char const *context) {
        switch (x.reply_type) {
            case AMQP_RESPONSE_NORMAL:
                return;

            case AMQP_RESPONSE_NONE:
                fprintf(stderr, "%s: missing RPC reply type!\n", context);
                break;

            case AMQP_RESPONSE_LIBRARY_EXCEPTION:
                fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x.library_error));
                break;

            case AMQP_RESPONSE_SERVER_EXCEPTION:
                switch (x.reply.id) {
                    case AMQP_CONNECTION_CLOSE_METHOD: {
                        amqp_connection_close_t *m =
                                (amqp_connection_close_t *)x.reply.decoded;
                        fprintf(stderr, "%s: server connection error %uh, message: %.*s\n",
                                context, m->reply_code, (int)m->reply_text.len,
                                (char *)m->reply_text.bytes);
                        break;
                    }
                    case AMQP_CHANNEL_CLOSE_METHOD: {
                        amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
                        fprintf(stderr, "%s: server channel error %uh, message: %.*s\n",
                                context, m->reply_code, (int)m->reply_text.len,
                                (char *)m->reply_text.bytes);
                        break;
                    }
                    default:
                        fprintf(stderr, "%s: unknown server error, method id 0x%08X\n",
                                context, x.reply.id);
                        break;
                }
                break;
        }

        exit(1);
    }

    static void dump_row(long count, int numinrow, int *chs) {
        int i;

        printf("%08lX:", count - numinrow);

        if (numinrow > 0) {
            for (i = 0; i < numinrow; i++) {
                if (i == 8) {
                    printf(" :");
                }
                printf(" %02X", chs[i]);
            }
            for (i = numinrow; i < 16; i++) {
                if (i == 8) {
                    printf(" :");
                }
                printf("   ");
            }
            printf("  ");
            for (i = 0; i < numinrow; i++) {
                if (isprint(chs[i])) {
                    printf("%c", chs[i]);
                } else {
                    printf(".");
                }
            }
        }
        printf("\n");
    }

    static int rows_eq(int *a, int *b) {
        int i;

        for (i = 0; i < 16; i++)
            if (a[i] != b[i]) {
                return 0;
            }

        return 1;
    }

    void amqp_dump(void const *buffer, size_t len) {
        unsigned char *buf = (unsigned char *)buffer;
        long count = 0;
        int numinrow = 0;
        int chs[16];
        int oldchs[16] = {0};
        int showed_dots = 0;
        size_t i;

        for (i = 0; i < len; i++) {
            int ch = buf[i];

            if (numinrow == 16) {
                int j;

                if (rows_eq(oldchs, chs)) {
                    if (!showed_dots) {
                        showed_dots = 1;
                        printf(
                                "          .. .. .. .. .. .. .. .. : .. .. .. .. .. .. .. ..\n");
                    }
                } else {
                    showed_dots = 0;
                    dump_row(count, numinrow, chs);
                }

                for (j = 0; j < 16; j++) {
                    oldchs[j] = chs[j];
                }

                numinrow = 0;
            }

            count++;
            chs[numinrow++] = ch;
        }

        dump_row(count, numinrow, chs);

        if (numinrow != 0) {
            printf("%08lX:\n", count);
        }
    }



    TEST(RabbitMq, declareExchange
    ) {
//        char const *hostname;
//        int port, status;
//        char const *exchange;
//        char const *exchangetype;
        amqp_socket_t *socket = NULL;
        amqp_connection_state_t conn;


        auto hostname = "localhost";
        auto port = 5672;
        auto exchange = "fmi_digiital_twin";
        auto exchangetype = "direct";

        conn = amqp_new_connection();

        socket = amqp_tcp_socket_new(conn);
        if (!socket) {
            die("creating TCP socket");
        }

        auto status = amqp_socket_open(socket, hostname, port);
        if (status) {
            die("opening TCP socket");
        }

        die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                                     "guest", "guest"),
                          "Logging in");
        amqp_channel_open(conn, 1);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

        amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange),
                              amqp_cstring_bytes(exchangetype), 0, 0, 0, 0,
                              amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange");






        char const *bindingkey="linefollower";

        amqp_bytes_t queuename;



        {
            amqp_queue_declare_ok_t *r = amqp_queue_declare(
                    conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
            die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
            queuename = amqp_bytes_malloc_dup(r->queue);
            if (queuename.bytes == NULL) {
                fprintf(stderr, "Out of memory while copying queue name");
                return ;
            }
        }

        amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange),
                        amqp_cstring_bytes(bindingkey), amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

        amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0,
                           amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");
//
//        {
//            for (;;) {
//                amqp_rpc_reply_t res;
//                amqp_envelope_t envelope;
//
//                amqp_maybe_release_buffers(conn);
//
//                res = amqp_consume_message(conn, &envelope, NULL, 0);
//
//                if (AMQP_RESPONSE_NORMAL != res.reply_type) {
//                    break;
//                }
//
//                printf("Delivery %u, exchange %.*s routingkey %.*s\n",
//                       (unsigned)envelope.delivery_tag, (int)envelope.exchange.len,
//                       (char *)envelope.exchange.bytes, (int)envelope.routing_key.len,
//                       (char *)envelope.routing_key.bytes);
//
//                if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
//                    printf("Content-type: %.*s\n",
//                           (int)envelope.message.properties.content_type.len,
//                           (char *)envelope.message.properties.content_type.bytes);
//                }
//                printf("----\n");
//
//                amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
//
//                std::cout << std::endl;
//                unsigned char *buf = (unsigned char *)envelope.message.body.bytes;
//                for(int j=0;j<envelope.message.body.len;j++){
//                    std::cout << buf[j];
//                }
//                std::cout << std::endl;
//
//                amqp_destroy_envelope(&envelope);
//            }
//        }
//
//
//
//









        die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                          "Closing channel");
        die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                          "Closing connection");
        die_on_error(amqp_destroy_connection(conn), "Ending connection");
    }
}