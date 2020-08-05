import topo_parser
import glob
import os

os.chdir(os.path.dirname(__file__))

def topo_process():
    current_path = os.getcwd()

    file_date_tuple_list = []
    for file in os.listdir(os.path.join(current_path)):
        if file.endswith(".txt"):
            d = os.path.getmtime(file)
            file_date_tuple = (file, d)
            file_date_tuple_list.append(file_date_tuple)
            filename = file

    #sort the tuple list by the second element which is the date
    file_date_tuple_list.sort(key=lambda x: x[1])
    for i in range(len(file_date_tuple_list)):
        filename = file_date_tuple_list[i][0]
        print("processing...", filename)
        topo_parser.topo_parser(filename, 2)


# filename = "Chirpbox_connectivity_sf7ch470000tp0topo_payload_len120(20200804115733350503).txt"
# topo_parser.topo_parser(filename, 2)

topo_process()