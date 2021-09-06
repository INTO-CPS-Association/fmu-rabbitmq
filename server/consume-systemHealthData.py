#!/usr/bin/env python3
import pika
import json
from datetime import datetime, timezone
import time
import threading

cosimTime = 0.0
newData = False
lock = threading.Lock()
thread_stop = False

connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
connectionPublish = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channelConsume = connection.channel()
channelPublish = connectionPublish.channel()

print("Declaring exchange")
channelConsume.exchange_declare(exchange='fmi_digital_twin_sh', exchange_type='direct')
channelPublish.exchange_declare(exchange='fmi_digital_twin_sh', exchange_type='direct')

print("Creating queue")
result = channelConsume.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue

result2 = channelPublish.queue_declare(queue='', exclusive=True)
qnamepub = result2.method.queue

channelConsume.queue_bind(exchange='fmi_digital_twin_sh', queue=queue_name,
                   routing_key='linefollower.system_health.from_cosim')

channelPublish.queue_bind(exchange='fmi_digital_twin_sh', queue=qnamepub,
                   routing_key='linefollower.system_health.to_cosim')

print(' [*] Waiting for logs. To exit press CTRL+C')
print(' [*] I am consuming and publishing information related to system health')

def callbackConsume(ch, method, properties, body):
    global newData, cosimTime
    print("\nReceived [x] %r" % body)
    #cosimTime = datetime.datetime.strptime(body, "%Y-%m-%dT%H:%M:%S.%f%z")
    with lock:
        newData = True
        cosimTime = body.decode()
        cosimTime = json.loads(cosimTime)
        #print(cosimTime)

def publishRtime():
    global newData
    while not thread_stop:
        if newData:
        #if True:
            with lock:
                newData = False
                msg = {}
                
                msg['rtime'] = datetime.now(timezone.utc).astimezone().isoformat(timespec='milliseconds')
                #msg['rtime'] = '2020-11-04T10:47:59.210000+01:00'

                #timet = datetime.datetime.strptime(timeNow.isoformat(timespec='milliseconds'), "%Y-%m-%dT%H:%M:%S.%f%z")
                #msg['time']= timet.isoformat(timespec='milliseconds')
                
                #msg['cosimtime'] = '2020-11-04T10:47:59.210000+01:00'
                msg['cosimtime'] = cosimTime["simAtTime"]
                print("\nSending [y] %s" % str(msg))
                channelPublish.basic_publish(exchange='fmi_digital_twin_sh',
                                    routing_key='linefollower.system_health.to_cosim',
                                    body=json.dumps(msg))
                #time.sleep(1)

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
