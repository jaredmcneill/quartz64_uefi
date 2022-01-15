# UEFI for PINE64 Quartz64 (Rockchip RK356X)

This repository contains a port of Tianocore EDK II to the PINE64 Quartz64 board.

https://www.pine64.org/quartz64a/

## Building an SD card image

Run `make sdcard`.

## Running

Connect a serial console to UART2 using settings 115200 8n1.

## eMMC controller Device-Specific Method (_DSM)

The divider register in the RK356x's SDHCI controller does not work, so to change the eMMC card clock we have to do it through the CRU. This is exposed to the OS as an ACPI Device-Specific Method (_DSM).

### Function 1: Set Card Clock

The _DSM control method parameters for the Set Card Clock function are as follows:

#### Arguments

 - **Arg0**: UUID = 434addb0-8ff3-49d5-a724-95844b79ad1f
 - **Arg1**: Revision = 0
 - **Arg2**: Function Index = 1
 - **Arg3**: Target card clock rate in Hz.

#### Return

The actual card clock rate in Hz. Will be less than or equal to the target clock rate. Returns 0 if the target clock rate could not be set.