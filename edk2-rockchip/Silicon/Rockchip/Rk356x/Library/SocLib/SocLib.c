/** @file
 *
 *  Misc SoC helpers for RK356x
 *
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/SocLib.h>

#include <IndustryStandard/Rk356x.h>

// The RK356x boot ROM stores the boot device in SRAM at this offset.
#define BOOT_DEVICE_REG                 (SYSTEM_SRAM + 0x10)

SOC_BOOT_DEVICE
SocGetBootDevice (
  VOID
  )
{
  UINT32 Value;
  
  Value = MmioRead32 (BOOT_DEVICE_REG);

  switch (Value) {
  case SOC_BOOT_DEVICE_NAND:
  case SOC_BOOT_DEVICE_EMMC:
  case SOC_BOOT_DEVICE_SPINOR:
  case SOC_BOOT_DEVICE_SPINAND:
  case SOC_BOOT_DEVICE_SD:
  case SOC_BOOT_DEVICE_USB:
    return Value;

  default:
    ASSERT (FALSE);
    return SOC_BOOT_DEVICE_UNKNOWN;
  }
}