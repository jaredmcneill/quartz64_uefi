/** @file
*  Thermal sensor ADC devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

// TS-ADC
Device (THRM) {
    Name (_HID, "PNP0C02")
    Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xFE710000, 0x100)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 147 }
    })

    // TS-ADC registers
    OperationRegion (REG, SystemMemory, 0xFE710000, 0x100)
    Field (REG, DWordAcc, NoLock, Preserve) {
        Offset  (0x20),
        DAT0,   32,
        DAT1,   32
    }

    // Data code table. Each entry represents a 5 degC step, starting at -45 degC
    Name (DTAB, Package () {
        0, 1584, 1620, 1652, 1688, 1720, 1756, 1788, 1824, 1856, 1892, 1924, 1956, 1992, 2024, 2060, 2092, 2128, 2160, 2196, 2228, 2264, 2300, 2332, 2368, 2400, 2436, 2468, 2500, 2536, 2572, 2604, 2636, 2672, 2704
    })

    // Convert table index to millidegrees Celcius
    Method (ITOC, 1) {
        Return ((Arg0 * 5000) - 45000)
    }

    // Convert raw data to millidegrees Celcius
    Method (DTOC, 1) {
        // Local0: Counter
        // Local1: Number of items in DTAB table
        // Local2: Current item value in DTAB
        // Local3: Interpolated offset
        // Local4: Previous item value in DTAB
        Local1 = SizeOf (DTAB)
        For (Local0 = 1, Local0 < Local1, Local0++) {
        Local2 = DerefOf (Index (DTAB, Local0))
        
        // Exact match from data code table, just return it
        If (Arg0 == Local2) {
            Return (ITOC (Local0))
        }

        // Interpolate
        If (Arg0 < Local2) {
            Local4 = DerefOf (Index (DTAB, Local0 - 1))
            Local3 = 5000 * (Arg0 - Local4);
            Local3 /= (Local2 - Local4);
            Return (ITOC (Local0 - 1) + Local3)
        }
        }

        // Not found
        Return (0)
    }

    // Convert raw data to tenths of degrees Kelvin
    Method (DTOK, 1) {
        Return ((DTOC (Arg0) + 273150) / 100)
    }

    // Get CPU temperature in tenths of degrees Kelvin
    Method (TMP0) {
        Return (DTOK (DAT0))
    }

    // Get GPU temperature in tenths of degrees Kelvin
    Method (TMP1) {
        Return (DTOK (DAT1))
    }
}

// Thermal zone (CPU)
ThermalZone (TCPU) {
    Name (_STR, Unicode ("CPU temperature"))
    Method (_TMP) {
        Return (\_SB.THRM.TMP0)
    }
    Name (_CRT, 3882) // 115C
    Name (_PSL, Package() {
        \_SB.CPU0,
        \_SB.CPU1,
        \_SB.CPU2,
        \_SB.CPU3
    })
}

// Thermal zone (GPU)
ThermalZone (TGPU) {
    Name (_STR, Unicode ("GPU temperature"))
    Method (_TMP) {
        Return (\_SB.THRM.TMP1)
    }
    Name (_CRT, 3882) // 115C
}