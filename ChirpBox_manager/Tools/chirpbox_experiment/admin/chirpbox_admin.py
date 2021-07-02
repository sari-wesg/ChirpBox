import sys
import time
import argparse
import logging
import os.path
from os import path
from pathlib import Path
import shutil
import glob
import json

# add path of chirpbox manager
sys.path.append(os.path.join(os.path.dirname(__file__), '..\\..\\..\\..\\ChirpBox_manager'))
sys.path.append(os.path.join(os.path.dirname(__file__),'..\\..\\..\\..\\ChirpBox_manager\\transfer_to_initiator'))
sys.path.append(os.path.join(os.path.dirname(__file__),'..\\..\\..\\..\\ChirpBox_manager\\Tools\\chirpbox_tool'))

import cbmng
import cbmng_exp_start
import cbmng_exp_method
import cbmng_exp_config
import lib.chirpbox_tool_cbmng_command
from lib.const import *
import Tools.toggle_check
import chirpbox_tool

logger = logging.getLogger(__name__)
logger.propagate = False
logger.setLevel(logging.DEBUG) # <<< Added Line
logging.getLogger('matplotlib.font_manager').disabled = True

# Log config:
# Create handlers
c_handler = logging.StreamHandler()
c_handler.setLevel(logging.DEBUG)

# Create formatters and add it to handlers
c_format = logging.Formatter('[%(filename)s:%(lineno)d] %(asctime)s - %(name)s - %(levelname)s - %(message)s')
c_handler.setFormatter(c_format)

# Add handlers to the logger
logger.addHandler(c_handler)

# Examples:
# logger.warning()
# logger.error()
# logger.info()
# logger.debug()

VERSION_STR = "chirpbox_admin v0.0.0"

DESCRIPTION_STR = VERSION_STR + """
(c)2021 by tianpei021@gmail.com
"""

ACTIONS_HELP_STR = """
list of paramters:

list of available actions:

list of combinations:

examples:
    chirpbox_admin.py -h
    chirpbox_admin.py -connect -version
"""

class ChirpBoxAdmin():

    def __init__(self):
        self._update_interval = 1 # update time 1 second
        # TODO:
        self._server_address = os.path.join(os.path.dirname(__file__), '..\\upload_files\\')
        self._test_address = os.path.join(self._server_address, 'testfiles\\')
        self._tested_address = os.path.join(self._test_address, 'tested\\')
        # create tested folder
        Path(self._tested_address).mkdir(parents=True, exist_ok=True)

    def bitmap_list_to_roundrobin(self, bitmap):
        bitmap = int(bitmap, 16)
        node_list = []
        i = 0
        while (bitmap):
            i += 1
            bitmap = bitmap >> 1
            if(bitmap & 0b1):
                node_list.append(i)

        bitmap_list = []
        for i in node_list:
            bitmap_list.append(hex(1 << i)[len("0x"):])

        return bitmap_list

    def is_toggle(self, bin_file):
        logger.debug("check_toggle")
        with open(os.path.join(os.path.dirname(__file__), '..\\..\\..\\..\\ChirpBox_manager\\Tools\\chirpbox_tool', CHIRPBOX_CONFIG_FILE)) as data_file:
            data = json.load(data_file)
            serial_sn = data['chirpbox_server_comport_ST_LINK_SN']
            com_port = data['chirpbox_server_comport']
        data_file.close()
        is_toggle_True = Tools.toggle_check.check_toggle(bin_file, serial_sn, com_port, 60)
        return is_toggle_True

    def setup_experiment(self, bin_file, config_file):
        # -ec
        experiment_config = "cbmng.py " + "-ec " + config_file
        cbmng.main(experiment_config.split())
        # -ef
        experiment_firmware = "cbmng.py " + "-ef " + bin_file
        cbmng.main(experiment_firmware.split())
        # -em
        experiment_method = "cbmng.py " + "-em " + config_file
        cbmng.main(experiment_method.split())

    def start_experiment(self, bin_file, config_file):
        lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_DISSEM_COMMAND, ["0"])
        for i in range(cbmng_exp_method.myExpMethodApproach().experiment_run_time):
            # start
            if (cbmng_exp_method.myExpMethodApproach().experiment_run_round.lower() == 'true'):
                bitmap_list = self.bitmap_list_to_roundrobin(cbmng_exp_method.myExpMethodApproach().experiment_run_bitmap)
                for bitmap in bitmap_list:
                    lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_START_COMMAND, [bitmap, "1"])
                    time.sleep(int(cbmng_exp_config.myExpConfApproach().experiment_duration) + 60) #waiting for the end of experiment
                time.sleep(180) #waiting for all nodes have GPS signal
            else:
                lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_START_COMMAND, [cbmng_exp_method.myExpMethodApproach().experiment_run_bitmap, "1"])
                time.sleep(180) #waiting for all nodes have GPS signal
            # collect
            lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_COLLECT_COMMAND, [cbmng_exp_method.myExpMethodApproach().start_address, cbmng_exp_method.myExpMethodApproach().end_address])

            # if self._connect is True:
            #     # connectivity
            #     chirpbox_tool_command = "chirpbox_tool.py " + "-sf 7-12 -tp 0 -f 470000,480000,490000 -pl 8 link_quality:measurement"
            #     chirpbox_tool.main(chirpbox_tool_command.split())
        # move files to tested file
        shutil.move(bin_file, os.path.join(self._tested_address, os.path.basename(bin_file)))
        shutil.move(config_file, os.path.join(self._tested_address, os.path.basename(config_file)))
        return True

    def manage_chirpbox(self):
        while True:
            logger.debug("manage_chirpbox")
            if self._connect is True:
                # connectivity
                chirpbox_tool_command = "chirpbox_tool.py " + "-sf 7-12 -tp 0 -f 470000,480000,490000 -pl 8 link_quality:measurement"
                chirpbox_tool.main(chirpbox_tool_command.split())
            if self._version is True:
                # collect version
                lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_VERSION_COMMAND)
            if(cbmng_exp_start.is_running() == False):
                # find the oldest bin file and config file from all files
                all_bin_files = glob.glob(self._test_address + "\\*.bin")
                if(len(all_bin_files) > 0):
                    the_oldest_bin_file = sorted(all_bin_files, key=lambda t: os.stat(t).st_mtime)[0]
                    the_oldest_config = the_oldest_bin_file[:-len(".bin")] + ".json"
                    if path.exists(the_oldest_config) is True:
                        # if toggle check is ok, start experiment
                        # if self.is_toggle(the_oldest_bin_file) is True:
                        self.setup_experiment(the_oldest_bin_file, the_oldest_config)
                        self.start_experiment(the_oldest_bin_file, the_oldest_config)
                        # if toggle check is not ok, remove bin and config file
                        # else:
                        #     logger.debug("remove")
                        #     os.remove(the_oldest_bin_file)
                        #     os.remove(the_oldest_config)
            time.sleep(1)


            time.sleep(self._update_interval)
        return True

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_admin', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-connect', '--connectivity', dest='connectivity', help='Is the connectivity is needed?')
        parser.add_argument('-version', '--version_collection', dest='version_collection', help='Is the version collection is needed?')
        args = parser.parse_args(argv)
        if args.connectivity is not None:
            self._connect = bool(args.connectivity.lower() == 'true')
        else:
            self._connect = 'false'
        if args.version_collection is not None:
            self._version = bool(args.version_collection.lower() == 'true')
        else:
            self._version = 'false'
        runtime_status = 0
        try:
            self.manage_chirpbox()
        except:
            runtime_status = 1

        if runtime_status:
            logger.info("sys.exit")
            sys.exit(runtime_status)

def main(argv):
    chirpboxadmin = ChirpBoxAdmin()
    chirpboxadmin.start(argv[1:])

if __name__ == "__main__":
    main(sys.argv)