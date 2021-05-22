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
   client.subscribe("application/2/#")

def on_message(client, userdata, msg):
   application_packets = json.loads(msg.payload.decode('utf-8'))
   print(application_packets)

   # TODO: in some condition, disconnect the client
   # char = str(msg.payload)
   # if char == 'xxx':
   #    client.disconnect()

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
# MQTT host and port
client.connect("192.168.137.70", 1883)
client.loop_forever()
