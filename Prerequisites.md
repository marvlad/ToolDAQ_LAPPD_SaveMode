# Prerequisites for the LAPPD ToolChains
## For new devices
```bash
#sudo apt-get update && apt-get upgrade && reboot
```
## General Prerequisites for ToolDAQ and compiler
```bash
#sudo apt-get install make cmake gcc g++ binutils libx11-dev libxpm-dev libxft-dev libxext-dev git dialog
```
## Prerequisites for the Data ToolChain
```bash
#sudo apt-get install libusb-1.0-0-dev
```
## Prerequisites for the Monitor ToolChain
```bash
#sudo apt-get install can-utils wiringpi
```
## Additional requirements for the Monitor ToolChain because of the CANBUS interface
Open the boot config file:
```bash
#sudo nano /boot/config.txt
```
Add the following lines at the end of file to set the interface:
```bash
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25 
dtoverlay=spi-bcm2835-overlay

#sudo reboot
```
Open the iterfaces file to control the canbus:
```bash
#sudo nano /etc/network/interfaces.txt
```
Add the following lines at the end of file to set the canbus to autostart on boot:
```bash
...

#sudo reboot
```

