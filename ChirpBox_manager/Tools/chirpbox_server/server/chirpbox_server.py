import sys
import time
import argparse
import logging

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
        self._start_time = time.time()

    def convert_int_string_to_list(self, my_str):
        if (my_str) is not None:
            temp = [(lambda sub: range(sub[0], sub[-1] + 1))(list(map(int, ele.split('-')))) for ele in my_str.split(',')]
            res = [b for a in temp for b in a]
            return res
        else:
            return None

    def add_file_to_test(self):
        logger.debug("add_file_to_test")
        # rename file with the add time

    def detect_new_file(self):
        logger.debug("detect_new_file")
        # detect new file in directory
        return True

    def manage_FUT(self):
        logger.debug("manage_FUT")
        while True:
            if self.detect_new_file() is True:
                self.add_file_to_test()
            time.sleep(1)

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_server', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-dir', '--directory', dest='directory', help='Input the directory of bin file and config file')
        args = parser.parse_args(argv)
        self._dir = args.directory

        logger.info(args)
        runtime_status = 0
        try:
            if len(self._dir) == 0:
                logger.error("No param")
                runtime_status = 1
            else:
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