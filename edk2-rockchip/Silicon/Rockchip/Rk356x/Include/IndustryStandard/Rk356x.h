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
#define PIPE_PHY_GRF(n)     (0xFDC70000UL + (n) * 0x10000)
#define PMUCRU_BASE         0xFDD00000UL
#define CRU_BASE            0xFDD20000UL
#define UART_BASE(n)        ((n) == 0 ? 0xFDD50000UL : (0xFE650000UL + ((n) - 1) * 0x10000))
#define GPIO_BASE(n)        ((n) == 0 ? 0xFDD60000UL : (0xFE740000UL + ((n) - 1) * 0x10000))
#define PIPE_PHY(n)         (0xFE820000UL + (n) * 0x10000)

#endif /* RK356X_H__ */
