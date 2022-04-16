/** @file
 *
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef SOCLIB_H__
#define SOCLIB_H__

typedef enum {
  SOC_BOOT_DEVICE_UNKNOWN   = 0,
  SOC_BOOT_DEVICE_NAND      = 1,
  SOC_BOOT_DEVICE_EMMC      = 2,
  SOC_BOOT_DEVICE_SPINOR    = 3,
  SOC_BOOT_DEVICE_SPINAND   = 4,
  SOC_BOOT_DEVICE_SD        = 5,
  SOC_BOOT_DEVICE_USB       = 10
} SOC_BOOT_DEVICE;

typedef enum {
  PMUIO2                    = 0,
  VCCIO1                    = 1,
  VCCIO2                    = 2,
  VCCIO3                    = 3,
  VCCIO4                    = 4,
  VCCIO5                    = 5,
  VCCIO6                    = 6,
  VCCIO7                    = 7,
} PMU_IO_DOMAIN;

typedef enum {
  VCC_1V8,
  VCC_3V3
} PMU_IO_VOLTAGE;

SOC_BOOT_DEVICE
SocGetBootDevice (
    VOID
    );

VOID
SocSetDomainVoltage (
    PMU_IO_DOMAIN IoDomain,
    PMU_IO_VOLTAGE IoVoltage
    );

#endif /* SOCLIB_H__ */