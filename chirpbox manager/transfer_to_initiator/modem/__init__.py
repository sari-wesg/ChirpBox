__author__ = 'Wijnand Modderman <maze@pyth0n.org>'
__copyright__ = [
                    'Copyright (c) 2010 Wijnand Modderman-Lenstra',
                    'Copyright (c) 1981 Chuck Forsberg'
                ]
__license__ = 'MIT'
__version__ = '0.2.4'

import gettext
from modem.protocol.xmodem import XMODEM
from modem.protocol.ymodem import YMODEM

gettext.install('modem')

# To satisfy import *
__all__ = [
    'XMODEM',
    'YMODEM',
]
