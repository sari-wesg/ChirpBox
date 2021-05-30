import logging
from lib.const import *
import json
import os.path

import sys
import os
sys.path.append(os.path.join(sys.path[0],'..\\..\\'))
sys.path.append(os.path.join(sys.path[0],'..\\..\\transfer_to_initiator'))
import cbmng
import time

logger = logging.getLogger(__name__)

class cbmng_command():

    def run_command_with_json(self, command_type, command_param):
        with open(os.path.dirname(__file__) + '\\..\\' + CHIRPBOX_CONFIG_FILE) as data_file:
            data = json.load(data_file)

            if (command_type == CHIRPBOX_LINK_COMMAND):
                sf_bitmap = str(command_param[0]) + " "
                freq = str(command_param[1]) + " "
                tp = str(command_param[2]) + " "
                pl = str(command_param[3]) + " "
                chirpbox_command = "cbmng.py " + CHIRPBOX_LINK_COMMAND + sf_bitmap + freq + tp + str(data['all_command_sf']) + " " + data['all_command_comport'] + " " + str(data['all_command_slot_number']) + " " + pl + str(data['all_command_tp'])
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

                chirpbox_command = "cbmng.py " + CHIRPBOX_COLLECT_COMMAND + str(data['coldata_payload_len']) + " " + str(data['coldata_command_sf']) + " " + data['all_command_comport'] + " " + str(data['all_command_slot_number']) + " " + str(data['all_command_tp']) + " " + data['all_command_bitmap'] + " " + CHIRPBOX_TOPODATA_FLASH_START + CHIRPBOX_TOPODATA_FLASH_END
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

            elif (command_type == CHIRPBOX_DISSEM_COMMAND):
                logger.error("jsjk")

            elif (command_type == CHIRPBOX_START_COMMAND):
                logger.error("jsjk")

            else:
                logger.error("Command wrong: %s", command_type)
