/** @file
*  CPU devices.
*
*  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>

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
    