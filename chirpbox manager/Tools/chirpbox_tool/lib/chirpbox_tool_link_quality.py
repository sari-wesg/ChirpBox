import logging
from lib.const import *
import lib.chirpbox_tool_cbmng_command
import csv
import networkx as nx
import numpy as np
import statistics
import matplotlib.pyplot as plt
import matplotlib.style as style
import matplotlib
from mpl_toolkits.axes_grid1 import make_axes_locatable
import seaborn as sns#style.use('seaborn-paper') #sets the size of the charts
from matplotlib.colors import LinearSegmentedColormap
import datetime
from pathlib import Path
import shutil
import collections
import pandas as pd
import os

logger = logging.getLogger(__name__)

style.use('seaborn-talk') #sets the size of the charts
matplotlib.rcParams['font.family'] = "arial"
sns.set_context('poster')  #Everything is larger

class link_quality():
    def __init__(self):
        self._file_start_name = "Chirpbox_connectivity"
        self._time_zone = False

    """ link measurement """
    def measurement(self, sf_list, tp_list, freq_list, payload_len_list):
        sf_list = [i - CHIRPBOX_LINK_MIN_SF for i in sf_list]
        sf_bitmap = 0
        for i in sf_list:
            sf_bitmap += 1 << i

        self._cbmng_command = lib.chirpbox_tool_cbmng_command.cbmng_command()

        for pl in payload_len_list:
            for freq in freq_list:
                for tp in tp_list:
                    self._cbmng_command.run_command_with_json(CHIRPBOX_LINK_COMMAND, [sf_bitmap, freq, tp, pl])

    """
    link processing
    plots: heatmap
    """
    def heatmap(self,
                data,
                row_labels,
                col_labels,
                ax=None,
                cbar_kw={},
                cbarlabel="",
                cbar_flag=False,
                alpha_value = 1,
                **kwargs):
        """
        Create a heatmap from a numpy array and two lists of labels.

        Parameters
        ----------
        data
            A 2D numpy array of shape (N, M).
        row_labels
            A list or array of length N with the labels for the rows.
        col_labels
            A list or array of length M with the labels for the columns.
        ax
            A `matplotlib.axes.Axes` instance to which the heatmap is plotted.  If
            not provided, use current axes or create a new one.  Optional.
        cbar_kw
            A dictionary with arguments to `matplotlib.Figure.colorbar`.  Optional.
        cbarlabel
            The label for the colorbar.  Optional.
        **kwargs
            All other arguments are forwarded to `imshow`.
        """

        if not ax:
            ax = plt.gca()

        # Plot the heatmap
        im = ax.imshow(data, **kwargs, alpha=alpha_value)
        im.set_clim(0, 100)

        # Adjust the size of the color bar
        if (cbar_flag == True):
            divider = make_axes_locatable(ax)
            cax = divider.append_axes("right", size="5%", pad=0.05)

            # Create colorbar
            cbar = ax.figure.colorbar(im, cax=cax, **cbar_kw)
            cbar.ax.tick_params(labelsize = 60)
            cbar.ax.set_ylabel(cbarlabel, rotation = -90, va = "bottom", fontsize = 80)
        else:
            cbar = False

        # We want to show all ticks...
        ax.set_xticks(np.arange(data.shape[1]))
        ax.set_yticks(np.arange(data.shape[0]))
        # ... and label them with the respective list entries.
        ax.set_xticklabels(col_labels,fontsize=60)
        ax.set_yticklabels(row_labels,fontsize=60)
        # ax.xaxis.labelpad = 50

        # Let the horizontal axes labeling appear on top.
        ax.tick_params(top=False, bottom=True, labeltop=False, labelbottom=True)

        # Rotate the tick labels and set their alignment.
        plt.setp(ax.get_xticklabels(),
                    rotation=90,
                    # rotation=0,
                    ha="center"
                    # rotation_mode="anchor"
                    )

        # Turn spines off and create white grid.
        for edge, spine in ax.spines.items():
            spine.set_visible(False)

        ax.set_xticks(np.arange(data.shape[1] + 1) - .5, minor=True)
        ax.set_yticks(np.arange(data.shape[0] + 1) - .5, minor=True)
        ax.grid(which="minor", color="w", linestyle='-', linewidth=3)
        ax.tick_params(which="minor", bottom=False, left=False)

        return im, cbar

    """
    link processing
    plots: matrix to plots with heatmap
    """
    def matrix_to_plot(self, link_infomation, link_matrix, node_list, directory_path):
        plt.rcParams["figure.figsize"] = (20, 20)
        fig, ax = plt.subplots()

        plt.tick_params(labelsize=30)

        plt.xlabel('Tx Node ID', fontsize=80)
        plt.ylabel('Rx Node ID', fontsize=80)
        colors = ["#FFFFFF", "#09526A"] # Experiment with this
        cm = LinearSegmentedColormap.from_list('test', colors, N=100)

        im, cbar = self.heatmap(link_matrix,
                                node_list,
                                node_list,
                                ax=ax,
                                cmap=cm,
                                cbar_flag = True,
                                alpha_value = 1,
                                cbarlabel="Packet Reception Rate (%)")

        filename = "Linkquality_UTC" + datetime.datetime.fromtimestamp(link_infomation[0]).strftime("%Y%m%d%H%M%S") + "_SF" + str('{0:02}'.format(link_infomation[1])) + "_CH" + str('{0:06}'.format(link_infomation[2])) + "_TP" + str('{0:02}'.format(link_infomation[3])) + "_PL" + str('{0:03}'.format(link_infomation[4]))
        logger.debug("save file named:%s", filename + ".png")

        ax.set_title(filename, fontsize=30)
        fig.tight_layout()
        Path(directory_path + "\\link_quality\\").mkdir(parents=True, exist_ok=True)
        plt.savefig(directory_path + "\\link_quality\\" + filename + ".png", bbox_inches='tight')

    """
    link processing
    csv data: processing the link quality in csv to plots
    """
    def processing_link_data_to_csv(self, link_infomation, link_matrix, snr_list, rssi_list, node_temp, id_list, directory_path):
        """
        convert csv files to csv with columns
        utc | sf | tp | frequency | payload length | min_SNR | max_SNR | min_RSSI | max_RSSI | max_hop | max_degree | min_degree | mean_degree | average_temp | node_degree (list) | node_temp (list) | node id with link (matrix)
        """
        node_num = len(node_temp)

        # Get the maximal hop
        max_hop = 0
        adjacent_matrix = np.zeros((node_num, node_num))
        for cnt_a_adj_c in range(node_num):
            for cnt_a_adj_r in range(node_num):
                if (link_matrix[cnt_a_adj_r, cnt_a_adj_c] != 0):
                    adjacent_matrix[cnt_a_adj_r, cnt_a_adj_c] = 1
        # G_DIR is a directional graph
        G_DIR = nx.from_numpy_matrix(np.array(adjacent_matrix), create_using = nx.MultiDiGraph())
        for cnt_degrees_rx in range(node_num):
            for cnt_degrees_tx in range(node_num):
                try:
                    hop = nx.shortest_path_length(G_DIR, source = cnt_degrees_tx, target = cnt_degrees_rx)
                except nx.NetworkXNoPath:
                    # logger.debug("No path from %s to %s", cnt_degrees_tx, cnt_degrees_rx)
                    pass
                if (max_hop < hop):
                    max_hop = hop
                    # logger.debug("So far, the maximal hop is from %s to %s with hop %s", cnt_degrees_tx, cnt_degrees_rx, max_hop)

        # Get the node degree
        node_degree = np.zeros(node_num)
        for cnt_degrees_rx in range(node_num):
            de = 0
            for cnt_degrees_tx in range(node_num):
                if(cnt_degrees_rx != cnt_degrees_tx):
                    if (link_matrix[cnt_degrees_rx, cnt_degrees_tx] != 0):
                        de = de + 1
            node_degree[cnt_degrees_rx] = de
        mean_degree = np.mean(node_degree)
        std_dev_degree = np.std(node_degree)
        min_degree = np.amin(node_degree)
        max_degree = np.amax(node_degree)

        # self.matrix_to_plot(link_infomation, link_matrix, id_list, directory_path)

        link_matrix_list = link_matrix.tolist()
        link_infomation.extend((min(snr_list), max(snr_list), min(rssi_list), max(rssi_list), max_hop, max_degree, min_degree, mean_degree, statistics.mean(node_temp)))
        link_infomation.insert(len(link_infomation), node_degree)
        link_infomation.insert(len(link_infomation), node_temp)
        link_infomation.insert(len(link_infomation), link_matrix_list)

        Path(directory_path + "\\link_quality\\").mkdir(parents=True, exist_ok=True)
        with open(directory_path + '\\link_quality\\' + 'link_quality.csv', 'a', newline='') as csvfile:
            writer= csv.writer(csvfile, delimiter=',')
            writer.writerow(link_infomation)

    """
    link processing
    unique csv data: processing the link_quality.csv
    """
    def chirpbox_link_csv_processing(self, directory_path):
        with open(directory_path + '\\link_quality\\link_quality.csv', newline='') as f:
            reader = csv.reader(f)
            list_reader = list(reader)
        unique_utc = [i[0] for i in list_reader]

        # find the utc with all sf data
        unique_utc = [item for item, count in collections.Counter(unique_utc).items() if count > (12 - 7)]

        # remove rows not in that utc
        try:
            os.remove(directory_path + '\\link_quality\\' + 'link_quality_all_sf.csv')
        except:
            logger.info("link_quality_all_sf.csv does not exist")
            pass

        with open(directory_path + '\\link_quality\\' + 'link_quality_all_sf.csv', 'a', newline='') as csvfile:
            writer= csv.writer(csvfile, delimiter=',')
            for row in list_reader:
                if (row[0] in unique_utc):
                    writer.writerow([datetime.datetime.fromtimestamp(int(row[0])).strftime("%Y-%m-%d %H:%M")] + row[1:])

        # draw plots with new csv
        df_link = pd.read_csv(directory_path + '\\link_quality\\link_quality_all_sf.csv',
                            sep=',',
                            names=["utc", "sf", "channel", "tx_power", "payload_len", "min_snr", "max_snr", "min_rssi", "max_rssi", "max_hop", "max_degree", "min_degree", "average_degree", "average_temperature", "node_degree", "node_temperature", "node_link"])

        # plots:
        ax = plt.gca()
        for sf in range(7, 13):
            df_link_sf = df_link.loc[df_link['sf'] == sf]
            plt.plot(df_link_sf['utc'], df_link_sf['min_snr'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'min_snr')
            plt.plot(df_link_sf['utc'], df_link_sf['max_snr'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'max_snr')
            plt.plot(df_link_sf['utc'], df_link_sf['min_rssi'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'min_rssi')
            plt.plot(df_link_sf['utc'], df_link_sf['max_rssi'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'max_rssi')
            plt.plot(df_link_sf['utc'], df_link_sf['max_hop'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'max_hop')
            plt.plot(df_link_sf['utc'], df_link_sf['min_degree'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'min_degree')
            plt.plot(df_link_sf['utc'], df_link_sf['max_degree'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'max_degree')
            plt.plot(df_link_sf['utc'], df_link_sf['average_degree'], linestyle='None', marker='o', markersize = 10, label = str(sf) + 'average_degree')

        plt.plot(df_link['utc'], df_link['average_temperature'], linestyle='None', marker='o', markersize = 10, label = 'average_temperature')

        legend = plt.legend(loc='upper right', edgecolor='k',fontsize = 10, fancybox=True, ncol=3)
        plt.show()

    """
    link processing
    .txt in folder: process txts in the folder to csv data and plots
    """
    def processing(self, sf_list, tp_list, freq_list, payload_len_list, id_list, directory_path):
        self._chirpbox_txt = lib.txt_to_csv.chirpbox_txt()

        # convert txt in directory to csv
        self._chirpbox_txt.chirpbox_txt_to_csv(directory_path, self._file_start_name)

        # convert csv files to csv with columns
        if CHIRPBOX_LINK_DATA is not True:
            try:
                shutil.rmtree(directory_path + "\\link_quality\\")
            except:
                logger.info("link_quality folder does not exist")
                pass
            self._chirpbox_txt.chirpbox_link_to_csv(directory_path, self._file_start_name, self._time_zone, id_list)

        self.chirpbox_link_csv_processing(directory_path)

        # remove all processed files in directory
        self._chirpbox_txt.chirpbox_delete_in_dir(directory_path, '.csv')
