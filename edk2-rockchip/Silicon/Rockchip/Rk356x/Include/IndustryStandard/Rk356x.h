/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef RK356X_H__
#define RK356X_H__

/* Reference clock rate */
#define CRU_CLKREF_RATE     24000000UL

/* Register base addresses */
#define PMU_GRF             0xFDC20000UL
#define CPU_GRF             0xFDC30000UL
#define PIPE_GRF            0xFDC50000UL
#define SYS_GRF             0xFDC60000UL
#define PIPE_PHY_GRF(n)     (0xFDC70000UL + (n) * 0x10000)
#define SYSTEM_SRAM         0xFDCC0000UL
#define PMUCRU_BASE         0xFDD00000UL
#define CRU_BASE            0xFDD20000UL
#define I2C0_BASE           0xFDD40000UL
#define UART_BASE(n)        ((n) == 0 ? 0xFDD50000UL : (0xFE650000UL + ((n) - 1) * 0x10000))
#define GPIO_BASE(n)        ((n) == 0 ? 0xFDD60000UL : (0xFE740000UL + ((n) - 1) * 0x10000))
#define PMU_BASE            0xFDD90000UL
#define GMAC1_BASE          0xFE010000UL
#define VOP_BASE            0xFE040000UL
#define HDMI_BASE           0xFE0A0000UL
#define PCIE2X1_APB_BASE    0xFE260000UL
#define GMAC0_BASE          0xFE2A0000UL
#define TRNG_BASE           0xFE388000UL
#define OTP_BASE            0xFE38C000UL
#define TSADC_BASE          0xFE710000UL
#define PIPE_PHY(n)         (0xFE820000UL + (n) * 0x10000)
#define PCIE2X1_S_BASE      0x300000000UL
#define PCIE2X1_DBI_BASE    0x3C0000000UL

#endif /* RK356X_H__ */
