#!/usr/bin/env python3
import pika
import json
import datetime
import time
import csv

#Create connection to rabbitMQ server
connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))

#Create channel
channel = connection.channel()

#Create exchange
channel.exchange_declare(exchange='fmi_digital_twin', exchange_type='direct')

#Create queue
result = channel.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue

#Bind queue
channel.queue_bind(exchange='fmi_digital_twin', queue=queue_name,
                   routing_key='data.from_cosim')

#Message rate
time_sleep = 0.1

#Recorded data
data = 'gazebo_playback_data-noround.csv'

print(' [*] Waiting for logs. To exit press CTRL+C, sleep time [ms]: ', time_sleep*1000)

#Read from file and publish to rabbitMQ
def publish():
    dt=datetime.datetime.strptime('2019-01-04T16:41:24+0200', "%Y-%m-%dT%H:%M:%S%z")
    print(dt)
    msg = {}
    msg['time']= dt.isoformat()
    msg['xpos']=0.0
    msg['ypos']=0.0
    with open(data, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            t = row['time']
            xpos = float(row['xpos'])
            ypos = float(row['ypos'])
            msg['xpos']=xpos
            msg['ypos']=ypos  
            timet = datetime.datetime.strptime(t, "%Y-%m-%dT%H:%M:%S.%f%z")
            msg['time']= timet.isoformat()
            print(" [x] Sent %s" % json.dumps(msg))
            channel.basic_publish(exchange='fmi_digital_twin',
						routing_key='data.to_cosim',
						body=json.dumps(msg))
            
            time.sleep(time_sleep)
   
#Callback for receiving from rabbitmq
def callback(ch, method, properties, body):
    print(" [x] %r" % body)
    if "waiting for input data for simulation" in str(body):
      publish()

#Setup consume
channel.basic_consume(
    queue=queue_name, on_message_callback=callback, auto_ack=True)

#Start consuming
channel.start_consuming()

#Close connection
connection.close()