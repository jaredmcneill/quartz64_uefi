/** @file
*  Differentiated System Description Table Fields (DSDT).
*
*  Copyright (c) 2020, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

DefinitionBlock ("DsdtTable.aml", "DSDT",
                 EFI_ACPI_6_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_REVISION,
                 "RKCP  ", "RK356X  ", FixedPcdGet32 (PcdAcpiDefaultOemRevision)) {
  Scope (_SB) {

    Device (CPU0) {
      Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
      Name (_UID, 0)  // _UID: Unique ID
    }
    Device (CPU1) {
      Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
      Name (_UID, 1)  // _UID: Unique ID
    }
    Device (CPU2) {   
      Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
      Name (_UID, 2)  // _UID: Unique ID
    }
    Device (CPU3) {   
      Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
      Name (_UID, 3)  // _UID: Unique ID
    }

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
    }

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

    // USB OHCI Host Controller
    Device (OHC0) {
      Name (_HID, "PRP0001")
      Name (_CLS, Package() { 0x0c, 0x03, 0x10 })
      Name (_UID, Zero)
      Name (_CCA, Zero)

      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", "generic-ohci" },
        }
      })

      Method (_CRS, 0x0, Serialized) {
          Name (RBUF, ResourceTemplate() {
              Memory32Fixed (ReadWrite, 0xFD840000, 0x40000)
              Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 163 }
          })
          Return (RBUF)
      }

      Device (RHUB) {
        Name (_ADR, 0)
        Device (PRT1) {
          Name (_ADR, 1)
          Name (_UPC, Package() {
            0xFF,         // Port is connectable
            0x00,         // Connector type - Type 'A'
            0x00000000,   // Reserved, must be zero
            0x00000000    // Reserved, must be zero
          })
          Name (_PLD, Package (0x01) {
            ToPLD (
              PLD_Revision            = 0x2,
              PLD_IgnoreColor         = 0x1,
              PLD_UserVisible         = 0x1,
              PLD_Panel               = "UNKNOWN",
              PLD_VerticalPosition    = "UPPER",
              PLD_HorizontalPosition  = "LEFT",
              PLD_Shape               = "HORIZONTALRECTANGLE",
              PLD_Ejectable           = 0x1,
              PLD_EjectRequired       = 0x1,
            )
          })
        } // PRT1
      } // RHUB
    } // OHC0

    // USB EHCI Host Controller
    Device (EHC0) {
        Name (_HID, "PNP0D20")
        Name (_UID, Zero)
        Name (_CCA, Zero)

        Method (_CRS, 0x0, Serialized) {
            Name (RBUF, ResourceTemplate() {
                Memory32Fixed (ReadWrite, 0xFD800000, 0x40000)
                Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 162 }
            })
            Return (RBUF)
        }

        Device (RHUB) {
          Name (_ADR, 0)
          Device (PRT1) {
            Name (_ADR, 1)
            Name (_UPC, Package() {
              0xFF,         // Port is connectable
              0x00,         // Connector type - Type 'A'
              0x00000000,   // Reserved, must be zero
              0x00000000    // Reserved, must be zero
            })
            Name (_PLD, Package (0x01) {
              ToPLD (
                PLD_Revision            = 0x2,
                PLD_IgnoreColor         = 0x1,
                PLD_UserVisible         = 0x1,
                PLD_Panel               = "UNKNOWN",
                PLD_VerticalPosition    = "UPPER",
                PLD_HorizontalPosition  = "LEFT",
                PLD_Shape               = "HORIZONTALRECTANGLE",
                PLD_Ejectable           = 0x1,
                PLD_EjectRequired       = 0x1,
              )
            })
          } // PRT1
        } // RHUB
    } // EHC0

    // USB OHCI Host Controller
    Device (OHC1) {
      Name (_HID, "PRP0001")
      Name (_CLS, Package() { 0x0c, 0x03, 0x10 })
      Name (_UID, One)
      Name (_CCA, Zero)

      Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", "generic-ohci" },
        }
      })

      Method (_CRS, 0x0, Serialized) {
          Name (RBUF, ResourceTemplate() {
              Memory32Fixed (ReadWrite, 0xFD8C0000, 0x40000)
              Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 166 }
          })
          Return (RBUF)
      }

      Device (RHUB) {
        Name (_ADR, 0)
        Device (PRT1) {
          Name (_ADR, 1)
          Name (_UPC, Package() {
            0xFF,         // Port is connectable
            0x00,         // Connector type - Type 'A'
            0x00000000,   // Reserved, must be zero
            0x00000000    // Reserved, must be zero
          })
          Name (_PLD, Package (0x01) {
            ToPLD (
              PLD_Revision            = 0x2,
              PLD_IgnoreColor         = 0x1,
              PLD_UserVisible         = 0x1,
              PLD_Panel               = "UNKNOWN",
              PLD_VerticalPosition    = "LOWER",
              PLD_HorizontalPosition  = "LEFT",
              PLD_Shape               = "HORIZONTALRECTANGLE",
              PLD_Ejectable           = 0x1,
              PLD_EjectRequired       = 0x1,
            )
          })
        } // PRT1
      } // RHUB
    } // OHC1

    // USB EHCI Host Controller
    Device (EHC1) {
      Name (_HID, "PNP0D20")
      Name (_UID, One)
      Name (_CCA, Zero)

      Method (_CRS, 0x0, Serialized) {
          Name (RBUF, ResourceTemplate() {
              Memory32Fixed (ReadWrite, 0xFD880000, 0x40000)
              Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 165 }
          })
          Return (RBUF)
      }

      Device (RHUB) {
        Name (_ADR, 0)
        Device (PRT1) {
          Name (_ADR, 1)
          Name (_UPC, Package() {
            0xFF,         // Port is connectable
            0x00,         // Connector type - Type 'A'
            0x00000000,   // Reserved, must be zero
            0x00000000    // Reserved, must be zero
          })
          Name (_PLD, Package (0x01) {
            ToPLD (
              PLD_Revision            = 0x2,
              PLD_IgnoreColor         = 0x1,
              PLD_UserVisible         = 0x1,
              PLD_Panel               = "UNKNOWN",
              PLD_VerticalPosition    = "LOWER",
              PLD_HorizontalPosition  = "LEFT",
              PLD_Shape               = "HORIZONTALRECTANGLE",
              PLD_Ejectable           = 0x1,
              PLD_EjectRequired       = 0x1,
            )
          })
        } // PRT1
      } // RHUB
    } // EHC1

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
    } // XHC0

    // PCIe
    Device (PCI0) {
      Name (_HID, "PNP0A08")
      Name (_CID, "PNP0A03")
      Name (_CCA, Zero)
      Name (_SEG, Zero)
      Name (_BBN, One)

      Name (_PRT, Package() {
        Package (4) { 0x0FFFF, 0, Zero, 104 },
        Package (4) { 0x0FFFF, 1, Zero, 104 },
        Package (4) { 0x0FFFF, 2, Zero, 104 },
        Package (4) { 0x0FFFF, 3, Zero, 104 }
      })

      Method (_CRS, 0, Serialized) {
        Name (RBUF, ResourceTemplate () {
          WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
            0,    // Granularity
            1,    // Range Minimum
            1,    // Range Maximum
            0,    // Translation Offset
            1,    // Length
          )
          DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
            0x00000000,   // Granularity
            0xF4000000,   // Range Minimum
            0xF5FFFFFF,   // Range Maximum
            0x00000000,   // Translation Offset
            0x02000000,   // Length
          )
          QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
            0x0000000000000000,   // Granularity
            0x0000000310000000,   // Range Minimum
            0x000000033FFEFFFF,   // Range Maximum
            0x0000000000000000,   // Translation Offset
            0x000000002FFF0000,   // Length
          )
          QWordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
            0,                    // Granularity
            0x0000,               // Range Minimum
            0xFFFF,               // Range Maximum
            0x000000033FFF0000,   // Translation Offset
            0x10000,              // Length
          )
        })
        return (RBUF)
      }

      Device (RES0) {
        Name (_HID, "PNP0C02")
        Name (_CRS, ResourceTemplate () {
          QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
            0x0000000000000000,   // Granularity
            0x0000000300000000,   // Range Minimum
            0x000000030FFFFFFF,   // Range Maximum
            0x0000000000000000,   // Translation Offset
            0x0000000010000000,   // Length
          )
          QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
            0x0000000000000000,   // Granularity
            0x00000003C0000000,   // Range Minimum
            0x00000003C03FFFFF,   // Range Maximum
            0x0000000000000000,   // Translation Offset
            0x0000000000400000,   // Length
          )
        })
      }

      // OS Control Handoff
      Name(SUPP, Zero) // PCI _OSC Support Field value
      Name(CTRL, Zero) // PCI _OSC Control Field value

      // See [1] 6.2.10, [2] 4.5
      Method(_OSC,4) {
        // Note, This code is very similar to the code in the PCIe firmware
        // specification which can be used as a reference
        // Check for proper UUID
        If(LEqual(Arg0,ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))) {
          // Create DWord-adressable fields from the Capabilities Buffer
          CreateDWordField(Arg3,0,CDW1)
          CreateDWordField(Arg3,4,CDW2)
          CreateDWordField(Arg3,8,CDW3)
          // Save Capabilities DWord2 & 3
          Store(CDW2,SUPP)
          Store(CDW3,CTRL)
          // Mask out Native HotPlug
          And(CTRL,0x1E,CTRL)
          // Always allow native PME, AER (no dependencies)
          // Never allow SHPC (no SHPC controller in this system)
          And(CTRL,0x1D,CTRL)

          If(LNotEqual(Arg1,One)) { // Unknown revision
            Or(CDW1,0x08,CDW1)
          }

          If(LNotEqual(CDW3,CTRL)) {  // Capabilities bits were masked
            Or(CDW1,0x10,CDW1)
          }
          // Update DWORD3 in the buffer
          Store(CTRL,CDW3)
          Return(Arg3)
        } Else {
          Or(CDW1,4,CDW1) // Unrecognized UUID
          Return(Arg3)
        }
      } // End _OSC
    }
    // PCI0

  } // Scope (_SB)
}
