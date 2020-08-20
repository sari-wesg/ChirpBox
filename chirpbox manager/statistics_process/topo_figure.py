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

# 3 10.476190476190476 4.425241963995788 3.0 18.0 0.7524471242877455
# 3 11.80952380952381 4.327311357270983 3.0 19.0 0.7489675251024104
# 3 12.0 4.976133515281193 4.0 20.0 0.7752743101152969
# 2 16.571428571428573 3.048363004434137 8.0 20.0 0.8661097271285673
# filename = "Chirpbox_connectivity_sf7ch470000tp0topo_payload_len200(20200813135615054714).txt"
filename = "Chirpbox_connectivity_sf7ch470000tp0topo_payload_len200(20200814215826425401).txt"
# filename = "Chirpbox_connectivity_sf7ch470000tp0topo_payload_len1(20200815033904576109).txt"
# filename = "Chirpbox_connectivity_sf12ch470000tp0topo_payload_len1(20200815034729199028).txt"
# filename = "Chirpbox_connectivity_sf7ch470000tp14topo_payload_len1(20200805170357607773).txt"

topo_parser.topo_parser(filename, 2)

# topo_process()