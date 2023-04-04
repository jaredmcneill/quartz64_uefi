/** @file

  Copyright 2021-2023, Jared McNeill <jmcneill@invisible.ca>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCIHOSTBRIDGEINIT_H__
#define PCIHOSTBRIDGEINIT_H__

#define PCIE_BASE                       FixedPcdGet64 (PcdPciExpressBaseAddress)
#define PCIE_SEGMENT                    ((PCIE_BASE - 0x300000000UL) / 0x40000000UL)
#define PCIE_SEGMENT_PCIE20             0
#define PCIE_SEGMENT_PCIE30X1           1
#define PCIE_SEGMENT_PCIE30X2           2

EFI_STATUS
InitializePciHost (
  VOID
  );

#endif /* PCIHOSTBRIDGEINIT_H__ */