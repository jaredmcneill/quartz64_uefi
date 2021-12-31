/** @file
*  DesignWare Mobile Storage Host Controller (SD/SDIO) devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// Mobile Storage Host Controller
Device (MSH0) {
    Name (_HID, "PRP0001")
    Name (_UID, 3)
    Name (_CCA, Zero)

    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", Package () { "rockchip,rk3568-dw-mshc", "rockchip,rk3288-dw-mshc" } },
            Package () { "fifo-depth", 0x100 },
            Package () { "max-frequency", 50000000 },
            Package () { "bus-width", 4 },
            Package () { "cap-sd-highspeed", 1 },
            Package () { "disable-wp", 1 },
        }
    })

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFE2B0000, 0x4000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 130 }
        })
        Return (RBUF)
    }
}

// Mobile Storage Host Controller
Device (MSH1) {
    Name (_HID, "PRP0001")
    Name (_UID, 4)
    Name (_CCA, Zero)

    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", Package () { "rockchip,rk3568-dw-mshc", "rockchip,rk3288-dw-mshc" } },
            Package () { "fifo-depth", 0x100 },
            Package () { "max-frequency", 50000000 },
            Package () { "bus-width", 4 },
            Package () { "cap-sd-highspeed", 1 },
            Package () { "cap-sdio-irq", 1 },
            Package () { "disable-wp", 1 },
            Package () { "non-removable", 1 }
        }
    })

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFE2C0000, 0x4000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 131 }
        })
        Return (RBUF)
    }

    Device (SDMM) {
        Method (_ADR) {
            Return (0)
        }
        Method (_RMV) {
            Return (0) // non-removable
        }
    }
}
