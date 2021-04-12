import sys
import time
import argparse
import lib.chirpbox_tool_link_quality
import lib.chirpbox_tool_voltage
import logging

logger = logging.getLogger(__name__)
logger.propagate = False
logger.setLevel(logging.INFO) # <<< Added Line
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

VERSION_STR = "chirpbox_tool v0.0.0"

DESCRIPTION_STR = VERSION_STR + """
(c)2021 by tianpei021@gmail.com
"""

ACTIONS_HELP_STR = """
list of paramters:
    -spreading_factor
    -tx_power
    -node_id

list of available actions:
    link_quality:measurement
    link_quality:processing
    voltage:measurement
    voltage:processing

examples:
    chirpbox_tool.py -h
    chirpbox_tool.py -sf 7-12 -tp 0-14 -id 0-20 link_quality:measurement
    chirpbox_tool.py -sf 7,8,9 -tp 0,1,2 -id 0,10,20 -dir "../../link_quality_folder" link_quality:processing
    chirpbox_tool.py -id 0,10,20 voltage:measurement
    chirpbox_tool.py -id 0,10,20 -dir "voltage_measurement_folder" voltage:processing
"""

class ChirpBoxTool():

    def __init__(self):
        self._start_time = time.time()

    def convert_int_string_to_list(self, my_str):
        if (my_str) is not None:
            temp = [(lambda sub: range(sub[0], sub[-1] + 1))(list(map(int, ele.split('-')))) for ele in my_str.split(',')]
            res = [b for a in temp for b in a]
            return res
        else:
            return None

    # TODO:
    def check_argument_format(self, cmd):
        if(cmd == 'link_quality'):
            logger.info("check sf and txpower, and node id")
        elif(cmd == 'voltage'):
            if self._id is None:
                logger.info("Error: please input node_id")
                return False

    def cmd_link_quality(self, params):
        self._action = lib.chirpbox_tool_link_quality.link_quality()
        if params[0] == 'measurement':
            self._action.measurement()
        elif params[0] == 'processing':
            self._action.processing()

    def cmd_voltage(self, params):
        self._action = lib.chirpbox_tool_voltage.voltage()
        if params[0] == 'measurement':
            self._action.measurement()
        elif params[0] == 'processing':
            self._action.processing(self._id, self._dir)

    def cmd(self, param):
        cmd = param[0]
        params = param[1:]
        if self.check_argument_format(cmd) == False:
            logger.error("argument wrong for cmd: %s", cmd)
            return 1
        if cmd == 'link_quality' and params:
            self.cmd_link_quality(params)
        elif cmd == 'voltage' and params:
            self.cmd_voltage(params)
        else:
            logger.error("action wrong with cmd: %s", cmd)
            return 1

    def start(self):
        parser = argparse.ArgumentParser(
            prog='chirpbox_tool', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-sf', '--spreading_factor', dest='spreading_factor', help='Input the spreading factor')
        parser.add_argument('-tp', '--tx_power', dest='tx_power', help='Input the transmit power')
        parser.add_argument('-id', '--node_id', dest='node_id', help='Input the node_id')
        parser.add_argument('-dir', '--directory_path', dest='directory_path', help='Input the directory path for processing')
        group_actions = parser.add_argument_group(title='actions')
        group_actions.add_argument('action', nargs='*', help='actions will be processed sequentially')
        args = parser.parse_args()
        self._sf = self.convert_int_string_to_list(args.spreading_factor)
        self._tp = self.convert_int_string_to_list(args.tx_power)
        self._id = self.convert_int_string_to_list(args.node_id)
        self._dir = args.directory_path
        logger.info(args)
        logger.debug("self._sf: %s, self._tp: %s, self._id: %s, self._dir: %s", self._sf, self._tp, self._id, self._dir)
        runtime_status = 0
        try:
            if len(args.action) == 0:
                logger.error("no action")
                runtime_status = 1
            for action in args.action:
                try:
                    runtime_status = self.cmd(action.split(':'))
                    if (runtime_status == 1):
                        break
                except:
                    logger.error("action format or argument wrong %s", action)
                    runtime_status = 1
        except:
            runtime_status = 1

        if runtime_status:
            logger.info("sys.exit")
            sys.exit(runtime_status)


if __name__ == "__main__":
    chirpboxtool = ChirpBoxTool()
    chirpboxtool.start()
