import enum
import numpy as np
import re
import networkx as nx
import os
import matplotlib.pyplot as plt
import matplotlib.style as style
import matplotlib
from mpl_toolkits.axes_grid1 import make_axes_locatable
import seaborn as sns#style.use('seaborn-paper') #sets the size of the charts
from matplotlib.colors import LinearSegmentedColormap

from PIL import Image

# Function to change the image size
def changeImageSize(maxWidth,
                    maxHeight,
                    image):

    widthRatio  = maxWidth/image.size[0]
    heightRatio = maxHeight/image.size[1]

    newWidth    = int(widthRatio*image.size[0])
    newHeight   = int(heightRatio*image.size[1])

    newImage    = image.resize((newWidth, newHeight))
    return newImage

current_node_id = 0
node_num = 0
node_num_row = 0

class STATE(enum.Enum):
    WAITING_FOR_R = 1
    WAITING_FOR_F = 2

# style
style.use('seaborn-talk') #sets the size of the charts
matplotlib.rcParams['font.family'] = "arial"
sns.set_context('poster')  #Everything is larger
# sns.set_context('paper')  #Everything is smaller
# sns.set_context('talk')  #Everything is sized for a presentation
#style.use('ggplot')
green_colors = ['#FFFFE5', '#F7FCB9', '#D9F0A3', '#ADDD8E', '#78C679', '#41AB5D', '#238443', '#006837', '#004529']
red_colors = ['#FFF5F0', '#FEE0D2', '#FCBBA1', '#FC9272', '#FB6A4A', '#EF3B2C', '#CB181D', '#A50F15', '#67000D']

def heatmap(data,
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


def annotate_heatmap(im,
                    data=None,
                    valfmt="{x:.2f}",
                    textcolors=["black", "white"],
                    threshold=None,
                    **textkw):
    """
    A function to annotate a heatmap.

    Parameters
    ----------
    im
        The AxesImage to be labeled.
    data
        Data used to annotate.  If None, the image's data is used.  Optional.
    valfmt
        The format of the annotations inside the heatmap.  This should either
        use the string format method, e.g. "$ {x:.2f}", or be a
        `matplotlib.ticker.Formatter`.  Optional.
    textcolors
        A list or array of two color specifications.  The first is used for
        values below a threshold, the second for those above.  Optional.
    threshold
        Value in data units according to which the colors from textcolors are
        applied.  If None (the default) uses the middle of the colormap as
        separation.  Optional.
    **kwargs
        All other arguments are forwarded to each call to `text` used to create
        the text labels.
    """

    if not isinstance(data, (list, np.ndarray)):
        data = im.get_array()

    # Normalize the threshold to the images color range.
    if threshold is not None:
        threshold = im.norm(threshold)
    else:
        threshold = im.norm(data.max()) / 2.

    # Set default alignment to center, but allow it to be
    # overwritten by textkw.
    kw = dict(horizontalalignment="center", verticalalignment="center")
    kw.update(textkw)

    # Get the formatter in case a string is supplied
    if isinstance(valfmt, str):
        valfmt = matplotlib.ticker.StrMethodFormatter(valfmt)

    # Loop over the data and create a `Text` for each "pixel".
    # Change the text's color depending on the data.
    texts = []
    for i in range(data.shape[0]):
        for j in range(data.shape[1]):
            kw.update(color=textcolors[int(im.norm(data[i, j]) > threshold)])
            text = im.axes.text(j, i, valfmt(data[i, j], None), **kw)
            texts.append(text)

    return texts


def topo_parser(filename, using_pos):
    print(filename)
    global node_num, node_num_row
    state = STATE.WAITING_FOR_R
    node_list = []
    # 1. compute the amount of nodes
    previous_line = ''
    str_i = ''
    i = 0
    k = 0
    with open(filename, 'r') as f:
        for line in f:
            if (line.startswith('f') and (previous_line.startswith('r 00'))):
                str_i += line[2:].strip() + ' '
                k += 1
                if (k == 2):
                    tmp = line.split()
                    node_num_row = int(tmp[1], base = 16) + int(tmp[2], base = 16) * 256
            # keep a copy of this line for the next loop
            previous_line = line
    # find the node_id
    pre_c = ''
    str_index = ''
    for i, c in enumerate(str_i):
        if (((i - 1) % 12) == 0) and (i != 0):
            str_index = '0x' + pre_c + c
            int_i = int(str_index, 16)
            node_list.append(int_i)
        pre_c = c
    i = 0
    for x in node_list:
        i += 1
        if ((x == 0) or (x == 255)) and (i != 1):
            break
    node_num = i - 1
    print(node_list)
    if (k == 1):
        node_num_row = node_num
    print(node_num, node_num_row)

    # 2. generate connectivity matrix
    con_mat = np.zeros((node_num, node_num))
    con_mat_temp = np.zeros((node_num, node_num))
    for cnt in range(node_num):
        con_mat[cnt, cnt] = 100

    # temperature of each nodes
    node_temp = []
    # 3. update the connectivity matrix
    with open(filename, 'r') as f:
        for line in f:
            if(line.startswith('rece_hash:')):
                tmp = line.split(',')
                current_node_id = int(tmp[0][10:])
                state = STATE.WAITING_FOR_F
            if(line.startswith('f ')):
                if(state == STATE.WAITING_FOR_F):
                    tmp = line.split()
                    for cnt in range(node_num_row):
                        reliability = 0
                        tx_id = int(tmp[cnt * 4 + 1], base = 16) + int(tmp[cnt * 4 + 2], base = 16) * 256
                        if (not (((cnt != 0) and (tx_id == 0)) or (tx_id > node_num - 1))):
                            reliability = (int(tmp[cnt * 4 + 3], base = 16) + int(tmp[cnt * 4 + 4], base = 16) * 256) / 100
                            if(int(node_list.index(current_node_id)) != int(node_list.index(tx_id))):
                                con_mat[node_list.index(current_node_id), node_list.index(tx_id)] = reliability

    # 4. update the connectivity matrix
    with open(filename, 'r') as f:
        for line in f:
            if(line.startswith('rece_hash:')):
                tmp = line.split(',')
                current_node_id = int(tmp[0][10:])
                state = STATE.WAITING_FOR_F
            if(line.startswith('f ')):
                if(state == STATE.WAITING_FOR_F):
                    tmp = line.split()
                    node_num_row_temp = int((node_num_row + 1) / 2) * 2 + 3
                    for cnt in range(node_num_row_temp):
                        reliability = 0
                        tx_id = int(tmp[cnt * 4 + 1], base = 16) + int(tmp[cnt * 4 + 2], base = 16) * 256
                        if (not (((cnt != 0) and (tx_id == 0)) or (tx_id > node_num - 1))):
                            reliability = (int(tmp[cnt * 4 + 3], base = 16) + int(tmp[cnt * 4 + 4], base = 16) * 256) / 100
                            if(int(node_list.index(current_node_id)) != int(node_list.index(tx_id))):
                                con_mat[node_list.index(current_node_id), node_list.index(tx_id)] = reliability
                        elif (cnt == node_num_row_temp - 1):
                            temperature = int(tmp[cnt * 4 + 1], base = 16)
                            if  ((temperature & int("0x80", 0)) == int("0x80", 0)):
                                temperature = 255 - temperature
                            else:
                                temperature = temperature * (-1)
                            node_temp.append([current_node_id, temperature])

    # 4. draw the connectivity matrix
    plt.rcParams["figure.figsize"] = (20, 20)
    fig, ax = plt.subplots()

    plt.tick_params(labelsize=30)

    plt.xlabel('Tx Node ID', fontsize=80)
    plt.ylabel('Rx Node ID', fontsize=80)
    colors = ["#FFFFFF", "#09526A"] # Experiment with this
    cm = LinearSegmentedColormap.from_list('test', colors, N=100)

    im, cbar = heatmap(con_mat,
                        node_list,
                        node_list,
                        ax=ax,
                        cmap=cm,
                        cbar_flag = True,
                        alpha_value = 1,
                        cbarlabel="Packet Reception Rate (%)")

    tmp = re.split(r'[().]', filename)
    # conf = tmp[0]
    txt_len = len("Chirpbox_connectivity_")
    conf = tmp[0][txt_len:]
    sequence_num = tmp[1]

    # ax.set_title("Connectivity matrix -- " + conf + " (" + sequence_num + ")", fontsize=30)
    fig.tight_layout()
    # TODO:
    # im.set_clim(0, 50)
    plt.savefig("Connectivity matrix -- " + conf + " (" + sequence_num + ").pdf", bbox_inches='tight')
    #plt.show()
    Connectivity_png = "Connectivity matrix -- " + conf + " (" + sequence_num + ").png"

    # 9 draw temperature
    print(node_temp)
    print(con_mat)
    # ax2 = ax.twinx()

    # # for node_id in range(node_num):
    # #     for node_id_temp in range(node_num):
    # #         con_mat_temp[node_id][node_id_temp] = node_temp[node_id][1]

    # for node_id in range(node_num):
    #     for node_id_temp in range(node_num):
    #         con_mat_temp[node_id_temp][node_id] = node_temp[node_id][1]


    # print(node_temp)
    # plt.rcParams["figure.figsize"] = (20, 20)
    # # fig, ax = plt.subplots()
    # plt.tick_params(labelsize=23)

    # plt.xlabel('TX Node ID', fontsize=23)
    # plt.ylabel('RX Node ID', fontsize=23)

    # im, cbar = heatmap(con_mat_temp,
    #                     node_list,
    #                     node_list,
    #                     ax=ax2,
    #                     cbar_flag = True,
    #                     alpha_value = 0.5,
    #                     cmap="RdPu")
    # fig.tight_layout()

    # tmp = re.split(r'[().]', filename)
    # # conf = tmp[0]
    # txt_len = len("Chirpbox_connectivity_")
    # conf = tmp[0][txt_len:]
    # sequence_num = tmp[1]

    # # ax.set_title("Connectivity matrix -- " + conf + " (" + sequence_num + ")", fontsize=30)
    # fig.tight_layout()
    # im.set_clim(0, 50)

    # plt.savefig(Connectivity_png, bbox_inches='tight')

    # 5. draw the topology
    # 5.1 Get a adjacent matrix, i.e., remove weight information
    plt.clf()
    adj_mat = np.zeros((node_num, node_num))
    for cnt_a_adj_c in range(node_num):
        for cnt_a_adj_r in range(node_num):
            if (con_mat[cnt_a_adj_r, cnt_a_adj_c] != 0):
                adj_mat[cnt_a_adj_r, cnt_a_adj_c] = 1
    print(adj_mat)
    # 5.2 G_DIR is a directional graph, G_UNDIR is a undirectional graph
    G_DIR = nx.from_numpy_matrix(np.array(adj_mat), create_using = nx.MultiDiGraph())
    G_UNDIR = nx.from_numpy_matrix(np.array(adj_mat))
    print(G_UNDIR)

    data_dir = "."
    posfilepath = r"\pos.npy"
    tmp_key = []
    for cnt in range(node_num):
        tmp_key.append(cnt)

    topo_drawing_mapping = dict(zip(tmp_key, node_list))
    G_UNDIR_MAPPING = nx.relabel_nodes(G_UNDIR, topo_drawing_mapping)
    # if os.path.exists(data_dir + posfilepath):
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
            pos = {0: [331, 309], 1: [458, 716], 2: [207, 415], 3: [340, 317], 4: [902, 284], 5: [878, 14], 6: [251, 340], 7: [320, 157], 8: [56, 217], 9: [257, 169], 10: [378, 307], 11: [329, 759], 12: [470, 242], 13: [423, 237], 14: [672, 496], 15: [424, 561], 16: [805, 526], 17: [628, 701], 18: [592, 562], 19: [763, 704], 20: [229, 645]}
        np.save(data_dir + posfilepath, pos)
    if (using_pos == 2):
        img = matplotlib.image.imread("area1.png")
        #plt.scatter(x,y,zorder=1)
        plt.imshow(img, zorder = 0)

    nx.draw(G_UNDIR_MAPPING, pos = pos, node_color = '#ADDD8E', with_labels = True)
    nx.draw_networkx_edges(G_UNDIR_MAPPING, pos = pos, edge_color = 'black', alpha = .1)
    nx.draw_networkx_labels(G_UNDIR_MAPPING, pos = pos, label_pos = 10.3)
    # plt.title('Network topology -- ' + conf + ' (' + sequence_num + ')', fontsize=30)
    plt.savefig('Network topology -- ' + conf + ' (' + sequence_num + ").pdf", bbox_inches='tight')
    #plt.show()
    #mplleaflet.show(fig=ax.figure)
    #area = smopy.Map((30.0, 119.8, 30.55, 120.5),z=10)


    # 6 Get the maximal hop
    max_hop = 0
    no_path = 0
    for cnt_degrees_rx in range(node_num):
        for cnt_degrees_tx in range(node_num):
            try:
                hop = nx.shortest_path_length(G_DIR, source = cnt_degrees_tx, target = cnt_degrees_rx)
            except nx.NetworkXNoPath:
                print("No path from " + str(node_list[cnt_degrees_tx]) + " to " + str(node_list[cnt_degrees_rx]))
                no_path = 1
            if (max_hop < hop):
                max_hop = hop
                print("So far, the maximal hop is from " + str(node_list[cnt_degrees_tx]) + " to " + str(node_list[cnt_degrees_rx]), max_hop)
        # if (no_path == 1):
        #     max_hop = max_hop + 100

    # 7 Get the Degree information of the network:
    degrees = np.zeros(node_num)
    for cnt_degrees_rx in range(node_num):
        de = 0
        for cnt_degrees_tx in range(node_num):
            if(cnt_degrees_rx != cnt_degrees_tx):
                if (con_mat[cnt_degrees_rx, cnt_degrees_tx] != 0):
                    de = de + 1
        degrees[cnt_degrees_rx] = de
    mean_degree = np.mean(degrees)
    std_dev_degree = np.std(degrees)
    min_degree = np.amin(degrees)
    max_degree = np.amax(degrees)
    print(degrees)

    # 8 Get the symmetry of links: (https://math.stackexchange.com/questions/2048817/metric-for-how-symmetric-a-matrix-is)
    c = con_mat
    c_t = np.transpose(c)
    c_sym = 0.5 * (c + c_t)
    c_anti = 0.5 * (c - c_t)
    # print(c_sym)
    # print(c_anti)
    norm_c_sym = np.linalg.norm(c_sym,ord=2,keepdims=True)
    norm_c_anti = np.linalg.norm(c_anti,ord=2,keepdims=True)
    # print(norm_c_sym)
    # print(norm_c_anti)
    sym = (norm_c_sym - norm_c_anti) / (norm_c_sym + norm_c_anti)
    # print(sym)

    # # 9 draw temperature
    # for node_id in range(node_num):
    #     for node_id_temp in range(node_num):
    #         con_mat_temp[node_id][node_id_temp] = node_temp[node_id][1]

    # plt.rcParams["figure.figsize"] = (20, 20)
    # fig, ax = plt.subplots()
    # plt.tick_params(labelsize=23)

    # plt.xlabel('TX Node ID', fontsize=23)
    # plt.ylabel('RX Node ID', fontsize=23)

    # im, cbar = heatmap(con_mat_temp,
    #                     node_list,
    #                     node_list,
    #                     ax=ax,
    #                     cbar_flag = True,
    #                     cmap="RdPu")
    # fig.tight_layout()

    # tmp = re.split(r'[().]', filename)
    # # conf = tmp[0]
    # txt_len = len("Chirpbox_connectivity_")
    # conf = tmp[0][txt_len:]
    # sequence_num = tmp[1]

    # # ax.set_title("Connectivity matrix -- " + conf + " (" + sequence_num + ")", fontsize=30)
    # fig.tight_layout()
    # im.set_clim(15, 35)
    # temperature_png = "temperature -- " + conf + " (" + sequence_num + ").png"
    # plt.savefig(temperature_png, bbox_inches='tight')

    # # Take two images for blending them together
    # image1 = Image.open(temperature_png)
    # image2 = Image.open(Connectivity_png)

    # # Make the images of uniform size
    # image3 = changeImageSize(2000, 2000, image1)
    # image4 = changeImageSize(2000, 2000, image2)

    # # Make sure images got an alpha channel
    # image5 = image3.convert("RGBA")
    # image6 = image4.convert("RGBA")

    # # alpha-blend the images with varying values of alpha
    # alphaBlended2 = Image.blend(image6, image5, alpha=.3)

    # # Display the alpha-blended images
    # # alphaBlended2.show()
    # alphaBlended_name = Connectivity_png
    # alphaBlended2.save(alphaBlended_name)

    # max_hop = 0
    # mean_degree = 0
    # std_dev_degree = 0
    # min_degree = 0
    # max_degree = 0
    # sym = [[0]*3]*3
    # node_temp = 0
    print("hhhhhhhhhhhhhhhhhhhhhhhhhhhh")
    print(max_hop, mean_degree, sym[0][0])
    print(max_hop, mean_degree, std_dev_degree, min_degree, max_degree, sym[0][0], node_temp)

    return [max_hop, mean_degree, std_dev_degree, min_degree, max_degree, sym[0][0], node_temp]