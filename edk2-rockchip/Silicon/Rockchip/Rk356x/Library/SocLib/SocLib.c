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


// PMU_GRF registers
#define PMU_GRF_IO_VSEL0                      (PMU_GRF + 0x0140)
#define PMU_GRF_IO_VSEL1                      (PMU_GRF + 0x0144)
#define PMU_GRF_IO_VSEL2                      (PMU_GRF + 0x0148)

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

VOID
SocSetDomainVoltage (
  PMU_IO_DOMAIN IoDomain,
  PMU_IO_VOLTAGE IoVoltage
  )
{
  UINT32 Mask;

  switch (IoDomain) {
  case PMUIO2:
    Mask = BIT1 | BIT5;
    if (IoVoltage == VCC_3V3) {
      MmioWrite32 (PMU_GRF_IO_VSEL2, (Mask << 16) | BIT5);
    } else {
      MmioWrite32 (PMU_GRF_IO_VSEL2, (Mask << 16) | BIT1);
    }
    break;

  case VCCIO1...VCCIO7:
    Mask = 1U << IoDomain;
    if (IoVoltage == VCC_3V3) {
      MmioWrite32 (PMU_GRF_IO_VSEL0, Mask << 16);
      MmioWrite32 (PMU_GRF_IO_VSEL1, (Mask << 16) | Mask);
    } else {
      MmioWrite32 (PMU_GRF_IO_VSEL0, (Mask << 16) | Mask);
      MmioWrite32 (PMU_GRF_IO_VSEL1, Mask << 16);
    }
    break;
  }
}