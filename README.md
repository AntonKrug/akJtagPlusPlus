# AK JTAG++

Experimental JTAG dongle implemented in C++

Allowing users to interact with JTAG enabled devices with USB, or in a standalone mode with LCD (and touch screen).

Limited to only minimalistic official (vanila) JTAG specification (no custom vendor commands) and 3.3V based devices. However it's possible to convert it to different voltages, or allow level conversion to range of voltages. And adding protocols/commands is possible as well if their specification is published.

[![Generic badge](https://img.shields.io/badge/License-GPLv2-green.svg)](https://github.com/AntonKrug/akJtagPlusPlus/blob/main/LICENSE) [![WIP](https://img.shields.io/badge/Work%20in%20progress%3F-yes-orange.svg)](https://github.com/AntonKrug/akJtagPlusPlus/commits/main) [![IDE](https://img.shields.io/badge/IDE-STM32%20CubeIDE%201.6-blue.svg)](https://www.st.com/en/development-tools/stm32cubeide.html) [![EDA](https://img.shields.io/badge/EDA-KiCAD-navy.svg)](https://www.kicad.org/)


# Build

Based upon STM32F429ZI-DISC1 devboard with small daughter board connected to it (schematics/board below). To build the binaries, the project needs to be imported to a STM32cubeIDE 1.6 workspace and built with a Ctrl + B.

# Daughter board schematics and PCB

KiCad project files are located in the [schematics](/schematics/adapterBoard) folder.

![photo](../assets/images/photo.jpg)

## Schematic

![schematic](../assets/images/schematic.png)

## PCB

![pcb](../assets/images/pcb.png)

