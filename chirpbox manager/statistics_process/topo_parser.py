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

import mplleaflet

current_node_id = 0
node_num = 0
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
    im = ax.imshow(data, **kwargs)
    
    # Adjust the size of the color bar
    divider = make_axes_locatable(ax)
    cax = divider.append_axes("right", size="5%", pad=0.05)

    # Create colorbar
    cbar = ax.figure.colorbar(im, cax=cax, **cbar_kw)
    cbar.ax.tick_params(labelsize = 23)
    cbar.ax.set_ylabel(cbarlabel, rotation = -90, va = "bottom", fontsize = 23)

    # We want to show all ticks...
    ax.set_xticks(np.arange(data.shape[1]))
    ax.set_yticks(np.arange(data.shape[0]))
    # ... and label them with the respective list entries.
    ax.set_xticklabels(col_labels)
    ax.set_yticklabels(row_labels)

    # Let the horizontal axes labeling appear on top.
    ax.tick_params(top=False, bottom=True, labeltop=False, labelbottom=True)

    # Rotate the tick labels and set their alignment.
    plt.setp(ax.get_xticklabels(),
             # rotation=90,
             rotation=0,
             ha="right",
             rotation_mode="anchor")

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
	global node_num
	state = STATE.WAITING_FOR_R
	node_list = []
	# 1. compute the amount of nodes
	with open(filename, 'r') as f:
		for line in f:
			if(line.startswith('r ')):
				tmp = line.split()
				print(line)
				current_node_id = int(tmp[1], base = 16) + int(tmp[2], base = 16) * 256
				node_list.append(current_node_id)
				node_num = node_num + 1
	# 2. generate connectivity matrix
	con_mat = np.zeros((node_num, node_num))
	for cnt in range(node_num):
		con_mat[cnt, cnt] = 100

	# 3. update the connectivity matrix
	with open(filename, 'r') as f:
	  	for line in f:
	   		if(line.startswith('r ')):
	   			print(line, end='')
	   			tmp = line.split()
	   			current_node_id = int(tmp[1], base = 16) + int(tmp[2], base = 16) * 256
	   			state = STATE.WAITING_FOR_F
	   		if(line.startswith('f ')):
	   			print(line, end='')
	   			if(state == STATE.WAITING_FOR_F):
	   				tmp = line.split()
	   				for cnt in range(node_num):
		   				tx_id = int(tmp[cnt * 4 + 1], base = 16) + int(tmp[cnt * 4 + 2], base = 16) * 256
		   				reliability = (int(tmp[cnt * 4 + 3], base = 16) + int(tmp[cnt * 4 + 4], base = 16) * 256) / 100
		   				if(int(node_list.index(current_node_id)) != int(node_list.index(tx_id))):
		   					con_mat[node_list.index(current_node_id), node_list.index(tx_id)] = reliability
	
	# 4. update the connectivity matrix
	with open(filename, 'r') as f:
	  	for line in f:
	   		if(line.startswith('r ')):
	   			print(line, end='')
	   			tmp = line.split()
	   			current_node_id = int(tmp[1], base = 16) + int(tmp[2], base = 16) * 256
	   			state = STATE.WAITING_FOR_F
	   		if(line.startswith('f ')):
	   			print(line, end='')
	   			if(state == STATE.WAITING_FOR_F):
	   				tmp = line.split()
	   				for cnt in range(node_num):
		   				tx_id = int(tmp[cnt * 4 + 1], base = 16) + int(tmp[cnt * 4 + 2], base = 16) * 256
		   				reliability = (int(tmp[cnt * 4 + 3], base = 16) + int(tmp[cnt * 4 + 4], base = 16) * 256) / 100
		   				if(int(node_list.index(current_node_id)) != int(node_list.index(tx_id))):
		   					con_mat[node_list.index(current_node_id), node_list.index(tx_id)] = reliability		   					

	# 4. draw the connectivity matrix
	plt.rcParams["figure.figsize"] = (20, 20)
	fig, ax = plt.subplots()

	plt.tick_params(labelsize=23)

	plt.xlabel('TX Node ID', fontsize=23)
	plt.ylabel('RX Node ID', fontsize=23)
	im, cbar = heatmap(con_mat,
	                   node_list,
	                   node_list,
	                   ax=ax,
	                   cmap="YlGn",
	                   cbarlabel="Receiving Packet Number")
    
	tmp = re.split(r'[().]', filename)
	conf = tmp[0]
	sequence_num = tmp[1]

	ax.set_title("Connectivity matrix -- " + conf + " (" + sequence_num + ")", fontsize=30)
	fig.tight_layout()
	im.set_clim(0, 100)
	plt.savefig("Connectivity matrix -- " + conf + " (" + sequence_num + ").pdf", bbox_inches='tight')
	#plt.show()

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
			pos = {0: [373, 338], 1: [230, 319], 2: [352, 175], 3: [409, 256]}
		# pos = {0: [373, 338], 1: [230, 319], 2: [352, 175], 3: [409, 256], 4: [249, 132], 5: [466, 184], 6: [545, 272], 7: [673, 283], 8: [217, 408], 18: [803, 267], 24: [296, 309]}
		np.save(data_dir + posfilepath, pos)
	if (using_pos == 2):		
		img = matplotlib.image.imread("area1.png")
		#plt.scatter(x,y,zorder=1)
		plt.imshow(img, zorder = 0)

	nx.draw(G_UNDIR_MAPPING, pos = pos, node_color = '#ADDD8E', with_labels = True)
	nx.draw_networkx_edges(G_UNDIR_MAPPING, pos = pos, edge_color = 'black', alpha = .1)
	nx.draw_networkx_labels(G_UNDIR_MAPPING, pos = pos, label_pos = 10.3)
	plt.title('Network topology -- ' + conf + ' (' + sequence_num + ')', fontsize=30)
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
				print("So far, the maximal hop is from " + str(node_list[cnt_degrees_tx]) + " to " + str(node_list[cnt_degrees_rx]))
		if (no_path == 1):
			max_hop = max_hop + 100
    
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

	return [max_hop, mean_degree, std_dev_degree, min_degree, max_degree, sym[0][0]]