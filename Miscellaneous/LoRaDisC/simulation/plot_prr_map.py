import matplotlib.pyplot as plt
import os
import networkx as nx
import matplotlib
import numpy as np
import matplotlib.pyplot as plt
import datetime
import sys
directory_path = os.path.dirname(__file__) # Current directory of file

link_infomation = [[100.0, 0.0, 0.0, 0.5, 0.0, 59.5, 84.5, 99.0, 100.0, 71.5, 5.0, 16.0, 0.0, 30.5, 9.5, 0.0, 0.0, 99.5, 99.5, 0.0, 0.0], [100.0, 0.0, 0.0, 28.0, 0.0, 84.0, 97.5, 100.0, 100.0, 81.0, 32.5, 37.5, 0.0, 43.0, 5.5, 8.5, 0.0, 99.0, 99.5, 3.0, 0.0], [100.0, 0.0, 0.0, 38.5, 0.0, 94.0, 100.0, 99.5, 100.0, 98.0, 50.5, 67.0, 0.0, 65.0, 45.0, 37.5, 0.0, 100.0, 100.0, 30.0, 0.0], [100.0, 14.5, 0.0, 72.5, 0.0, 88.0, 97.5, 99.5, 100.0, 98.0, 57.5, 70.5, 0.0, 83.0, 54.5, 61.5, 0.0, 100.0, 100.0, 53.0, 0.0], [100.0, 34.5, 0.0, 82.0, 1.0, 100.0, 100.0, 100.0, 100.0, 99.5, 77.0, 90.0, 10.0, 85.5, 90.5, 87.5, 0.0, 100.0, 95.5, 77.0, 1.0], [100.0, 53.5, 0.0, 94.0, 14.0, 100.0, 100.0, 100.0, 99.5, 99.0, 94.0, 87.5, 11.0, 96.0, 84.5, 95.5, 0.0, 100.0, 100.0, 97.5, 13.0]]


link_matrix = [[100.0, 95.0, 0.0, 5.0, 0.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 50.0, 100.0, 100.0, 20.0, 0.0, 100.0, 95.0, 100.0, 5.0], [0.0, 100.0, 0.0, 30.0, 100.0, 100.0, 100.0, 100.0, 0.0, 0.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 85.0, 100.0], [0.0, 55.0, 100.0, 10.0, 85.0, 100.0, 75.0, 60.0, 0.0, 20.0, 100.0, 100.0, 100.0, 0.0, 90.0, 100.0, 100.0, 95.0, 85.0, 100.0, 85.0], [0.0, 0.0, 0.0, 100.0, 0.0, 100.0, 10.0, 100.0, 100.0, 100.0, 100.0, 65.0, 0.0, 5.0, 50.0, 0.0, 0.0, 100.0, 100.0, 85.0, 100.0], [0.0, 100.0, 65.0, 0.0, 100.0, 0.0, 0.0, 0.0, 0.0, 0.0, 85.0, 95.0, 100.0, 0.0, 55.0, 80.0, 95.0, 100.0, 50.0, 80.0, 45.0], [100.0, 70.0, 0.0, 75.0, 0.0, 100.0, 100.0, 10.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 90.0, 30.0, 0.0, 80.0, 95.0, 15.0, 95.0], [100.0, 100.0, 0.0, 20.0, 0.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 100.0, 90.0, 100.0], [100.0, 0.0, 0.0, 70.0, 0.0, 0.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 100.0, 100.0, 0.0, 100.0, 100.0, 100.0, 0.0], [100.0, 0.0, 0.0, 100.0, 0.0, 100.0, 75.0, 50.0, 100.0, 100.0, 0.0, 90.0, 0.0, 100.0, 0.0, 60.0, 0.0, 95.0, 100.0, 5.0, 0.0], [0.0, 0.0, 0.0, 100.0, 0.0, 100.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 0.0, 100.0, 0.0, 0.0, 0.0, 100.0, 5.0, 0.0, 0.0], [10.0, 95.0, 0.0, 95.0, 0.0, 100.0, 95.0, 100.0, 100.0, 0.0, 100.0, 100.0, 100.0, 95.0, 95.0, 100.0, 100.0, 100.0, 90.0, 100.0, 90.0], [100.0, 95.0, 0.0, 65.0, 90.0, 100.0, 95.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 95.0, 100.0, 100.0, 35.0, 90.0, 100.0, 90.0], [0.0, 100.0, 0.0, 0.0, 95.0, 90.0, 100.0, 0.0, 50.0, 0.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 95.0, 100.0, 100.0], [20.0, 100.0, 0.0, 20.0, 0.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 65.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 100.0, 5.0, 10.0], [25.0, 100.0, 0.0, 85.0, 10.0, 100.0, 100.0, 100.0, 100.0, 95.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 95.0, 100.0], [0.0, 95.0, 0.0, 95.0, 60.0, 100.0, 95.0, 100.0, 100.0, 5.0, 100.0, 100.0, 100.0, 100.0, 95.0, 100.0, 100.0, 100.0, 90.0, 100.0, 90.0], [0.0, 95.0, 0.0, 0.0, 90.0, 0.0, 0.0, 0.0, 0.0, 0.0, 100.0, 100.0, 0.0, 0.0, 100.0, 100.0, 100.0, 100.0, 90.0, 100.0, 0.0], [100.0, 100.0, 0.0, 100.0, 40.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 100.0, 100.0, 100.0, 75.0, 100.0, 100.0, 100.0, 100.0], [100.0, 100.0, 0.0, 100.0, 0.0, 100.0, 100.0, 100.0, 100.0, 15.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 90.0, 5.0], [10.0, 90.0, 0.0, 85.0, 80.0, 90.0, 90.0, 95.0, 100.0, 95.0, 100.0, 100.0, 100.0, 85.0, 85.0, 100.0, 100.0, 90.0, 80.0, 100.0, 80.0], [0.0, 100.0, 0.0, 100.0, 5.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 70.0, 100.0, 100.0, 90.0, 100.0]]

def matrix_to_topology(link_infomation, link_matrix, node_num, sf_id, tp):

    node_list = list(range(node_num))
    # Get a adjacent matrix, i.e., remove weight information
    plt.rcParams["figure.figsize"] = (20, 20)
    fig = plt.gcf()
    ax = plt.gca()

    # adjacent_matrix = np.zeros((node_num, node_num))
    # for cnt_a_adj_c in range(node_num):
    #     for cnt_a_adj_r in range(node_num):
    #         if (link_matrix[cnt_a_adj_r, cnt_a_adj_c] != 0):
    #             adjacent_matrix[cnt_a_adj_r, cnt_a_adj_c] = 1

    # G_DIR is a directional graph, G_UNDIR is a undirectional graph
    # G_DIR = nx.from_numpy_matrix(np.array(adjacent_matrix), create_using = nx.MultiDiGraph())
    G_UNDIR = nx.from_numpy_matrix(np.array(link_matrix))
    gateway_matrix = [[0]]
    G_UNDIR_gateway = nx.from_numpy_matrix(np.array(gateway_matrix))


    posfilepath = r"\pos.npy"
    tmp_key = []
    for cnt in range(node_num):
        tmp_key.append(cnt)

    tmp_key_gateway = []
    for cnt in range(1):
        tmp_key_gateway.append(cnt)

    topo_drawing_mapping = dict(zip(tmp_key, node_list))
    G_UNDIR_MAPPING = nx.relabel_nodes(G_UNDIR, topo_drawing_mapping)
    topo_drawing_mapping_gateway = dict(zip(tmp_key_gateway, [0]))
    G_UNDIR_MAPPING_gateway = nx.relabel_nodes(G_UNDIR_gateway, topo_drawing_mapping)

    # node positions according to the map
    pos = {0: [356, 277], 1: [463, 758], 2: [1007, 213], 3: [378, 292], 4: [1017, 37], 5: [214, 385], 6: [282, 300], 7: [375, 86], 8: [75, 121], 9: [305, 106], 10: [628, 772], 11: [316, 772], 12: [531, 210], 13: [473, 217], 14: [702, 544], 15: [429, 570], 16: [874, 617], 17: [427, 294], 18: [632, 602], 19: [811, 780], 20: [182, 610]}
    pos_gateway = {0: [455, 318]}
    np.save(directory_path + posfilepath, pos)
    # if (using_pos == 2):
    dirname = os.path.dirname(__file__)
    # node map file
    imgname = os.path.join(dirname, 'topology_map.png')
    img = matplotlib.image.imread(imgname)
    plt.imshow(img, zorder = 0)


    d = {i : link_infomation[sf_id][i] for i in range(0, node_num)}

    # 1. draw
    # node color by weight, '#70AD47', '#ED1F24'
    low, *_, high = sorted(d.values())
    norm = matplotlib.colors.Normalize(vmin=low, vmax=high, clip=True)
    mapper = matplotlib.cm.ScalarMappable(norm=norm, cmap=matplotlib.cm.YlGn)
    node_color_list = [mapper.to_rgba(i) for i in d.values()]
    nx.draw_networkx_nodes(G_UNDIR_MAPPING, pos = pos, node_color = node_color_list, node_size=1500, linewidths = 1, edgecolors = '#000000')

    # 2. label
    # change the label for control node
    # raw_labels = ["C"] + [str(x) for x in range(1, len(node_list))]
    raw_labels = [str(x) for x in range(0, len(node_list))]
    node_list.append(node_num)
    lab_node = dict(zip(node_list, raw_labels))
    nx.draw_networkx_labels(G_UNDIR_MAPPING, pos = pos, labels=lab_node, font_size = 18, font_weight = 'bold', font_color = '#000000')

    # for gateway, same procedure
    nx.draw_networkx_nodes(G_UNDIR_MAPPING_gateway, pos = pos_gateway, node_color = '#ED1F24', node_size=1500, linewidths = 1, edgecolors = '#000000')

    gateway_label = ["G"]
    lab_node = {0: "G"}
    nx.draw_networkx_labels(G_UNDIR_MAPPING, pos = pos_gateway, labels=lab_node, font_size = 18, font_weight = 'bold', font_color = '#000000')

    # 3. save fig
    legend_text = "SF_" + str('{0:02}'.format(sf_id + 7)) + "_TP" + str('{0:02}'.format(tp))
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

    plot_suffix = ".png"
    filename = "PRR" + "_SF" + str('{0:02}'.format(sf_id + 7)) + "_TP" + str('{0:02}'.format(tp)) + plot_suffix

    fig.canvas.start_event_loop(sys.float_info.min) #workaround for Exception in Tkinter callback
    plt.savefig(directory_path + "\\" + filename, bbox_inches='tight')
    plt.close(fig)


for i in range(6):
    matrix_to_topology(link_infomation, link_matrix, 21, i, 0)
