/** @file
  PCI Host Bridge Library instance for Rockchip Rk356x

  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

#include "PciHostBridgeInit.h"


#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()


STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath[3] = {
  {
    {
      {
        ACPI_DEVICE_PATH,
        ACPI_DP,
        {
          (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
          (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
        }
      },
      EISA_PNP_ID(0x0A08), // PCI Express
      0
    },

    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        END_DEVICE_PATH_LENGTH,
        0
      }
    }
  },
  {
    {
      {
        ACPI_DEVICE_PATH,
        ACPI_DP,
        {
          (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
          (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
        }
      },
      EISA_PNP_ID(0x0A08), // PCI Express
      1
    },

    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        END_DEVICE_PATH_LENGTH,
        0
      }
    }
  },
  {
    {
      {
        ACPI_DEVICE_PATH,
        ACPI_DP,
        {
          (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
          (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
        }
      },
      EISA_PNP_ID(0x0A08), // PCI Express
      2
    },

    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        END_DEVICE_PATH_LENGTH,
        0
      }
    }
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN *Count
  )
{
  PCI_ROOT_BRIDGE     *RootBridge;
  EFI_STATUS          Status;

  Status = InitializePciHost (
    PCIE2X1_APB,
    PCIE2X1_DBI,
    PCIE2X1_SEGMENT,
    PCIE2X1_S,
    1, 2,
    0xFFU, 0, /* always powered on */
    3, 17
  );
  if (EFI_ERROR (Status)) {
    *Count = 0;
    return NULL;
  }

  Status = InitializePciHost (
    PCIE3X1_APB,
    PCIE3X1_DBI,
    PCIE3X1_SEGMENT,
    PCIE3X1_S,
    1, 3,
    0xFFU, 0, /* always powered on */
    0, 0
  );
  if (EFI_ERROR (Status)) {
    *Count = 0;
    return NULL;
  }

  Status = InitializePciHost (
    PCIE3X2_APB,
    PCIE3X2_DBI,
    PCIE3X2_SEGMENT,
    PCIE3X2_S,
    2, 3,
    0xFFU, 0, /* always powered on */
    0, 14
  );
  if (EFI_ERROR (Status)) {
    *Count = 0;
    return NULL;
  }


  *Count = 3;
  RootBridge = AllocateZeroPool (*Count * sizeof(PCI_ROOT_BRIDGE));

  RootBridge[0].Segment     = PCIE2X1_SEGMENT;

  RootBridge[0].Supports    = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
                            EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
                            EFI_PCI_ATTRIBUTE_ISA_IO_16 |
                            EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO | \
                            EFI_PCI_ATTRIBUTE_VGA_MEMORY | \
                            EFI_PCI_ATTRIBUTE_VGA_IO_16  | \
                            EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
  RootBridge[0].Attributes  = RootBridge[0].Supports;

  RootBridge[0].DmaAbove4G            = TRUE;
  RootBridge[0].ResourceAssigned      = FALSE;
  RootBridge[0].NoExtendedConfigSpace = FALSE;

  RootBridge[0].AllocationAttributes  = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
                                      EFI_PCI_HOST_BRIDGE_MEM64_DECODE;

  RootBridge[0].Bus.Base              = 0;
  RootBridge[0].Bus.Limit             = 1;
  RootBridge[0].Io.Base               = 0x0000;
  RootBridge[0].Io.Limit              = 0x0000 + 0x10000 - 1;
  RootBridge[0].Io.Translation        = MAX_UINT64 - 0x00000003BFFF0000 + 1;
  RootBridge[0].Mem.Base              = 0xF0000000;
  RootBridge[0].Mem.Limit             = 0xF0000000 + 0x02000000 - 1;

  RootBridge[0].MemAbove4G.Base       = 0x0000000390000000;
  RootBridge[0].MemAbove4G.Limit      = 0x0000000390000000 + 0x000000002FFF0000 - 1;

  //
  // No separate ranges for prefetchable and non-prefetchable BARs
  //
  RootBridge[0].PMem.Base             = MAX_UINT64;
  RootBridge[0].PMem.Limit            = 0;
  RootBridge[0].PMemAbove4G.Base      = MAX_UINT64;
  RootBridge[0].PMemAbove4G.Limit     = 0;

  RootBridge[0].DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath[0];

  RootBridge[1].Segment     = PCIE3X1_SEGMENT;

  RootBridge[1].Supports    = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
                            EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
                            EFI_PCI_ATTRIBUTE_ISA_IO_16 |
                            EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO | \
                            EFI_PCI_ATTRIBUTE_VGA_MEMORY | \
                            EFI_PCI_ATTRIBUTE_VGA_IO_16  | \
                            EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
  RootBridge[1].Attributes  = RootBridge[1].Supports;

  RootBridge[1].DmaAbove4G            = TRUE;
  RootBridge[1].ResourceAssigned      = FALSE;
  RootBridge[1].NoExtendedConfigSpace = FALSE;

  RootBridge[1].AllocationAttributes  = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
                                      EFI_PCI_HOST_BRIDGE_MEM64_DECODE;

  RootBridge[1].Bus.Base              = 0;
  RootBridge[1].Bus.Limit             = 1;
  RootBridge[1].Io.Base               = 0x0000;
  RootBridge[1].Io.Limit              = 0x0000 + 0x10000 - 1;
  RootBridge[1].Io.Translation        = MAX_UINT64 - 0x00000003BFFF0000 + 1;
  RootBridge[1].Mem.Base              = 0xF0000000;
  RootBridge[1].Mem.Limit             = 0xF0000000 + 0x02000000 - 1;

  RootBridge[1].MemAbove4G.Base       = 0x0000000390000000;
  RootBridge[1].MemAbove4G.Limit      = 0x0000000390000000 + 0x000000002FFF0000 - 1;

  //
  // No separate ranges for prefetchable and non-prefetchable BARs
  //
  RootBridge[1].PMem.Base             = MAX_UINT64;
  RootBridge[1].PMem.Limit            = 0;
  RootBridge[1].PMemAbove4G.Base      = MAX_UINT64;
  RootBridge[1].PMemAbove4G.Limit     = 0;

  RootBridge[1].DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath[1];


  RootBridge[2].Segment     = PCIE3X2_SEGMENT;

  RootBridge[2].Supports    = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
                            EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
                            EFI_PCI_ATTRIBUTE_ISA_IO_16 |
                            EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO | \
                            EFI_PCI_ATTRIBUTE_VGA_MEMORY | \
                            EFI_PCI_ATTRIBUTE_VGA_IO_16  | \
                            EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
  RootBridge[2].Attributes  = RootBridge[2].Supports;

  RootBridge[2].DmaAbove4G            = TRUE;
  RootBridge[2].ResourceAssigned      = FALSE;
  RootBridge[2].NoExtendedConfigSpace = FALSE;

  RootBridge[2].AllocationAttributes  = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
                                      EFI_PCI_HOST_BRIDGE_MEM64_DECODE;

  RootBridge[2].Bus.Base              = 0;
  RootBridge[2].Bus.Limit             = 1;
  RootBridge[2].Io.Base               = 0x0000;
  RootBridge[2].Io.Limit              = 0x0000 + 0x10000 - 1;
  RootBridge[2].Io.Translation        = MAX_UINT64 - 0x00000003BFFF0000 + 1;
  RootBridge[2].Mem.Base              = 0xF0000000;
  RootBridge[2].Mem.Limit             = 0xF0000000 + 0x02000000 - 1;

  RootBridge[2].MemAbove4G.Base       = 0x0000000390000000;
  RootBridge[2].MemAbove4G.Limit      = 0x0000000390000000 + 0x000000002FFF0000 - 1;

  //
  // No separate ranges for prefetchable and non-prefetchable BARs
  //
  RootBridge[2].PMem.Base             = MAX_UINT64;
  RootBridge[2].PMem.Limit            = 0;
  RootBridge[2].PMemAbove4G.Base      = MAX_UINT64;
  RootBridge[2].PMemAbove4G.Limit     = 0;

  RootBridge[2].DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath[2];

  return RootBridge;
}

/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE *Bridges,
  UINTN           Count
  )
{
  FreePool (Bridges);
}

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE                        HostBridgeHandle,
  VOID                              *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  UINTN                             RootBridgeIndex;
  DEBUG ((EFI_D_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((EFI_D_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              (sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr) /
               sizeof (mPciHostBridgeLibAcpiAddressSpaceTypeStr[0])
               )
              );
      DEBUG ((EFI_D_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen, Descriptor->AddrRangeMax
              ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((EFI_D_ERROR, "     Granularity/SpecificFlag = %ld / %02x%s\n",
                Descriptor->AddrSpaceGranularity, Descriptor->SpecificFlag,
                ((Descriptor->SpecificFlag &
                  EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
                  ) != 0) ? L" (Prefetchable)" : L""
                ));
      }
    }
    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}
