from asyncio.windows_events import NULL
from mimetypes import init
import os
from time import sleep

def fetchStatus():
    """获取实验结果，output: true:有实验，正在运行 false：空闲（上一个实验运行完成）"""
    path = r"D:\TP\Study\ChirpBox\ChirpBox_manager\Tools\chirpbox_experiment\upload_files\testfiles"
    haveFile = "idle"
    for i in os.listdir(path):
        # 如果是缓存文件：continue
        if(i.find("~$") == 0):
            continue
        newpath = path + "\\" + i
        if(os.path.isfile(newpath)):
            haveFile = "busy"
            break
    return haveFile

def fetchResult_time():
    """output:isnew(该文件是不是崭新出炉的)  filename(最新的那个文件名)"""
    search_path = r"D:\TP\Study\ChirpBox\ChirpBox_manager\tmp"
    file_dict = {}
    result_list = os.listdir(search_path)
    for i in result_list:
        new_file = os.path.join(search_path ,  i)
        ctime = os.stat(new_file).st_ctime
        file_dict[ctime] = i
    max_ctime = max(file_dict.keys())
    newest_file = file_dict[max_ctime]

    return newest_file

def run_config():
    """自动化运行1和2脚本"""
    os.system("python -u d:\TP\Study\ChirpBox\ChirpBox_manager\Tools\chirpbox_experiment\admin\chirpbox_admin.py -connect false -version false")
    time.sleep(10)
    os.system("python -u d:\TP\Study\ChirpBox\ChirpBox_manager\Tools\chirpbox_experiment\server\chirpbox_server.py")
    time.sleep(10)

def postExp(bin_path , json_path):
    """input: absolute path of binfile and jsonfile
        部署实验
    """
    os.system(r"python -u d:\TP\Study\ChirpBox\ChirpBox_manager\Tools\chirpbox_experiment\user\chirpbox_user.py -bin "+bin_path+" -config "+json_path+" -user TP")
    sleep(10)





