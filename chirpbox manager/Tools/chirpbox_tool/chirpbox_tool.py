import sys
import time
import argparse
import lib.chirpbox_tool_link_quality
import lib.chirpbox_tool_voltage
import logging
from lib.const import *


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

    def check_argument_format(self, cmd, params):
        if(cmd == 'link_quality'):
            logger.info("check sf and txpower, and node id")
            if self._sf is not None:
                if (max(self._sf) > CHIRPBOX_LINK_MAX_SF or min(self._sf) < CHIRPBOX_LINK_MIN_SF):
                    logger.error("SF wrong with %s", self._sf)
                    return False
            if self._tp is not None:
                if (max(self._tp) > CHIRPBOX_LINK_MAX_TP or min(self._tp) < CHIRPBOX_LINK_MIN_TP):
                    logger.error("TP wrong with %s", self._tp)
                    return False
            if self._f is not None:
                if (max(self._f) > CHIRPBOX_LINK_MAX_FREQ or min(self._f) < CHIRPBOX_LINK_MIN_FREQ):
                    logger.error("Freq wrong with %s", self._f)
                    return False
            if self._pl is not None:
                if (max(self._pl) > CHIRPBOX_LINK_MAX_PL or min(self._pl) < CHIRPBOX_LINK_MIN_PL):
                    logger.error("Payload length wrong with %s", self._pl)
                    return False

            if (params == 'measurement'):
                if (self._sf is None) or (self._tp is None) or (self._f is None) or (self._pl is None):
                    logger.error("Lack param in %s:%s", cmd, params)
                    return False
            elif (params == 'processing'):
                if (self._id is None) or (self._dir is None):
                    logger.error("Lack param in %s:%s", cmd, params)
                    return False
            else:
                logger.error("Action wrong with %s:%s", cmd, params)
                return False
        elif(cmd == 'voltage'):
            if (params == 'measurement'):
                return True
            elif (params == 'processing'):
                if (self._id is None) or (self._dir is None):
                    logger.error("Lack param in %s:%s", cmd, params)
                    return False
            else:
                logger.error("Action wrong with %:%", cmd, params)
                return False

    def cmd_link_quality(self, params):
        self._action = lib.chirpbox_tool_link_quality.link_quality()
        if params == 'measurement':
            self._action.measurement(self._sf, self._tp, self._f, self._pl)
        elif params == 'processing':
            self._action.processing(self._sf, self._tp, self._f, self._pl, self._id, self._dir, self._plot,self._pdate, self._data)

    def cmd_voltage(self, params):
        self._action = lib.chirpbox_tool_voltage.voltage()
        if params == 'measurement':
            self._action.measurement()
        elif params == 'processing':
            self._action.processing(self._id, self._dir)

    def cmd(self, param):
        cmd = param[0]
        params = param[1]
        if self.check_argument_format(cmd, params) == False:
            logger.error("Argument wrong for cmd: %s", cmd)
            return 1
        if cmd == 'link_quality' and params:
            self.cmd_link_quality(params)
        elif cmd == 'voltage' and params:
            self.cmd_voltage(params)
        else:
            logger.error("Action wrong with cmd: %s", cmd)
            return 1

    def start(self, argv):
        parser = argparse.ArgumentParser(
            prog='chirpbox_tool', formatter_class=argparse.RawTextHelpFormatter, description=DESCRIPTION_STR, epilog=ACTIONS_HELP_STR)
        parser.add_argument('-sf', '--spreading_factor', dest='spreading_factor', help='Input the spreading factor')
        parser.add_argument('-tp', '--tx_power', dest='tx_power', help='Input the transmit power')
        parser.add_argument('-f', '--frequency', dest='frequency', help='Input the transmit frequency')
        parser.add_argument('-pl', '--payload_len', dest='payload_len', help='Input the payload length')
        parser.add_argument('-id', '--node_id', dest='node_id', help='Input the node_id')
        parser.add_argument('-dir', '--directory_path', dest='directory_path', help='Input the directory path for processing')
        parser.add_argument('-plot', '--plot_type', dest='plot_type', help='Input the type of plots after processing')
        parser.add_argument('-pdate', '--plot_date', dest='plot_date', help='Input the start and end date of processed plot: 2021-04-22,2021-04-23')
        parser.add_argument('-data', '--data_exist', dest='data_exist', help='Input the state of processed data: True/False')
        group_actions = parser.add_argument_group(title='actions')
        group_actions.add_argument('action', nargs='*', help='actions will be processed sequentially')
        args = parser.parse_args(argv)
        self._sf = self.convert_int_string_to_list(args.spreading_factor)
        self._tp = self.convert_int_string_to_list(args.tx_power)
        self._f = self.convert_int_string_to_list(args.frequency)
        self._pl = self.convert_int_string_to_list(args.payload_len)
        self._id = self.convert_int_string_to_list(args.node_id)
        self._dir = args.directory_path
        if (args.plot_type is not None):
            self._plot = [x.strip() for x in args.plot_type.split(',')]
        else:
            self._plot = None
        if (args.plot_date is not None):
            self._pdate = [x.strip() for x in args.plot_date.split(',')]
        else:
            self._pdate = None
        self._data = args.data_exist
        logger.info(args)
        runtime_status = 0
        try:
            if len(args.action) == 0:
                logger.error("No action")
                runtime_status = 1
            for action in args.action:
                try:
                    runtime_status = self.cmd(action.split(':'))
                    if (runtime_status == 1):
                        break
                except:
                    logger.error("Action format or argument wrong %s", action)
                    runtime_status = 1
        except:
            runtime_status = 1

        if runtime_status:
            logger.info("sys.exit")
            sys.exit(runtime_status)

def main(argv):
    chirpboxtool = ChirpBoxTool()
    chirpboxtool.start(argv[1:])

if __name__ == "__main__":
    main(sys.argv)