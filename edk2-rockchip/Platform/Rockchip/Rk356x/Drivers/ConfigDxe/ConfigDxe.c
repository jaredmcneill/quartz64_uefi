/** @file
 *
 *  Copyright (c) 2023, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2019 - 2020, ARM Limited. All rights reserved.
 *  Copyright (c) 2018 - 2020, Andrei Warkentin <andrey.warkentin@gmail.com>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <ConfigVars.h>
#include "ConfigDxeFormSetGuid.h"
#include "ConfigDxe.h"

extern UINT8 ConfigDxeHiiBin[];
extern UINT8 ConfigDxeStrings[];

typedef struct {
  VENDOR_DEVICE_PATH VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL End;
} HII_VENDOR_DEVICE_PATH;

STATIC HII_VENDOR_DEVICE_PATH mVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    CONFIGDXE_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

STATIC EFI_STATUS
InstallHiiPages (
  VOID
  )
{
  EFI_STATUS     Status;
  EFI_HII_HANDLE HiiHandle;
  EFI_HANDLE     DriverHandle;

  DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (&DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mVendorDevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiHandle = HiiAddPackages (&gConfigDxeFormSetGuid,
                DriverHandle,
                ConfigDxeStrings,
                ConfigDxeHiiBin,
                NULL);

  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mVendorDevicePath,
           NULL);
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}


STATIC EFI_STATUS
SetupVariables (
  VOID
  )
{
  UINTN      Size;
  UINT32     Var32;
  EFI_STATUS Status;

  /*
   * Create the vars with default value.
   * If we don't, forms won't be able to update.
   */

  Size = sizeof (UINT32);
  Status = gRT->GetVariable (L"SystemTableMode",
                  &gConfigDxeFormSetGuid,
                  NULL, &Size, &Var32);
  if (EFI_ERROR (Status)) {
    Status = PcdSet32S (PcdSystemTableMode, PcdGet32 (PcdSystemTableMode));
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}


STATIC VOID
ApplyVariables (
  VOID
  )
{
  // Nothing to do yet
}


STATIC
VOID
EFIAPI
RemoveTables (
  EFI_EVENT           Event,
  VOID                *Context
  )
{
  switch (PcdGet32 (PcdSystemTableMode)) {
  case SYSTEM_TABLE_MODE_BOTH:
  case SYSTEM_TABLE_MODE_ACPI:
    break;
  default:
    DEBUG ((DEBUG_ERROR, "Removing ACPI tables (user config)\n"));
    gBS->InstallConfigurationTable (&gEfiAcpiTableGuid, NULL);
    gBS->InstallConfigurationTable (&gEfiAcpi10TableGuid, NULL);
    break;
  }  
}


EFI_STATUS
EFIAPI
ConfigInitialize (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_EVENT                       EndOfDxeEvent;

  Status = SetupVariables ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Couldn't not setup NV vars: %r\n", Status));
  }

  ApplyVariables ();

  Status = InstallHiiPages ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Couldn't install ConfigDxe configuration pages: %r\n", Status));
  }

  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, RemoveTables,
                               NULL, &gEfiEndOfDxeEventGroupGuid, &EndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, RemoveTables,
                               NULL, &gEfiAcpiTableGuid, &EndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);
  
  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, RemoveTables,
                               NULL, &gEfiAcpi10TableGuid, &EndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
