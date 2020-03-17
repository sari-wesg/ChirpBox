import glob
import os
from modem.const import *
from modem.tools import log
from modem.protocol.xmodem import XMODEM
from modem import error

from modem.crc import crc32_byte

class YMODEM(XMODEM):
    '''
    YMODEM protocol implementation, expects an object to read from and an
    object to write to.
    '''

    protocol = PROTOCOL_YMODEM

    def send(self, pattern, retry=10, timeout=60):
        '''
        Send one or more files via the YMODEM protocol.

            >>> print modem.send('*.txt')
            True

        Returns ``True`` upon succesful transmission or ``False`` in case of
        failure.
        '''

        # Get a list of files to send
        filenames = glob.glob(pattern)
        # filenames = 'mixer.bin'
        if not filenames:
            return True

        # initialize protocol
        error_count = 0
        crc_mode = 0

        start_char = self._wait_recv(error_count, timeout, retry)

        if start_char:
            crc_mode = 1 if (start_char == CRC) else 0
        else:
            log.error(error.ABORT_PROTOCOL)
            # Already aborted
            return False

        for filename in filenames:
            # Send meta data packet
            sequence = 0
            error_count = 0
            # Add filesize
            filesize = os.path.getsize(filename)
            data = ''.join([os.path.basename(filename), '\x00',
                    str(filesize), '\x00'])

            log.debug(error.DEBUG_START_FILENAME % (filename,))
            # Pick a suitable packet length for the filename
            packet_size = 128 if (len(data) < 128) else 1024

            # Packet padding
            data = data.ljust(packet_size, '\0')

            # Calculate checksum
            crc = crc32_byte(list(bytearray(data.encode())))

            # Emit packet
            if not self._send_packet(sequence, data, packet_size, crc_mode,
                crc, error_count, retry, timeout):
                self.abort(timeout=timeout)
                return False

            # Wait for <CRC> before transmitting the file contents
            error_count = 0
            if not self._wait_recv(error_count, timeout, retry):
                self.abort(timeout)
                return False

            filedesc = open(filename, 'rb')

            # AT THIS POINT
            # - PACKET 0 WITH METADATA TRANSMITTED
            # - INITIAL <CRC> OR <NAK> ALREADY RECEIVED

            if not self._send_stream(filedesc, crc_mode, retry, timeout):
                log.error(error.ABORT_SEND_STREAM)
                return False

            # AT THIS POINT
            # - FILE CONTENTS TRANSMITTED
            # - <EOT> TRANSMITTED
            # - <ACK> RECEIVED

            filedesc.close()
            # WAIT A <CRC> BEFORE NEXT FILE
            error_count = 0
            if not self._wait_recv(error_count, timeout, retry):
                log.error(error.ABORT_INIT_NEXT)
                # Already aborted
                return False

        # End of batch transmission, send NULL file name
        sequence = 0
        error_count = 0
        packet_size = 128
        data = '\x00' * packet_size
        crc = crc32_byte(list(bytearray(data.encode())))

        # Emit packet
        if not self._send_packet(sequence, data, packet_size, crc_mode, crc,
            error_count, retry, timeout):
            log.error(error.ABORT_SEND_PACKET)
            # Already aborted
            return False

        # All went fine
        return True
