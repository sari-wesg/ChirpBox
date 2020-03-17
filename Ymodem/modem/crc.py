# crc-ccitt (xmodem) algorithm
POLYNOMIAL = 0x1021
PRESET = 0

def _initial(c):
    crc = 0
    c = c << 8
    for j in range(8):
        if (crc ^ c) & 0x8000:
            crc = (crc << 1) ^ POLYNOMIAL
        else:
            crc = crc << 1
        c = c << 1
    return crc

_tab = [ _initial(i) for i in range(256) ]

def _update_crc(crc, c):
    cc = 0xff & c
    tmp = (crc >> 8) ^ cc
    crc = (crc << 8) ^ _tab[tmp & 0xff]
    crc = crc & 0xffff
    return crc

def crc32(str):
    crc = PRESET
    for c in str:
        crc = _update_crc(crc, ord(c))
    return crc

def crc32_byte(p):
    crc = PRESET
    for i in range(len(p)):
        crc = _update_crc(crc, p[i])
    return crc
