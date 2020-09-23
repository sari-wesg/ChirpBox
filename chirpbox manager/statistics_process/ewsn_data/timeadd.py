import datetime

def time_add(time_string):
    time_list = []
    time_list.append(int(time_string[:4]))
    time_list.append(int(time_string[6:7]))
    time_list.append(int(time_string[8:11]))
    time_list.append(int(time_string[11:13]))
    time_list.append(int(time_string[14:16]))
    time_list.append(int(time_string[17:19]))
    time_list.append(int(time_string[40:45]))
    t = datetime.datetime(time_list[0], time_list[1], time_list[2], time_list[3], time_list[4], time_list[5])
    add_t = t + datetime.timedelta(seconds=time_list[6])
    # print(time_list[6]*243.7507)
    print("python3 process_pc_1.py -b " + str(t.year) + '-' + str(t.month) + '-' + str(t.day) + '-' + str(t.hour) + '-' + str(t.minute) + '-' + str(t.second) + ' -e ' + str(add_t.year) + '-' + str(add_t.month) + '-' + str(add_t.day) + '-' + str(add_t.hour) + '-' + str(add_t.minute) + '-' + str(add_t.second)+' -p ./gangting')
    print("python3 process_pc_1.py -b " + str(t.year) + '-' + str(t.month) + '-' + str(t.day) + '-' + str(t.hour) + '-' + str(t.minute) + '-' + str(t.second) + ' -e ' + str(add_t.year) + '-' + str(add_t.month) + '-' + str(add_t.day) + '-' + str(add_t.hour) + '-' + str(add_t.minute) + '-' + str(add_t.second)+' -p ./chepeng')
    print(str(add_t.year) + '-' + str(add_t.month) + '-' + str(add_t.day) + ' ' + str(add_t.hour) + ':' + str(add_t.minute) + ':' + str(add_t.second))


# python3 process_pc.py -b 2020-9-11-14-4-1 -e 2020-9-11-14-38-1 -p ./gangting


time_string = "2020-09-17 16:20:22,2020-09-17 16:06:00,102"
time_add(time_string)

