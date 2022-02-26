/** @file
*  GMAC devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// Gigabit Media Access Controller (GMAC0)
Device (MAC0) {
    Name (_HID, "PRP0001")
    Name (_UID, 2)
    Name (_CCA, Zero)
    Name (_STA, FixedPcdGet8(PcdMac0Status))

    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", Package () { "snps,dwmac-4.20a", "snps,dwmac" } },
            Package () { "interrupt-names", Package () { "macirq", "eth_wake_irq" } },
            Package () { "snps,mixed-burst", 1 },
            Package () { "snps,tso", 1 },
            Package () { "snps,axi-config", "AXIC" },
        }
    })

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFE2A0000, 0x10000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 59 }
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 56 }
        })
        Return (RBUF)
    }

    Name (AXIC, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "snps,wr_osr_lmt", 4 },
            Package () { "snps,rd_osr_lmt", 8 },
            Package () { "snps,blen", Package () { 0, 0, 0, 0, 16, 8, 4 } },
        }
    })
}

// Gigabit Media Access Controller (GMAC1)
Device (MAC1) {
    Name (_HID, "PRP0001")
    Name (_UID, 3)
    Name (_CCA, Zero)
    Name (_STA, FixedPcdGet8(PcdMac1Status))

    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", Package () { "snps,dwmac-4.20a", "snps,dwmac" } },
            Package () { "interrupt-names", Package () { "macirq", "eth_wake_irq" } },
            Package () { "snps,mixed-burst", 1 },
            Package () { "snps,tso", 1 },
            Package () { "snps,axi-config", "AXIC" },
        }
    })

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFE010000, 0x10000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 64 }
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 61 }
        })
        Return (RBUF)
    }

    Name (AXIC, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "snps,wr_osr_lmt", 4 },
            Package () { "snps,rd_osr_lmt", 8 },
            Package () { "snps,blen", Package () { 0, 0, 0, 0, 16, 8, 4 } },
        }
    })
}