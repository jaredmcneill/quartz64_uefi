/** @file
 *
 *  Copyright (c) 2023, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) 2007-2009, Intel Corporation. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H

#include <Guid/EventGroup.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/LoadedImage.h>
#include <Library/SocLib.h>

typedef struct {
  union {
    UINTN                      FvBase;
    EFI_FIRMWARE_VOLUME_HEADER *VolumeHeader;
  };
  UINTN                      FvLength;
  UINTN                      Offset;
  UINTN                      NumOfBlocks;
  EFI_DEVICE_PATH_PROTOCOL   *Device;
  UINT32                     MediaId;
  BOOLEAN                    Dirty;
} EFI_FW_VOL_INSTANCE;

extern EFI_FW_VOL_INSTANCE *mFvInstance;

typedef struct {
  MEDIA_FW_VOL_DEVICE_PATH  FvDevPath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

EFI_STATUS
GetFvbInfo (
  IN  UINT64                            FvLength,
  OUT EFI_FIRMWARE_VOLUME_HEADER        **FvbInfo
  );

EFI_STATUS
FvbSetVolumeAttributes (
  IN OUT EFI_FVB_ATTRIBUTES_2 *Attributes
  );

EFI_STATUS
FvbGetVolumeAttributes (
  OUT EFI_FVB_ATTRIBUTES_2 *Attributes
  );

EFI_STATUS
FvbGetPhysicalAddress (
  OUT EFI_PHYSICAL_ADDRESS *Address
  );

EFI_STATUS
EFIAPI
FvbInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );


VOID
EFIAPI
FvbClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

EFI_STATUS
FvbGetLbaAddress (
  IN  EFI_LBA Lba,
  OUT UINTN   *LbaAddress,
  OUT UINTN   *LbaLength,
  OUT UINTN   *NumOfBlocks
  );

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_FVB_ATTRIBUTES_2                              *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                           *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_PHYSICAL_ADDRESS                        *Address
  );

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN CONST EFI_LBA                                     Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  );

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN CONST EFI_LBA                                      Lba,
  IN CONST UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN       EFI_LBA                                      Lba,
  IN       UINTN                                        Offset,
  IN OUT   UINTN                                        *NumBytes,
  IN       UINT8                                        *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  );

VOID
InstallProtocolInterfaces (
  IN EFI_FW_VOL_BLOCK_DEVICE *FvbDevice
  );

VOID
InstallVirtualAddressChangeHandler (
  VOID
  );

VOID
InstallDiskNotifyHandler (
  VOID
  );

VOID
InstallDumpVarEventHandlers (
  VOID
);

#endif
