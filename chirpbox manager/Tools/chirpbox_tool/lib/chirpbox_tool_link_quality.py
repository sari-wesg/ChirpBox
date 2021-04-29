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
import seaborn as sns
from matplotlib.colors import LinearSegmentedColormap
import datetime
from pathlib import Path
import shutil
import collections
import pandas as pd
import os
import glob
import json
import math
import matplotlib.dates as mdates
import ast
from PIL import Image
import sys

logger = logging.getLogger(__name__)

#style.use('seaborn-paper') #sets the size of the charts
style.use('seaborn-talk') #sets the size of the charts
matplotlib.rcParams['font.family'] = "arial"
sns.set_context('poster')  #Everything is larger
#sns.set_context('paper')  #Everything is smaller
#sns.set_context('talk')  #Everything is sized for a presentation
#style.use('ggplot')
red_colors = ['#fff5f0', '#fee0d2', '#fcbba1', '#fc9272', '#fb6a4a', '#ef3b2c', '#cb181d', '#a50f15', '#67000d', '#4e000d']
green_colors = ['#ffffe5', '#f7fcb9', '#d9f0a3', '#addd8e', '#78c679', '#41ab5d', '#238443', '#006837', '#004529', '#003729']

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
        ax.set_xticklabels(col_labels,fontsize=35)
        ax.set_yticklabels(row_labels,fontsize=35)
        # ax.xaxis.labelpad = 50

        # Let the horizontal axes labeling appear on top.
        ax.tick_params(top=False, bottom=True, labeltop=False, labelbottom=True)

        # Rotate the tick labels and set their alignment.
        plt.setp(ax.get_xticklabels(),
                    # rotation=90,
                    rotation=0,
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
    plots: matrix to heatmap
    """
    def matrix_to_heatmap(self, link_infomation, link_matrix, node_list, directory_path):
        plt.rcParams["figure.figsize"] = (20, 20)
        fig, ax = plt.subplots()

        plt.tick_params(labelsize=30)

        plt.xlabel('Tx Node ID', fontsize=80)
        plt.ylabel('Rx Node ID', fontsize=80)
        colors = ["#FFFFFF", "#004529"] # Experiment with this
        cm = LinearSegmentedColormap.from_list('test', colors, N=100)

        im, cbar = self.heatmap(link_matrix,
                                node_list,
                                node_list,
                                ax=ax,
                                cmap=cm,
                                cbar_flag = True,
                                alpha_value = 1,
                                cbarlabel="Packet Reception Rate (%)")

        filename = "Heatmap" + "_SF" + str('{0:02}'.format(link_infomation[1])) + "_CH" + str('{0:06}'.format(link_infomation[2])) + "_TP" + str('{0:02}'.format(link_infomation[3])) + "_PL" + str('{0:03}'.format(link_infomation[4])) + "_UTC" + datetime.datetime.fromtimestamp(int(link_infomation[0])).strftime("%Y-%m-%d %H-%M") + self._plot_suffix
        logger.debug("save heatmap %s", filename)

        # ax.set_title(filename, fontsize=30)
        fig.tight_layout()
        plt.savefig(directory_path + "\\link_quality\\" + filename, bbox_inches='tight')
        plt.close(fig)



    """
    link processing
    plots: matrix to topology
    """
    def matrix_to_topology(self, link_infomation, link_matrix, node_list, using_pos, max_hop_id, directory_path):
        node_num = len(node_list)
        # Get a adjacent matrix, i.e., remove weight information
        plt.rcParams["figure.figsize"] = (20, 20)
        fig = plt.gcf()

        adjacent_matrix = np.zeros((node_num, node_num))
        for cnt_a_adj_c in range(node_num):
            for cnt_a_adj_r in range(node_num):
                if (link_matrix[cnt_a_adj_r, cnt_a_adj_c] != 0):
                    adjacent_matrix[cnt_a_adj_r, cnt_a_adj_c] = 1

        # G_DIR is a directional graph, G_UNDIR is a undirectional graph
        G_DIR = nx.from_numpy_matrix(np.array(adjacent_matrix), create_using = nx.MultiDiGraph())
        G_UNDIR = nx.from_numpy_matrix(np.array(link_matrix))

        data_dir = "."
        posfilepath = r"\pos.npy"
        tmp_key = []
        for cnt in range(node_num):
            tmp_key.append(cnt)

        topo_drawing_mapping = dict(zip(tmp_key, node_list))
        G_DIR_MAPPING = nx.relabel_nodes(G_DIR, topo_drawing_mapping)
        G_UNDIR_MAPPING = nx.relabel_nodes(G_UNDIR, topo_drawing_mapping)

        if (using_pos == 1):
            print(data_dir + posfilepath)
            txt = np.load(data_dir + posfilepath, allow_pickle = True)
            fixed_pos = txt.item()
            fixed_nodes = fixed_pos.keys()
            pos = nx.spring_layout(G_UNDIR_MAPPING, pos = fixed_pos, fixed = fixed_nodes, scale=3)
        else:
            if (using_pos == 0):
                pos = nx.spring_layout(G_UNDIR_MAPPING)
            if (using_pos == 2):
                pos = {0: [356, 277], 1: [463, 758], 2: [1007, 213], 3: [378, 292], 4: [1017, 37], 5: [214, 385], 6: [282, 300], 7: [375, 86], 8: [75, 121], 9: [305, 106], 10: [427, 294], 11: [316, 772], 12: [531, 210], 13: [473, 217], 14: [702, 544], 15: [429, 570], 16: [874, 617], 17: [628, 772], 18: [632, 602], 19: [811, 780], 20: [182, 610]}
            np.save(data_dir + posfilepath, pos)
        if (using_pos == 2):
            img = matplotlib.image.imread("area.png")
            # plt.scatter(x,y,zorder=1)
            plt.imshow(img, zorder = 0)

        d = dict(G_UNDIR_MAPPING.degree)
        # 1. draw
        # nx.draw(G_DIR_MAPPING, pos = pos, node_size = 0)
        nx.draw(G_UNDIR_MAPPING, pos = pos, node_size = 0)
        color_map = []
        for node in G_UNDIR_MAPPING:
            if node == 0:
                color_map.append('#ED1F24')
            else:
                color_map.append('#70AD47')
        # 2. node
        # node size by weight
        # nx.draw_networkx_nodes(G_UNDIR_MAPPING, pos = pos, node_color = color_map, node_size=[20 + v * 200 for v in d.values()], linewidths = 1, edgecolors = '#000000')
        # node color by weight
        low, *_, high = sorted(d.values())
        norm = matplotlib.colors.Normalize(vmin=low, vmax=high, clip=True)
        mapper = matplotlib.cm.ScalarMappable(norm=norm, cmap=matplotlib.cm.YlGn)
        node_color_list = [mapper.to_rgba(i) for i in d.values()]
        nx.draw_networkx_nodes(G_UNDIR_MAPPING, pos = pos, node_color = node_color_list, node_size=1500, linewidths = 1, edgecolors = '#000000')
        # 3. edge
        # edge color is related to the weight
        edges,weights = zip(*nx.get_edge_attributes(G_UNDIR_MAPPING,'weight').items())
        nx.draw_networkx_edges(G_UNDIR_MAPPING,pos = pos,edgelist=edges, edge_color=weights, width=1, edge_cmap=plt.cm.binary)
        # 4. label
        # change the label for control node
        raw_labels = ["C"] + [str(x) for x in range(1, len(node_list))]
        lab_node = dict(zip(G_UNDIR_MAPPING.nodes, raw_labels))
        nx.draw_networkx_labels(G_UNDIR_MAPPING, pos = pos, labels=lab_node, font_size = 18, font_weight = 'bold', font_color = '#000000')
        # 5. max hop in direction graph
        path = nx.shortest_path(G_DIR_MAPPING,source=max_hop_id[0],target=max_hop_id[1])
        path_edges = list(zip(path,path[1:]))
        color_map = []
        size_map = []
        for node in path:
            if node == max_hop_id[0]:
                color_map.append('#ED1F24')
            else:
                color_map.append('#1F67ED')
            size_map.append(20 + d[node] * 200)
        # nx.draw_networkx_nodes(G_DIR_MAPPING,pos,nodelist=path,node_color=color_map, node_size= size_map, linewidths = 0.5, edgecolors = '#000000')
        # edge label as hop number
        labels_list = []
        i = 0
        for edge in path_edges:
            i += 1
            labels_list.append(edge)
            labels_list.append(str(i))
        labels = {labels_list[i]: labels_list[i + 1] for i in range(0, len(labels_list), 2)}
        nx.draw_networkx_edges(G_DIR_MAPPING,pos = pos,edgelist=path_edges,edge_color='#ED1F24', width=5,arrowstyle='-|>', arrowsize=50)
        # nx.draw_networkx_edge_labels(G_DIR_MAPPING,pos = pos,edge_labels=labels, label_pos=0.5, font_size=18, font_color='#ED1F24',bbox=dict(boxstyle="round", fc="#FFFFFF",ec="#FFFFFF",alpha=0.5, lw=0))
        # 6. save fig
        filename = "Networktopology" + "_SF" + str('{0:02}'.format(link_infomation[1])) + "_CH" + str('{0:06}'.format(link_infomation[2])) + "_TP" + str('{0:02}'.format(link_infomation[3])) + "_PL" + str('{0:03}'.format(link_infomation[4])) + "_UTC" + datetime.datetime.fromtimestamp(int(link_infomation[0])).strftime("%Y-%m-%d %H-%M") + self._plot_suffix
        logger.debug("save topology %s", filename)

        fig.canvas.start_event_loop(sys.float_info.min) #workaround for Exception in Tkinter callback
        plt.savefig(directory_path + "\\link_quality\\" + filename, bbox_inches='tight')
        plt.close(fig)




    """
    link processing
    csv data: processing the link quality in csv named "link_quality.csv"
    """
    def processing_link_data_to_csv(self, link_infomation, link_matrix, snr_list, rssi_list, node_temp, id_list, directory_path, filename):
        """
        convert csv files to csv with columns
        utc | sf | tp | frequency | payload length | min_SNR | max_SNR | min_RSSI | max_RSSI | max_hop | max_hop_id | max_degree | min_degree | mean_degree | average_temp | node_degree (list) | node_temp (list) | node id with link (matrix)
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
                    max_hop_id = [int(cnt_degrees_tx), int(cnt_degrees_rx)]
                    # logger.debug("So far, the maximal hop is from %s to %s with hop %s", cnt_degrees_tx, cnt_degrees_rx, max_hop)
        if (max_hop == 0):
            max_hop = CHIRPBOX_LINK_MAXHOP_ERROR
            max_hop_id = [0, 0]

        # Get the node degree
        node_degree = np.zeros(node_num)
        for cnt_degrees_rx in range(node_num):
            de = 0
            for cnt_degrees_tx in range(node_num):
                if(cnt_degrees_rx != cnt_degrees_tx):
                    if (link_matrix[cnt_degrees_rx, cnt_degrees_tx] != 0):
                        de = de + 1 * link_matrix[cnt_degrees_rx, cnt_degrees_tx] / 100
            node_degree[cnt_degrees_rx] = de
        mean_degree = np.mean(node_degree)
        std_dev_degree = np.std(node_degree)
        min_degree = np.amin(node_degree)
        max_degree = np.amax(node_degree)

        link_matrix_list = link_matrix.tolist()
        if ((len(snr_list) > 0) and (len(rssi_list) > 0)):
            link_infomation.extend((min(snr_list), max(snr_list), min(rssi_list), max(rssi_list), max_hop, max_hop_id, max_degree, min_degree, mean_degree, statistics.mean(node_temp)))
        else:
            link_infomation.extend(('null', 'null', 'null', 'null', max_hop, max_hop_id, max_degree, min_degree, mean_degree, statistics.mean(node_temp)))
        link_infomation.insert(len(link_infomation), node_degree)
        link_infomation.insert(len(link_infomation), node_temp)
        link_infomation.insert(len(link_infomation), link_matrix_list)

        # read and save weather data
        weather_file = filename[:-len(".csv")] + ".json"

        try:
            with open(weather_file) as data_file:
                weather_data = json.load(data_file)
                link_infomation.extend((weather_data['temperature']['temp']-273.15, weather_data['wind']['speed'], weather_data['wind']['deg'], weather_data['pressure']['press'], weather_data['humidity']))
        except:
            link_infomation.extend(('null', 'null', 'null', 'null', 'null'))
            pass

        Path(directory_path + "\\link_quality\\").mkdir(parents=True, exist_ok=True)
        with open(directory_path + '\\link_quality\\' + 'link_quality.csv', 'a', newline='') as csvfile:
            writer= csv.writer(csvfile, delimiter=',')
            writer.writerow(link_infomation)

    """
    link processing
    clean csv data: processing the link_quality.csv to "link_quality_all_sf.csv" and draw plots based on the plot type
    """
    def chirpbox_link_csv_processing(self, sf_list, freq_list, id_list, directory_path, plot_type):
        if "pdf" in plot_type:
            self._plot_suffix = ".pdf"
        else:
            self._plot_suffix = ".png"
        # open the link quality data file
        try:
            with open(directory_path + '\\link_quality\\link_quality.csv', newline='') as f:
                reader = csv.reader(f)
                list_reader = list(reader)
        except:
            logger.error("No link quality data!")
            return False

        if (sf_list is None):
            sf_list = list(range(7, 13))
        if (freq_list is None):
            freq_list = [470000, 480000, 490000]

        # find the utc with all sf data
        unique_utc = [i[0] for i in list_reader]
        unique_utc = [item for item, count in collections.Counter(unique_utc).items() if count > (12 - 7)]

        # remove rows not in that utc, and write into a new file
        try:
            os.remove(directory_path + '\\link_quality\\' + 'link_quality_all_sf.csv')
        except:
            pass
        with open(directory_path + '\\link_quality\\' + 'link_quality_all_sf.csv', 'a', newline='') as csvfile:
            writer= csv.writer(csvfile, delimiter=',')
            for row in list_reader:
                if (row[0] in unique_utc):
                    writer.writerow(row)

        # draw plots from the new csv
        df_link = pd.read_csv(directory_path + '\\link_quality\\link_quality_all_sf.csv',
                            sep=',',
                            names=["utc", "sf", "channel", "tx_power", "payload_len", "min_snr", "max_snr", "min_rssi", "max_rssi", "max_hop", "max_hop_id", "max_degree", "min_degree", "average_degree", "average_temperature", "node_degree", "node_temperature", "node_link", "temp", "wind_speed", "wind_deg", "pressure", "humidity"])

        if (df_link.empty):
            logger.error("link data is None")
            return False
        # plot1: temperature:
        if "temperature" in plot_type:
            for freq in freq_list:
                fig = plt.figure(figsize=(25, 12))
                ax = plt.gca()
                ax2 = ax.twinx()
                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    dates=[datetime.datetime.fromtimestamp(ts) for ts in df_link_sf['utc']]

                    # ax.plot(dates, df_link_sf['min_snr'], linestyle='-', marker='o', markersize = 20, label = "SF" + str(sf) + 'min_snr')
                    # ax.plot(dates, df_link_sf['max_snr'], linestyle='-', marker='o', markersize = 20, label = "SF" + str(sf) + 'max_snr')
                    # ax.plot(dates, df_link_sf['min_rssi'], linestyle='-', marker='o', markersize = 20, label = "SF" + str(sf) + 'min_rssi')
                    # ax.plot(dates, df_link_sf['max_rssi'], linestyle='-', marker='o', markersize = 20, label = "SF" + str(sf) + 'max_rssi')
                    # ax.plot(dates, df_link_sf['max_hop'], linestyle='-', marker='o', markersize = 20, label = "SF" + str(sf) + 'max_hop')
                    # ax.plot(dates, df_link_sf['min_degree'], linestyle='-', marker='o', markersize = 20, label = "SF" + str(sf) + 'min_degree')
                    # ax.plot(dates, df_link_sf['max_degree'], linestyle='-', marker='o', markersize = 20, label = "SF" + str(sf) + 'max_degree')

                    ax.plot(dates, df_link_sf['average_degree'], linestyle='-', marker='o', markersize = 10, color = red_colors[sf - 7 + 2], label = "SF" + str(sf))

                    if (sf == sf_list[0]):
                        ax2.plot(dates, df_link_sf['average_temperature'], linestyle='--', marker='o', markersize = 10, label = 'temperature')

                ax.xaxis.set_major_locator(mdates.DayLocator(interval=1))
                # ax.xaxis.set_major_locator(mdates.MonthLocator(interval=1))
                ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d'))

                ax.set_ylabel('Average degree', fontsize = 50)
                ax2.set_ylabel('Aveage node temperature ($^\circ$C)', fontsize = 50)
                title = "Average degree and temperature on " + str(freq)
                # ax.set_title(title, fontsize = 50, pad=20)

                # axis range
                ax.set_xlim([pd.to_datetime('2021-04-22 00:00'), pd.to_datetime('2021-04-28 00:00')])
                ax.set_ylim(math.floor(min(df_link['average_degree'])), math.ceil(max(df_link['average_degree'])))
                ax2.set_ylim(math.floor(min(df_link['average_temperature'])), math.ceil(max(df_link['average_temperature'])))

                # Ticks and labels
                ax.tick_params(labelsize=50)
                ax2.tick_params(labelsize=50)
                ax.tick_params(which='major',width=5, length=10)
                ax2.tick_params(which='major',width=5, length=10)

                df_link_freq = df_link.loc[(df_link['channel'] == int(freq))]
                # ax.set_xlabel('Average degree among ' + str(len(id_list)) + ' nodes on ChirpBox',fontsize=50)
                plt.xticks(fontsize = 15)
                legend = ax.legend(loc='upper right',fontsize = 24, ncol=2, bbox_to_anchor=(1.0,1.0))
                legend = ax2.legend(loc='upper right',fontsize = 24, ncol=2, bbox_to_anchor=(1.0, 1.0 - 0.18))

                plt.savefig(directory_path + '\\link_quality\\' + title + self._plot_suffix, bbox_inches='tight')

                fig.canvas.manager.full_screen_toggle() # toggle fullscreen mode
                # plt.show(block=False)
                # plt.pause(3)
                plt.close(fig)

        # plot2: degree:
        if "degree" in plot_type:
            for freq in freq_list:
                fig = plt.figure(figsize=(25, 12))
                ax = plt.gca()
                ax2 = ax.twinx()

                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    ax.plot(df_link_sf['average_degree'], df_link_sf['min_rssi'], linestyle='None', marker='o', markersize = 20, color = red_colors[sf - 7 + 2], label = "SF" + str(sf) + 'min_rssi')
                    ax2.plot(df_link_sf['average_degree'], df_link_sf['min_snr'], linestyle='None', marker='^', markersize = 25, color = red_colors[sf - 7 + 2], label = "SF" + str(sf) + 'min_snr')

                ax.set_ylabel('Minimal received RSSI', fontsize = 50)
                ax2.set_ylabel('Minimal received SNR', fontsize = 50)
                title = "Minimal RSSI and SNR with degree on frequency " + str(freq)
                # ax.set_title(title, fontsize = 50, pad=20)

                # axis range
                ax.set_xlim(math.floor(min(df_link['average_degree'])), math.ceil(max(df_link['average_degree'])))
                ax.set_ylim(math.floor(min(df_link['min_rssi'])), math.ceil(max(df_link['min_rssi'])))
                ax2.set_ylim(math.floor(min(df_link['min_snr'])), math.ceil(max(df_link['min_snr'])))

                # Ticks and labels
                ax.tick_params(labelsize=50)
                ax2.tick_params(labelsize=50)
                ax.tick_params(which='major',width=5, length=10)
                ax2.tick_params(which='major',width=5, length=10)

                x_list = df_link['average_degree'].tolist()
                x_list = range(math.floor(min(x_list)), math.ceil(max(x_list))+1, 1)
                ax.set_xticks(x_list,minor=True)
                ax2.set_xticks(x_list,minor=True)
                # ax.set_yticks(range(math.floor(min(df_link['min_rssi'])), math.ceil(max(df_link['min_rssi'])) + 1, 1),minor=True)
                # ax2.set_yticks(range(math.floor(min(df_link['min_snr'])), math.ceil(max(df_link['min_snr'])) + 1, 1),minor=True)
                ax.set_xlabel('Average degree among ' + str(len(id_list)) + ' nodes on ChirpBox', fontsize=50)

                # ax2.set_xticklabels(x_list, ha='right', rotation=45)
                plt.xticks(x_list, x_list, fontsize = 15)
                # ax.set_xticks(fontsize=15)

                legend = ax.legend(loc='upper right',fontsize = 24, ncol=2, bbox_to_anchor=(1.0,1.0))
                legend = ax2.legend(loc='upper right',fontsize = 24, ncol=2, bbox_to_anchor=(1.0, 1.0 - 0.18))

                plt.savefig(directory_path + '\\link_quality\\' + title + self._plot_suffix, bbox_inches='tight')

                fig.canvas.manager.full_screen_toggle() # toggle fullscreen mode
                # plt.show(block=False)
                # plt.pause(3)
                plt.close(fig)

        # plot3: heatmap:
        if "heatmap" in plot_type:
            for freq in freq_list:
                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    for index, row in df_link_sf.iterrows():
                        link_infomation = row[:5].values.tolist()
                        # get node link matrix
                        index_no = df_link_sf.columns.get_loc('node_link')
                        # convert panda to list
                        link_matrix = ast.literal_eval(row[index_no])
                        # convert list to numpy
                        link_matrix = np.array(link_matrix)
                        self.matrix_to_heatmap(link_infomation, link_matrix, id_list, directory_path)

            # plots to gif
            try:
                self.processing_link_gif(directory_path, 'Heatmap', freq_list, sf_list)
            except:
                logger.error("No heatmap plots for GIF")
                pass

        # plot4: topology:
        if "topology" in plot_type:
            try:
                using_pos = [i for i in plot_type if i.startswith('using_pos')][0]
                using_pos = int(using_pos[-1:])
            except:
                logger.info("No using_pos is input")
                using_pos = 0
            for freq in freq_list:
                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    for index, row in df_link_sf.iterrows():
                        link_infomation = row[:5].values.tolist()
                        # get node link matrix
                        index_no = df_link_sf.columns.get_loc('node_link')
                        # convert panda to list
                        link_matrix = ast.literal_eval(row[index_no])
                        # convert list to numpy
                        link_matrix = np.array(link_matrix)

                        # get node link matrix
                        index_no = df_link_sf.columns.get_loc('max_hop_id')
                        # convert panda to list
                        max_hop_id = ast.literal_eval(row[index_no])
                        self.matrix_to_topology(link_infomation, link_matrix, id_list, using_pos, max_hop_id, directory_path)

            # plots to gif
            try:
                self.processing_link_gif(directory_path, 'Networktopology', freq_list, sf_list)
            except:
                logger.error("No topology plots for GIF")
                pass

        if (self._plot_suffix == '.png'):
            try:
                self._chirpbox_txt.chirpbox_delete_in_dir(directory_path + '\\link_quality\\', self._plot_suffix)
            except:
                pass

        return True


    """
    link processing
    gif: plots to gif
    """
    def processing_figure_to_gif(self, directory_path, file_prefix):
        # filepaths
        fp_in = directory_path + file_prefix + '*.png'
        fp_out = directory_path + file_prefix + '.gif'
        logger.debug("save gif %s", fp_out)

        # https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html#gif
        img, *imgs = [Image.open(f) for f in sorted(glob.glob(fp_in))]
        img.save(fp=fp_out, format='GIF', append_images=imgs,save_all=True, duration=400, loop=0)


    """
    link processing
    gif: link quality plots to gif
    """
    def processing_link_gif(self, directory_path, file_prefix, freq_list, sf_list):
        for freq in freq_list:
            for sf in sf_list:
                file_prefix1 = file_prefix + "_SF" + str('{0:02}'.format(sf)) + "_CH" + str('{0:06}'.format(freq))
                self.processing_figure_to_gif(directory_path + '\\link_quality\\', file_prefix1)

    """
    link processing
    .txt in folder: process txts in the folder to csv data/plots/gif
    """
    def processing(self, sf_list, tp_list, freq_list, payload_len_list, id_list, directory_path, plot_type):
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

        self.chirpbox_link_csv_processing(sf_list, freq_list, id_list, directory_path, plot_type)

        # remove all processed files in directory
        self._chirpbox_txt.chirpbox_delete_in_dir(directory_path, '.csv')
