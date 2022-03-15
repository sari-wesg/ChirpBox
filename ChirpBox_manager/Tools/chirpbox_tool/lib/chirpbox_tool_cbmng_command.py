import logging
from lib.const import *
import json
import os.path

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__),'..\\..\\..\\'))
import cbmng

logger = logging.getLogger(__name__)

class cbmng_command():

    def run_command_with_json(self, command_type, command_param = ''):
        with open(os.path.dirname(__file__) + '\\..\\' + CHIRPBOX_CONFIG_FILE) as data_file:
            data = json.load(data_file)

            if (command_type == CHIRPBOX_LINK_COMMAND):
                sf_bitmap = str(command_param[0]) + " "
                freq = str(command_param[1]) + " "
                tp = str(command_param[2]) + " "
                pl = str(command_param[3]) + " "
                chirpbox_command = "cbmng.py " + CHIRPBOX_LINK_COMMAND + sf_bitmap + freq + tp + str(data['all_command_sf']) + " " + data['all_command_comport'] + " " + str(data['flooding_command_bitmap']) + " " + pl + str(data['all_command_tp'])
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

                chirpbox_command = "cbmng.py " + CHIRPBOX_COLLECT_COMMAND + str(data['coldata_payload_len']) + " " + str(data['coldata_command_sf']) + " " + data['all_command_comport'] + " " + str(data['collect_command_slot_number']) + " " + str(data['all_command_tp']) + " " + data['all_command_bitmap'] + " " + CHIRPBOX_TOPODATA_FLASH_START + CHIRPBOX_TOPODATA_FLASH_END
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

            elif (command_type == CHIRPBOX_DISSEM_COMMAND):
                upgrade_daemon_or_FUT = str(command_param[0])
                chirpbox_command = "cbmng.py " + CHIRPBOX_DISSEM_COMMAND + str(data['daemon_version']) + " " + str(data['coldata_payload_len']) + " " + str(data['dissem_packet_number_per_round']) + " " + str(data['all_command_sf']) + " " + data['all_command_comport'] + " " + str(data['all_command_bitmap']) + " " + str(data['dissem_command_slot_number']) + " " + str(data['all_command_sf']) + " " + str(data['collect_command_slot_number']) + " " + str(data['all_command_tp']) + " " + str(data['all_command_bitmap']) + " " + upgrade_daemon_or_FUT
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

            elif (command_type == CHIRPBOX_START_COMMAND):
                upgrade_bitmap = str(command_param[0]) + " "
                flash_protection = str(command_param[1]) + " "
                chirpbox_command = "cbmng.py " + CHIRPBOX_START_COMMAND + flash_protection + str(data['daemon_version']) + " " + str(data['all_command_sf']) + " " + data['all_command_comport'] + " " + upgrade_bitmap + str(data['flooding_command_slot_number']) + " " + str(data['all_command_tp'])
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

            elif (command_type == CHIRPBOX_COLLECT_COMMAND):
                flash_start = str(command_param[0]) + " "
                flash_end = str(command_param[1]) + " "
                chirpbox_command = "cbmng.py " + CHIRPBOX_COLLECT_COMMAND + str(data['coldata_payload_len']) + " " + str(data['coldata_command_sf']) + " " + data['all_command_comport'] + " " + str(data['collect_command_slot_number']) + " " + str(data['all_command_tp']) + " " + data['all_command_bitmap'] + " " + flash_start + flash_end
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

            elif (command_type == CHIRPBOX_VERSION_COMMAND):
                chirpbox_command = "cbmng.py " + CHIRPBOX_VERSION_COMMAND + str(data['coldata_command_sf']) + " " + data['all_command_comport'] + " " + str(data['collect_command_slot_number']) + " " + str(data['all_command_tp'])
                logger.info(chirpbox_command.split())
                cbmng.main(chirpbox_command.split())

            else:
                logger.error("Command wrong: %s", command_type)
