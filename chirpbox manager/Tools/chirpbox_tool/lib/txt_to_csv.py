import logging
from lib.const import *
import glob
import csv
from datetime import datetime, timezone
import struct

logging.basicConfig(format='[%(filename)s:%(lineno)d] %(message)s',
                    level=logging.DEBUG)

class chirpbox_txt():
    def __init__(self):
        self._txt_suffix = ".txt"
        self._csv_suffix = ".csv"

    def convert_bytes_to_value(self, bytes_list, value_format):
        value_bytes = bytes_list[int(value_format[1]):int(value_format[1])+int(value_format[2])]
        return int("".join(value_bytes), 16)

    def read_nodes_value_from_csv(self, csv_filename, value_format):
        node_id_value_list = [[],[]]
        with open(csv_filename, "rt") as infile:
            read = csv.reader(infile)
            for row in read:
                node_id = row[0]
                value = self.convert_bytes_to_value(row[1:], value_format)

                node_id_value_list[0].append(node_id)
                node_id_value_list[1].append(value)

        # logging.debug(node_id_value_list)
        return node_id_value_list

    def chirpbox_txt_to_csv(self, directory_path):
        # 1. read txt file list
        txt_files = glob.glob(directory_path + "\*" + self._txt_suffix)
        logging.debug("txt_files: %s", txt_files)

        # 2. loop txt file list, and save node id and payload bytes to csv
        for filename in txt_files:
            node_txt_list = []

            with open(filename, 'rt') as fd:
                for line in fd:
                    if line.startswith(LORADISC_HEADER_C) and " " in line:
                        # read node id and the next line
                        node_id = line[len(LORADISC_HEADER_C):len(LORADISC_HEADER_C)+LORADISC_NODE_ID_LEN]
                        next_line = next(fd)

                        # insert new node id
                        node_txt_list.insert(len(node_txt_list),[node_id]) if node_id not in [x[0] for x in node_txt_list] else False

                        # insert lines behind node id without LORADISC_PAYLOAD_C and "\n"
                        node_id_position = [x[0] for x in node_txt_list].index(node_id)
                        node_txt_list[node_id_position].append(next_line[len(LORADISC_PAYLOAD_C):-1])

            # combine list elements with " " and split the string with " "
            for x in node_txt_list:
                i = node_txt_list.index(x)
                node_txt_list[i] = (" ".join(x)).split()

            # 3. save list to csv
            with open(filename[:-4] + ".csv", "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerows(node_txt_list)

            # logging.debug(node_txt_list)

    def chirpbox_csv_with_utc(self, directory_path, utc_start_name, utc_time_zone, value_format):
        # 1. read csv file list
        csv_files = glob.glob(directory_path + "\*" + self._csv_suffix)
        logging.debug("csv_files: %s", csv_files)

        # 2. loop csv files, and read node id and payload bytes from csv
        utc_list = []
        node_id_with_value = []
        for filename in csv_files:

            # convert to utc timestamp
            if filename.startswith(directory_path+"\\"+utc_start_name):
                utc_string = filename[-len("2000-01-31-12-00-00.csv"):-len(".csv")]
                dt = datetime(int(utc_string[0:4]), int(utc_string[5:7]), int(utc_string[8:10]), int(utc_string[11:13]), int(utc_string[14:16]), int(utc_string[17:19]), 0)

                if(utc_time_zone == True):
                    utc_list.insert(len(utc_list),int(dt.replace(tzinfo=timezone.utc).timestamp()))
                else:
                    utc_list.insert(len(utc_list),int(dt.timestamp()))

                node_id_with_value.append(self.read_nodes_value_from_csv(filename, value_format))

        # logging.debug(node_id_with_value)
        # logging.debug(utc_list)

        return (utc_list, node_id_with_value)
