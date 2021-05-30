import sys
import time
import argparse
import logging
import os.path
from os import path
from pathlib import Path
import shutil
import csv
import glob

# add path of chirpbox manager
sys.path.append(os.path.join(sys.path[0], '..\\..\\..\\..\\ChirpBox_manager'))
sys.path.append(os.path.join(sys.path[0],'..\\..\\..\\..\\ChirpBox_manager\\transfer_to_initiator'))
sys.path.append(os.path.join(sys.path[0],'..\\..\\..\\..\\ChirpBox_manager\\Tools\\chirpbox_tool'))
import cbmng
import cbmng_exp_start
import lib.chirpbox_tool_cbmng_command
from lib.const import *

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
"""

class ChirpBoxAdmin():

    def __init__(self):
        self._update_interval = 1 # update time 1 second
        # TODO:
        self._server_address = os.path.join(sys.path[0], '..\\upload_files\\')
        self._test_address = os.path.join(self._server_address, 'testfiles\\')
        self._tested_address = os.path.join(self._test_address, 'tested\\')
        # create tested folder
        Path(self._tested_address).mkdir(parents=True, exist_ok=True)

    # TODO:
    def setup_chirpbox(self):
        logger.debug("setup_chirpbox")
        return True

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
        # move files to tested file
        shutil.move(bin_file, os.path.join(self._tested_address, os.path.basename(bin_file)))
        shutil.move(config_file, os.path.join(self._tested_address, os.path.basename(config_file)))

    def start_experiment(self):
        # dissem
        lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_DISSEM_COMMAND, "jj")
        logger.debug(expmethapp.experiment_run_time)
        # start
        lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_START_COMMAND, "jj")
        # collect
        lib.chirpbox_tool_cbmng_command.cbmng_command.run_command_with_json(self, CHIRPBOX_COLLECT_COMMAND, "jj")
        return True

    def manage_chirpbox(self):
        logger.debug("manage_chirpbox")
        while True:
            if(cbmng_exp_start.is_running() == False):
                # find the oldest bin file and config file from all files
                all_bin_files = glob.glob(self._test_address + "\\*.bin")
                if(len(all_bin_files) > 0):
                    the_oldest_bin_file = sorted(all_bin_files, key=lambda t: os.stat(t).st_mtime)[0]
                    the_oldest_config = the_oldest_bin_file[:-len(".bin")] + ".json"
                    if path.exists(the_oldest_config) is True:
                        self.setup_experiment(the_oldest_bin_file, the_oldest_config)
                        self.start_experiment()


            time.sleep(self._update_interval)
        return True

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_admin', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-setup', '--setup_admin', dest='setup_admin', help='Input if is the first time using admin')
        args = parser.parse_args(argv)
        self._setup = args.setup_admin

        logger.info(args)
        runtime_status = 0
        try:
            if self._setup is not None and (int(self._setup) == 1):
                self.setup_chirpbox()
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