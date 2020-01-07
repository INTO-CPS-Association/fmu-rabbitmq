#!/usr/bin/env python3
import pika
import json
import datetime


connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()

dt=datetime.datetime.strptime('2019-01-04T16:40:24+0200', "%Y-%m-%dT%H:%M:%S%z")

print(dt);

msg = {}
msg['time']= dt.isoformat()
msg['level']=0


for  i in range(1,100):
	channel.basic_publish(exchange='fmi_digiital_twin',
                      routing_key='linefollower',
                      body=json.dumps(msg))
	print(" [x] Sent %s" % json.dumps(msg))
	dt = dt + datetime.timedelta(seconds=1)
	msg['time']= dt.isoformat()

connection.close()
