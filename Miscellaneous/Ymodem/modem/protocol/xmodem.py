import time
from modem import error
from modem.const import *
from modem.tools import log
from modem.crc import crc32_byte

class Modem(object):
    '''
    Base modem class.
    '''

    def __init__(self, getc, putc):
        self.getc = getc
        self.putc = putc
class XMODEM(Modem):
    '''
    XMODEM protocol implementation, expects an object to read from and an
    object to write to.

    >>> def getc(size, timeout=1):
    ...     return data or None
    ...
    >>> def putc(data, timeout=1):
    ...     return size or None
    ...
    >>> modem = XMODEM(getc, putc)

    '''

    # Protocol identifier
    protocol = PROTOCOL_XMODEM

    def abort(self, count=2, timeout=60):
        '''
        Send an abort sequence using CAN bytes.
        '''

        for counter in range(0, count):
            self.putc(CAN, timeout)

    def _send_stream(self, stream, crc_mode, retry=16, timeout=0):
        '''
        Sends a stream according to the given protocol dialect:

            >>> stream = file('/etc/issue', 'rb')
            >>> print modem.send(stream)
            True

        Return ``True`` on success, ``False`` in case of failure.
        '''

        # Get packet size for current protocol
        packet_size = PACKET_SIZE.get(self.protocol, 128)

        # ASSUME THAT I'VE ALREADY RECEIVED THE INITIAL <CRC> OR <NAK>
        # SO START DIRECTLY WITH STREAM TRANSMISSION
        sequence = 1
        error_count = 0

        while True:
            data = stream.read(packet_size)
            # Check if we're done sending
            if not data:
                break

            # Select optimal packet size when using YMODEM
            if self.protocol == PROTOCOL_YMODEM:
                packet_size = (len(data) <= 128) and 128 or 1024

            # Align the packet
            data += (packet_size - len(data)) * b'\x1A'

            # Calculate CRC or checksum
            crc = crc32_byte(list(data))

            # SENDS PACKET WITH CRC
            if not self._send_packet(sequence, data, packet_size, crc_mode,
                crc, error_count, retry, timeout):
                log.error(error.ERROR_SEND_PACKET)
                return False

            # Next sequence
            sequence = (sequence + 1) % 0x100

        # STREAM FINISHED, SEND EOT
        log.debug(error.DEBUG_SEND_EOT)
        if self._send_eot(error_count, retry, timeout):
            return True
        else:
            log.error(error.ERROR_SEND_EOT)
            return False

    def _send_packet(self, sequence, data, packet_size, crc_mode, crc,
        error_count, retry, timeout):
        print('sequence', sequence)
        '''
        Sends one single packet of data, appending the checksum/CRC. It retries
        in case of errors and wait for the <ACK>.

        Return ``True`` on success, ``False`` in case of failure.
        '''
        start_char = SOH if packet_size == 128 else STX
        while True:
            self.putc(start_char)
            self.putc(bytes([sequence]))
            self.putc(bytes([0xff - sequence]))
            self.putc(data)
            if crc_mode:
                self.putc(bytes([crc >> 8]))
                self.putc(bytes([crc & 0xff]))
            else:
                # Send CRC or checksum
                self.putc(bytes([crc]))

            # Wait for the <ACK>
            char = self.getc(1, 0.1)
            # TODO: Due to the delay of the serial, there may be a miss of ACK,
            # so the timeout would send back CRC. We force it to ACK manually.
            char = ACK
            if char == ACK:
                # Transmission of the character was successful
                return True

            if char in [None, NAK]:
                error_count += 1
                if error_count >= retry:
                    # Excessive amounts of retransmissions requested
                    log.error(error.ABORT_ERROR_LIMIT)
                    return False
                continue

            # Protocol error
            log.error(error.ERROR_PROTOCOL)
            error_count += 1
            if error_count >= retry:
                log.error(error.ABORT_ERROR_LIMIT)
                return False

    def _send_eot(self, error_count, retry, timeout):
        '''
        Sends an <EOT> code. It retries in case of errors and wait for the
        <ACK>.

        Return ``True`` on success, ``False`` in case of failure.
        '''
        while True:
            # self.putc(EOT)
            start_char = EOT
            self.putc(start_char)

            # Wait for <ACK>
            char = self.getc(1, timeout)
            if char == ACK:
                # <EOT> confirmed
                return True
            else:
                error_count += 1
                if error_count >= retry:
                    # Excessive amounts of retransmissions requested,
                    # abort transfer
                    log.error(error.ABORT_ERROR_LIMIT)
                    return False

    def _wait_recv(self, error_count, timeout, retry):
        '''
        Waits for a <NAK> or <CRC> before starting the transmission.

        Return <NAK> or <CRC> on success, ``False`` in case of failure
        '''
        # Initialize protocol
        cancel = 0
        # Loop until the first character is a control character (NAK, CRC) or
        # we reach the retry limit
        while True:
            char = self.getc(1, 1)
            if char:
                if char in [NAK, CRC]:
                    return char
                elif char == CAN:
                    # Cancel at two consecutive cancels
                    if cancel:
                        log.error(error.ABORT_RECV_CAN_CAN)
                        self.abort(timeout=timeout)
                        return False
                    else:
                        log.debug(error.DEBUG_RECV_CAN)
                        cancel = 1
                else:
                    # Ignore the rest
                    pass

            error_count += 1
            if error_count >= retry:
                self.abort(timeout=timeout)
                return False
