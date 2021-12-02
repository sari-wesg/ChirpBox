import logging

from traitlets.traitlets import Int
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
import pytz
local = pytz.timezone ("Asia/Shanghai")

import plotly
plotly.offline.init_notebook_mode(connected=True)
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import plotly.express as px
from plotly.offline import iplot

import operator


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
blue_colors = ['#ebebff', '#d8d8ff', '#c4c4ff', '#b1b1ff', '#9d9dff', '#7676ff', '#4e4eff', '#3b3bff', '#2727ff', '#1414ff']
three_colors = ['#3b3bff', '#238443', '#cb181d']

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
        # plt.close('all')

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

        filename = "Heatmap" + "_SF" + str('{0:02}'.format(link_infomation[2])) + "_CH" + str('{0:06}'.format(link_infomation[3])) + "_TP" + str('{0:02}'.format(link_infomation[4])) + "_PL" + str('{0:03}'.format(link_infomation[5])) + "_UTC" + datetime.datetime.fromtimestamp(int(link_infomation[0])).strftime("%Y-%m-%d %H-%M") + self._plot_suffix
        logger.debug("save heatmap %s", filename)

        legend_text = "SF" + str('{0:02}'.format(link_infomation[2])) + "_CH" + str('{0:06}'.format(link_infomation[3])) + "_TP" + str('{0:02}'.format(link_infomation[4])) + "_PL" + str('{0:03}'.format(link_infomation[5])) + "\n" + datetime.datetime.fromtimestamp(int(link_infomation[0])).strftime("%Y-%m-%d %H:%M")

        ax.set_title(legend_text, fontsize=30)
        fig.tight_layout()
        plt.savefig(directory_path + "\\link_quality\\" + filename, bbox_inches='tight')
        plt.close(fig)
        # plt.close('all')
        # plt.ioff()


    """
    link processing
    plots: matrix to topology
    """
    def matrix_to_topology(self, link_infomation, link_matrix, node_list, using_pos, max_hop_id, directory_path):
        node_num = len(node_list)
        # Get a adjacent matrix, i.e., remove weight information
        plt.rcParams["figure.figsize"] = (20, 20)
        fig = plt.gcf()
        ax = plt.gca()

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
                pos = {0: [356, 277], 1: [463, 758], 2: [1007, 213], 3: [378, 292], 4: [1017, 37], 5: [214, 385], 6: [282, 300], 7: [375, 86], 8: [75, 121], 9: [305, 106], 10: [628, 772], 11: [316, 772], 12: [531, 210], 13: [473, 217], 14: [702, 544], 15: [429, 570], 16: [874, 617], 17: [427, 294], 18: [632, 602], 19: [811, 780], 20: [182, 610]}
            np.save(data_dir + posfilepath, pos)
        if (using_pos == 2):
            dirname = os.path.dirname(__file__)
            imgname = os.path.join(dirname, '../area.png')
            img = matplotlib.image.imread(imgname)
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
        legend_text = "SF" + str('{0:02}'.format(link_infomation[2])) + "_CH" + str('{0:06}'.format(link_infomation[3])) + "_TP" + str('{0:02}'.format(link_infomation[4])) + "_PL" + str('{0:03}'.format(link_infomation[5])) + "\n" + datetime.datetime.fromtimestamp(int(link_infomation[0])).strftime("%Y-%m-%d %H:%M")
        props = dict(boxstyle="round", facecolor="w", alpha=0.5)
        ax.text(
            0.02,
            0.98,
            legend_text,
            transform=ax.transAxes,
            fontsize=24,
            verticalalignment="top",
            bbox=props,
        )
        filename = "Networktopology" + "_SF" + str('{0:02}'.format(link_infomation[2])) + "_CH" + str('{0:06}'.format(link_infomation[3])) + "_TP" + str('{0:02}'.format(link_infomation[4])) + "_PL" + str('{0:03}'.format(link_infomation[5])) + "_UTC" + datetime.datetime.fromtimestamp(int(link_infomation[0])).strftime("%Y-%m-%d %H-%M") + self._plot_suffix
        logger.debug("save topology %s", filename)

        fig.canvas.start_event_loop(sys.float_info.min) #workaround for Exception in Tkinter callback
        plt.savefig(directory_path + "\\link_quality\\" + filename, bbox_inches='tight')
        plt.close(fig)
        # plt.close('all')
        # plt.ioff()


    def get_symmetry_of_matrix(self, the_matrix):
        # Get the symmetry of links: (https://math.stackexchange.com/questions/2048817/metric-for-how-symmetric-a-matrix-is)
        c = the_matrix
        c_t = np.transpose(c)
        c_sym = 0.5 * (c + c_t)
        c_anti = 0.5 * (c - c_t)
        # print(c_sym)
        # print(c_anti)
        norm_c_sym = np.linalg.norm(c_sym,ord=2,keepdims=True)
        norm_c_anti = np.linalg.norm(c_anti,ord=2,keepdims=True)
        # print(norm_c_sym)
        # print(norm_c_anti)
        symmetry = ((norm_c_sym - norm_c_anti) / (norm_c_sym + norm_c_anti))[0][0]
        # print(symmetry)
        return symmetry

    """
    link processing
    csv data: processing the link quality in csv named "link_quality.csv"
    """
    def processing_link_data_to_csv(self, link_infomation, link_matrix, snr_list, rssi_list, snr_avg, rssi_avg, node_temp, id_list, directory_path, filename, max_rssi_matrix, avg_rssi_matrix, min_rssi_matrix, max_snr_matrix, avg_snr_matrix, min_snr_matrix):
        """
        convert csv files to csv with columns
        utc | time | sf | tp | frequency | payload length | min_SNR | max_SNR | avg_snr | min_RSSI | max_RSSI | avg_rssi | max_hop | max_hop_id | max_degree | min_degree | mean_degree | average_temp | symmetry | node_degree (list) | node_temp (list) | node id with link (matrix) | max_rssi_matrix | avg_rssi_matrix | min_rssi_matrix | max_snr_matrix | avg_snr_matrix | min_snr_matrix
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
                        # de = de + 1 * link_matrix[cnt_degrees_rx, cnt_degrees_tx] / 100
                        de = de + 1
            node_degree[cnt_degrees_rx] = de
        mean_degree = np.mean(node_degree)
        std_dev_degree = np.std(node_degree)
        min_degree = np.amin(node_degree)
        max_degree = np.amax(node_degree)

        symmetry = self.get_symmetry_of_matrix(link_matrix)
        symmetry_matrix = np.zeros((len(id_list), len(id_list)), dtype = object)
        for i in range(node_num):
            for j in range(node_num):
                if (link_matrix[i][j] == 0) and (link_matrix[j][i] == 0):
                    symmetry_matrix[i][j] = 'null'
                    symmetry_matrix[j][i] = 'null'
                else:
                    tmp_symmetry = self.get_symmetry_of_matrix(np.array([[link_matrix[i][i],link_matrix[i][j]],[link_matrix[j][i],link_matrix[j][j]]]))
                    symmetry_matrix[i][j] = tmp_symmetry
                    symmetry_matrix[j][i] = tmp_symmetry

        link_matrix_list = link_matrix.tolist()
        max_rssi_matrix = max_rssi_matrix.tolist()
        avg_rssi_matrix = avg_rssi_matrix.tolist()
        min_rssi_matrix = min_rssi_matrix.tolist()
        max_snr_matrix = max_snr_matrix.tolist()
        avg_snr_matrix = avg_snr_matrix.tolist()
        min_snr_matrix = min_snr_matrix.tolist()
        symmetry_matrix = symmetry_matrix.tolist()
        # if any(x == 0 for x in node_temp) is False:
        if True:
            if ((len(snr_list) > 0) and (len(rssi_list) > 0)):
                link_infomation.extend((min(snr_list), max(snr_list), snr_avg, min(rssi_list), max(rssi_list), rssi_avg, max_hop, max_hop_id, max_degree, min_degree, mean_degree, statistics.mean(node_temp), symmetry))
            else:
                link_infomation.extend(('null', 'null', 'null', 'null', 'null', 'null', max_hop, max_hop_id, max_degree, min_degree, mean_degree, statistics.mean(node_temp), symmetry))
            link_infomation.insert(len(link_infomation), node_degree)
            link_infomation.insert(len(link_infomation), node_temp)
            link_infomation.insert(len(link_infomation), link_matrix_list)
            link_infomation.insert(len(link_infomation), max_rssi_matrix)
            link_infomation.insert(len(link_infomation), avg_rssi_matrix)
            link_infomation.insert(len(link_infomation), min_rssi_matrix)
            link_infomation.insert(len(link_infomation), max_snr_matrix)
            link_infomation.insert(len(link_infomation), avg_snr_matrix)
            link_infomation.insert(len(link_infomation), min_snr_matrix)
            link_infomation.insert(len(link_infomation), symmetry_matrix)

            # read and save weather data
            weather_file = filename[:-len(".csv")] + ".json"

            try:
                with open(weather_file) as data_file:
                    weather_data = json.load(data_file)
                    link_infomation.extend((weather_data['temperature']['temp']-273.15, weather_data['wind']['speed'], weather_data['wind']['deg'], weather_data['pressure']['press'], weather_data['humidity']))
            except:
                link_infomation.extend(('null', 'null', 'null', 'null', 'null'))
                pass

            with open(directory_path + '\\link_quality\\' + 'link_quality.csv', 'a', newline='') as csvfile:
                writer= csv.writer(csvfile, delimiter=',')
                writer.writerow(link_infomation)

    def check_list_stdev(self, the_list):
        try:
            return statistics.stdev(the_list)
        except:
            return 'null'

    """
    link processing
    clean csv data: processing the link_quality.csv to "link_quality_all_sf.csv" and draw plots based on the plot type
    """
    def chirpbox_link_csv_processing(self, sf_list, freq_list, id_list, directory_path, plot_type, plot_time, rx_tx_node = [0, 0], plot_y = 'null', shade_list = 'null'):
        if plot_time is not None:
            plot_data_start = plot_time[0]
            plot_data_end = plot_time[1]
            plot_time_start = plot_time[2]
            plot_time_end = plot_time[3]
            # convert time to UTC time
            plot_data_start1 = datetime.datetime.strptime(plot_data_start, "%Y-%m-%d %H:%M:%S")
            plot_data_start1 = local.localize(plot_data_start1, is_dst=None)
            utc_dt_start = datetime.datetime.timestamp(plot_data_start1)
            plot_data_end1 = datetime.datetime.strptime(plot_data_end, "%Y-%m-%d %H:%M:%S")
            plot_data_end1 = local.localize(plot_data_end1, is_dst=None)
            utc_dt_end = datetime.datetime.timestamp(plot_data_end1)

        if plot_type is None:
            return False
        else:
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

        df_link = pd.read_csv(directory_path + '\\link_quality\\link_quality.csv', header=0,
                            sep=',',
                            names= ["utc", "the_time", "sf", "channel", "tx_power", "payload_len", "min_snr", "max_snr", "avg_snr", "min_rssi", "max_rssi", "avg_rssi", "max_hop", "max_hop_id", "max_degree", "min_degree", "average_degree", "average_temperature", "symmetry", "node_degree", "node_temperature", "node_link", "max_rssi_matrix", "avg_rssi_matrix", "min_rssi_matrix", "max_snr_matrix", "avg_snr_matrix", "min_snr_matrix", "symmetry_matrix", "weather_temperature", "wind_speed", "wind_deg", "pressure", "humidity"])

        if (df_link.empty):
            logger.error("link data is None")
            return False

        if plot_time is not None:
            # date:
            df_link = df_link.loc[(df_link['utc'] >= int(utc_dt_start)) & (df_link['utc'] <= int(utc_dt_end))]
            # time:
            df_link['datetime'] = [datetime.datetime.fromtimestamp(x).strftime("%Y-%m-%d %H:%M:%S") for x in df_link['utc']]
            df_link = df_link.set_index('datetime')
            df_link.index = pd.to_datetime(df_link.index)
            # df_link = df_link.between_time(plot_time_start, plot_time_end)

            # time (hour) of day
            df_link['timehour'] = [datetime.datetime.fromtimestamp(x).strftime("%H") for x in df_link['utc']]
            # day of week
            df_link['day'] = [datetime.datetime.fromtimestamp(x).strftime("%A") for x in df_link['utc']]

        df_link['rx_temperature'] = [eval(x)[rx_tx_node[0]] for x in df_link['node_temperature']]
        df_link['tx_temperature'] = [eval(x)[rx_tx_node[1]] for x in df_link['node_temperature']]
        df_link['rxtx_temperature'] = [abs(eval(x)[rx_tx_node[0]] - eval(x)[rx_tx_node[1]]) for x in df_link['node_temperature']]
        df_link['max_link_rssi'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['max_rssi_matrix']]
        df_link['avg_link_rssi'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['avg_rssi_matrix']]
        df_link['min_link_rssi'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['min_rssi_matrix']]
        df_link['max_link_snr'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['max_snr_matrix']]
        df_link['avg_link_snr'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['avg_snr_matrix']]
        df_link['min_link_snr'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['min_snr_matrix']]
        df_link['degree'] = [sum(k > 0 for k in eval(x)[rx_tx_node[0]]) for x in df_link['node_link']]
        df_link['PRR'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['node_link']]
        df_link['link_symmetry'] = [eval(x)[rx_tx_node[0]][rx_tx_node[1]] for x in df_link['symmetry_matrix']]
        df_link['temperature_diff'] = [max(eval(x)[1:]) - min(eval(x)[1:]) for x in df_link['node_temperature']]
        df_link['max_temperature'] = [max(eval(x)) for x in df_link['node_temperature']]
        df_link['min_temperature'] = [min(eval(x)) for x in df_link['node_temperature']]
        # print(df_link.loc[df_link['temperature_diff'] == 35])

        # plot with plotly in timeline:
        plot_title = []
        if plot_y != "null" and plot_y[0] != "subplot_PRR":
            if plot_y[0] == "degree":
                name_y1 = "Number of neighbours"
            if plot_y[0] == "average_degree":
                name_y1 = "Average number of neighbours"
            elif plot_y[0] == "PRR":
                name_y1 = "PRR"
            elif plot_y[0] == "link_symmetry":
                name_y1 = "Symmetry"

            if plot_y[1] == "rx_temperature" or plot_y[1] == "tx_temperature" or plot_y[1] == "weather_temperature" or plot_y[1] == "average_temperature":
                name_y2 = "Temperature"
            elif plot_y[1] == "rxtx_temperature":
                name_y2 = "Temperature difference"
            elif plot_y[1] == "wind_speed":
                name_y2 = "Wind speed"
            elif plot_y[1] == "pressure":
                name_y2 = "Pressure"
            elif plot_y[1] == "humidity":
                name_y2 = "Humidity"

            for freq in freq_list:
                # plot the data
                fig = make_subplots(specs=[[{"secondary_y": True}]])

                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    dates=[datetime.datetime.fromtimestamp(ts) for ts in df_link_sf['utc']]

                    fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf[plot_y[0]], name = name_y1,
                    marker=dict(
                                # size=10,
                                color=red_colors[8 - 7 + 3],
                            ),
                            mode='lines+markers'
                            ))

                    if (sf == sf_list[-1]):
                        fig.add_trace(go.Scatter(x=dates, y=df_link_sf[plot_y[1]], name = name_y2,
                        marker=dict(
                                    # size=10,
                                    color='black',
                                ),
                        mode='lines+markers',
                        line = dict(width=2, dash='dot')
                        ), secondary_y=True)

                    if shade_list != "null":
                        layout_shapes = []
                        for i in range(int(len(shade_list)/2)):
                            layout_shapes.append(
                                {
                                    'type': 'rect',
                                    'xref': 'x',
                                    'yref': 'paper',
                                    'x0': shade_list[i*2],
                                    'y0': 0,
                                    'x1': shade_list[i*2+1],
                                    'y1': 1,
                                    'fillcolor': '#ff0000',
                                    'opacity': 0.2,
                                    'line': {
                                        'width': 0,
                                    }
                                }
                            )

                        fig.update_layout(shapes=layout_shapes)

                fig.update_layout(
                    # title = fig_title,
                    # legend=dict(x=-0.01, y=-0.9, orientation="h")
                    margin=dict(l=20, r=20, t=20, b=20),
                    autosize=False,
                    width=500,
                    height=500,
                    )

                fig.update_layout(legend=dict(
                    yanchor="top",
                    # y=0.99,
                    y=0.16,
                    xanchor="right",
                    x=0.935,
                    font = dict(size = 18)
                ))
                # fig.update_layout(showlegend=False)

                # Set x-axis title
                fig_title = str("{:.1f}".format(freq/1000)) + " MHz" + " collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                # fig.update_xaxes(title_text=fig_title, title_font = {"size": 24}, title_standoff = 2)
                fig.update_xaxes(tickfont = {"size": 20}, showgrid=True)
                # Set y-axes titles
                if name_y1 == "Number of neighbours" or name_y1 == "Average number of neighbours" or name_y1 == "Symmetry":
                    fig.update_yaxes(title_text=name_y1[0].upper() + name_y1[1:], title_font = {"size": 24}, title_standoff = 2, secondary_y=False, tickfont = {"size": 20}, showgrid=True)
                elif name_y1 == "PRR":
                    fig.update_yaxes(title_text=name_y1[0].upper() + name_y1[1:], title_font = {"size": 24}, title_standoff = 2, secondary_y=False, tickfont = {"size": 20}, showgrid=True, ticksuffix=".0%")

                if name_y2 == "Temperature" or name_y2 == "Temperature difference":
                    fig.update_yaxes(title_text="Temperature (°C)", title_font = {"size": 24}, title_standoff = 2, secondary_y=True, tickfont = {"size": 20}, showgrid=True)
                elif name_y2 == "Wind speed":
                    fig.update_yaxes(title_text="Wind speed (m/s)", title_font = {"size": 24}, title_standoff = 2, secondary_y=True, tickfont = {"size": 20}, showgrid=True)
                elif name_y2 == "Pressure":
                    fig.update_yaxes(title_text="Pressure (hPa)", title_font = {"size": 24}, title_standoff = 2, secondary_y=True, tickfont = {"size": 20}, showgrid=True)
                elif name_y2 == "Humidity":
                    fig.update_yaxes(title_text="Humidity (%)", title_font = {"size": 24}, title_standoff = 2, secondary_y=True, tickfont = {"size": 20}, showgrid=True)

                fig.show()

                # Change figure title for saving
                fig_title = plot_y[0][0].upper() + plot_y[0][1:] + " and " + name_y2 + " at frequency " + str("{:.1f}".format(freq/1000)) + " MHz" + "<br>" + "collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                fig_title = fig_title.replace('<br>', ' ')
                fig_title = fig_title.replace(':', '-')
                fig.write_image(directory_path + '\\link_quality\\' + fig_title + self._plot_suffix, engine="kaleido")

        # plot with plotly in timeline:
        plot_title = []
        if plot_y[0] == "subplot_PRR":
            for freq in freq_list:
                # plot the data
                fig = make_subplots(rows=3, cols=1,
                                    shared_xaxes=True,
                                    vertical_spacing=0.02,
                                    specs=[[{"secondary_y": True}],
                                        [{"secondary_y": True}],
                                        [{"secondary_y": True}]])
                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    dates=[datetime.datetime.fromtimestamp(ts) for ts in df_link_sf['utc']]

                    fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf["PRR"], name = "PRR",
                    marker=dict(
                                # size=10,
                                color=red_colors[8 - 7 + 3],
                            ),
                            mode='lines+markers'
                            ), row=1, col=1)
                    fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf["PRR"], name = "PRR1",
                    marker=dict(
                                # size=10,
                                color=red_colors[8 - 7 + 3],
                            ),
                            mode='lines+markers'
                            ), row=2, col=1)
                    fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf["PRR"], name = "PRR2",
                    marker=dict(
                                # size=10,
                                color=red_colors[8 - 7 + 3],
                            ),
                            mode='lines+markers'
                            ), row=3, col=1)

                    if (sf == sf_list[-1]):
                        fig.add_trace(go.Scatter(x=dates, y=df_link_sf["weather_temperature"], name = "Temperature",
                        marker=dict(
                                    # size=10,
                                    color='black',
                                ),
                        mode='lines+markers',
                        line = dict(width=2, dash='dot')
                        ), secondary_y=True, row=1, col=1)
                    if (sf == sf_list[-1]):
                        fig.add_trace(go.Scatter(x=dates, y=df_link_sf["wind_speed"], name = "Wind speed",
                        marker=dict(
                                    # size=10,
                                    color=green_colors[7],
                                ),
                        mode='lines+markers',
                        line = dict(width=2, dash='dot')
                        ), secondary_y=True, row=2, col=1)
                    if (sf == sf_list[-1]):
                        fig.add_trace(go.Scatter(x=dates, y=df_link_sf["humidity"], name = "Humidity",
                        marker=dict(
                                    # size=10,
                                    color=blue_colors[9],
                                ),
                        mode='lines+markers',
                        line = dict(width=2, dash='dot')
                        ), secondary_y=True, row=3, col=1)

                    if shade_list != "null":
                        layout_shapes = []
                        for i in range(int(len(shade_list)/2)):
                            layout_shapes.append(
                                {
                                    'type': 'rect',
                                    'xref': 'x',
                                    'yref': 'paper',
                                    'x0': shade_list[i*2],
                                    'y0': 0,
                                    'x1': shade_list[i*2+1],
                                    'y1': 1,
                                    'fillcolor': '#ff0000',
                                    'opacity': 0.2,
                                    'line': {
                                        'width': 0,
                                    }
                                }
                            )

                        fig.update_layout(shapes=layout_shapes)

                fig.update_layout(
                    # title = fig_title,
                    # legend=dict(x=-0.01, y=-0.9, orientation="h")
                    margin=dict(l=20, r=20, t=20, b=20),
                    autosize=False,
                    # width=500,
                    width=900,
                    height=760,
                    )

                fig.update_layout(legend=dict(
                    yanchor="top",
                    y=1.07,
                    # y=0.16,
                    xanchor="right",
                    x=0.935,
                    font = dict(size = 20)
                ),
                    legend_orientation ="h")
                # fig.update_layout(showlegend=False)
                for trace in fig['data']:
                    if(trace['name'] == "PRR1" or trace['name'] == "PRR2"): trace['showlegend'] = False

                # Set x-axis title
                fig_title = str("{:.1f}".format(freq/1000)) + " MHz" + " collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                # fig.update_xaxes(title_text=fig_title, title_font = {"size": 24}, title_standoff = 2)
                fig.update_xaxes(tickfont = {"size": 20}, showgrid=True)
                # Set y-axes titles
                fig.update_yaxes(title_text="PRR", title_font = {"size": 24}, title_standoff = 2, secondary_y=False, tickfont = {"size": 20}, showgrid=True, ticksuffix=".0%")
                fig.update_yaxes(title_text="Temperature (°C)", title_font = {"size": 24}, title_standoff = 2, secondary_y=True, tickfont = {"size": 20}, showgrid=True, row=1, col=1)
                fig.update_yaxes(title_text="Wind speed (m/s)", title_font = {"size": 24}, title_standoff = 2, secondary_y=True, tickfont = {"size": 20}, showgrid=True, row=2, col=1)
                fig.update_yaxes(title_text="Humidity (%)", title_font = {"size": 24}, title_standoff = 2, secondary_y=True, tickfont = {"size": 20}, showgrid=True, row=3, col=1)

                fig.show()

                # Change figure title for saving
                fig_title = plot_y[0][0].upper() + plot_y[0][1:] + " and " + "weather" + " at frequency " + str("{:.1f}".format(freq/1000)) + " MHz" + "<br>" + "collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                fig_title = fig_title.replace('<br>', ' ')
                fig_title = fig_title.replace(':', '-')
                fig.write_image(directory_path + '\\link_quality\\' + fig_title + self._plot_suffix, engine="kaleido")

        # plot with plotly: degree/PRR and receiver node temperature:
        if "max_min_temperature" in plot_type:
            for freq in freq_list:
                # plot the data
                fig = make_subplots(specs=[[{"secondary_y": False}]])

                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    dates=[datetime.datetime.fromtimestamp(ts) for ts in df_link_sf['utc']]

                    fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf["max_temperature"], name="Max temperature",
                    marker=dict(
                                # size=10,
                                color=red_colors[9 - 7 + 3],
                            ),
                            mode='lines+markers'
                            ))
                    fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf["min_temperature"], name="Min temperature",
                    marker=dict(
                                # size=10,
                                color=green_colors[10 - 7 + 3],
                            ),
                            mode='lines+markers'
                            ))

                    fig.add_trace(go.Scatter(x=dates, y=df_link_sf["max_temperature"], fill='tonexty',  fillcolor='rgba(169,169,169,0.3)', line=dict(color='rgba(26,150,65,0)')))
                    fig.add_trace(go.Scatter(x=dates, y=df_link_sf["min_temperature"], fill='tonexty', fillcolor='rgba(26,150,65,0)', line=dict(color='rgba(26,150,65,0)')))

                    if (sf == sf_list[-1]):
                        fig.add_trace(go.Scatter(x=dates, y=df_link_sf['weather_temperature'], name='Weather temperature',
                        marker=dict(
                                    # size=10,
                                    color='black',
                                ),
                        mode='lines+markers',
                        line = dict(width=2, dash='dot')
                        ), secondary_y=False)

                fig.update_layout(
                    # title = fig_title,
                    # legend=dict(x=-0.01, y=-0.9, orientation="h")
                    margin=dict(l=20, r=20, t=20, b=20),
                    autosize=False,
                    width=900,
                    height=500,
                    )

                fig.update_layout(legend=dict(
                    yanchor="top",
                    y=0.99,
                    # y=0.16,
                    xanchor="right",
                    x=0.995,
                    # x=0.735,
                    # x=0.775
                    font = dict(size = 18),
                    traceorder = 'normal'
                ))
                # fig.update_layout(showlegend=False)

                # set showlegend property by name of trace
                for trace in fig['data']:
                    if(trace['name'] == None): trace['showlegend'] = False

                # Set x-axis title
                fig_title = str("{:.1f}".format(freq/1000)) + " MHz" + " collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                # fig.update_xaxes(title_text=fig_title, title_font = {"size": 24}, title_standoff = 2)
                fig.update_xaxes(tickfont = {"size": 20}, showgrid=True)
                # Set y-axes titles
                fig.update_yaxes(title_text="Temperature (°C)", title_font = {"size": 24}, title_standoff = 2, secondary_y=False, tickfont = {"size": 20}, showgrid=True)

                fig.show()

                # Change figure title for saving
                fig_title ="Max and min temperature at frequency " + str("{:.1f}".format(freq/1000)) + " MHz" + "<br>" + "collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                fig_title = fig_title.replace('<br>', ' ')
                fig_title = fig_title.replace(':', '-')
                fig.write_image(directory_path + '\\link_quality\\' + fig_title + self._plot_suffix, engine="kaleido")

        # plot with plotly: degree/PRR and receiver node temperature:
        plot_title = []
        if "PRR_RSSI_plot" in plot_type:
            plot_title.append("PRR")
        if len(plot_title) > 0:
            for _plot_type in plot_title:
                for freq in freq_list:
                    # plot the data
                    # fig = make_subplots(specs=[[{"secondary_y": False}]])
                    fig = go.Figure()

                    for sf in sf_list:
                        df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                        dates=[datetime.datetime.fromtimestamp(ts) for ts in df_link_sf['utc']]

                        fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf[_plot_type], name=_plot_type[0].upper() + _plot_type[1:],
                        # fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf[_plot_type], name='Average RSSI',
                        marker=dict(
                                    # size=10,
                                    # color=green_colors[sf - 7 + 3],
                                    color=red_colors[8 - 7 + 3],
                                ),
                                mode='lines+markers',
                                yaxis="y2"
                                ))

                        fig = fig.add_trace(go.Scatter(x = dates, y = df_link_sf['avg_rssi'], name='Average RSSI',
                        marker=dict(
                                    # size=10,
                                    # color=green_colors[sf - 7 + 3],
                                    color=red_colors[8 - 7 + 3],
                                ),
                                mode='lines+markers',
                            line = dict(width=2, dash='dot')
                                ))

                        if (sf == sf_list[-1]):
                            fig.add_trace(go.Scatter(x=dates, y=df_link_sf['rx_temperature'], name='Temperature',
                            marker=dict(
                                        # size=10,
                                        color='black',
                                    ),
                            mode='lines+markers',
                            # mode='lines',
                            line = dict(width=2, dash='dot'),
                            yaxis="y3"
                            ))

                        shade_list = ["2021-05-07 00:51", "2021-05-07 12:51", "2021-05-08 04:51", "2021-05-08 14:51", "2021-05-09 04:51", "2021-05-09 12:51", "2021-05-10 04:51", "2021-05-10 14:51"]
                        layout_shapes = []
                        for i in range(int(len(shade_list)/2)):
                            layout_shapes.append(
                                {
                                    'type': 'rect',
                                    'xref': 'x',
                                    'yref': 'paper',
                                    'x0': shade_list[i*2],
                                    'y0': 0,
                                    'x1': shade_list[i*2+1],
                                    'y1': 1,
                                    'fillcolor': '#ff0000',
                                    'opacity': 0.2,
                                    'line': {
                                        'width': 0,
                                    }
                                }
                            )

                        fig.update_layout(shapes=layout_shapes)

                    # Create axis objects
                    fig.update_layout(
                        xaxis=dict(
                            domain=[0.16, 1]
                        ),
                        yaxis=dict(
                            title="Average RSSI (dBm)",
                            title_font = {"size": 24}, title_standoff = 2,
                            tickfont = {"size": 20}
                        ),
                        yaxis2=dict(
                            title=_plot_type[0].upper() + _plot_type[1:],
                            # title_font = {"size": 24}, title_standoff = 2, ticksuffix=".00%",
                            title_font = {"size": 24}, title_standoff = 2, ticksuffix=".0%",
                            tickfont = {"size": 20},
                            anchor="free",
                            overlaying="y",
                            side="left",
                            position=0
                        ),
                        yaxis3=dict(
                            title="Temperature (°C)",
                            title_font = {"size": 24}, title_standoff = 2,
                            tickfont = {"size": 20},
                            anchor="x",
                            overlaying="y",
                            side="right"
                        )
                    )

                    fig.update_layout(
                        # title = fig_title,
                        # legend=dict(x=-0.01, y=-0.9, orientation="h")
                        margin=dict(l=20, r=20, t=20, b=20),
                        autosize=False,
                        width=900,
                        height=500,
                        )

                    fig.update_layout(legend=dict(
                        yanchor="top",
                        y=0.99,
                        xanchor="left",
                        x=0.76,
                        font = dict(size = 18)
                        # x=0.775
                    ))
                    # fig.update_layout(showlegend=False)

                    # Set x-axis title
                    fig_title = str("{:.1f}".format(freq/1000)) + " MHz" + " collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                    fig.update_xaxes(tickfont = {"size": 20})
                    # Set y-axes titles
                    fig.show()

                    # Change figure title for saving
                    fig_title = _plot_type[0].upper() + _plot_type[1:] + " and temperature at frequency " + str("{:.1f}".format(freq/1000)) + " MHz" + "<br>" + "collected from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                    fig_title = fig_title.replace('<br>', ' ')
                    fig_title = fig_title.replace(':', '-')
                    fig.write_image(directory_path + '\\link_quality\\' + fig_title + self._plot_suffix, engine="kaleido")

        # plot with plotly: min/avg/max RSSI with rx node temperature:
        plot_rssi_snr = []
        if "MAX_RSSI_temperature_plot" in plot_type:
            plot_rssi_snr.append("max")
        if "AVG_RSSI_temperature_plot" in plot_type:
            plot_rssi_snr.append("avg")
        if "MIN_RSSI_temperature_plot" in plot_type:
            plot_rssi_snr.append("min")
        if "MAX_link_RSSI_temperature_plot" in plot_type:
            plot_rssi_snr.append("max_link")
        if "AVG_link_RSSI_temperature_plot" in plot_type:
            plot_rssi_snr.append("avg_link")
        if "MIN_link_RSSI_temperature_plot" in plot_type:
            plot_rssi_snr.append("min_link")
        if len(plot_rssi_snr) > 0:
            for _plot_type in plot_rssi_snr:
                for freq in freq_list:
                    # plot the data
                    fig = make_subplots(specs=[[{"secondary_y": False}]])
                    for sf in sf_list:
                        df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]

                        rssi_list = []
                        rssi_list_std = []
                        for tmp in list(set(df_link_sf['rx_temperature'].to_list())):
                            df_tmp = df_link_sf.loc[(df_link_sf['rx_temperature'] == tmp) ]
                            rssi_list.append(statistics.mean(df_tmp[_plot_type + '_rssi'].to_list()))
                            rssi_list_std.append(self.check_list_stdev(df_tmp[_plot_type + '_rssi'].to_list()))
                        fig = fig.add_trace(go.Scatter(x = list(set(df_link_sf['rx_temperature'].to_list())), y = rssi_list, name='SF - ' +str(sf),
                        marker=dict(
                                    size=10,
                                    color=red_colors[sf - 7 + 3],
                                    symbol = 'circle',
                                ),mode='lines+markers',
                                    error_y=dict(
                                    type='data',
                                    symmetric=True,
                                    array=rssi_list_std,
                                    # thickness=10,
                                    width=5,
                                    color=red_colors[sf - 7 + 3],
                                    )))

                    title_string = ''
                    if _plot_type == "max" or _plot_type == "max_link":
                        title_string = "Maximum"
                    elif _plot_type == "avg" or _plot_type == "avg_link":
                        title_string = "Average"
                    elif _plot_type == "min" or _plot_type == "min_link":
                        title_string = "Minimum"
                    fig_title = "Temperature (\u00B0C)"

                    fig.update_layout(
                        # title = fig_title,
                        # legend=dict(x=-0.01, y=-0.9, orientation="h")
                        margin=dict(l=20, r=20, t=20, b=20),
                        autosize=False,
                        width=900,
                        height=500,
                        )
                    fig.update_layout(legend=dict(
                        yanchor="top",
                        y=0.99,
                        xanchor="left",
                        x=0.85,
                        # x=0.775
                        font = dict(size = 18)
                    ))
                    # Set x-axis title
                    fig.update_xaxes(title_text=fig_title, title_font = {"size": 24}, title_standoff = 2, tickfont = {"size": 20})

                    # Set y-axes titles
                    fig.update_yaxes(title_text= title_string + " RSSI (dBm)", title_font = {"size": 24}, title_standoff = 2, secondary_y=False, tickfont = {"size": 20})
                    fig.show()

                    # Change figure title for saving
                    fig_title = title_string + " RSSI with " + "node temperature at frequency " + str("{:.1f}".format(freq/1000)) + " MHz" + "<br>" + "from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                    fig_title = fig_title.replace('<br>', ' ')
                    fig_title = fig_title.replace(':', '-')
                    fig.write_image(directory_path + '\\link_quality\\' + fig_title + self._plot_suffix, engine="kaleido")

        # plot with plotly: min/avg/max RSSI/SNR with SF with frequencies:
        plot_rssi_snr = []
        if "RSSI_SF_Freq_plot" in plot_type:
            plot_rssi_snr.append("RSSI")
        if "SNR_SF_Freq_plot" in plot_type:
            plot_rssi_snr.append("SNR")
        if len(plot_rssi_snr) > 0:
            for _plot_type in plot_rssi_snr:
                # plot the data
                fig = make_subplots(specs=[[{"secondary_y": True}]])
                # _plot_rssi_snr = ["max", "min"]
                _plot_rssi_snr = ["max", "avg", "min"]
                for _plot_rssi_snr_tmp in _plot_rssi_snr:
                    for freq in freq_list:
                        df_link_freq = df_link.loc[(df_link['channel'] == int(freq))]

                        list_mean = [np.median((df_link_freq.loc[(df_link_freq['sf'] == int(sf))])[_plot_rssi_snr_tmp + '_' + _plot_type.lower()]) for sf in sf_list]

                        # list_error_bar = [np.std((df_link_freq.loc[(df_link_freq['sf'] == int(sf))])[_plot_rssi_snr_tmp + '_' + _plot_type.lower()]) for sf in sf_list]
                        list_error_bar_up = [np.max((df_link_freq.loc[(df_link_freq['sf'] == int(sf))])[_plot_rssi_snr_tmp + '_' + _plot_type.lower()]) for sf in sf_list]
                        list_error_bar_down = [np.min((df_link_freq.loc[(df_link_freq['sf'] == int(sf))])[_plot_rssi_snr_tmp + '_' + _plot_type.lower()]) for sf in sf_list]

                        list_error_bar_up = list(map(operator.sub, list_error_bar_up, list_mean))
                        list_error_bar_down = list(map(operator.sub, list_mean, list_error_bar_down))

                        symbol_list = ['circle', 'square', 'diamond']

                        fig = fig.add_trace(go.Scatter(x = sf_list, y = list_mean,
                        name = _plot_rssi_snr_tmp[0].upper() + _plot_rssi_snr_tmp[1:] + " - " + str("{:.1f}".format(freq/1000)) + " MHz",
                        marker=dict(
                                    size=8,
                                    color=three_colors[freq_list.index(freq)],
                                    symbol = symbol_list[_plot_rssi_snr.index(_plot_rssi_snr_tmp)],
                                ),mode='markers',
                                error_y=dict(
                                    type='data',
                                    symmetric=False,
                                    array=list_error_bar_up,
                                    arrayminus=list_error_bar_down,
                                    # thickness=10,
                                    width=5,
                                    )
                                ))

                fig_title = "SF 7 - 12 at frequency " + str("{:.1f}".format(470000/1000)) + "/" + str("{:.1f}".format(480000/1000)) + "/" + str("{:.1f}".format(490000/1000)) + " MHz" + "<br>" + "from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                fig.update_layout(legend=dict(
                    yanchor="top",
                    y=0.99,
                    xanchor="left",
                    x=0.95
                ))

                fig.update_layout(
                    # title = fig_title,
                    # legend=dict(x=-0.01, y=-0.9, orientation="h")
                    margin=dict(l=20, r=20, t=20, b=20),
                    autosize=False,
                    width=900,
                    height=500,
                    )

                # Set x-axis title
                fig.update_xaxes(title_text=fig_title, title_font = {"size": 16}, title_standoff = 2)

                # Set y-axes titles
                fig.update_yaxes(title_text=_plot_type.upper(), title_font = {"size": 16}, title_standoff = 2, secondary_y=False)
                fig.show()

                # Change figure title for saving
                fig_title = "Maximum/average/minimum " + _plot_type + " with SF 7 - 12 at frequency " + str("{:.1f}".format(470000/1000)) + "/" + str("{:.1f}".format(480000/1000)) + "/" + str("{:.1f}".format(490000/1000)) + " MHz" + "<br>" + "from "+ datetime.datetime.fromtimestamp(int(utc_dt_start)).strftime("%m-%d-%Y") + " to " + datetime.datetime.fromtimestamp(int(utc_dt_end)).strftime("%m-%d-%Y")

                fig_title = fig_title.replace('<br>', ' ')
                fig_title = fig_title.replace(':', '-')
                fig_title = fig_title.replace('/', '-')
                fig.write_image(directory_path + '\\link_quality\\' + fig_title + self._plot_suffix, engine="kaleido")

        # plot1: heatmap:
        if "heatmap" in plot_type:
            for freq in freq_list:
                for sf in sf_list:
                    df_link_sf = df_link.loc[(df_link['sf'] == sf) & (df_link['channel'] == int(freq))]
                    for index, row in df_link_sf.iterrows():
                        link_infomation = row[:6].values.tolist()
                        # get node link matrix
                        index_no = df_link_sf.columns.get_loc('node_link')
                        # convert panda to list
                        link_matrix = ast.literal_eval(row[index_no])
                        # convert list to numpy
                        link_matrix = np.array(link_matrix)

                        if plot_time is not None:
                            if (int(row[0]) >= int(utc_dt_start)) and (int(row[0]) <= int(utc_dt_end)):
                                self.matrix_to_heatmap(link_infomation, link_matrix, id_list, directory_path)
                        else:
                            self.matrix_to_heatmap(link_infomation, link_matrix, id_list, directory_path)

            # plots to gif
            if(self._plot_suffix == '.png'):
                try:
                    self.processing_link_gif(directory_path, 'Heatmap', freq_list, sf_list)
                except:
                    logger.error("No heatmap plots for GIF")
                    pass

        # plot2: topology:
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
                        link_infomation = row[:6].values.tolist()
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

                        if plot_time is not None:
                            if (int(row[0]) >= int(utc_dt_start)) and (int(row[0]) <= int(utc_dt_end)):
                                self.matrix_to_topology(link_infomation, link_matrix, id_list, using_pos, max_hop_id, directory_path)
                        else:
                            self.matrix_to_topology(link_infomation, link_matrix, id_list, using_pos, max_hop_id, directory_path)

            # plots to gif
            if(self._plot_suffix == '.png'):
                try:
                    self.processing_link_gif(directory_path, 'Networktopology', freq_list, sf_list)
                except:
                    logger.error("No topology plots for GIF")
                    pass

        # if (self._plot_suffix == '.png'):
        #     try:
        #         self._chirpbox_txt.chirpbox_delete_in_dir(directory_path + '\\link_quality\\', self._plot_suffix)
        #     except:
        #         pass

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
        img.save(fp=fp_out, format='GIF', append_images=imgs,save_all=True, duration=1000, loop=0)


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
    def processing(self, sf_list, tp_list, freq_list, payload_len_list, id_list, directory_path, plot_type, plot_time, data_exist):
        if ("True" in data_exist) is False:
            try:
                shutil.rmtree(directory_path + "\\link_quality\\")
            except:
                logger.info("link_quality folder does not exist")
                pass
            # convert csv files to csv with columns:
            # initialization
            self._chirpbox_txt = lib.txt_to_csv.chirpbox_txt()
            # convert txt in directory to csv
            self._chirpbox_txt.chirpbox_txt_to_csv(directory_path, self._file_start_name)
            # convert csv in directory to the specified csv file
            self._chirpbox_txt.chirpbox_link_to_csv(directory_path, self._file_start_name, self._time_zone, id_list)
            # remove all processed files in directory
            self._chirpbox_txt.chirpbox_delete_in_dir(directory_path, '.csv')

        # TODO:
        rx_tx_node = [4, 0]
        self.chirpbox_link_csv_processing(sf_list, freq_list, id_list, directory_path, plot_type, plot_time, rx_tx_node)

