/** @file
*  Watchdog (WDT_NS) device.
*
*  Copyright (c) 2023, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// WDT_NS
Device (WDOG) {
    Name (_HID, "PRP0001")
    Name (_UID, 7)
    Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xFE600000, 0x100)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 181 }
    })
    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", "snps,dw-wdt" },
            Package () { "clock-frequency", 24000000 },
        }
    })
}