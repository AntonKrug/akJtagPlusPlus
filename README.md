# AK JTAG++

Experimental JTAG dongle implemented in C++

[![Generic badge](https://img.shields.io/badge/License-GPLv2-green.svg)](https://github.com/AntonKrug/akJtagPlusPlus/blob/main/LICENSE)

[![WIP](https://img.shields.io/badge/Work%20in%20progress%3F-yes-orange.svg)](https://github.com/AntonKrug/akJtagPlusPlus/commits/main)

[![IDE](https://img.shields.io/badge/IDE-STM32%20CubeIDE%201.6-blue.svg)](https://www.st.com/en/development-tools/stm32cubeide.html)

[![EDA](https://img.shields.io/badge/EDA-KiCAD-navy.svg)](https://www.kicad.org/)


# Build

Based upon STM32F429ZI-DISC1 devboard with small daughter board connected to it (schematics/board below). To build the binaries, the project needs to be imported to a STM32cubeIDE 1.6 workspace and built with a Ctrl + B.

# Daughter board schematics and PCB

KiCad project files are located in the [schematics](/schematics/adapterBoard) folder.

![photo](../assets/images/photo.jpg)

## Schematic

![schematic](../assets/images/schematic.png)

## PCB

![pcb](../assets/images/pcb.png)

