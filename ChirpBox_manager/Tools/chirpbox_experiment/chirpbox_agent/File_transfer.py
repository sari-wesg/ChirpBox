from asyncio.windows_events import NULL
from functools import total_ordering
from http import client
from socket import *
import time,datetime, tqdm, select, os
from xml.sax.xmlreader import InputSource
server_ip = "47.98.212.156"
server_port = 9999
def post_result(Expfile_name):
    client_socket = socket(AF_INET, SOCK_STREAM)
    client_socket.connect((server_ip, 9999))
    Expfile_name = r"D:\TP\Study\ChirpBox\ChirpBox_manager\tmp" + "\\" + Expfile_name
    file_size = os.path.getsize (Expfile_name)
    client_socket.send (f"{file_size}".encode())
    while(1):
        x = client_socket.recv (1024).decode ()
        if x == 'ACK':
            break
    progress = tqdm.tqdm (range (file_size), f"发送bin文件", unit = "B", unit_divisor=1024)
    try:
        with open (Expfile_name, "rb") as f:
            for _ in range(int(file_size)):
                bytes_read = f.read(4096)
                if not bytes_read:
                    break
                client_socket.sendall(bytes_read)
                progress.update(len(bytes_read))
    except Exception as e:
        client_socket.shutdown (2)
        client_socket.close ()
    time.sleep(1)
    client_socket.shutdown(2)
    client_socket.close()



def getbin():
    client_socket = socket(AF_INET, SOCK_STREAM)
    client_socket.connect((server_ip, server_port))
    """File download"""
    path = r"D:\TP\Study\ChirpBox\ChirpBox_manager\Tools\chirpbox_experiment\upload_files\testfiles\storage"
    time_upload = datetime.datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
    file_size = client_socket.recv (1024).decode ()
    file_size = int (file_size)
    progress = tqdm.tqdm(range (file_size), f"接收bin文件", unit="B",unit_divisor=1024,unit_scale=True)
    try:
        with open(path + "\\" + time_upload + ".bin","wb") as f:
            for _ in range(file_size):
                byte_read = client_socket.recv(4096)
                if not byte_read:
                    break
                f.write(byte_read)
                progress.update(len(byte_read))
    except Exception as e:
        print (e)
        client_socket.shutdown (2)
        client_socket.close ()
        print("Experiment download fail")
    client_socket.shutdown(2)
    client_socket.close()
    return time_upload

def get_json(name):
    client_socket = socket(AF_INET, SOCK_STREAM)
    client_socket.setsockopt (SOL_SOCKET, SO_RCVBUF,1024)
    client_socket.connect((server_ip, server_port))
    """Json download"""
    path = r"D:\TP\Study\ChirpBox\ChirpBox_manager\Tools\chirpbox_experiment\upload_files\testfiles\storage"
    file_size = client_socket.recv (1024).decode ()
    file_size = int (file_size)
    progress = tqdm.tqdm(range (file_size), f"接收json文件", unit="B",unit_divisor=1024,unit_scale=True)
    try:
        with open(path + "\\" + name + ".json","wb") as f:
            for _ in range(file_size):
                byte_read = client_socket.recv(4096)
                if not byte_read:
                    break
                f.write(byte_read)
                progress.update(len(byte_read))
    except Exception:
        client_socket.shutdown (2)
        client_socket.close ()
        print("Experiment download fail")
    client_socket.shutdown(2)
    client_socket.close()
