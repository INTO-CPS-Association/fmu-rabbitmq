#!/usr/bin/env python3
import pika
import json
import datetime
import time
import csv
import rospy
import threading
from geometry_msgs import Pose
from geometry_msgs import Vector3

desktopRobotti_playback_file = "gazebo_playback_data.csv"
with open(desktopRobotti_playback_file, mode='a') as csv_file:
    fieldnames = ['time', 'xpos', 'ypos', 'obst_xpos', 'obst_ypos']
    writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
    writer.writeheader()

# variable that indicates whether it is ok to start sending data to the co-sim
sim_time_step = 0.1
sim_duration = 10

#variables below are accessed by several threads!!!
lock_cosim_send_status = threading.Lock()
cosim_send_status = False 
lock_data_obst = threading.Lock()
data_new_obst = True
lock_data_robot = threading.Lock()
data_new_robot = True
robot_location = [0.0, 0.0]
##this is hardcoded now, however TODO make this work in case of a dynamic number of obstacles
obs_location = [1000.0, 1000.0]

#rabbitmq stuff
parameters = pika.ConnectionParameters(host='localhost')
connection = pika.BlockingConnection(parameters)
#connection = pika.SelectConnection(parameters=parameters, on_open_callback=on_open)
channel = connection.channel()
print("Declaring exchange")
channel.exchange_declare(exchange='fmi_digital_twin', exchange_type='direct')
print("Creating queue")
result = channel.queue_declare(queue='', exclusive=True)
queue_name = result.method.queue
channel.queue_bind(exchange='fmi_digital_twin', queue=queue_name,
                   routing_key='linefollower')
    
def callback_DR_position(data):
    global data_new_robot, robot_location
    #rospy.loginfo("I heard %f, %f", data.pose[1].position.x, data.pose[1].position.y)
    with lock_data_robot:
        data_new_robot = True
        robot_location[0] = data.position.x
        robot_location[1] = data.position.y

def callback_DR_obst(data):
    global data_new_obst, robot_location
    #rospy.loginfo("I heard %f, %f", data.pose[1].position.x, data.pose[1].position.y)
    with lock_data_obst:
        data_new_obst = True
        obs_location[0] = data.x
        obs_location[1] = data.y
    
# Step #4
def rabbitmq_callback(ch, method, properties, body):
    global data_new_robot, data_new_obst, cosim_send_status

    if "waiting for input data for simulation" in str(body):
        print("now I can publish to rabbitmq")
        with lock_cosim_send_status:
            cosim_send_status = True
    i = 0
    rate = 0.1
    #while i <= sim_duration and cosim_send_status and not rospy.is_shutdown():
    while cosim_send_status and not rospy.is_shutdown():
        if data_new_obst or data_new_robot:
            msg = {}

            #print("previous time: "+str(previous_rostime))
            rt = rospy.get_rostime()
            rostime = rt.secs + rt.nsecs * 1e-09
            #rostime = round(rostime,1)

            print(rostime)
            #print(datetime.datetime.utcfromtimestamp(rostime).isoformat(timespec='milliseconds'))
            rostimeISO = datetime.datetime.strptime(datetime.datetime.utcfromtimestamp(rostime).isoformat(timespec='milliseconds')+'+0100', "%Y-%m-%dT%H:%M:%S.%f%z")
            print(rostimeISO.isoformat())
            msg['time']= rostimeISO.isoformat(timespec='milliseconds')
            
            #dt = dt + datetime.timedelta(seconds=sim_time_step)
            #msg['time']= dt.isoformat()
            msg['xpos'] = robot_location[0]
            msg['ypos'] = robot_location[1]
            msg['obs_xpos'] = obs_location[0]
            msg['obs_ypos'] = obs_location[1]

            msg['obstacles'] = "12:0.24,0.0;3.0,2.45"
            with open(desktopRobotti_playback_file, mode='a') as csv_file:
                writer = csv.DictWriter(csv_file, fieldnames=['time', 'xpos', 'ypos', 'obst_xpos', 'obst_ypos'])
                writer.writerow({'time': msg['time'], 'xpos': msg['xpos'], 'ypos': msg['ypos'], 'obst_xpos': msg['obs_xpos'], 'obst_ypos': msg['obs_ypos']})
            
            channel.basic_publish(exchange='fmi_digital_twin',
                                    routing_key='linefollower',
                                    body=json.dumps(msg))
            print("BODY: " + str(json.dumps(msg)))
            print("published to rabbitmq at simu time: "+str(rostimeISO.isoformat()))
            #print("published to rabbitmq at fake time: "+str(dt))

            if data_new_robot:
                with lock_data_robot:
                    data_new_robot = False
            else:
                with lock_data_obst:
                    data_new_obst = False
            #i += sim_time_step
            i += rate
            rospy.sleep(rate)

    print("Either rospy shutdown or simulation over")
    connection.close()

try:
    ## Init ros node and subscriber to topic /gazebo/model_states
    rospy.init_node('listener', anonymous=True)
    rospy.loginfo("Setting up the ros subsriber to gazebo model states")
    rospy.Subscriber("/prox_dect/pose", Pose, callback_DR_position)
    rospy.Subscriber("/prox_dect/nearest_obstacle", Vector3, callback_DR_obst)
    ## 
    print(' [*] Waiting for logs. To exit press CTRL+C')
    channel.basic_consume(queue=queue_name, on_message_callback=rabbitmq_callback, auto_ack=True)
    channel.start_consuming()

# Catch a Keyboard Interrupt to make sure that the connection is closed cleanly
except KeyboardInterrupt:
    print("keyboard")
    # Gracefully close the connection
    rospy.signal_shutdown("Shutdown due to: KeyboardInterrupt")
    channel.stop_consuming()
    connection.close()
finally:
    print("Exiting...")

