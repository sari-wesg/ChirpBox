import logging
from lib.const import *
import lib.txt_to_csv
import matplotlib.pyplot as plt
import datetime

logging.basicConfig(format='[%(filename)s:%(lineno)d] %(message)s',
                    level=logging.DEBUG)

class voltage():
    def __init__(self):
        self._file_utc_start_name = "version"
        self._time_zone = False
        self._voltage_value_format = ["little endian", CHIRPBOX_VOLTAGE_START, CHIRPBOX_VOLTAGE_LEN]

    def utc_node_value_plot(self, node_id, utc_list, node_id_value_list):
        node_value_in_row = []
        for i in range(len(self._utc_list)):
            node_value_in_row.append(self._node_values[i][1])
        node_value_in_row = [list(x) for x in zip(*node_value_in_row)]

        for nodes in node_id:
            node_id_index = self._node_values[i][0].index('{:02x}'.format(int(nodes)))

            plt.plot([datetime.datetime.fromtimestamp(int(ts)) for ts in self._utc_list], node_value_in_row[node_id_index], label=nodes, marker = '.', markersize = 10)

            plt.legend(loc="upper left")
        plt.show()

    def measurement(self):
        logging.debug("measurement")
        # 1. colloct from all nodes
        # 2. save results in a folder named voltage_measurement_from_utc_to_utc

    def processing(self, node_id, directory_path):
        self._chirpbox_txt = lib.txt_to_csv.chirpbox_txt()

        # convert txt in directory to csv
        self._chirpbox_txt.chirpbox_txt_to_csv(directory_path)

        # convert csv files to utc list and node id with values
        (self._utc_list, self._node_values) = self._chirpbox_txt.chirpbox_csv_with_utc(directory_path, self._file_utc_start_name, self._time_zone, self._voltage_value_format)

        logging.debug("utc_list: %s, node_values: %s", self._utc_list, self._node_values)

        self.utc_node_value_plot(node_id, self._utc_list, self._node_values)
