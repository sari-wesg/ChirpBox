"""
TODO:
1. Replace your topic with "application/2/#", see details in https://www.chirpstack.io/gateway-bridge/integrate/generic-mqtt/
2. Input config for MQTT and MYSQL
3. Change the MYSQL and MQTT table format if required
"""
""" ------------------------------- CONFIG ------------------------------- """

MYSQL_HOST = "127.0.0.1"
MYSQL_USER = "lorawan"
MYSQL_PASSWD = '1111'
MYSQL_PORT = 3306
MYSQL_DATABASE = "LoRaWANChirpBox"
MYSQL_TABLE = ""


MQTT_HOST = "192.168.137.155"
MQTT_PORT = 1883

""" --------------------------------------------------------------------- """

import paho.mqtt.client as mqtt
import json
import MySQLdb
from datetime import datetime
import time
import base64

""" ------------------------------- MYSQL ------------------------------- """
def create_table(host, user, password, port, database):
   # create database if not exist
   db = MySQLdb.connect(host=host, user=user, passwd=password, port=port)
   cursor = db.cursor()
   cursor.execute("CREATE database if not exists `%s`" % (database))

   #use database
   cursor.execute("USE `%s`" % (database))

   # create table with current time
   table_name = str("LoRaGateway_") + datetime.today().strftime("%Y_%m_%d_%H_%M_%S")
   # table_name = str("LoRa_") + str(int(time.time()))
   cursor.execute(
      """
         CREATE TABLE IF NOT EXISTS `%s`
         (
            `time_utc` VARCHAR(200) NOT NULL,
            `frequency` INT UNSIGNED,
            `spreadingFactor` INT UNSIGNED,
            `length` INT UNSIGNED,
            `data` VARCHAR(1000) NOT NULL
         )ENGINE=InnoDB DEFAULT CHARSET=utf8;
      """
      % (table_name))

   db.close()

   return table_name

def application_packets_to_table(utc_time, application_packets):
   print(MYSQL_TABLE)
   #connect server
   db = MySQLdb.connect(host=MYSQL_HOST, user=MYSQL_USER, passwd=MYSQL_PASSWD, port=MYSQL_PORT)
   cursor = db.cursor()

   #use database
   cursor.execute("USE `%s`" % (MYSQL_DATABASE))

   #insert into table
   mysql_add_word = ("INSERT INTO {table} "
                     "VALUES (%s,%s,%s,%s,%s)")
   mysql_data_word = (utc_time, application_packets["txInfo"]["frequency"],application_packets["txInfo"]["loRaModulationInfo"]['spreadingFactor'],int(len(base64.b64decode(application_packets["phyPayload"]).hex())/2),base64.b64decode(application_packets["phyPayload"]).hex())
   print(int(len(base64.b64decode(application_packets["phyPayload"]).hex())/2))

   cursor.execute(mysql_add_word.format(table=MYSQL_TABLE), mysql_data_word)
   db.commit()


""" ------------------------------- MQTT ------------------------------- """
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
   print("Connected with result code "+str(rc))
   client.subscribe("gateway/b827ebfffe1b55ff/event/up/#")

def on_message(client, userdata, msg):
   application_packets = json.loads(msg.payload.decode('utf-8'))
   utc_time = int(time.time())
   print(utc_time, application_packets)
   try:
      application_packets_to_table(utc_time, application_packets)
   except:
      pass
   # TODO: if in some condition, disconnect the client
   # char = str(msg.payload)
   # if char == 'xxx':
   #    client.disconnect()

def MQTT_loop():
   client = mqtt.Client()
   client.on_connect = on_connect
   client.on_message = on_message
   # MQTT host and port
   client.connect(MQTT_HOST, MQTT_PORT)
   client.loop_forever()

""" ------------------------------- EXCUTE ------------------------------- """

MYSQL_TABLE = create_table(MYSQL_HOST, MYSQL_USER, MYSQL_PASSWD, MYSQL_PORT, MYSQL_DATABASE)

MQTT_loop()