/** @file
*  USB3 devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// USB XHCI Host Controller
Device (XHC0) {
    Name (_HID, "PNP0D10")
    Name (_UID, Zero)
    Name (_CCA, Zero)

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFCC00000, 0x400000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 201 }
        })
        Return (RBUF)
    }

    Name (_STA, FixedPcdGet8(PcdXhc0Status))
} // XHC0

// USB XHCI Host Controller
Device (XHC1) {
    Name (_HID, "PNP0D10")
    Name (_UID, One)
    Name (_CCA, Zero)

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFD000000, 0x400000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 202 }
        })
        Return (RBUF)
    }

    Name (_STA, FixedPcdGet8(PcdXhc1Status))
} // XHC0