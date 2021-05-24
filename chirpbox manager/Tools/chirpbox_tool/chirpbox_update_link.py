import os
import glob
import sys
import time
import argparse
import chirpbox_tool
import lib.chirpbox_tool_link_quality
import lib.chirpbox_tool_voltage
import logging
from lib.const import *
import shutil
from pathlib import Path


logger = logging.getLogger(__name__)
logger.propagate = False
logger.setLevel(logging.DEBUG)  # <<< Added Line
logging.getLogger('matplotlib.font_manager').disabled = True

# Log config:
# Create handlers
c_handler = logging.StreamHandler()
c_handler.setLevel(logging.DEBUG)

# Create formatters and add it to handlers
c_format = logging.Formatter(
    '[%(filename)s:%(lineno)d] %(asctime)s - %(name)s - %(levelname)s - %(message)s')
c_handler.setFormatter(c_format)

# Add handlers to the logger
logger.addHandler(c_handler)

# Examples:
# logger.warning()
# logger.error()
# logger.info()
# logger.debug()

VERSION_STR = "chirpbox_update_link v0.0.0"

DESCRIPTION_STR = VERSION_STR + """
(c)2021 by tianpei021@gmail.com
"""

ACTIONS_HELP_STR = """
list of paramters:
    --spreading_factor
    --tx_power
    --frequency
    --payload_len
    --node_id
    --directory_path
    --plot_type
    --plot_date
    --data_exist

list of available actions:
    link_quality:measurement
    link_quality:processing
    voltage:measurement
    voltage:processing

list of combinations:
    chirpbox_tool.py -sf -tp -f -pl link_quality:measurement
    chirpbox_tool.py (-sf -f -data_exist -pdate) -id -dir -plot link_quality:processing
    chirpbox_tool.py voltage:measurement
    chirpbox_tool.py -id -dir voltage:processing

examples:
    chirpbox_tool.py -h
    chirpbox_tool.py -sf 7-12 -tp 0-14 -f 470000,480000,490000 -pl 8-10 link_quality:measurement
    chirpbox_tool.py -sf 7-12 -f 470000,480000,490000 -plot RSSI_SF_Freq_plot,SNR_SF_Freq_plot,average_degree,MIN_RSSI_SNR_Degree,heatmap,topology,using_pos2,pdf -pdate 2021-04-23,2021-04-24 -id 0-20 -dir "tmp" link_quality:processing
    chirpbox_tool.py -id 0-20 voltage:measurement
    chirpbox_tool.py -id 0,10,20 -dir "tmp" voltage:processing
"""
class ChirpBoxUpdateLink():

    def __init__(self):
        self._start_time = time.time()

    def check_new_file(self, savedSet, mypath, file_start, file_suffix):
        # Retrieve a set of file paths
        nameSet = set()
        for file in os.listdir(mypath):
            fullpath = os.path.join(mypath, file)
            if os.path.isfile(fullpath) and os.stat(fullpath).st_size > 0 and file.startswith(file_start) and file.endswith(file_suffix):
                nameSet.add(fullpath)

        # Compare set with saved set to find new files
        newSet = nameSet-savedSet
        if(len(newSet)):
            # Update saved set
            savedSet = nameSet

        return (len(newSet), savedSet)

    def generate_topology(self, files, mypath):
        try:
            shutil.rmtree(mypath + "\\topology\\")
        except:
            pass
        Path(mypath+"\\topology\\").mkdir(parents=True, exist_ok=True)
        for file in files:
            shutil.copyfile(file, os.path.join(mypath+"\\topology\\", os.path.basename(file)))
            logger.debug(os.path.basename(file))
            chirpbox_tool_command = "chirpbox_tool.py" + " -sf 7-12 -f 470000,480000,490000 -plot topology,using_pos2,png -id 0-20 -dir " + str(mypath+"\\topology\\") + " -data False link_quality:processing"
            logger.debug(chirpbox_tool_command.split())
            chirpbox_tool.main(chirpbox_tool_command.split())
        logger.debug("generate_topology")

    def select_update_file(self, mypath, file_start, file_suffix):
        # Start by initializing variables:
        savedSet = set()
        while True:
            newset_len, savedSet = self.check_new_file(savedSet, mypath, file_start, file_suffix)
            time.sleep(self._time)
            logger.debug("check")
            if(newset_len):
                latest_files = []
                for freq in self._f:
                    files = glob.glob(mypath + "\\" + file_start + "sf_bitmap63ch" + str(freq) + "*" + file_suffix)
                    sorted_by_mtime_ascending = sorted(
                        files, key=lambda t: os.stat(t).st_mtime)
                    latest_files.extend(sorted_by_mtime_ascending[-self._num:])
                self.generate_topology(latest_files, mypath)

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_update_link', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-sf', '--spreading_factor',
                            dest='spreading_factor', help='Input the spreading factor')
        parser.add_argument('-f', '--frequency', dest='frequency',
                            help='Input the transmit frequency')
        parser.add_argument('-dir', '--directory_path', dest='directory_path',
                            help='Input the directory path for processing')
        parser.add_argument('-data', '--data_exist', dest='data_exist',
                            help='Input the state of processed data: True/False')
        parser.add_argument('-num', '--update_file_num', dest='update_file_num',
                            help='Update number of file per time')
        parser.add_argument('-time', '--time_interval', dest='time_interval',
                            help='Input the update time interval in second')
        group_actions = parser.add_argument_group(title='actions')
        group_actions.add_argument(
            'action', nargs='*', help='actions will be processed sequentially')
        args = parser.parse_args(argv)
        self._chirpbox_tool = chirpbox_tool.ChirpBoxTool()

        self._sf = self._chirpbox_tool.convert_int_string_to_list(
            args.spreading_factor)
        self._f = self._chirpbox_tool.convert_int_string_to_list(
            args.frequency)
        self._dir = args.directory_path
        self._dir = 'D:\TP\Study\wesg\Chirpbox\ChirpBox manager\y'
        self._data = args.data_exist
        self._num = self._chirpbox_tool.convert_int_string_to_list(
        args.update_file_num)[0]
        self._time = self._chirpbox_tool.convert_int_string_to_list(
        args.time_interval)[0]
        logger.info(args)
        runtime_status = 0
        try:
            if len(self._dir) == 0:
                logger.error("No directory")
                runtime_status = 1
            try:
                file_start = "Chirpbox_connectivity_"
                file_suffix = ".txt"
                self.select_update_file(self._dir, file_start, file_suffix)
            except:
                logger.error("Action format or argument wrong %s", action)
                runtime_status = 1
        except:
            runtime_status = 1

        if runtime_status:
            logger.info("sys.exit")
            sys.exit(runtime_status)


def main(argv):
    chirpboxupdatelink = ChirpBoxUpdateLink()
    chirpboxupdatelink.start(argv[1:])

if __name__ == "__main__":
    main(sys.argv)



# select_update_file("D:\TP\Study\wesg\Chirpbox\ChirpBox manager\y", "Chirpbox_connectivity_", ".txt", 10, 1)
