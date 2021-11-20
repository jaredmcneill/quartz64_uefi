# UEFI for PINE64 Quartz64 (Rockchip RK356X)

This repository contains a port of Tianocore EDK II to the PINE64 Quartz64 board.

https://www.pine64.org/quartz64a/

## Building an SD card image

Run `make sdcard`.

## Running

Connect a serial console to UART2 using settings 115200 8n1.

Note that the initial bootloader will print some messages at a different baud rate so if you see garbage on the serial port you may need to reset your terminal emulator.

