#!/usr/bin/env python3
import pika
import json
import datetime
import time

connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()

print("Declaring exchange")
channel.exchange_declare(exchange='fmi_digital_twin', exchange_type='direct')

print("Creating queue")
result = channel.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue

channel.queue_bind(exchange='fmi_digital_twin', queue=queue_name,
                   routing_key='linefollower')

print(' [*] Waiting for logs. To exit press CTRL+C')


def publish():
#    channel.stop_consuming()
    dt=datetime.datetime.strptime('2019-01-04T16:41:24+0200', "%Y-%m-%dT%H:%M:%S%z")

    print(dt);

    msg = {}
    msg['time']= dt.isoformat()
    msg['level']=0


    for  i in range(1,(10+1)*10):
#            time.sleep(0.01)
            channel.basic_publish(exchange='fmi_digital_twin',
                          routing_key='linefollower',
                          body=json.dumps(msg))
            print(" [x] Sent %s - relative time %s" % (json.dumps(msg),str(0.1*i)))
            dt = dt + datetime.timedelta(seconds=0.1)
            msg['time']= dt.isoformat()
            msg['level']=msg['level']+1
            if msg['level'] > 2:
                    msg['level']=0



def callback(ch, method, properties, body):
    print(" [x] %r" % body)
    if "waiting for input data for simulation" in str(body):
      publish()




channel.basic_consume(
    queue=queue_name, on_message_callback=callback, auto_ack=True)

channel.start_consuming()

connection.close()
