#!/usr/bin/env python3
import pika
import json
import datetime
import time
import threading
import datetime

cosimTime = 0.0
newData = False
lock = threading.Lock()
thread_stop = False

connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
connectionPublish = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channelConsume = connection.channel()
channelPublish = connectionPublish.channel()

print("Declaring exchange")
channelConsume.exchange_declare(exchange='fmi_digital_twin', exchange_type='direct')
channelPublish.exchange_declare(exchange='fmi_digital_twin', exchange_type='direct')

print("Creating queue")
result = channelConsume.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue
result2 = channelPublish.queue_declare(queue='', exclusive=True)
queue_name2 = result2.method.queue

channelConsume.queue_bind(exchange='fmi_digital_twin', queue=queue_name,
                   routing_key='system_health_from_cosim')

channelPublish.queue_bind(exchange='fmi_digital_twin', queue=queue_name2,
                   routing_key='system_health_to_cosim')

print(' [*] Waiting for logs. To exit press CTRL+C')
print(' [*] I am consuming and publishing information related to system health')

def callbackConsume(ch, method, properties, body):
    global newData, cosimTime
    print("Received [x] %r" % body)
    #cosimTime = datetime.datetime.strptime(body, "%Y-%m-%dT%H:%M:%S.%f%z")
    with lock:
        newData = True
        cosimTime = body.decode()
        #print(cosimTime)

def publishRtime():
    global newData
    while not thread_stop:
        if newData:
            with lock:
                newData = False
                msg = {}
                
                timeNow = datetime.datetime.now()
                msg['rtime'] = timeNow.isoformat(timespec='milliseconds')+"0100"
                msg['cosimtime'] = cosimTime
                channelPublish.basic_publish(exchange='fmi_digital_twin',
                                    routing_key='system_health_rtime',
                                    body=json.dumps(msg))

channelConsume.basic_consume(
    queue=queue_name, on_message_callback=callbackConsume, auto_ack=True)

try:
    thread = threading.Thread(target = publishRtime)
    thread.start()
    channelConsume.start_consuming()
except KeyboardInterrupt:
    print("Exiting...")
    channelConsume.stop_consuming()
    thread_stop = True
    connection.close()
