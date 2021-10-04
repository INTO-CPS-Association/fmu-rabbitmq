#!/usr/bin/env python3
import pika
import json
import datetime
import time
import csv
import pandas as pd

csvFilePath = r'ur_robot.csv'
# csvFilePath = r'ur_robot_clean.csv'

connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))

channel = connection.channel()
print("Declaring exchange")
channel.exchange_declare(exchange='fmi_digital_twin_cd', exchange_type='direct')
print("Creating queue")
result = channel.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue
channel.queue_bind(exchange='fmi_digital_twin_cd', queue=queue_name,
                   routing_key='linefollower.data.from_cosim')

time_sleep = 0.002

print(' [*] Waiting for logs. To exit press CTRL+C, sleep time [ms]: ', time_sleep*1000)

def publish():
    dt=datetime.datetime.strptime('2019-01-04T16:41:24+0200', "%Y-%m-%dT%H:%M:%S%z")
    print(dt)

    df = pd.read_csv(csvFilePath,delim_whitespace=True)
    df['time'] = df['time'].apply(lambda x: datetime.datetime.fromtimestamp(x).astimezone().isoformat())
    data_dict = df.to_dict('index') 

    for i in data_dict:
        if (i == 0):
            continue
        msg = data_dict[i]
        # print(" [x] Sent %s" % json.dumps(msg).encode('utf-8'))
        print(" [x] Sent seqno ", msg['seqno'])
        channel.basic_publish(exchange='fmi_digital_twin_cd',
                                            routing_key='linefollower.data.to_cosim',
                                            body=json.dumps(msg).encode('utf-8'))
        #input("Press Enter to Continue")
        time.sleep(time_sleep)
        # time.sleep(.01)
        # time.sleep(.001)


def callback(ch, method, properties, body):
    print(" [x] %r" % body)
    if "waiting for input data for simulation" in str(body):
      publish()
channel.basic_consume(
    queue=queue_name, on_message_callback=callback, auto_ack=True)
channel.start_consuming()
connection.close()

