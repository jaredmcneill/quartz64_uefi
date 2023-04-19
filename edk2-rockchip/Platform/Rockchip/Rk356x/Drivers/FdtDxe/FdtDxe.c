/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2017,2021 Andrey Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) 2016, Linaro, Ltd. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/Rk356x.h>
#include <libfdt.h>
#include <Guid/Fdt.h>
#include <ConfigVars.h>

STATIC VOID *mFdtImage;

STATIC
EFI_STATUS
CleanMemoryNodes (
  VOID
  )
{
  INTN Node;
  INT32 Retval;

  Node = fdt_path_offset (mFdtImage, "/memory");
  if (Node < 0) {
    return EFI_SUCCESS;
  }

  /*
   * Remove bogus memory nodes which can make the booted
   * OS go crazy and ignore the UEFI map.
   */
  DEBUG ((DEBUG_INFO, "Removing bogus /memory\n"));
  Retval = fdt_del_node (mFdtImage, Node);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "Failed to remove /memory\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FixUartSpeed (
  VOID
  )
{
  INTN Node;
  const char *StdOutPath;
  char Buf[80];
  UINT8 UartIndex;

  Node = fdt_path_offset (mFdtImage, "/chosen");
  if (Node < 0) {
    return EFI_SUCCESS;
  }

  StdOutPath = fdt_getprop (mFdtImage, Node, "stdout-path", NULL);
  if (StdOutPath == NULL) {
    return EFI_SUCCESS;
  }

  if (FixedPcdGet64 (PcdSerialRegisterBase) == UART_BASE (0)) {
    UartIndex = 0;
  } else {
    UartIndex = 1 + (FixedPcdGet64 (PcdSerialRegisterBase) - UART_BASE (1)) / 0x10000;
  }
  AsciiSPrint (Buf, sizeof (Buf), "serial%u:%lun8", UartIndex, FixedPcdGet64 (PcdUartDefaultBaudRate));
  fdt_setprop_string (mFdtImage, Node, "stdout-path", Buf);

  return EFI_SUCCESS;
}

#ifdef QUARTZ64
STATIC
EFI_STATUS
FixMultiPhy1Mode (
  VOID
  )
{
  INTN Node;

  Node = fdt_path_offset (mFdtImage, "/sata@fc400000");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "Node /sata@fc400000 not found"));
    return EFI_SUCCESS;
  }
  fdt_setprop_string (mFdtImage, Node, "status",
                      PcdGet32 (PcdMultiPhy1Mode) == MULTIPHY_MODE_SEL_SATA ? "okay" : "disabled");

  Node = fdt_path_offset (mFdtImage, "/usb@fd000000");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "Node /usb@fd000000 not found"));
    return EFI_SUCCESS;
  }
  fdt_setprop_string (mFdtImage, Node, "status",
                      PcdGet32 (PcdMultiPhy1Mode) == MULTIPHY_MODE_SEL_USB3 ? "okay" : "disabled");

  return EFI_SUCCESS;
}
#endif

/**
  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
FdtDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  INT32      Retval;
  EFI_STATUS Status;
  UINTN      FdtSize;
  VOID       *FdtImage = NULL;

  switch (PcdGet32 (PcdSystemTableMode)) {
  case SYSTEM_TABLE_MODE_BOTH:
  case SYSTEM_TABLE_MODE_DT:
    break;
  default:
    DEBUG ((DEBUG_ERROR, "Not installing FDT (user config)\n"));
    return EFI_SUCCESS;
  }

  FdtImage = (VOID*)(UINTN)PcdGet64 (PcdFdtBaseAddress);
  Retval = fdt_check_header (FdtImage);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "No devicetree passed from loader.\n"));
    return EFI_NOT_FOUND;
  }

  FdtSize = fdt_totalsize (FdtImage);

  /*
   * Probably overkill.
   */
  FdtSize += EFI_PAGE_SIZE * 2;
  Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (FdtSize), (EFI_PHYSICAL_ADDRESS*)&mFdtImage);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate devicetree: %r\n", Status));
    goto out;
  }

  Retval = fdt_open_into (FdtImage, mFdtImage, FdtSize);
  if (Retval != 0) {
     DEBUG ((DEBUG_ERROR, "fdt_open_into failed: %d\n", Retval));
     goto out;
  }

  /*
   * These are all best-effort.
   */

  Status = CleanMemoryNodes ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to clean memory nodes: %r\n", Status);
  }

  Status = FixUartSpeed ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to fix UART speed: %r\n", Status);
  }

#ifdef QUARTZ64
  Status = FixMultiPhy1Mode ();
  if (EFI_ERROR (Status)) {
    Print (L"Failed to fix MULTI-PHY1 mode: %r\n", Status);
  }
#endif

  DEBUG ((DEBUG_INFO, "Installed devicetree at address %p\n", mFdtImage));
  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, mFdtImage);
  if (EFI_ERROR (Status)) {
     DEBUG ((DEBUG_ERROR, "Couldn't register devicetree: %r\n", Status));
     goto out;
  }

out:
  if (EFI_ERROR(Status)) {
    if (mFdtImage != NULL) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS) mFdtImage, EFI_SIZE_TO_PAGES (FdtSize));
    }
  }
  return Status;
}
