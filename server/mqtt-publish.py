import time
import paho.mqtt.client as mqtt


port=1883
url="localhost"

def on_log(client, userdata, level, buf):
    print("LOG %s, %s, %s, %s"%(client, userdata, level, buf));

def on_message(client, data, msg):
    print(msg.topic + " " + str(msg.payload))

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("mychannel/myresource", 1)

#    print("Transmitting packages")
#    topic = "mytopic" 
#    payload = "hej"
#    for i in range(1,10):
 #     client.publish(topic, payload, hostname=hostname)


client = mqtt.Client()
client.username_pw_set("other", password="other")
client.on_connect = on_connect
client.on_log = on_log
client.on_message = on_message
client.connect(url, port, 60)

client.loop_start()

while 1:
    # Publish a message every second
    client.publish("mychannel/myresource", "Hello World", 1)
    time.sleep(1)
