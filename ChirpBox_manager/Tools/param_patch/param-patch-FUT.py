import json

""" constant for the parameter struct in ChirpBox setting """

class FUT_CONST(object):
    __slots__ = ()
    SECTION_DATA_ALIGNMENT_LENGTH = 4  # pragma data_alignment = 4 (uint32_t)
    SECTION_ADDR = 0x610

    CUSTOM_ADDR = SECTION_ADDR
    CUSTOM_LENGTH = 0x255

""" read parameters from the json file and write it to the daemon firmware """

def fut_param_patch(param_filename, bin_filename):
    CONST = FUT_CONST()
    # read settings from json
    with open(param_filename) as data_file:
        data = json.load(data_file)

    FUT_CUSTOM_list = []
    for i in data['FUT_CUSTOM_list'].split(','):
        FUT_CUSTOM_list.append(int(i, 16))

    # write settings to the json
    with open(bin_filename, 'r+b') as fh:
        fh.seek(CONST.CUSTOM_ADDR)
        for i in FUT_CUSTOM_list:
            fh.write((bytearray.fromhex("{0:08X}".format(i)))[::-1])

"""
usage example:
fut_param_patch('example_config_method.json', 'FUT.bin')

"""

fut_param_patch('example_config_method.json', 'FUT.bin')
