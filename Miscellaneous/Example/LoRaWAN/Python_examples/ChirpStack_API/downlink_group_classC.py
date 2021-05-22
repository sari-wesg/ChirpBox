import os
import sys

import grpc
from chirpstack_api.as_pb.external import api
import time

# Configuration.

# This must point to the API interface.
# The gateway host address / network translator address of the gateway
server = "TODO:"

# The API token (retrieved using the web-interface).
api_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJjaGlycHN0YWNrLWFwcGxpY2F0aW9uLXNlcnZlciIsImF1ZCI6ImNoaXJwc3RhY2stYXBwbGljYXRpb24tc2VydmVyIiwic3ViIjoidXNlciIsInVzZXJuYW1lIjoiYWRtaW4ifQ.jQjVFrYYj9wbGmI7QKb90INACuZ8Po-jHAcm9-C1eVM"

def group_send_on_packet():
  # Connect without using TLS.
  channel = grpc.insecure_channel(server)

  # Device-queue API client.
  client = api.MulticastGroupServiceStub(channel)

  # Define the API key meta-data.
  auth_token = [("authorization", "Bearer %s" % api_token)]

  # Construct request.
  req = api.EnqueueMulticastQueueItemRequest()
  req.multicast_queue_item.multicast_group_id = "TODO:"
  req.multicast_queue_item.data = bytes([0x01, 0x02, 0x03])
  req.multicast_queue_item.f_port = 10
  req.multicast_queue_item.f_cnt = 1

  resp = client.Enqueue(req, metadata=auth_token)

  # Print the downlink frame-counter value.
  print("frame-counter:", resp.f_cnt)

def LoRaWAN_ClassC_Group_downlink(num_packet, downlink_command_interval_s):
  for i in range(num_packet):
    # the i_th downlink
    print("i_th downlink:", i)

    group_send_on_packet()
    time.sleep(downlink_command_interval_s)

# Enqueue 100 downlink packets per 10-second to the group
number_of_downlink = 100
downlink_command_interval_s = 10
LoRaWAN_ClassC_Group_downlink(number_of_downlink, downlink_command_interval_s)