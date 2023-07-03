#!/usr/bin/env python3
import pika
import time
import threading

cosimTime = 0.0
newData = False
lock = threading.Lock()
thread_stop = False

connectionPublish = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channelPublish = connectionPublish.channel()

print("Declaring exchange")
channelPublish.exchange_declare(exchange='exch', exchange_type='direct')

print("Creating queue")
queue_name = 'boasorte'
result2 = channelPublish.queue_declare(queue=queue_name, exclusive=False, auto_delete=True)

channelPublish.queue_bind(exchange='exch', queue=queue_name,
                   routing_key='routeHere')

print(' [*] Waiting for logs. To exit press CTRL+C')
print(' [*] I am consuming and publishing information related to system health')

try:
    ping_count = 0
    while ping_count < 100:
        ping_count = ping_count + 1
        message = "Back ping count: " + str(ping_count)
        print("\nSending [y] %s" % message)
        channelPublish.basic_publish(exchange='exch',
                            routing_key='routeHere',
                            body=message)
        time.sleep(1)

except KeyboardInterrupt:
    print("Exiting...")
    thread_stop = True
