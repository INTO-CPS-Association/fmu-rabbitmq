#!/usr/bin/env python3
import rospy
from std_msgs.msg import Bool
import pika
import json

rospy.init_node('publisher', anonymous=True)
cmd_pub = rospy.Publisher('/em_stop', Bool, queue_size=1)

connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()

print("Declaring exchange")
channel.exchange_declare(exchange='fmi_digital_twin', exchange_type='direct')

print("Creating queue")
result = channel.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue

channel.queue_bind(exchange='fmi_digital_twin', queue=queue_name,
                   routing_key='from_cosim')

print(' [*] Waiting for logs. To exit press CTRL+C')

def callback(ch, method, properties, body):
    print("Received [x] %r" % body)
    # Set robotti linear velocity to 0. Publish an ackerman message
    cmd = True
    # Publish the velocity command
    cmd_pub.publish(cmd)

channel.basic_consume(
    queue=queue_name, on_message_callback=callback, auto_ack=True)

channel.start_consuming()

connection.close()
