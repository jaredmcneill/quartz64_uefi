/** @file
*  UART devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// UART2 DW APB
Device (URT2) {
    Name (_HID, "HISI0031")
    Name (_CID, "8250dw");
    Name (_UID, 2)
    Name (_STA, 0xF)
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

// UART3 DW APB
Device (URT3) {
    Name (_HID, "HISI0031")
    Name (_CID, "8250dw");
    Name (_UID, 3)
    Name (_STA, FixedPcdGet8(PcdUart3Status))
    Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xFE670000, 0x0001000)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 151 }
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

// UART4 DW APB
Device (URT4) {
    Name (_HID, "HISI0031")
    Name (_CID, "8250dw");
    Name (_UID, 4)
    Name (_STA, FixedPcdGet8(PcdUart4Status))
    Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xFE680000, 0x0001000)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 152 }
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