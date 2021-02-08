# Chirpbox
This repository contains all the designing materials on Chirpbox including designs/connections of hardware, daemon software, patch files required to be compiled with user's codes together, other software tools, and even BOMs used for replication.

# Code layout
The code directory is similar to the following:
```
📦ChirpBox
│      📜README.md
│      📜LICENSE.md
│
└──────📂Hardware
│      │      📜Frame.dxf
│      └──────📂PCB
│      └──────📂SCH
│
└──────📂Chirpbox manager
│      │      📜cbmng.py
│      │      📜cbmng_common.py
│      │      ...
│      └──────📂transfer_to_initiator
│      └──────📂JojoDiff
│      ...
│
└──────📂Daemon
│      └──────📂Drivers
│      └──────📂Src
│      ...
│
└──────📂Miscellaneous
│      │      📜GPS_firmware.ino
│      └──────📂Patch
│      └──────📂Toggle
│      ...
│
```
`/Hardware` schematics of the ChirpBox node
`/Chirpbox manager` script manager to execute tasks such as monitor nodes' status, file dissemination and collection
`/Daemon` source codes of daemon firmware
`/Miscellaneous` available tools and codes for users, for example codes for bank switch