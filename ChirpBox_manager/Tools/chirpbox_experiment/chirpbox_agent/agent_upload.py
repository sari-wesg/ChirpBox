from asyncio.windows_events import NULL
import imp
from turtle import st
from chirpbox_schedule import *
import File_transfer
import paho.mqtt.client as mqtt


mqttClient = mqtt.Client()

# connect MQTT server
def mqtt_connect():
    MQTTHOST = "47.98.212.156"
    MQTTPORT = 1883  # MQTT端口

    mqttClient.connect(MQTTHOST, MQTTPORT, 60)
    mqttClient.loop_start()

# 发布消息
def mqtt_publish(name):
    '''向服务器发送结果请求'''
    mqtt_connect()
    mqttClient.publish("Cloud", '0001', 2)
    mqttClient.loop_stop()  # 断开连接
    File_transfer.post_result(name)


if __name__ == '__main__':
    last_file = fetchResult_time()
    while(1):
        sleep(1)
        if fetchStatus() == 'idle' and last_file != fetchResult_time():
            mqtt_publish(fetchResult_time())
            last_file = fetchResult_time()
            sleep(10)
