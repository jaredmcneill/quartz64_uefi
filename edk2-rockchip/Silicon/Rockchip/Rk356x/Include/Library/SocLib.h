/** @file
 *
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef SDRAMLIB_H__
#define SDRAMLIB_H__

typedef enum {
  SOC_BOOT_DEVICE_UNKNOWN   = 0,
  SOC_BOOT_DEVICE_NAND      = 1,
  SOC_BOOT_DEVICE_EMMC      = 2,
  SOC_BOOT_DEVICE_SPINOR    = 3,
  SOC_BOOT_DEVICE_SPINAND   = 4,
  SOC_BOOT_DEVICE_SD        = 5,
  SOC_BOOT_DEVICE_USB       = 10
} SOC_BOOT_DEVICE;

SOC_BOOT_DEVICE
SocGetBootDevice (
    VOID
    );

#endif /* SDRAMLIB_H__ */