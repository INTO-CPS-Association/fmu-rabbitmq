#!/usr/bin/env python3
import pika
import json
import datetime
import time
import ssl

credentials = pika.PlainCredentials('rbmq-fmu', '6ASUs62hyj2T')
context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
parameters = pika.ConnectionParameters(host='b-14c95d1b-b988-4039-a4fe-b5c6744b8a97.mq.eu-north-1.amazonaws.com',
                                       port=5671,
                                       virtual_host='/',
                                       credentials=credentials,
                                       ssl_options=pika.SSLOptions(context)
                                       )
connection = pika.BlockingConnection(parameters)
# connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))

channel = connection.channel()

print("Declaring exchange")
channel.exchange_declare(exchange='fmi_digital_twin_cd', exchange_type='direct')

print("Creating queue")
result = channel.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue

channel.queue_bind(exchange='fmi_digital_twin_cd', queue=queue_name,
                   routing_key='linefollower.data.from_cosim')

print(' [*] Waiting for logs. To exit press CTRL+C')
print(' [*] I am consuming the commands sent from rbMQ')

def callback(ch, method, properties, body):
    print("Received [x] %r" % body)


channel.basic_consume(
    queue=queue_name, on_message_callback=callback, auto_ack=True)

channel.start_consuming()

connection.close()
