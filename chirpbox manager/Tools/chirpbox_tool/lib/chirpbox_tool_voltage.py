import logging
from lib.const import *
import lib.txt_to_csv
import matplotlib.pyplot as plt
import datetime

logger = logging.getLogger(__name__)

class voltage():
    def __init__(self):
        self._file_utc_start_name = "version"
        self._time_zone = False
        self._voltage_value_format = ["little endian", CHIRPBOX_VOLTAGE_START, CHIRPBOX_VOLTAGE_LEN]

    def utc_node_value_plot(self, node_id, utc_list, node_id_value_list):
        for nodes in node_id:
            node_value_in_row = []
            for i in range(len(self._utc_list)):
                try:
                    node_id_index = self._node_values[i][0].index('{:02x}'.format(int(nodes)))
                    node_value_in_row.append(self._node_values[i][1][node_id_index])
                except:
                    node_value_in_row.append(CHIRPBOX_ERROR_VALUE)
                    logger.warning("cannot find node %s in list %s at datetime: %s\n", nodes, self._node_values[i][0], datetime.datetime.fromtimestamp(int(self._utc_list[i])))
            logger.debug("plot for node %s with value: %s\n", nodes, node_value_in_row)

            plt.plot([datetime.datetime.fromtimestamp(int(ts)) for ts in self._utc_list], node_value_in_row, label=nodes, marker = '.', markersize = 10)

            plt.legend(loc="upper left")
        plt.show()

    def measurement(self):
        logger.debug("measurement")
        # 1. colloct from all nodes
        # 2. save results in a folder named voltage_measurement_from_utc_to_utc

    def processing(self, node_id, directory_path):
        self._chirpbox_txt = lib.txt_to_csv.chirpbox_txt()

        # convert txt in directory to csv
        self._chirpbox_txt.chirpbox_txt_to_csv(directory_path)

        # convert csv files to utc list and node id with values
        (self._utc_list, self._node_values) = self._chirpbox_txt.chirpbox_csv_with_utc(directory_path, self._file_utc_start_name, self._time_zone, self._voltage_value_format)

        logger.debug("utc_list: %s, node_values: %s\n", self._utc_list, self._node_values)

        self.utc_node_value_plot(node_id, self._utc_list, self._node_values)
