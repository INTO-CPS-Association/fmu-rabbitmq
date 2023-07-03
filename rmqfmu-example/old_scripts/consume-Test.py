#!/usr/bin/env python3
import pika
import threading

cosimTime = 0.0
newData = False
lock = threading.Lock()
thread_stop = False

connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
connectionPublish = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channelConsume = connection.channel()
channelPublish = connectionPublish.channel()

print("Declaring exchange")
channelConsume.exchange_declare(exchange='fmi_digital_twin_sh', exchange_type='direct')
channelPublish.exchange_declare(exchange='fmi_digital_twin_sh', exchange_type='direct')

print("Creating queue")
result = channelConsume.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue
result2 = channelPublish.queue_declare(queue='boasorte', exclusive=False, auto_delete=False)
queue_name2 = result2.method.queue

channelConsume.queue_bind(exchange='fmi_digital_twin_sh', queue=queue_name,
                   routing_key='linefollower.system_health.from_cosim')

channelPublish.queue_bind(exchange='fmi_digital_twin_sh', queue="boasorte",
                   routing_key='linefollower.system_health.to_cosim')

print(' [*] Waiting for logs. To exit press CTRL+C')
print(' [*] I am consuming and publishing information related to system health')

def callbackConsume(ch, method, properties, body):
    global newData, cosimTime
    print("\nReceived [x] %r" % body)
    with lock:
        newData = True
        cosimTime = body.decode()

def publishRtime():
    global newData
    ping_count = 0
    while not thread_stop:
        if newData:
            ping_count = ping_count + 1
            message = "Back ping count: " + str(ping_count)
            with lock:
                newData = False
                msg = {}

                print("\nSending [y] %s" % str(msg))
                channelPublish.basic_publish(exchange='fmi_digital_twin_sh',
                                    routing_key='linefollower.system_health.to_cosim',
                                    body=message)
                #time.sleep(1)

channelConsume.basic_consume(
    queue=queue_name, on_message_callback=callbackConsume, auto_ack=True)

try:
    thread = threading.Thread(target = publishRtime)
    thread.start()
    channelConsume.start_consuming()
except KeyboardInterrupt:
    print("Exiting...")
    channelConsume.stop_consuming()
    thread_stop = True
    connection.close()
