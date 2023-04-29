/** @file
*
*  Copyright (c) 2022, Patrick Wildt <patrick@blueri.se>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/


#ifndef __EMMC_H__
#define __EMMC_H__

#include <Protocol/EmbeddedGpio.h>

#define SD_MMC_HC_CLOCK_CTRL    0x2C
#define SD_MMC_HC_HOST_CTRL2    0x3E

// eMMC Registers
#define SDMMC_BACKEND_POWER     ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x104)
#define DWCMSHC_HOST_CTRL3      ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x508)
#define EMMC_EMMC_CTRL          ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x52C)
#define EMMC_DLL_CTRL           ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x800)
#define EMMC_DLL_RXCLK          ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x804)
#define EMMC_DLL_TXCLK          ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x808)
#define EMMC_DLL_STRBIN         ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x80C)
#define EMMC_DLL_STATUS0        ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x840)
#define EMMC_DLL_STATUS1        ((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) + 0x844)

#define EMMC_DLL_CTRL_SRST                 BIT0
#define EMMC_DLL_CTRL_START                BIT1
#define EMMC_DLL_CTRL_START_POINT_DEFAULT  (5 << 16)
#define EMMC_DLL_CTRL_INCREMENT_DEFAULT    (2 << 8)

#define EMMC_DLL_RXCLK_DLYENA              BIT27
#define EMMC_DLL_RXCLK_NO_INVERTER         BIT29

#define EMMC_DLL_TXCLK_DLYENA              BIT27
#define EMMC_DLL_TXCLK_TAPNUM_DEFAULT      (0x10 << 0)
#define EMMC_DLL_TXCLK_TAPNUM_FROM_SW      BIT24

#define EMMC_DLL_STRBIN_DLYENA             BIT27
#define EMMC_DLL_STRBIN_TAPNUM_DEFAULT     (0x3 << 0)

#define EMMC_DLL_STATUS0_DLL_LOCK          BIT8
#define EMMC_DLL_STATUS0_DLL_TIMEOUT       BIT9

#endif  // __EMMC_H__
