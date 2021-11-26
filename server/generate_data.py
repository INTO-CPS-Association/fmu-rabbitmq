#!/usr/bin/env python3
import pika
import json
import datetime
import time
import csv
connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()
print("Declaring exchange")
channel.exchange_declare(exchange='test', exchange_type='direct')
print("Creating queue")
time_sleep = 0.1
data = 'dt_test.csv'
print(' [*] Waiting for logs. To exit press CTRL+C, sleep time [ms]: ', time_sleep*1000)
def publish():
    time.sleep(5)
    dt=datetime.datetime.strptime('2019-01-04T16:41:24+0200', "%Y-%m-%dT%H:%M:%S%z")
    print(dt)
    msg = {}
    msg['time']= dt.isoformat()
    msg['valve']=0.0
    msg['level']=0.0
    i = 1
    with open(data, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:

            msg = {}
            t = row['time']
            ypos = float(row['ypos'])
            msg['level']=ypos           
            #msg['seqno']=i
            i = i +1
			#dt = dt+ datetime.timedelta(seconds=float(row['step-size']))
			#msg['time']= dt.isoformat()
            timet = datetime.datetime.strptime(t, "%Y-%m-%dT%H:%M:%S.%f%z")
            msg['time']= timet.isoformat()
            print(" [x] Sent %s" % json.dumps(msg))
            channel.basic_publish(exchange='test',
						routing_key='level',
						body=json.dumps(msg))


            msg = {}
            t = row['time']
            xpos = float(row['xpos'])
            msg['valve']=xpos         
            #msg['seqno']=i
			#dt = dt+ datetime.timedelta(seconds=float(row['step-size']))
			#msg['time']= dt.isoformat()
            timet = datetime.datetime.strptime(t, "%Y-%m-%dT%H:%M:%S.%f%z")
            msg['time']= timet.isoformat()
            print(" [x] Sent %s" % json.dumps(msg))
            channel.basic_publish(exchange='test',
						routing_key='valve',
						body=json.dumps(msg))
            #input("Press Enter to Continue")
            time.sleep(time_sleep)

publish()

connection.close()