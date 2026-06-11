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
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/AcpiLib.h>
#include <ConfigVars.h>

/** ACPI tables built from Platform/Rockchip/Rk356x/AcpiTables/$(PLATFORM_NAME).inf */
STATIC CONST EFI_GUID  mAcpiTableFile = {
  0x0FBE0D20, 0x3528, 0x4F07, { 0x83, 0x8B, 0x9A, 0x71, 0x1C, 0x62, 0x65, 0x4f }
};

/** ROC-RK3566-PC ACPI-only tables (Dsdt-AcpiOnly.asl / Gmac-MAC1.asl). */
STATIC CONST EFI_GUID  mRocRk3566PcAcpiOnlyTableFile = {
  0x7C4E9A21, 0x6B3D, 0x4F8E, { 0x9C, 0x12, 0xA0, 0xC1, 0x35, 0x66, 0xAC, 0x01 }
};

STATIC CONST CHAR8  mRocRk3566PcFamilyName[] = "ROC-RK3566-PC";

STATIC
BOOLEAN
IsRocRk3566Pc (
  VOID
  )
{
  CONST CHAR8  *FamilyName;

  FamilyName = (CONST CHAR8 *)PcdGetPtr (PcdFamilyName);
  if (FamilyName == NULL) {
    return FALSE;
  }

  return AsciiStrCmp (FamilyName, mRocRk3566PcFamilyName) == 0;
}

STATIC
CONST EFI_GUID *
SelectAcpiTableFile (
  VOID
  )
{
  if (IsRocRk3566Pc () &&
      (PcdGet32 (PcdSystemTableMode) == SYSTEM_TABLE_MODE_ACPI)) {
    DEBUG ((
      DEBUG_INFO,
      "ROC-RK3566-PC: installing ACPI-only tables (enriched GMAC1 DSDT)\n"
      ));
    return &mRocRk3566PcAcpiOnlyTableFile;
  }

  return &mAcpiTableFile;
}

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

  return LocateAndInstallAcpiFromFv (SelectAcpiTableFile ());
}
