import logging
from lib.const import *
import lib.txt_to_csv
import matplotlib.pyplot as plt
import datetime
import matplotlib.dates as mdates
import pytz
local = pytz.timezone ("Asia/Shanghai")

logger = logging.getLogger(__name__)

class voltage():
    def __init__(self):
        self._file_start_name = "version"
        self._time_zone = False
        self._voltage_value_format = ["little endian", CHIRPBOX_VOLTAGE_START, CHIRPBOX_VOLTAGE_LEN]

    def utc_node_value_plot(self, node_id, node_id_value_list, plot_date):
        if plot_date is not None:
            plot_data_start = plot_date[0]
            plot_data_end = plot_date[1]
            # convert time to UTC time
            plot_data_start1 = datetime.datetime.strptime(plot_data_start, "%Y-%m-%d")
            plot_data_start1 = local.localize(plot_data_start1, is_dst=None)
            utc_dt_start = datetime.datetime.timestamp(plot_data_start1)
            plot_data_end1 = datetime.datetime.strptime(plot_data_end, "%Y-%m-%d")
            plot_data_end1 = local.localize(plot_data_end1, is_dst=None)
            utc_dt_end = datetime.datetime.timestamp(plot_data_end1)

        fig = plt.figure(figsize=(25, 12))
        ax = plt.gca()
        for nodes in node_id:
            node_value_in_row = []
            for i in range(len(self._utc_list)):
                try:
                    node_id_index = node_id_value_list[i][0].index('{:02x}'.format(int(nodes)))
                    node_value_in_row.append(node_id_value_list[i][1][node_id_index]/1000)
                except:
                    node_value_in_row.append(CHIRPBOX_ERROR_VALUE)
                    logger.warning("cannot find node %s in list %s at datetime: %s\n", nodes, node_id_value_list[i][0], datetime.datetime.fromtimestamp(int(self._utc_list[i])))
            logger.debug("plot for node %s with value: %s\n", nodes, node_value_in_row)

            if plot_date is not None:
                ax.set_xlim(plot_data_start1,plot_data_end1)
            ax.xaxis.set_major_locator(mdates.DayLocator(interval=1))
            # ax.xaxis.set_major_locator(mdates.MonthLocator(interval=1))
            ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d'))

            plt.plot([datetime.datetime.fromtimestamp(int(ts)) for ts in self._utc_list], node_value_in_row, label=nodes, marker = '.', markersize = 10)

            plt.legend(loc="upper left")

        # fig.canvas.manager.full_screen_toggle() # toggle fullscreen mode
        plt.show()

    def measurement(self):
        logger.debug("measurement")
        # 1. colloct from all nodes
        # 2. save results in a folder named voltage_measurement_from_utc_to_utc

    def processing(self, node_id, directory_path, plot_date):
        self._chirpbox_txt = lib.txt_to_csv.chirpbox_txt()

        # convert txt in directory to csv
        self._chirpbox_txt.chirpbox_txt_to_csv(directory_path, self._file_start_name)

        # convert csv files to utc list and node id with values
        (self._utc_list, self._node_values) = self._chirpbox_txt.chirpbox_csv_with_utc(directory_path, self._file_start_name, self._time_zone, self._voltage_value_format)

        logger.debug("utc_list: %s, node_values: %s\n", self._utc_list, self._node_values)

        self.utc_node_value_plot(node_id, self._node_values, plot_date)

        # remove all processed files in directory
        self._chirpbox_txt.chirpbox_delete_in_dir(directory_path, '.csv')
