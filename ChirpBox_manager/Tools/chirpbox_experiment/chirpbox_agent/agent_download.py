import time
import paho.mqtt.client as mqtt
from socket import *
import File_transfer, chirpbox_schedule

def on_connect(client, userdata, flags, rc):
    """一旦连接成功, 回调此方法"""
    print('connected to mqtt with result code ', rc)
    ### 订阅该测试床的主题   0001
    client.subscribe("0001", 2)

def on_message(client, userdata, msg):
    print("订阅到消息")
    """一旦订阅到消息, 回调此方法"""
    payword = str(msg.payload.decode('gb2312'))
    ### 创建套接字，回收实验结果
    print(payword)
    name = File_transfer.getbin()
    File_transfer.get_json(name)
    chirpbox_schedule.postExp("D:\\TP\\Study\\ChirpBox\\ChirpBox_manager\\Tools\\chirpbox_experiment\\upload_files\\testfiles\\storage\\" + name + ".bin", "D:\\TP\\Study\\ChirpBox\\ChirpBox_manager\\Tools\\chirpbox_experiment\\upload_files\\testfiles\\storage\\" + name + ".json")

def server_conenet(client):
    """尝试连接mqtt"""
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect("47.98.212.156", 1883, 60)
    client.loop_forever()


def server_main():
    client_id = time.strftime('%Y%m%d%H%M%S', time.localtime(time.time()))
    client = mqtt.Client(client_id, transport='tcp')
    server_conenet(client)


if __name__ == '__main__':
    server_main()