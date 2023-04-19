/** @file
 *
 *  Copyright (c) 2023, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2020, Andrei Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) 2020, ARM Limited. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef CONFIG_VARS_H
#define CONFIG_VARS_H

typedef struct {
#define SYSTEM_TABLE_MODE_ACPI 0
#define SYSTEM_TABLE_MODE_BOTH 1
#define SYSTEM_TABLE_MODE_DT   2
  UINT32 Mode;
} SYSTEM_TABLE_MODE_VARSTORE_DATA;

typedef struct {
#define CPUCLOCK_LOW     0
#define CPUCLOCK_DEFAULT 1
#define CPUCLOCK_MAX     2
#define CPUCLOCK_CUSTOM  3
  UINT32 Clock;
} CPUCLOCK_VARSTORE_DATA;

typedef struct {
  UINT32 Clock;
} CUSTOM_CPUCLOCK_VARSTORE_DATA;

typedef struct {
#define MULTIPHY_MODE_SEL_USB3 0
#define MULTIPHY_MODE_SEL_PCIE 1
#define MULTIPHY_MODE_SEL_SATA 2
  UINT32 Mode;
} MULTIPHY_MODE_VARSTORE_DATA;

typedef struct {
  BOOLEAN Mode;
} FAN_VARSTORE_DATA;

#endif /* CONFIG_VARS_H */