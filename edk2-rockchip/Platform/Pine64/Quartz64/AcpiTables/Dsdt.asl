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
    } // EHC0

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
    } // OHC0

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
    } // OHC1

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

  } // Scope (_SB)
}
