/** @file
*  eMMC Controller devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// eMMC Controller
Device (EMMC) {
    Name (_HID, "RKCP0D40")
    Name (_UID, 0)
    Name (_CCA, Zero)

    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "sdhci-caps-mask", 0x70000FF00 },   // Bits [15:8]: BASE_CLK_FREQ
                                                             // Bit [32]: SDR50_SUPPORT
                                                             // Bit [33]: SDR104_SUPPORT
                                                             // Bit [34]: DDR50_SUPPORT
            Package () { "sdhci-caps", 0x3200 }              // BASE_CLK_FREQ = 50 MHz
        }
    })

    // CRU_CLKSEL_CON28
    OperationRegion (CRU, SystemMemory, 0xFDD20170, 0x4)
    Field (CRU, DWordAcc, NoLock, Preserve) {
        CS28,   32
    }

    Method (_DSM, 4) {
        If (LEqual (Arg0, ToUUID("434addb0-8ff3-49d5-a724-95844b79ad1f"))) {
            Switch (ToInteger (Arg2)) {
                Case (0) {
                    Return (0x3)
                }
                Case (1) {
                    Local0 = DerefOf (Arg3 [0])
                    If (Local0 >= 200000000) {
                        CS28 = 0x70001000
                        Return (200000000)
                    }
                    If (Local0 >= 150000000) {
                        CS28 = 0x70002000
                        Return (150000000)
                    }
                    If (Local0 >= 100000000) {
                        CS28 = 0x70003000
                        Return (100000000)
                    }
                    If (Local0 >= 50000000) {
                        CS28 = 0x70004000
                        Return (50000000)
                    }
                    If (Local0 >= 24000000) {
                        CS28 = 0x70000000
                        Return (24000000)
                    }
                    if (Local0 >= 375000) {
                        CS28 = 0x70005000
                        Return (375000)
                    }
                    Return (0)
                }
            }
        }
        Return (0)
    }

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFE310000, 0x10000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 51 }
        })
        Return (RBUF)
    }
}