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
from os import path


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
    --frequency
    --link_directory_path
    --web_directory_path
    --update_file_num
    --time_interval

list of available actions:
    link_quality:measurement
    link_quality:processing
    voltage:measurement
    voltage:processing

examples:
    chirpbox_update_link.py -h
    chirpbox_update_link.py -f 470000,480000,490000 -link_dir 'D:\TP\Study\wesg\Chirpbox\Chirpbox_manager\tmp' -web_dir 'D:\TP\Study\Hexo-ChirpBox\blog\themes\vexo\source\images\topology' -time 1 -num 1
"""


class ChirpBoxUpdateLink():

    def __init__(self):
        self._start_time = time.time()

    def generate_topology(self, files, mypath):
        try:
            shutil.rmtree(mypath + "\\topology\\")
        except:
            pass
        Path(mypath+"\\topology\\").mkdir(parents=True, exist_ok=True)
        # move file to subfolders
        for file in files:
            shutil.copyfile(file, os.path.join(
                mypath+"\\topology\\", os.path.basename(file)))
        # generate topology figures
        chirpbox_tool_command = "chirpbox_tool.py" + " -sf 7-12 -f 470000,480000,490000 -plot topology,using_pos2,png -id 0-20 -dir " + \
            str(mypath+"\\topology\\") + " -data False link_quality:processing"
        try:
            chirpbox_tool.main(chirpbox_tool_command.split())
        except:
            logger.error("generate_topology")
        figures = glob.glob(mypath+"\\topology\\link_quality\\" + "\*" + ".png")
        # split figures based on freqency to subfolders
        for freq in self._f:
            Path(mypath+"\\topology\\"+str(freq)).mkdir(parents=True, exist_ok=True)
        for figure in figures:
            figure_freq = int(os.path.basename(figure)[len("Networktopology_SF07_CH"):len("Networktopology_SF07_CH470000")])
            shutil.copyfile(figure, os.path.join(
                mypath+"\\topology\\"+str(figure_freq), os.path.basename(figure)))

        # generate GIF on freqency
        for freq in self._f:
            file_prefix = "Networktopology"
            self.link_quality = lib.chirpbox_tool_link_quality.link_quality()
            try:
                self.link_quality.processing_figure_to_gif(mypath+"\\topology\\"+str(freq)+"\\", file_prefix)
                # rename figure based on frequency
                os.rename(mypath+"\\topology\\"+str(freq)+"\\"+"Networktopology.gif", mypath+"\\topology\\"+str(freq)+"\\"+"Networktopology_" + str(freq) + ".gif")
            except:
                pass

        # move GIF to website
        for freq in self._f:
            if path.exists(os.path.join(mypath, "topology", str(freq), "Networktopology_" + str(freq) + ".gif")) is True:
                os.remove(os.path.join(self.web_dir, "Networktopology_" + str(freq) + ".gif"))
                shutil.copyfile(mypath+"\\topology\\"+str(freq)+"\\"+"Networktopology_" + str(freq) + ".gif", os.path.join(self.web_dir, "Networktopology_" + str(freq) + ".gif"))

        # update website
        self.update_website()

    def update_website(self):
        logger.debug("update_website")
        os.system("python " + str(Path(__file__).parent.absolute())+"\\web_update_automation.py")

    def check_new_file(self, savedSet, mypath, file_start, file_suffix):
        # Retrieve a set of file paths
        nameSet = set()
        for file in os.listdir(mypath):
            fullpath = os.path.join(mypath, file)
            if os.path.isfile(fullpath) and os.stat(fullpath).st_size > 153600 and file.startswith(file_start) and file.endswith(file_suffix):
                nameSet.add(fullpath)

        # Compare set with saved set to find new files
        newSet = nameSet-savedSet
        if(len(newSet)):
            # Update saved set
            savedSet = nameSet

        return (len(newSet), savedSet)

    def select_update_file(self, mypath, file_start, file_suffix):
        # Start by initializing variables:
        savedSet = set()
        i = 0
        while True:
            newset_len, savedSet = self.check_new_file(
                savedSet, mypath, file_start, file_suffix)
            time.sleep(self._time)
            logger.debug("check")
            if(newset_len):
                latest_files = []
                for freq in self._f:
                    files = glob.glob(
                        mypath + "\\" + file_start + "sf_bitmap63ch" + str(freq) + "*" + file_suffix)
                    sorted_by_mtime_ascending = sorted(
                        files, key=lambda t: os.stat(t).st_mtime)
                    latest_files.extend(sorted_by_mtime_ascending[-self._num:])
                self.generate_topology(latest_files, mypath)
                # in case of "fail to allocate bitmap"
                i += 1
                if (i > 10):
                    sys.exit(0)

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_update_link', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-f', '--frequency', dest='frequency',
                            help='Input the transmit frequency')
        parser.add_argument('-link_dir', '--link_directory_path', dest='link_directory_path',
                            help='Input the directory path for processing')
        parser.add_argument('-web_dir', '--web_directory_path', dest='web_directory_path',
                            help='Input the directory path for website')
        parser.add_argument('-num', '--update_file_num', dest='update_file_num',
                            help='Update number of file per time')
        parser.add_argument('-time', '--time_interval', dest='time_interval',
                            help='Input the update time interval in second')
        group_actions = parser.add_argument_group(title='actions')
        group_actions.add_argument(
            'action', nargs='*', help='actions will be processed sequentially')
        args = parser.parse_args(argv)
        self._chirpbox_tool = chirpbox_tool.ChirpBoxTool()

        self._f = self._chirpbox_tool.convert_int_string_to_list(
            args.frequency)
        self.link_dir = args.link_directory_path
        self.web_dir = args.web_directory_path
        self._num = self._chirpbox_tool.convert_int_string_to_list(
            args.update_file_num)[0]
        self._time = self._chirpbox_tool.convert_int_string_to_list(
            args.time_interval)[0]
        logger.info(args)
        runtime_status = 0
        try:
            if (len(self.link_dir) == 0) or (len(self.web_dir) == 0):
                logger.error("No directory")
                runtime_status = 1
            try:
                file_start = "Chirpbox_connectivity_"
                file_suffix = ".txt"
                self.select_update_file(self.link_dir, file_start, file_suffix)
            except:
                logger.error("Action format or argument wrong %s", action)
                runtime_status = 1
        except:
            runtime_status = 1

        if runtime_status:
            logger.info("sys.exit")
            # sys.exit(runtime_status)
            restart = True
            if restart == True:
                os.execl(sys.executable, os.path.abspath(__file__), *sys.argv)
            else:
                print("\nThe programm will me closed...")
                sys.exit(0)

def main(argv):
    chirpboxupdatelink = ChirpBoxUpdateLink()
    chirpboxupdatelink.start(argv[1:])


if __name__ == "__main__":
    main(sys.argv)
