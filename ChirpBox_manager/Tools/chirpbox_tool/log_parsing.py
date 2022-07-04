import binascii
import datetime

LENGTH_MESG = 64
# LENGTH_FILE_NAME = 32
# LENGTH_LINE = 2
LENGTH_AGRUMENT = 32
LENGTH_VALUE = 32
NUMBER_MESG = int(2048/LENGTH_MESG)

class Message:
    def __init__(self):
        self.FILE_NAMGE = ''
        self.LINE = 0
        self.ARGUMENT = ''
        self.VALUE = 0
        self.VALUE_1 = 0
Message = Message()

total_node_num = 21
node = 0
mesg_original = ''
Mesg_file_name = ''
Mesg_line = 0
Mesg_agrument = ''
Mesg_value = 0

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
    target_str = ''
    for i in range(0,len(ori_string),8):
        integ = int(ori_string[i:i+2],16) + int(ori_string[i+2:i+4],16) * 256 + \
                int(ori_string[i+4:i+6],16) * 65536 + int(ori_string[i+6:i+8],16) * 16777216
        chara = str(integ)
        target_str = target_str + chara + '\t'
        if (i / 8) >= percent_count - 1:
            break
    return target_str

with open("C:\\Users\\tecop\\Desktop\\"+'test'+".txt","w") as f_target:

    for node in range(total_node_num):
        #open original txt
        f_original = open("C:\\Users\\tecop\\Desktop\\test_example(20220704214652893061).txt")
        line = f_original.readline()
        f_target.write("Node "+str(node)+"\n"+"{\n")
        while line:
            if line[0:9] == "rece_hash":
                line = f_original.readline()
                if (int(line[2:4],16)) == node:
                    # print('node '+str(int(line[2:4],16))+':')
                    line = f_original.readline()
                    mesg_original = mesg_original + line[2:-1]
                    mesg_original = mesg_original.replace(' ','')
                    # print(mesg_original)

            line = f_original.readline()
        #end of original file loop
        print(node)
        for i in range(0,int((len(mesg_original)/2 + LENGTH_MESG - 1)/LENGTH_MESG)-1):

            f_target.write("Mesg "+str(i+1)+":\n")

            # extract one mesg
            Mesg_splitted = mesg_original[i*LENGTH_MESG*2:(i+1)*LENGTH_MESG*2]

            # split parts
            Message.ARGUMENT = str_to_ascii(Mesg_splitted[0:LENGTH_AGRUMENT*2])
            print(Message.ARGUMENT)
            Message.VALUE = str_to_dec_4byte(Mesg_splitted[(LENGTH_AGRUMENT)*2 : (LENGTH_AGRUMENT+LENGTH_VALUE)*2])
            print(Message.VALUE)
            f_target.write("\tArgument: \n\t"+Message.ARGUMENT+"\n")
            f_target.write("\tValue:"+Message.VALUE+"\n")
            f_target.write("\n")
        # Count = 0
        mesg_original = ''
        f_original.close()
        f_target.write("}\n\n")
    # end of node loop
#end of target txt loop






