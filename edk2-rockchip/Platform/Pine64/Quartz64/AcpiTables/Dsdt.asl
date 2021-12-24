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

    Device (CPU0)
    {
        Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
        Name (_UID, 0)  // _UID: Unique ID
    }
    Device (CPU1)
    {
        Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
        Name (_UID, 1)  // _UID: Unique ID
    }
    Device (CPU2)
    {   
        Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
        Name (_UID, 2)  // _UID: Unique ID
    }
    Device (CPU3)
    {   
        Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
        Name (_UID, 3)  // _UID: Unique ID
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
      Name (_HID, "RKCP0DFF")
      Name (_CLS, Package() { 0x0c, 0x03, 0x10 })
      Name (_UID, Zero)
      Name (_CCA, Zero)

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
      Name (_HID, "RKCP0DFF")
      Name (_CLS, Package() { 0x0c, 0x03, 0x10 })
      Name (_UID, One)
      Name (_CCA, Zero)

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
