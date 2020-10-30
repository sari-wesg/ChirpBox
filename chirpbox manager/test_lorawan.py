import cbmng
import numpy as np
import time

import os
import sys

import grpc
from chirpstack_api.as_pb.external import api
import time
import datetime
# Configuration.

# This must point to the API interface.
# server = "192.168.137.37:8080"
server = "3m364v9352.imdo.co:31566"

# The DevEUI for which you want to enqueue the downlink.
dev_eui = bytes([0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x16])

# The API token (retrieved using the web-interface).
api_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJjaGlycHN0YWNrLWFwcGxpY2F0aW9uLXNlcnZlciIsImF1ZCI6ImNoaXJwc3RhY2stYXBwbGljYXRpb24tc2VydmVyIiwibmJmIjoxNTkwMTM2NTg2LCJleHAiOjE2MDg2MjYxODYsInN1YiI6InVzZXIiLCJ1c2VybmFtZSI6ImFkbWluIn0.pbdjWCSeg1TGvJuh-r_f0rPcJ0_UhZynwoCX1mEBgUY"

def group_send_on_packet():
    # Connect without using TLS.
    channel = grpc.insecure_channel(server)

    # Device-queue API client.
    client = api.MulticastGroupServiceStub(channel)

    # Define the API key meta-data.
    auth_token = [("authorization", "Bearer %s" % api_token)]

    # Construct request.
    req = api.EnqueueMulticastQueueItemRequest()
    req.multicast_queue_item.multicast_group_id = "9b4006a9-fa24-4736-b4a9-951132051290"
    # req.multicast_queue_item.data = bytes([0x01, 0x02, 0x03])
    req.multicast_queue_item.data = bytes([0x01, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x03])
    req.multicast_queue_item.f_port = 10
    req.multicast_queue_item.f_cnt = 1
    print((len(req.multicast_queue_item.data)))

    resp = client.Enqueue(req, metadata=auth_token)

    # Print the downlink frame-counter value.
    print(resp.f_cnt)

def lorawan_class_dl(num_packet):
    filename_log = "classc" + datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S") + ".txt"
    with open(filename_log, 'a') as f:
        line = "begin\r"
        f.write(line)
        line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        f.write(line_write + "\r")
        f.close()
    for i in range(num_packet):
        print(i)
        group_send_on_packet()
        time.sleep(3.7)
    with open(filename_log, 'a') as f:
        line = "end\r"
        f.write(line)
        line_write = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        f.write(line_write + "\r")
        f.close()



def generate_start():
    for i in range(10):
        # start
        task_start_run = "cbmng.py " + "-start " + "0 " + "ade0 " + "7 " + "com11 " + "1fffdf " + "100 " + "14 "
        print(task_start_run.split())
        cbmng.main(task_start_run.split())
        time.sleep(200)
        lorawan_class_dl(240)
        time.sleep(500)

        # col trace
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "100 " + "14 " + "1fffff "+'080FD000 '+'080FD800'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())
    exit(0)

com_serial = "com11 "

generate_start()


tx 88.949,16.16349384258
rx 363.78,66.1047992676
sleep 3547.271,189.8034746699

272.07176778008
65297.2242672192
65392.2941287,66843.0875662393
65475.8724610,67353.3354850514
65435.4192767,67410.1660689024

0.42036 s
214 epoch
42.036 s
8995.704 s

1355
increase 563.89%

rx 0.408,74.14029
tx 0.42036,120.919160232
sleep 41.20764,2204.893072716
2,399.952522948
513589.839910872
156357.984


6.63837638
3.38470492

LoRaBlink with limitation, the duration and energy is 6.6 and 3.4 more than that without 
Duration and energy are 6.6 and 3.4 more than without limitation