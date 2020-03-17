'''
XMODEM Protocol bytes
=====================

.. data:: SOH

   Indicates a packet length of 128 (X/Y)

.. data:: STX

   Indicates a packet length of 1024 (X/Y)

.. data:: EOT

   End of transmission (X/Y)

.. data:: ACK

   Acknowledgement (X/Y)

.. data:: XON

   Enable out of band flow control (Z)

.. data:: XOFF

   Disable out of band flow control (Z)

.. data:: NAK

   Negative acknowledgement (X/Y)

.. data:: CAN

   Cancel (X/Y)

.. data:: CRC

   Cyclic redundancy check (X/Y)

'''

SOH         = chr(0x01)
STX         = chr(0x02)
EOT         = chr(0x04)
ACK         = chr(0x06)
XON         = chr(0x11)
XOFF        = chr(0x13)
NAK         = chr(0x15)
CAN         = chr(0x18)
CRC         = chr(0x43)

# MODEM Protocol types
PROTOCOL_XMODEM     = 0x00
PROTOCOL_YMODEM     = 0x01

PACKET_SIZE = {
   PROTOCOL_XMODEM:    128,
   PROTOCOL_YMODEM:    1024,
}
