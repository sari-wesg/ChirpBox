import sys
import time
import argparse
import logging
import os.path
from os import path
from pathlib import Path
import shutil
import csv
import json

# add path of chirpbox tools
sys.path.append(os.path.join(os.path.dirname(__file__), '..\\..\\..\\Tools\\'))
import param_patch.param_patch_FUT

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

VERSION_STR = "chirpbox_server v0.0.0"

DESCRIPTION_STR = VERSION_STR + """
(c)2021 by tianpei021@gmail.com
"""

ACTIONS_HELP_STR = """
list of paramters:

list of available actions:

list of combinations:

examples:
    chirpbox_server.py -h
"""

class ChirpBoxServer():

    def __init__(self):
        # TODO:
        self._server_address = os.path.join(os.path.dirname(__file__), '..\\upload_files\\')
        # create sub folder "testfiles" in the user upload directory
        Path(os.path.join(self._server_address, 'testfiles\\')).mkdir(parents=True, exist_ok=True)
        self._test_address = os.path.join(self._server_address, 'testfiles\\')

    def convert_int_string_to_list(self, my_str):
        if (my_str) is not None:
            temp = [(lambda sub: range(sub[0], sub[-1] + 1))(list(map(int, ele.split('-')))) for ele in my_str.split(',')]
            res = [b for a in temp for b in a]
            return res
        else:
            return None

    def check_new_file(self, directory, file_prefix="", file_suffix=""):
        savedList = []
        if path.exists(os.path.join(directory, 'file_list.csv')) is True:
            with open(os.path.join(directory, 'file_list.csv'), newline='') as f:
                reader = csv.reader(f)
                savedList = list(reader)
                if (len(savedList) > 0):
                    savedList = savedList[0]

        savedSet = set(savedList)

        # Retrieve a set of file paths
        nameSet = set()
        for file in os.listdir(directory):
            fullpath = os.path.join(directory, file)
            if os.path.isfile(fullpath) and os.stat(fullpath).st_size > 0 and file.startswith(file_prefix) and (file.endswith(file_suffix)):
                nameSet.add(fullpath)

        # Compare set with saved set to find new files
        newSet = nameSet-savedSet
        if(len(newSet)):
            # Update saved set
            savedSet = nameSet
        savedList = list(savedSet)

        # write the updated list to csv
        with open(os.path.join(directory, 'file_list.csv'), 'w+', newline='') as csvfile:
            writer= csv.writer(csvfile, delimiter=',')
            writer.writerow(savedList)
        csvfile.close()

        return list(newSet)

    def patch_file(self, bin_file, config_file):
        logger.info("patching...")
        param_patch.param_patch_FUT.fut_param_patch(bin_file, config_file)
        return True

    def update_config(self, config):
        with open(config, "r") as jsonFile:
            data = json.load(jsonFile)

        # TODO: should add triscale (https://romain-jacob.github.io/triscale/)
        data["experiment_run_time"] = 3

        with open(config, "w") as jsonFile:
            json.dump(data, jsonFile)

    def add_file_to_test(self, bin_file, config_file):
        logger.info("adding to test...")
        try:
            # move config
            shutil.copyfile(config_file, os.path.join(self._test_address, os.path.basename(config_file)))
            # move bin
            shutil.copyfile(bin_file, os.path.join(self._test_address, os.path.basename(bin_file)))
            # update config
            self.update_config(os.path.join(self._test_address, os.path.basename(config_file)))
        except:
            pass

    def detect_new_file(self):
        # detect new file in directory
        logger.info("detecting...")
        newList = self.check_new_file(self._server_address, file_suffix = ".bin")
        for i in range(len(newList)):
            bin_file = os.path.join(self._server_address, os.path.basename(newList[i]))
            config_file = os.path.join(self._server_address, os.path.basename(newList[i][:-len(".bin")]) + ".json")
            if path.exists(config_file) is True and self.patch_file(bin_file, config_file) is True:
                self.add_file_to_test(bin_file, config_file)
        return True

    def manage_FUT(self):
        while True:
            self.detect_new_file()
            time.sleep(int(self._update))

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_server', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        self._update = 1
        runtime_status = 0
        try:
            self.manage_FUT()
        except:
            runtime_status = 1

        if runtime_status:
            logger.info("sys.exit")
            sys.exit(runtime_status)

def main(argv):
    chirpboxserver = ChirpBoxServer()
    chirpboxserver.start(argv[1:])

if __name__ == "__main__":
    main(sys.argv)