/** @file

  Copyright 2021-2023, Jared McNeill <jmcneill@invisible.ca>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCIHOSTBRIDGEINIT_H__
#define PCIHOSTBRIDGEINIT_H__

#define PCIE_SEGMENT_PCIE20             0
#define PCIE_SEGMENT_PCIE30X1           1
#define PCIE_SEGMENT_PCIE30X2           2

#define PCIE2X1_APB                     0xFE260000
#define PCIE3X1_APB                     0xFE270000
#define PCIE3X2_APB                     0xFE280000

#define PIPE_PHY0                       0xFE820000
#define PIPE_PHY1                       0xFE830000
#define PIPE_PHY2                       0xFE840000

#define PCIE30_PHY                      0xFE8C0000 /* 128KB */

#define PCIE2X1_S                       0x300000000UL /* 1024MB */
#define PCIE3X1_S                       0x340000000UL /* 1024MB */
#define PCIE3X2_S                       0x380000000UL /* 1024MB */

#define PCIE2X1_DBI                     0x3C0000000 /* 4MB */
#define PCIE3X1_DBI                     0x3C0400000 /* 4MB */
#define PCIE3X2_DBI                     0x3C0800000 /* 4MB */

#define PCIE2X1_SEGMENT                 ((PCIE2X1_S - 0x300000000UL) / 0x40000000UL)
#define PCIE3X1_SEGMENT                 ((PCIE3X1_S - 0x300000000UL) / 0x40000000UL)
#define PCIE3X2_SEGMENT                 ((PCIE3X2_S - 0x300000000UL) / 0x40000000UL)


EFI_STATUS
InitializePciHost (
  EFI_PHYSICAL_ADDRESS     ApbBase,
  EFI_PHYSICAL_ADDRESS     DbiBase,
  EFI_PHYSICAL_ADDRESS     PcieSegment,
  EFI_PHYSICAL_ADDRESS     BaseAddress,
  UINT32                   NumLanes,
  UINT32                   LinkSpeed,
  UINT32                   PowerGpioBank,
  UINT32                   PowerGpioPin,
  UINT32                   ResetGpioBank,
  UINT32                   ResetGpioPin
  );

#endif /* PCIHOSTBRIDGEINIT_H__ */