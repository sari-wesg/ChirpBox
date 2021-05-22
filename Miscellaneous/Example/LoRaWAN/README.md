# LoRaWAN
This repository provides example code for building a LoRaWAN network on given hardwares. See more details in [LoRaWAN on ChirpBox](https://chirpbox.github.io/LoRaWAN/).
# End node
The end nodes' [example project](https://github.com/pei-tian/LoRaWAN-ChirpBox/tree/master/STM32CubeExpansion_LRWAN_V1.3.1/Projects/STM32L476RG-Nucleo/Applications/LoRa/End_Node/STM32CubeIDE/sx1276mb1mas) is based on [I-CUBE-LRWAN](https://www.st.com/en/embedded-software/i-cube-lrwan.html), where modifications are within the define-marco `CHIRPBOX_LORAWAN`.

# Python example
Python examples are related to the LoRa gateway's built-in server [ChirpStack](https://www.chirpstack.io/).

## Hardware:
- End node: [NUCLEO-L476RG](https://www.st.com/en/evaluation-tools/nucleo-l476rg.html) + [SX1276MB1MAS](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1276mb1mas)
- Gateway: [RAK831 Pilot](https://uk.pi-supply.com/products/rak831-pilot-gateway-professional-demonstration-setup?lang=zh) / [RAK7243](https://store.rakwireless.com/products/rak7243c-pilot-gateway?utm_source=rak7243&utm_medium=footer&variant=26682434355300)
