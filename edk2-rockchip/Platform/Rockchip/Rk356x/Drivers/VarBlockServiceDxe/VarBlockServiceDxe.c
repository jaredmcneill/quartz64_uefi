/** @file
 *
 *  Copyright (c) 2023, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (C) 2015, Red Hat, Inc.
 *  Copyright (c) 2006-2014, Intel Corporation. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include "VarBlockService.h"

// Non-discoverable device path, must match the format used in
// NonDiscoverableDeviceRegistrationLib.c
#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH          Vendor;
  UINT64                      BaseAddress;
  UINT8                       ResourceType;
  EFI_DEVICE_PATH_PROTOCOL    End;
} NON_DISCOVERABLE_DEVICE_PATH;
#pragma pack ()

//
// Minimum delay to enact before reset, when variables are dirty (in μs).
// Needed to ensure that devices have time to flush their write cache
// after updating the NV vars.
//
#define PLATFORM_RESET_DELAY    3500000

VOID *mDiskIoRegistration;


VOID
InstallProtocolInterfaces (
  IN EFI_FW_VOL_BLOCK_DEVICE *FvbDevice
  )
{
  EFI_STATUS Status;
  EFI_HANDLE FwbHandle;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *OldFwbInterface;

  //
  // Find a handle with a matching device path that has supports FW Block
  // protocol.
  //
  Status = gBS->LocateDevicePath (&gEfiFirmwareVolumeBlockProtocolGuid,
                  &FvbDevice->DevicePath, &FwbHandle);
  if (EFI_ERROR (Status)) {
    //
    // LocateDevicePath fails so install a new interface and device path.
    //
    FwbHandle = NULL;
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &FwbHandle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    &FvbDevice->FwVolBlockInstance,
                    &gEfiDevicePathProtocolGuid,
                    FvbDevice->DevicePath,
                    NULL
                  );
    ASSERT_EFI_ERROR (Status);
  } else if (IsDevicePathEnd (FvbDevice->DevicePath)) {
    //
    // Device already exists, so reinstall the FVB protocol
    //
    Status = gBS->HandleProtocol (
                    FwbHandle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID**)&OldFwbInterface
                  );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->ReinstallProtocolInterface (
                    FwbHandle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    OldFwbInterface,
                    &FvbDevice->FwVolBlockInstance
                  );
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // There was a FVB protocol on an End Device Path node
    //
    ASSERT (FALSE);
  }
}


STATIC
VOID
EFIAPI
FvbVirtualAddressChangeEvent (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
/*++

  Routine Description:

    Fixup internal data so that EFI can be called in virtual mode.

  Arguments:

    (Standard EFI notify event - EFI_EVENT_NOTIFY)

  Returns:

    None

--*/
{
  EfiConvertPointer (0x0, (VOID**)&mFvInstance->FvBase);
  EfiConvertPointer (0x0, (VOID**)&mFvInstance->VolumeHeader);
  EfiConvertPointer (0x0, (VOID**)&mFvInstance->Device);
  EfiConvertPointer (0x0, (VOID**)&mFvInstance);
}


VOID
InstallVirtualAddressChangeHandler (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_EVENT VirtualAddressChangeEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FvbVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &VirtualAddressChangeEvent
                );
  ASSERT_EFI_ERROR (Status);
}


STATIC
EFI_STATUS
DoDump (
  IN EFI_DEVICE_PATH_PROTOCOL *Device,
  IN UINT32 MediaId
  )
{
  EFI_STATUS Status;
  EFI_DISK_IO_PROTOCOL *DiskIo = NULL;
  EFI_HANDLE Handle;

  Status = gBS->LocateDevicePath (
                  &gEfiDiskIoProtocolGuid,
                  &Device,
                  &Handle
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID**)&DiskIo
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DiskIo->WriteDisk (
                      DiskIo,
                      MediaId,
                      mFvInstance->Offset,
                      mFvInstance->FvLength,
                      (VOID*)mFvInstance->FvBase
                    );

  return Status;
}


STATIC
VOID
EFIAPI
DumpVars (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  EFI_STATUS Status;
  RETURN_STATUS PcdStatus;

  if (mFvInstance->Device == NULL) {
    DEBUG ((DEBUG_INFO, "Variable store not found?\n"));
    return;
  }

  if (!mFvInstance->Dirty) {
    DEBUG ((DEBUG_INFO, "Variables not dirty, not dumping!\n"));
    return;
  }

  Status = DoDump (mFvInstance->Device, mFvInstance->MediaId);
  if (EFI_ERROR (Status)) {
    CHAR16* DevicePathText = ConvertDevicePathToText (mFvInstance->Device, FALSE, FALSE);
    DEBUG ((DEBUG_ERROR, "Couldn't dump '%s'\n", DevicePathText));
    if (DevicePathText != NULL) {
      gBS->FreePool (DevicePathText);
    }
    ASSERT_EFI_ERROR (Status);
    return;
  }

  DEBUG ((DEBUG_INFO, "Variables dumped!\n"));

  //
  // Add a reset delay to give time for slow/cached devices
  // to flush the NV variables write to permanent storage.
  // But only do so if this won't reduce an existing user-set delay.
  //
  if (PcdGet32 (PcdPlatformResetDelay) < PLATFORM_RESET_DELAY) {
    PcdStatus = PcdSet32S (PcdPlatformResetDelay, PLATFORM_RESET_DELAY);
    ASSERT_RETURN_ERROR (PcdStatus);
  }

  mFvInstance->Dirty = FALSE;
}


VOID
ReadyToBootHandler (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  EFI_STATUS Status;
  EFI_EVENT ImageInstallEvent;
  VOID *ImageRegistration;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DumpVars,
                  NULL,
                  &ImageInstallEvent
                );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->RegisterProtocolNotify (
                  &gEfiLoadedImageProtocolGuid,
                  ImageInstallEvent,
                  &ImageRegistration
                );
  ASSERT_EFI_ERROR (Status);

  DumpVars (NULL, NULL);
  Status = gBS->CloseEvent (Event);
  ASSERT_EFI_ERROR (Status);
}


VOID
InstallDumpVarEventHandlers (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_EVENT ResetEvent;
  EFI_EVENT ReadyToBootEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DumpVars,
                  NULL,
                  &gRk356xEventResetGuid,
                  &ResetEvent
                );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ReadyToBootHandler,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                );
  ASSERT_EFI_ERROR (Status);
}

STATIC
BOOLEAN
IsBootDevice (
  IN EFI_HANDLE Handle
  )
{
  SOC_BOOT_DEVICE BootDevice;
  EFI_DEV_PATH *Device;
  NON_DISCOVERABLE_DEVICE_PATH *DevicePath;
  CHAR16 *DevicePathText;
  
  BootDevice = SocGetBootDevice ();
  Device = (EFI_DEV_PATH *) DevicePathFromHandle (Handle);
  DevicePathText = ConvertDevicePathToText (&Device->DevPath, FALSE, FALSE);

  DEBUG ((DEBUG_INFO, "IsBootDevice: DevPath %s type 0x%x subtype 0x%x\n", DevicePathText, Device->DevPath.Type, Device->DevPath.SubType));
  gBS->FreePool (DevicePathText);

  if (Device->DevPath.Type == HARDWARE_DEVICE_PATH && Device->DevPath.SubType == HW_MEMMAP_DP) {
    DEBUG ((DEBUG_INFO, "IsBootDevice: HW_MEMMAP_DP device @ 0x%lX\n", Device->MemMap.StartingAddress));
    if (BootDevice == SOC_BOOT_DEVICE_SD) {
      return Device->MemMap.StartingAddress == PcdGet32 (PcdMshcDxeBaseAddress);
    }
  }

  if (Device->DevPath.Type == HARDWARE_DEVICE_PATH && Device->DevPath.SubType == HW_VENDOR_DP &&
      CompareGuid (&Device->Vendor.Guid, &gEdkiiNonDiscoverableDeviceProtocolGuid)) {
    DEBUG ((DEBUG_INFO, "IsBootDevice: HW_VENDOR_DP (non-discoverable) device @ 0x%lX\n", ((NON_DISCOVERABLE_DEVICE_PATH *)Device)->BaseAddress));
    if (BootDevice == SOC_BOOT_DEVICE_EMMC) {
      DevicePath = (NON_DISCOVERABLE_DEVICE_PATH *) Device;
      return DevicePath->BaseAddress == PcdGet32 (PcdEmmcDxeBaseAddress);
    }
  }

  return FALSE;
}

VOID
EFIAPI
OnDiskIoInstall (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  EFI_STATUS Status;
  UINTN HandleSize;
  EFI_HANDLE Handle;
  EFI_DEVICE_PATH_PROTOCOL *Device;
  EFI_BLOCK_IO_PROTOCOL *BlkIo;
  CHAR16 *DevicePathText = NULL;

  if (mFvInstance->Device != NULL) {
    //
    // We've already found the variable store before,
    // and that device is not removed from the system.
    //
    return;
  }

  while (TRUE) {
    HandleSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mDiskIoRegistration,
                    &HandleSize,
                    &Handle
                  );
    if (Status == EFI_NOT_FOUND) {
      break;
    }

    ASSERT_EFI_ERROR (Status);

    if (!IsBootDevice (Handle)) {
      continue;
    }

    Device = NULL;
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiBlockIoProtocolGuid,
                    (VOID*)&BlkIo
                  );
    if (EFI_ERROR (Status)) {
      continue;
    }
    Device = DuplicateDevicePath (DevicePathFromHandle (Handle));
    ASSERT (Device != NULL);

    if (DevicePathText != NULL) {
      gBS->FreePool (DevicePathText);
    }
    DevicePathText = ConvertDevicePathToText (Device, FALSE, FALSE);

    if (!BlkIo->Media->MediaPresent) {
      DEBUG ((DEBUG_ERROR, "VarBlockService: [%s] Media not present!\n", DevicePathText));
      continue;
    }
    if (BlkIo->Media->ReadOnly) {
      DEBUG ((DEBUG_ERROR, "VarBlockService: [%s] Media is read-only!\n", DevicePathText));
      continue;
    }

    Status = DoDump (Device, BlkIo->Media->MediaId);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "VarBlockService: [%s] Couldn't update %a\n", DevicePathText));
      ASSERT_EFI_ERROR (Status);
      continue;
    }

    if (mFvInstance->Device != NULL) {
      gBS->FreePool (mFvInstance->Device);
    }

    DEBUG ((DEBUG_INFO, "VarBlockService: [%s] Found variable store!\n", DevicePathText));
    mFvInstance->Device = Device;
    mFvInstance->MediaId = BlkIo->Media->MediaId;
    break;
  }

  if (DevicePathText != NULL) {
    gBS->FreePool (DevicePathText);
  }
}


VOID
InstallDiskNotifyHandler (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_EVENT Event;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnDiskIoInstall,
                  NULL,
                  &Event
                );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->RegisterProtocolNotify (
                  &gEfiDiskIoProtocolGuid,
                  Event,
                  &mDiskIoRegistration
                );
  ASSERT_EFI_ERROR (Status);
}
