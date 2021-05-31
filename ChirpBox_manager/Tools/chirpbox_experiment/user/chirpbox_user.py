import sys
import argparse
import logging
import os.path
from os import path
import shutil
import datetime

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

VERSION_STR = "chirpbox_user v0.0.0"

DESCRIPTION_STR = VERSION_STR + """
(c)2021 by tianpei021@gmail.com
"""

ACTIONS_HELP_STR = """
list of paramters:

list of available actions:

list of combinations:

examples:
    chirpbox_user.py -h
    chirpbox_user.py -bin -config -user
"""

class ChirpBoxUser():

    def __init__(self):
        # TODO:
        self._server_address = os.path.join(os.path.dirname(__file__), '..\\upload_files\\')

    def convert_int_string_to_list(self, my_str):
        if (my_str) is not None:
            temp = [(lambda sub: range(sub[0], sub[-1] + 1))(list(map(int, ele.split('-')))) for ele in my_str.split(',')]
            res = [b for a in temp for b in a]
            return res
        else:
            return None

    def check_param(self):

        # check bin file:
        if path.exists(self._bin) is False:
            logger.error("Check bin file!")
            return False
        # check config files:
        if path.exists(self._config) is False:
            logger.error("Check config file!")
            return False
        # check user name:
        if self._user_name is None:
            logger.error("Check user name!")
            return False
        return True

    def upload_FUT(self):
        # rename your bin file and config and upload to the server address
        time_upload = datetime.datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
        upload_bin = self._server_address + self._user_name + "_" + time_upload + ".bin"
        upload_config = self._server_address + self._user_name + "_" + time_upload + ".json"
        shutil.copyfile(self._bin, upload_bin)
        shutil.copyfile(self._config, upload_config)

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_user', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-bin', '--bin_file_FUT', dest='bin_file_FUT', help='Input the bin file name of FUT')
        parser.add_argument('-config', '--config_file_FUT', dest='config_file_FUT', help='Input the config file name of FUT')
        parser.add_argument('-user', '--user_name', dest='user_name', help='Input the user name')
        args = parser.parse_args(argv)
        self._bin = args.bin_file_FUT
        self._config = args.config_file_FUT
        self._user_name = args.user_name

        logger.info(args)
        runtime_status = 0
        try:
            if self.check_param() is False:
                logger.error("Check parameters!")
                runtime_status = 1
            else:
                self.upload_FUT()
        except:
            runtime_status = 1

        if runtime_status:
            logger.info("sys.exit")
            sys.exit(runtime_status)

def main(argv):
    chirpboxuser = ChirpBoxUser()
    chirpboxuser.start(argv[1:])

if __name__ == "__main__":
    main(sys.argv)