/** @file
 *
 *  ACPI support for the Quartz64 platform
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
#include <ConfigVars.h>

STATIC CONST EFI_GUID mAcpiTableFile = {
  0x0FBE0D20, 0x3528, 0x4F07, { 0x83, 0x8B, 0x9A, 0x71, 0x1C, 0x62, 0x65, 0x4f }
};

EFI_STATUS
EFIAPI
PlatformAcpiDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  switch (PcdGet32 (PcdSystemTableMode)) {
  case SYSTEM_TABLE_MODE_BOTH:
  case SYSTEM_TABLE_MODE_ACPI:
    break;
  default:
    DEBUG ((DEBUG_ERROR, "Not installing ACPI tables (user config)\n"));
    return EFI_SUCCESS;
  }

  return LocateAndInstallAcpiFromFv (&mAcpiTableFile);
}