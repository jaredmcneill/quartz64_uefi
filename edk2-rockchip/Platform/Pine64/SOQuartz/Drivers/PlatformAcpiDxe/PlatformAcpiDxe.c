/** @file
 *
 *  ACPI support for the SOQuartz platform
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2017,2021 Andrey Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) 2016, Linaro, Ltd. All rights reserved.
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/AcpiLib.h>

STATIC CONST EFI_GUID mAcpiTableFile = {
  0xDD22B2BE, 0x6A59, 0x11EC, { 0x9A, 0x0B, 0xEF, 0x31, 0xBD, 0x30, 0x79, 0xC7 }
};

EFI_STATUS
EFIAPI
PlatformAcpiDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  return LocateAndInstallAcpiFromFv (&mAcpiTableFile);
}
