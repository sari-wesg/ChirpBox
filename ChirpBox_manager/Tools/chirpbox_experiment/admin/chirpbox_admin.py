import sys
import time
import argparse
import logging
import os.path
from os import path
from pathlib import Path
import shutil
import csv

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
        self._dirname = os.path.dirname(__file__)
        # TODO:
        self._server_address = os.path.join(self._dirname, '../upload_files/')
        # create sub folder "testfiles" in the user upload directory
        Path(os.path.join(self._server_address, 'testfiles/')).mkdir(parents=True, exist_ok=True)
        self._test_address = os.path.join(self._server_address, 'testfiles/')

    # TODO:
    def setup_chirpbox(self):
        logger.debug("setup_chirpbox")
        return True

    def manage_chirpbox(self):
        logger.debug("manage_chirpbox")
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