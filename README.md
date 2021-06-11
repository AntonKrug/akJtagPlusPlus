# AK JTAG++

Experimental JTAG dongle implemented in C++

Allowing users to interact with JTAG enabled devices with USB, or in a standalone mode with LCD (and touch screen).

Limited to only minimalistic official (vanila) JTAG specification (no custom vendor commands) and 3.3V based devices. However it's possible to convert it to different voltages, or allow level conversion to range of voltages. And adding protocols/commands is possible as well if their specification is published.

[![Generic badge](https://img.shields.io/badge/License-GPLv2-green.svg)](https://github.com/AntonKrug/akJtagPlusPlus/blob/main/LICENSE) [![WIP](https://img.shields.io/badge/Work%20in%20progress%3F-yes-orange.svg)](https://github.com/AntonKrug/akJtagPlusPlus/commits/main) [![IDE](https://img.shields.io/badge/IDE-STM32%20CubeIDE%201.6-blue.svg)](https://www.st.com/en/development-tools/stm32cubeide.html) [![EDA](https://img.shields.io/badge/EDA-KiCAD-navy.svg)](https://www.kicad.org/)


# Bit-banging

The JTAG signals are bitbanged in SW, but written in assembly to make sure that the duty cycle stays the way it's expected as C/C++ can produce fast code, but it wouldn't gurantee the timing of the signals. In my assembly the sampling of read signals is happening around 2/3 of the clock period.

Depending on the revision the specifics sometimes change, but I try to keep it around 9MHz and around 50% duty cycle. For example if I wanted to have 8MHz exactly, then I couldn't have exact 50% duty cycle, because 168MHz / 8MHz is 21 clocks per period and can't be split evenly to 50:50 and small deviation from 50% duty cycle is necesary for some TCK frequencies.

![scope](../assets/images/tck-50.png)

# Build

Based upon [STM32F429ZI-DISC1](https://www.st.com/en/evaluation-tools/32f429idiscovery.html) devboard with small daughter board connected to it (schematics/board below). To build the binaries, the project needs to be imported to a STM32cubeIDE 1.6 workspace and built with a Ctrl + B.

# Daughter board schematics and PCB

KiCad project files are located in the [schematics](/schematics/adapterBoard) folder.

![photo](../assets/images/photo.jpg)

## Schematic

![schematic](../assets/images/schematic.png)

# USB

Is implemented by using the [STM32F429ZI-DISC1](https://www.st.com/en/evaluation-tools/32f429idiscovery.html) USB port, PID:VID has to be determined yet.
The class of device is HID, with fairly small report size (32bytes) and no extra drivers are necesary (bundled drivers with the OS are fine) and application
can use HID RAW APIs to interact with the device directly.


## PCB

![pcb](../assets/images/pcb.png)

# References

https://en.wikipedia.org/wiki/Joint_Test_Action_Group

https://en.wikipedia.org/wiki/JTAG

http://openocd.org/doc/html/Reset-Configuration.html

http://openocd.org/doc-release/html/JTAG-Commands.html

http://openocd.org/doc/doxygen/html/ftdi_8c.html
