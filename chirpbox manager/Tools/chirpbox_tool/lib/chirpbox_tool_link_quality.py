import logging
from lib.const import *
import lib.chirpbox_tool_cbmng_command

logger = logging.getLogger(__name__)

class link_quality():
    def __init__(self):
        logger.debug("link_quality")

    def measurement(self, sf_list, tp_list, freq_list, payload_len_list):
        sf_list = [i - CHIRPBOX_LINK_MIN_SF for i in sf_list]
        sf_bitmap = 0
        for i in sf_list:
            sf_bitmap += 1 << i

        self._cbmng_command = lib.chirpbox_tool_cbmng_command.cbmng_command()

        for pl in payload_len_list:
            for freq in freq_list:
                for tp in tp_list:
                    self._cbmng_command.run_command_with_json(CHIRPBOX_LINK_COMMAND, [sf_bitmap, freq, tp, pl])

    def processing(self):
        logger.debug("processing")
