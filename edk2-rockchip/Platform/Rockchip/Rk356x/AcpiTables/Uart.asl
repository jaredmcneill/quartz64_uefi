/** @file
*  UART devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// UART DW APB
Device (COM0) {
    Name (_HID, "HISI0031")
    Name (_CID, "8250dw");
    Name (_UID, Zero)
    Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite,
                        FixedPcdGet64 (PcdSerialRegisterBase),
                        0x0001000)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 150 }
    })
    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "clock-frequency", FixedPcdGet32 (PcdSerialClockRate) },
            Package () { "reg-shift", 2 },
            Package () { "reg-io-width", 4 },
        }
    })
}