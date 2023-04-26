# UEFI for Rockchip RK356X based Single Board Computers

This repository contains a port of Tianocore EDK II to various Rockchip RK356x based boards:

* [PINE64 Quartz64 Model A](https://www.pine64.org/quartz64a/) / [PINE64 Quartz64 Model B](https://www.pine64.org/quartz64b/) / [PINE64 SOQuartz](https://wiki.pine64.org/wiki/SOQuartz)
* [Firefly ROC-RK3566-PC](https://en.t-firefly.com/product/industry/rocrk3566pc.html) / [StationPC Station M2](https://www.stationpc.com/product/stationm2)
* [Firefly ROC-RK3568-PC](https://en.t-firefly.com/product/industry/rocrk3568pc.html) / [StationPC Station P2](https://www.stationpc.com/product/stationp2)
* [Radxa ROCK3 Computing Module](https://wiki.radxa.com/Rock3/CM3)

## Operating system support

| OS | Version | Supported hardware | Notes |
| --- | --- | --- | --- |
| ESXi-Arm | 1.12 | HDMI, USB2, USB3, serial, PCIe, ethernet | |
| Fedora | 38 | HDMI, USB2, USB3, serial, PCIe, thermal sensors | Needs `irqchip.gicv3_nolpi=1` for MSI support |
| FreeBSD | 14.0-CURRENT | ? | Mangled serial output, boot stuck waiting for random seed |
| NetBSD | 9.99.x | HDMI, USB2, USB3, serial, SD card, PCIe, eMMC, SATA, ethernet, thermal sensors, watchdog | |
| OpenBSD | 7.0-current | HDMI, USB2, USB3, serial | To use HDMI console, enter `set tty fb0` at the bootloader prompt. |
| Ubuntu | 21.04 | HDMI, USB2, USB3, serial, PCIe, thermal sensors | Needs `irqchip.gicv3_nolpi=1` for MSI support |
| Windows PE | ? | HDMI, USB3, PCIe | BSOD when plugging device in to USB2 port (#2) |

## Building an SD card / eMMC image

If you want to build the image, checkout the repository and run:
`$ make sdcard`

Prebuild images are also provided for stable ports and are available in the [release section](https://github.com/jaredmcneill/quartz64_uefi/releases).

**Note:** The ROCK3 Compute Module port is still work in progress: as such no prebuild images are released for those boards.

## Installing

In order to have the board booting in UEFI, the firmware has to be located in one of the supported boot devices.

### PINE64 Quartz64 Model A
TBD

### Firefly ROC-RK3566-PC / Firefly ROC-RK3568-PC
#### SD card
The easiest way to test and use the UEFI firmware on this kind of boards is to flash it on an SD card.

**NOTE**: the operation is destructive, all data on the flash will be wiped!

1. Insert your SD card in your host and identify the device associated with it:
   ~~~
   $ sudo lsblk
   ~~~
1. Write the firmware on the SD card, using: 
   ~~~
   $ sudo dd if=ROC-RK3566-PC_EFI.img of=/dev/$device_identified_in_previous_step
   ~~~
   OR
   ~~~
   $ sudo dd if=ROC-RK3568-PC_EFI.img of=/dev/$device_identified_in_previous_step
   ~~~
   according to your board.
1. Insert the SD card in the board: profit! The SD card need to remain in place because from now on it will be needed to boot the device.

#### eMMC
If you plan not to use the embedded eMMC for storing your OS, because i.e. you plan to use a more performant NVMe device, you could use that eMMC as storage for the UEFI firmware.

This is slightly more complex procedure, but it will allow you to use the SD card slot for other uses. In order to reconfigure the board, it will be user a Linux  system.

**NOTE**: the procedure below does not substitute the [official documentation from the vendor](https://wiki.t-firefly.com/en/ROC-RK3566-PC/index.html), is a summary of the needed steps. In case of doubts, rely on the vendor support: don't open issues on this project if you have trouble in flashing the board, ask to Firefly!

1. Obtain `Linux Upgrade Tool` and `RK3566/RK3568 NorFlash2eMMCLoader` from the [official Firefly Download page](https://en.t-firefly.com/doc/download/93.html)
1. Enter in `Loader` mode via:
    1. Press `Recover` button next to the audio jack while connecting to the PC via USB cable (more direct and less 'invasive' if you have access to the entire board)
    
    OR
    
    1. Press `MaskRom` button near the GPIO pins while connecting to the PC via USB cable (more impactful approach, but needed if you have the StationPC M2)
    2. Verify the SBC is detected and in `MaskRom` mode:
       ~~~
       $ sudo ./upgrade_tool LD
       Program Log will save in the /root/upgrade_tool/log/
       List of rockusb connected(1)
       DevNo=1 Vid=0x2207,Pid=0x350a,LocationID=13     Mode=Maskrom    SerialNo=
       ~~~
    1. Switch to `Loader` mode by flashing `RK356x_NorFlash2eMMCLoader_20211209.img`:
       ~~~
       $ sudo ./upgrade_tool UF RK356x_NorFlash2eMMCLoader_20211209.img
       Program Log will save in the /root/upgrade_tool/log/
       Loading firmware...
       Support Type:RK3568     FW Ver:b.0.00   FW Time:2021-12-09 08:59:21
       Loader ver:1.01 Loader Time:2021-12-09 08:55:06
       Upgrade firmware ok.
       ~~~
2. Verify the SBC is detected and in `Loader` mode:
   ~~~
   $ sudo ./upgrade_tool LD
   Program Log will save in the /root/upgrade_tool/log/
   List of rockusb connected(1)
   DevNo=1 Vid=0x2207,Pid=0x350a,LocationID=13     Mode=Loader     SerialNo=
   ~~~
   if the board is not in `Loader` mode, or is not detected at all:
   ~~~
   $ sudo ./upgrade_tool LD
   Program Log will save in the /root/upgrade_tool/log/
   List of rockusb connected(0)
   ~~~
   wait 10 seconds and repeat the command. If the board is still not detected or not in `Loader` mode, repeat the procedure from the beginning.
1. Check the original partition table: the default is similar to the following:
   ~~~
   $ sudo ./upgrade_tool PL
   Program Log will save in the /root/upgrade_tool/log/
   Partition Info(gpt):
   NO  LBA        Size       Name
   01  0x00002000 0x00002000 uboot
   02  0x00006000 0x00002000 trust
   03  0x00008000 0x00002000 misc
   04  0x0000a000 0x00002000 dtbo
   05  0x0000c000 0x00000800 vbmeta
   06  0x0000c800 0x00010000 boot
   07  0x0001c800 0x00002000 security
   08  0x0001e800 0x00030000 recovery
   09  0x0004e800 0x000c0000 backup
   10  0x0010e800 0x000c0000 cache
   11  0x001ce800 0x00008000 metadata
   12  0x001d6800 0x00614000 super
   13  0x007ea800 0x00100000 oem
   14  0x008ea800 0x06ba97c0 userdata
   ~~~
1. Overwrite the internal eMMC with the UEFI firmware.

   **NOTE**: the operation is destructive, all data on the eMMC will be wiped off!
   ~~~
   $ sudo ./upgrade_tool WL 0 ROC-RK3566-PC_EFI.img
   ~~~
   OR
   ~~~
   $ sudo ./upgrade_tool WL 0 ROC-RK3568-PC_EFI.img
   ~~~
   according to your board.
   
   The expected output is:
   ~~~
   Program Log will save in the /root/upgrade_tool/log/
   Write LBA from file (100%)
   ~~~
1. Check the flash has been successful by reading the partition table
   ~~~
   $ sudo ./upgrade_tool PL
   Program Log will save in the /root/upgrade_tool/log/
   Partition Info(gpt):
   NO  LBA        Size       Name
   01  0x00000040 0x00003fc0 loader
   02  0x00004000 0x00004000 uboot
   03  0x00008000 0x00008000 env
   ~~~
1. Reboot and enjoy your UEFI powered SBC.

### Radxa ROCK3 Computing Module
TBD

## Running

Connect a serial console to UART2 using settings `115200 8n1`.

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
