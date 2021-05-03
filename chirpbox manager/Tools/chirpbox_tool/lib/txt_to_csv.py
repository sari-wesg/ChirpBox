import logging
from lib.const import *
import glob
import csv
from datetime import datetime, timezone
import os
import numpy as np
import lib.chirpbox_tool_link_quality
from pathlib import Path
import statistics

logger = logging.getLogger(__name__)

class chirpbox_txt():
    def __init__(self):
        self._txt_suffix = ".txt"
        self._csv_suffix = ".csv"

    def twos_complement(self, hexstr, bits):
        value = int(hexstr,16)
        if value & (1 << (bits-1)):
            value -= 1 << bits
        return value

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

        # logger.debug(node_id_value_list)
        return node_id_value_list

    def chirpbox_delete_in_dir(self, directory_path, file_suffix):
        for _files in glob.glob(directory_path + "\*" + file_suffix):
            os.remove(_files)

    def chirpbox_txt_to_csv(self, directory_path, file_start_name):
        # 1. read txt file list
        txt_files = glob.glob(directory_path + "\\" + file_start_name + "*" + self._txt_suffix)
        logger.debug("txt_files: %s\n", txt_files)
        # 2. loop txt file list, and save node id and payload bytes to csv
        for filename in txt_files:
            node_txt_list = []

            with open(filename, 'rt') as fd:
                for line in fd:
                    if line.startswith(LORADISC_HEADER_C) and " " in line:
                        # read node id and the next line
                        node_id = line[len(LORADISC_HEADER_C):len(LORADISC_HEADER_C)+LORADISC_NODE_ID_LEN]
                        try:
                            next_line = next(fd)
                        except:
                            next_line = None
                            logger.info("Next line is the end of the file %s", filename)
                            pass

                        if next_line is not None:

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

            # logger.debug(node_txt_list)

    def chirpbox_csv_with_utc(self, directory_path, file_start_name, utc_time_zone, value_format):
        # 1. read csv file list
        csv_files = glob.glob(directory_path + "\\" + file_start_name + "*" + self._csv_suffix)
        logger.debug("csv_files: %s\n", csv_files)

        # 2. loop csv files, and read node id and payload bytes from csv
        utc_list = []
        node_id_with_value = []
        for filename in csv_files:

            # convert to utc timestamp
            utc_string = filename[-len("2000-01-31-12-00-00.csv"):-len(".csv")]
            dt = datetime(int(utc_string[0:4]), int(utc_string[5:7]), int(utc_string[8:10]), int(utc_string[11:13]), int(utc_string[14:16]), int(utc_string[17:19]), 0)

            if(utc_time_zone == True):
                utc_list.insert(len(utc_list),int(dt.replace(tzinfo=timezone.utc).timestamp()))
            else:
                utc_list.insert(len(utc_list),int(dt.timestamp()))

            node_id_with_value.append(self.read_nodes_value_from_csv(filename, value_format))

        # logger.debug(node_id_with_value)
        # logger.debug(utc_list)

        return (utc_list, node_id_with_value)

    def check_link_node_total_num(self, node_total_num, node_value_list):
        value_pos = int(int(node_total_num * CHIRPBOX_LINK_VALUE_LEN + CHIRPBOX_LINK_MIN_VALUE_LEN - 1) / CHIRPBOX_LINK_MIN_VALUE_LEN) * CHIRPBOX_LINK_MIN_VALUE_LEN
        check = True
        try:
            for i in range(value_pos + 1, value_pos + CHIRPBOX_LINK_MIN_VALUE_LEN):
                if(node_value_list[i] != '00'):
                    check = False
                    break
        except:
            check = False
            pass
        return check

    def chirpbox_link_to_csv(self, directory_path, file_start_name, utc_time_zone, id_list):
        self._link_processing = lib.chirpbox_tool_link_quality.link_quality()

        # 1. read csv file list
        csv_files = glob.glob(directory_path + "\\" + file_start_name + "*" + self._csv_suffix)
        logger.debug("csv_files: %s\n", csv_files)
        logger.debug("row: %s\n", id_list)

        Path(directory_path + "\\link_quality\\").mkdir(parents=True, exist_ok=True)
        with open(directory_path + '\\link_quality\\' + 'link_quality.csv', 'a', newline='') as csvfile:
            writer= csv.writer(csvfile, delimiter=',')
            writer.writerow(["utc", "sf", "channel", "tx_power", "payload_len", "min_snr", "max_snr", "avg_snr", "min_rssi", "max_rssi", "avg_rssi", "max_hop", "max_hop_id", "max_degree", "min_degree", "average_degree", "average_temperature", "node_degree", "node_temperature", "node_link", "temp", "wind_speed", "wind_deg", "pressure", "humidity"])

        # 2. loop csv files, and read node id and payload bytes from csv
        node_id_with_value = []
        for filename in csv_files:
            logger.debug("filename:%s", filename)

            utc_string = filename[-len("20210418102702100827).csv"):-len(").csv")]
            dt = datetime(int(utc_string[0:4]), int(utc_string[4:6]), int(utc_string[6:8]), int(utc_string[8:10]), int(utc_string[10:12]), int(utc_string[12:14]), int(utc_string[14:16]))
            if(utc_time_zone == True):
                utc_value = int(dt.replace(tzinfo=timezone.utc).timestamp())
            else:
                utc_value = int(dt.timestamp())
            # sf:
            sf_string = filename[-len("63ch480000tp00topo_payload_len008(20210418102702100827).csv"):-len("63ch480000tp00topo_payload_len008(20210418102702100827).csv")+2]
            sf_list = []
            for i in range(6):
                if ((int(sf_string) >> i & 0b1) != 0):
                    sf_list.append(i)
            channel = int(filename[-len("480000tp00topo_payload_len008(20210418102702100827).csv"):-len("480000tp00topo_payload_len008(20210418102702100827).csv")+6])
            tx_power = int(filename[-len("00topo_payload_len008(20210418102702100827).csv"):-len("00topo_payload_len008(20210418102702100827).csv")+2])
            payload_len = int(filename[-len("008(20210418102702100827).csv"):-len("008(20210418102702100827).csv")+3])

            # obtain the node total number
            with open(filename, "rt") as infile:
                read = csv.reader(infile)
                list_read = list(read)
                try:
                    node_total_num = int(list_read[-1][0], 16) + 1
                    node_csv_list = [i[0] for i in list_read]
                except:
                    logger.info("Cannot find node infomation in file %s", filename)
                    continue
                infile.close

            id_list_hex = ['{:02x}'.format(i) for i in id_list]

            # verify the node total number is right
            if self.check_link_node_total_num(node_total_num, list_read[0][1:]) is True:
                for sf in sf_list:
                    breaker = False
                    # link matrix:
                    received_snr = []
                    received_rssi = []
                    received_snr_avg = []
                    received_rssi_avg = []
                    node_temperature = []
                    link_matrix = np.zeros((len(id_list), len(id_list)))
                    with open(filename, "rt") as infile:
                        read = csv.reader(infile)
                        list_read = list(read)
                        for id_hex in id_list_hex:
                            try:
                                node_id_position_in_csv = node_csv_list.index(id_hex)
                                node_id_position_in_id_list = id_list_hex.index(id_hex)
                                node_temp = list_read[node_id_position_in_csv][(sf+1) * (node_total_num+1) * CHIRPBOX_LINK_VALUE_LEN + 1 - CHIRPBOX_LINK_MIN_VALUE_LEN: (sf+1) * (node_total_num+1) * CHIRPBOX_LINK_VALUE_LEN + 1 - CHIRPBOX_LINK_MIN_VALUE_LEN + CHIRPBOX_LINK_TEMP_LEN]
                                node_temp = int("".join(node_temp), base = 16)
                                if ((node_temp & int("0x80", 0)) == int("0x80", 0)):
                                    node_temp = 255 - node_temp
                                else:
                                    node_temp = node_temp * (-1)
                                node_temperature.append(node_temp)
                            except:
                                logger.info("No infomation for node %s in %s", int(id_hex,16), filename)
                                breaker = True
                                break
                            for id in id_list:
                                node_id_position_in_id_list1 = id_list.index(id)
                                try:
                                    value_sf_position = sf * (node_total_num + 1) * CHIRPBOX_LINK_VALUE_LEN + 1
                                    value_reliability_position = value_sf_position + id * CHIRPBOX_LINK_VALUE_LEN

                                    node_reliability = list_read[node_id_position_in_csv][value_reliability_position : value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN]
                                    # change to little endian byte order
                                    node_reliability.reverse()
                                    node_reliability = int("".join(node_reliability), 16) / float(100)
                                    link_matrix[node_id_position_in_id_list, node_id_position_in_id_list1] = node_reliability

                                    # if the reliability is not 0, read snr and rssi information
                                    if (node_reliability != 0):
                                        # SNR:
                                        snr_min = list_read[node_id_position_in_csv][value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN: value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN]
                                        snr_min = self.twos_complement("".join(snr_min), 8)
                                        snr_max = list_read[node_id_position_in_csv][value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN: value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2]
                                        snr_max = self.twos_complement("".join(snr_max), 8)
                                        if not ((snr_min == -1) and (snr_max == -1)):
                                            received_snr.extend((snr_min, snr_max))
                                        # RSSI:
                                        rssi_min = list_read[node_id_position_in_csv][value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2: value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2 + CHIRPBOX_LINK_RSSI_LEN]
                                        rssi_min.reverse()
                                        rssi_min = self.twos_complement("".join(rssi_min), 16)
                                        rssi_max = list_read[node_id_position_in_csv][value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2 + CHIRPBOX_LINK_RSSI_LEN: value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2 + CHIRPBOX_LINK_RSSI_LEN*2]
                                        rssi_max.reverse()
                                        rssi_max = self.twos_complement("".join(rssi_max), 16)
                                        if not ((rssi_min == -1) and (rssi_max == -1)):
                                            received_rssi.extend((rssi_min, rssi_max))
                                        # Average of SNR and RSSI:
                                        received_packet_num = int(node_reliability * CHIRPBOX_LINK_PACKET_NUM / 100)

                                        snr_total = list_read[node_id_position_in_csv][value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2 + CHIRPBOX_LINK_RSSI_LEN*2: value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2 + CHIRPBOX_LINK_RSSI_LEN*2 + CHIRPBOX_LINK_AVG_LEN]
                                        snr_total.reverse()
                                        snr_total = self.twos_complement("".join(snr_total), 16)
                                        snr_avg = snr_total / received_packet_num
                                        received_snr_avg.append(snr_avg)

                                        rssi_total = list_read[node_id_position_in_csv][value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2 + CHIRPBOX_LINK_RSSI_LEN*2 + CHIRPBOX_LINK_AVG_LEN: value_reliability_position + CHIRPBOX_LINK_RELIABILITY_LEN + CHIRPBOX_LINK_SNR_LEN*2 + CHIRPBOX_LINK_RSSI_LEN*2 + CHIRPBOX_LINK_AVG_LEN*2]
                                        rssi_total.reverse()
                                        rssi_total = self.twos_complement("".join(rssi_total), 16)
                                        rssi_avg = rssi_total / received_packet_num
                                        received_rssi_avg.append(rssi_avg)
                                except:
                                    breaker = True
                                    logger.info("No infomation for node %s in node %s with sf %s in file %s", id, id_hex, sf + CHIRPBOX_LINK_SF7, filename)
                                    break
                            if breaker:
                                break
                    if breaker:
                        break
                    if breaker is not True:
                        for cnt in range(len(id_list)):
                            link_matrix[cnt, cnt] = 100
                        # logger.debug("link_matrix with sf %s \n%s", sf + CHIRPBOX_LINK_SF7, link_matrix)
                        # logger.debug("snr with sf %s \n%s", sf + CHIRPBOX_LINK_SF7, received_snr)
                        # logger.debug("rssi with sf %s \n%s", sf + CHIRPBOX_LINK_SF7, received_rssi)
                        # logger.debug("temp with sf %s \n%s", sf + CHIRPBOX_LINK_SF7, node_temperature)
                        self._link_processing.processing_link_data_to_csv([utc_value, sf + CHIRPBOX_LINK_SF7, channel, tx_power, payload_len], link_matrix, received_snr, received_rssi, statistics.mean(received_snr_avg), statistics.mean(received_rssi_avg), node_temperature, id_list, directory_path, filename)
                        # self._link_processing.processing_link_data_to_csv([datetime.fromtimestamp(int(utc_value)).strftime("%Y-%m-%d %H:%M"), sf + CHIRPBOX_LINK_SF7, channel, tx_power, payload_len], link_matrix, received_snr, received_rssi, node_temperature, id_list, directory_path, filename)