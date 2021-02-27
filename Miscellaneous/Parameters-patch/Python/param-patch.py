import json

""" constant for the parameter struct in ChirpBox setting """


class CHIRPBOX_CONST(object):
    __slots__ = ()
    SECTION_DATA_ALIGNMENT_LENGTH = 4  # pragma data_alignment = 4 (uint32_t)
    SECTION_ADDR = 0x200

    UID_LIST_ADDR = SECTION_ADDR
    UID_LIST_LENGTH = 0xFF

    UID_VERSION_ADDR = UID_LIST_ADDR + UID_LIST_LENGTH * SECTION_DATA_ALIGNMENT_LENGTH
    UID_VERSION_LENGTH = 1

    FREQUENCY_ADDR = UID_VERSION_ADDR + \
        UID_VERSION_LENGTH * SECTION_DATA_ALIGNMENT_LENGTH
    FREQUENCY_LENGTH = 1


""" read parameters from the json file and write it to the daemon firmware """


def param_patch(param_filename, bin_filename):
    CONST = CHIRPBOX_CONST()
    # read settings from json
    with open(param_filename) as data_file:
        data = json.load(data_file)

    UID_list = []
    for i in data['UID_list'].split(','):
        UID_list.append(int(i, 16))

    UID_version = int(data['UID_version'], 16)
    Frequency = int(data['Frequency'], 10)

    # write settings to the json
    with open(bin_filename, 'r+b') as fh:
        fh.seek(CONST.UID_LIST_ADDR)
        for i in UID_list:
            fh.write((bytearray.fromhex("{0:08X}".format(i)))[::-1])
        fh.seek(CONST.UID_VERSION_ADDR)
        fh.write((bytearray.fromhex("{0:08X}".format(UID_version)))[::-1])
        fh.seek(CONST.FREQUENCY_ADDR)
        fh.write((bytearray.fromhex("{0:08X}".format(Frequency)))[::-1])


"""

usage:
param_patch('testbed-param-settings.json', 'Daemon.bin')

"""
