#!/usr/bin/env python3
import pika
import json
import time

from datetime import datetime, timezone
import csv
connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()
print("Declaring exchange")
channel.exchange_declare(exchange='fmi_digital_twin_cd', exchange_type='direct')
print("Creating queue")
result = channel.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue
channel.queue_bind(exchange='fmi_digital_twin_cd', queue=queue_name,
                   routing_key='linefollower.data.to_cosim')
time_sleep = 0.1
data = 'gazebo_playback_data.csv'
print(' [*] Waiting for logs. To exit press CTRL+C, sleep time [ms]: ', time_sleep*1000)
def publish():
    dt=datetime.strptime('2019-01-04T16:41:24+0200', "%Y-%m-%dT%H:%M:%S%z")
    print(dt)
    msg = {}
    msg['time']= dt.isoformat()
    
    msg['time']= datetime.now(tz = datetime.now().astimezone().tzinfo).isoformat(timespec='milliseconds')
    msg['xpos']=0.0
    msg['ypos']=0.0
    i = 1
    with open(data, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            t = row['time']
            xpos = float(row['xpos'])
            ypos = float(row['ypos'])
            msg['xpos']=xpos
            msg['ypos']=ypos           
            msg['seqno']=i
            i = i +1
			#dt = dt+ datetime.timedelta(seconds=float(row['step-size']))
			#msg['time']= dt.isoformat()
            timet = datetime.strptime(t, "%Y-%m-%dT%H:%M:%S.%f%z")
            msg['time']= timet.isoformat()

            msg['time']= datetime.now(tz = datetime.now().astimezone().tzinfo).isoformat(timespec='milliseconds')
            print(" [x] Sent %s" % json.dumps(msg))
            channel.basic_publish(exchange='fmi_digital_twin_cd',
						routing_key='linefollower.data.to_cosim',
						body=json.dumps(msg))
            #input("Press Enter to Continue")
            time.sleep(time_sleep)
   
def callback(ch, method, properties, body):
    print(" [x] %r" % body)
    if "waiting for input data for simulation" in str(body):
      publish()
channel.basic_consume(
    queue=queue_name, on_message_callback=callback, auto_ack=True)
channel.start_consuming()
connection.close()