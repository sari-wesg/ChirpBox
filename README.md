# ChirpBox
This repository contains all the designing materials on ChirpBox including designs of hardware, daemon software, patch files required to be compiled with user's codes together, other software tools, and BOMs used for replication. For more details on how to use ChirpBox, please check out our step-by-step [tutorials](https://chirpbox.github.io/) to get started.

# Code layout
The ChirpBox directory is simplified to the following:
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
└──────📂Chirpbox_manager
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
- `/Hardware`: schematics of the ChirpBox node
- `/Chirpbox_manager`: script manager to execute tasks such as monitor nodes' status, file dissemination and collection
- `/Daemon`: source codes of daemon firmware
- `/Miscellaneous`: available tools and codes for users, for example codes for bank switch

# License

Unless otherwise noted (e.g. [STM32 standard peripheral libraries](https://www.st.com/en/embedded-software/stm32-standard-peripheral-libraries.html)), codes and schematics are licensed under **Creative Commons Attribution Share Alike 4.0 International (CC BY-SA 4.0)**.

[![License: CC BY-SA 4.0](https://img.shields.io/badge/License-CC%20BY--SA%204.0-lightgrey.svg)](http://creativecommons.org/licenses/by-sa/4.0/)