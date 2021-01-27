#!/usr/bin/env python3
import pika
import json
import datetime
import time
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
print(' [*] Waiting for logs. To exit press CTRL+C')
def publish():
    dt=datetime.datetime.strptime('2019-01-04T16:41:24+0200', "%Y-%m-%dT%H:%M:%S%z")
    print(dt)
    msg = {}
    msg['time']= dt.isoformat()
    msg['xpos']=0.0
    msg['ypos']=0.0

    with open('gazebo_playback_data-noround.csv', newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            t = row['time']
            xpos = float(row['xpos'])
            ypos = float(row['ypos'])
            msg['xpos']=xpos
            msg['ypos']=ypos            
            msg['obs_xpos']=1000
            msg['obs_ypos']=1000
            msg['obstacles']="youza"
            msg['test_int']=1.6
			#dt = dt+ datetime.timedelta(seconds=float(row['step-size']))
			#msg['time']= dt.isoformat()
            timet = datetime.datetime.strptime(t, "%Y-%m-%dT%H:%M:%S.%f%z")
            msg['time']= timet.isoformat()
            print(" [x] Sent %s" % json.dumps(msg))
            channel.basic_publish(exchange='fmi_digital_twin_cd',
						routing_key='linefollower.data.to_cosim',
						body=json.dumps(msg))
            #input("Press Enter to Continue")
            time.sleep(.1)
   
def callback(ch, method, properties, body):
    print(" [x] %r" % body)
    if "waiting for input data for simulation" in str(body):
      publish()
channel.basic_consume(
    queue=queue_name, on_message_callback=callback, auto_ack=True)
channel.start_consuming()
connection.close()