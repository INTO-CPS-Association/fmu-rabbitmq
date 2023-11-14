#include "gtest/gtest.h"

#include "rabbitmq/RabbitmqHandler.h"

using namespace std;
namespace {


    TEST(RabbitMq, basicSendRecieve
    ) {
        auto key = "basicSendReceive";
        auto key_from_cosim = "basic";
        RabbitmqHandler handler("localhost", 5672, "guest", "guest", "testing_exchange", "direct", key, key_from_cosim, "/");

        ASSERT_TRUE(handler.open());
        cout << "connected" <<endl;

        handler.bind();
        cout << "bound" <<endl;
        handler.publish(key, "test");
        cout << "published" <<endl;

        string message;
        bool gotData = handler.consume(message);
        cout << "consumed " <<gotData<< " " <<message<< endl;

        ASSERT_TRUE(gotData);
        ASSERT_STREQ("test", message.c_str());
    }
}