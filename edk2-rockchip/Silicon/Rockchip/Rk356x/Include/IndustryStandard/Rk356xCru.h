/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef RK356XCRU_H__
#define RK356XCRU_H__

/* PLLs */
#define CRU_APLL            0
#define CRU_DPLL            1
#define CRU_GPLL            2
#define CRU_CPLL            3
#define CRU_NPLL            4
#define CRU_VPLL            5

/* PMU PLLs */
#define PMUCRU_PPLL         0
#define PMUCRU_HPLL         1

/* PLL registers */
#define CRU_PLL_CON0(n)     (CRU_BASE + (n) * 0x20 + 0x0)
#define CRU_PLL_CON1(n)     (CRU_BASE + (n) * 0x20 + 0x4)
#define CRU_PLL_CON2(n)     (CRU_BASE + (n) * 0x20 + 0x8)
#define CRU_PLL_CON3(n)     (CRU_BASE + (n) * 0x20 + 0xc)

/* PLL_CON0 fields */
#define CRU_PLL_CON0_BYPASS                      (1U << 15)
#define CRU_PLL_CON0_POSTDIV1_SHIFT              12
#define CRU_PLL_CON0_POSTDIV1_MASK               (0x7U << CRU_PLL_CON0_POSTDIV1_SHIFT)
#define CRU_PLL_CON0_FBDIV_MASK                  0xfffU

/* PLL_CON1 fields */
#define CRU_PLL_CON1_DSMPD                       (1U << 12)
#define CRU_PLL_CON1_POSTDIV2_SHIFT              6
#define CRU_PLL_CON1_POSTDIV2_MASK               (0x7 << CRU_PLL_CON1_POSTDIV2_SHIFT)
#define CRU_PLL_CON1_REFDIV_MASK                 0x3fU

/* PLL_CON2 fields */
#define CRU_PLL_CON2_FRACDIV_MASK                0xffffffU

/* CLKSEL registers */
#define CRU_CLKSEL_CON(n)   (CRU_BASE + (n) * 0x4 + 0x0100)

/* CLKSEL_CON00 fields */
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_SHIFT    6
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_MASK     (1U << CRU_CLKSEL_CON00_CLK_CORE_I_SEL_SHIFT)
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_APLL     0
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_GPLL     1
#define CRU_CLKSEL_CON00_CLK_CORE0_DIV_MASK      0x1fU

/* CLKSEL_CON30 fields */
#define CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_SHIFT    12
#define CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_MASK     (0x7U << CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_SHIFT)
#define CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_SHIFT    8
#define CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_MASK     (0x7U << CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_SHIFT)

/* GATE registers */
#define CRU_GATE_CON(n)     (CRU_BASE + (n) * 0x4 + 0x0300)

/* SOFTRST registers */
#define CRU_SOFTRST_CON(n)  (CRU_BASE + (n) * 0x4 + 0x0400)

/* PMU PLL registers */
#define PMUCRU_PLL_CON0(n)  (PMUCRU_BASE + (n) * 0x40 + 0x0)
#define PMUCRU_PLL_CON1(n)  (PMUCRU_BASE + (n) * 0x40 + 0x4)
#define PMUCRU_PLL_CON2(n)  (PMUCRU_BASE + (n) * 0x40 + 0x8)
#define PMUCRU_PLL_CON3(n)  (PMUCRU_BASE + (n) * 0x40 + 0xc)
#define PMUCRU_PLL_CON4(n)  (PMUCRU_BASE + (n) * 0x40 + 0xc)

/* PMUCLKSEL registers */
#define PMUCRU_PMUCLKSEL_CON(n)   (PMUCRU_BASE + (n) * 0x4 + 0x0100)

/* PMUCLKSEL_CON09 fields */
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_SHIFT(n)   ((n) * 4)
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_MASK(n)    (0x7U << PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_SHIFT(n))
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_SHIFT(n)   ((n) * 4 + 3)
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_MASK(n)    (0x1U << PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_SHIFT(n))

#endif /* RK356XCRU_H__ */
