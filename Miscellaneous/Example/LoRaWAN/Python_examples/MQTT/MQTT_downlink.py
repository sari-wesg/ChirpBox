"""
TODO:
1. Replace your topic with "application/2/#", see details in https://www.chirpstack.io/gateway-bridge/integrate/generic-mqtt/
2. Input MQTT host and port
"""

import paho.mqtt.client as mqtt
import json

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
   print("Connected with result code "+str(rc))
   client.subscribe("application/1/#")

##Publisher connects to MQTT broker
mqttc= mqtt.Client("python_pub")
mqttc.connect("[2400:dd02:1008:15:5281:bf7b:42c9:e25c]", 1883)
mqttc.loop_start()

data = {"confirmed": False, "fPort": 10, "data": "aGVsbG8="}
the_json = json.dumps(data) # Encode the data
print(the_json)
while True:
   mqttc.publish("application/1/device/00000000001e0037", the_json)