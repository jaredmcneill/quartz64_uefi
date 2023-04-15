/** @file
*  SATA devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

Device (SAT0) {
    Name (_HID, "RKCP0161")
    Name (_UID, 0)
    Name (_CLS, Package() { 0x01, 0x06, 0x01 })
    Name (_CCA, Zero)

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFC000000, 0x1000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 126 }
        })
        Return (RBUF)
    }

    Name (_STA, FixedPcdGet8(PcdSata0Status))
} // SAT2

Device (SAT1) {
    Name (_HID, "RKCP0161")
    Name (_UID, 1)
    Name (_CLS, Package() { 0x01, 0x06, 0x01 })
    Name (_CCA, Zero)

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFC400000, 0x1000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 127 }
        })
        Return (RBUF)
    }

    Name (_STA, FixedPcdGet8(PcdSata1Status))
} // SAT2

Device (SAT2) {
    Name (_HID, "RKCP0161")
    Name (_UID, 2)
    Name (_CLS, Package() { 0x01, 0x06, 0x01 })
    Name (_CCA, Zero)

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFC800000, 0x1000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 128 }
        })
        Return (RBUF)
    }

    Name (_STA, FixedPcdGet8(PcdSata2Status))
} // SAT2
