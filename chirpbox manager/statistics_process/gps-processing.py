import binascii
import pandas as pd
import os
os.getcwd()

import pytz, datetime
local = pytz.timezone ("Asia/Shanghai")

LENGTH_MESG = 8
LENGTH_CHANNEL = 4
LENGTH_TX_NUM = 1
LENGTH_GPS_TIME_HOUR = 1
LENGTH_GPS_TIME_MIN = 1
LENGTH_GPS_TIME_SEC = 1
NUMBER_MESG = int(2048/LENGTH_MESG)
ROUND_NUM_NODE = int(2048/224)+1
class Message:
    def __init__(self):
        self.CHANNEL = 0
        self.TX_NUM = 0
        self.GPS_TIME_HOUR = 0
        self.GPS_TIME_MIN = 0
        self.GPS_TIME_SEC = 0
Message = Message()

total_node_num = 21
node = 0
mesg_original = ''

percent_count = 0

def str_to_ascii( ori_string ):
    target_str = ''
    global percent_count
    for i in range(0,len(ori_string),2):
        integ = int(ori_string[i:i+2],16)
        if integ <= 32 or integ >= 127:
            integ = 32
        chara = chr(integ)
        target_str = target_str + chara
    percent_count = target_str.count('%')
    return target_str

def str_to_dec_4byte( ori_string ):
    integ = 0
    for i in range(0,len(ori_string),8):
        integ = int(ori_string[i:i+2],16) + int(ori_string[i+2:i+4],16) * 256 + \
                int(ori_string[i+4:i+6],16) * 65536 + int(ori_string[i+6:i+8],16) * 16777216
        if i >= percent_count:
            break
    return integ


Round_Count = 0
node_tx_data = [[0]*6]
# with open("Processed_Data.txt","w") as f_target:
for node in range(total_node_num):
    #open original txt
    f_original = open("sf12_partnodes.txt")
    line = f_original.readline()
    # f_target.write("Node "+str(node)+":\n"+"{\n")
    while line:
        if line[0:9] == "rece_hash":
            line = f_original.readline()
            if (int(line[2:4],16)) == node:
                # print('node '+str(int(line[2:4],16))+':')
                line = f_original.readline()
                mesg_original = mesg_original + line[2:-1]

                mesg_original = mesg_original.replace(' ','')
                # print(mesg_original)

                Round_Count = Round_Count + 1
                # print(node, Round_Count)
        line = f_original.readline()
    #end of original file loop
    mesg_original = mesg_original[0:-384]
    for i in range(0,NUMBER_MESG-1):

        # extract one mesg
        Mesg_splitted = mesg_original[i*LENGTH_MESG*2:(i+1)*LENGTH_MESG*2]
        # split parts
        # if (Mesg_splitted[0:LENGTH_CHANNEL*2] != 'ffffffff'):
        Message.CHANNEL = str_to_dec_4byte(Mesg_splitted[0:LENGTH_CHANNEL*2])
        Message.TX_NUM = int(Mesg_splitted[LENGTH_CHANNEL*2 : (LENGTH_CHANNEL+1)*2],16)
        Message.GPS_TIME_HOUR = int((Mesg_splitted[(LENGTH_CHANNEL+LENGTH_TX_NUM)*2 : (LENGTH_CHANNEL+LENGTH_TX_NUM+LENGTH_GPS_TIME_HOUR)*2]), 16)
        Message.GPS_TIME_MIN = int(Mesg_splitted[(LENGTH_CHANNEL+LENGTH_TX_NUM+LENGTH_GPS_TIME_HOUR)*2 : (LENGTH_CHANNEL+LENGTH_TX_NUM+LENGTH_GPS_TIME_HOUR+LENGTH_GPS_TIME_MIN)*2], 16)
        Message.GPS_TIME_SEC = int(Mesg_splitted[(LENGTH_CHANNEL+LENGTH_TX_NUM+LENGTH_GPS_TIME_HOUR+LENGTH_GPS_TIME_MIN)*2 : (LENGTH_CHANNEL+LENGTH_TX_NUM+LENGTH_GPS_TIME_HOUR+LENGTH_GPS_TIME_HOUR+LENGTH_GPS_TIME_MIN+LENGTH_GPS_TIME_SEC)*2], 16)
        if((Message.GPS_TIME_HOUR != 0xff) and (Message.GPS_TIME_SEC != 0xff)):
            if (((Message.GPS_TIME_HOUR == 0) and (Message.GPS_TIME_MIN == 0) and (Message.GPS_TIME_SEC == 0)) != 1):
            # TODO:
                if ((Message.GPS_TIME_HOUR < 16) or ((Message.GPS_TIME_HOUR == 16) and Message.GPS_TIME_MIN <= 4)):
                # if ((Message.GPS_TIME_HOUR < 16)):
                # node_num_list = [16, 19, 11, 5, 4, 9, 13, 12]
                # if (node in node_num_list):

                    # convert to timestamp
                    timestring = "2020-8-16 " + str(Message.GPS_TIME_HOUR) + ":" + str(Message.GPS_TIME_MIN) + ":" + str(Message.GPS_TIME_SEC)
                    naive = datetime.datetime.strptime (timestring, "%Y-%m-%d %H:%M:%S")
                    local_dt = local.localize(naive, is_dst=None)
                    utc_dt = datetime.datetime.timestamp(local_dt)
                    # print(timestring, int(utc_dt))

                    node_tx_data.append([node, Message.TX_NUM, Message.CHANNEL, int(utc_dt)])

            # node_tx_data[Round_Count - ROUND_NUM_NODE + i] = [node, Message.TX_NUM, Message.CHANNEL, Message.GPS_TIME_HOUR, Message.GPS_TIME_MIN, Message.GPS_TIME_SEC]
    mesg_original = ''

    f_original.close()

# remove first row
del node_tx_data[0]
df = pd.DataFrame(node_tx_data,columns=['node_id', 'TX_NUM','CHANNEL','GPS_UTC'])
print(df)

df.to_csv('gps_lorawan.csv')



