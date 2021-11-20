/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef MULTIPHYLIB_H__
#define MULTIPHYLIB_H__

typedef enum {
  MULTIPHY_MODE_USB3,
  MULTIPHY_MODE_PCIE,
  MULTIPHY_MODE_SATA,
} MULTIPHY_MODE;

EFI_STATUS
MultiPhySetMode (
  IN UINT8 Index,
  IN MULTIPHY_MODE Mode
  );

#endif /* MULTIPHYLIB_H__ */
