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
#define CRU_PLL_CON0_BYPASS                      BIT15
#define CRU_PLL_CON0_POSTDIV1_SHIFT              12
#define CRU_PLL_CON0_POSTDIV1_MASK               (0x7U << CRU_PLL_CON0_POSTDIV1_SHIFT)
#define CRU_PLL_CON0_FBDIV_MASK                  0xfffU

/* PLL_CON1 fields */
#define CRU_PLL_CON1_DSMPD                       BIT12
#define CRU_PLL_CON1_LOCK_STATUS                 BIT10
#define CRU_PLL_CON1_POSTDIV2_SHIFT              6
#define CRU_PLL_CON1_POSTDIV2_MASK               (0x7 << CRU_PLL_CON1_POSTDIV2_SHIFT)
#define CRU_PLL_CON1_REFDIV_MASK                 0x3fU

/* PLL_CON2 fields */
#define CRU_PLL_CON2_FRACDIV_MASK                0xffffffU

/* MODE registers */
#define CRU_MODE_CON00      (CRU_BASE + 0x00C0)

/* CRU_MODE_CON00 fields */
#define CRU_MODE_CON00_CLK_PLL_MODE_SHIFT(n)     ((n) * 2)
#define CRU_MODE_CON00_CLK_PLL_MODE_MASK(n)      (0x3U << CRU_MODE_CON00_CLK_PLL_MODE_SHIFT(n))

/* CLKSEL registers */
#define CRU_CLKSEL_CON(n)   (CRU_BASE + (n) * 0x4 + 0x0100)

/* CLKSEL_CON00 fields */
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_SHIFT    6
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_MASK     (1U << CRU_CLKSEL_CON00_CLK_CORE_I_SEL_SHIFT)
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_APLL     0
#define CRU_CLKSEL_CON00_CLK_CORE_I_SEL_GPLL     1
#define CRU_CLKSEL_CON00_CLK_CORE0_DIV_MASK      0x1fU

/* CLKSEL_CON28 fields */
#define CRU_CLKSEL_CON28_CCLK_EMMC_SEL_SHIFT     12
#define CRU_CLKSEL_CON28_CCLK_EMMC_SEL_MASK      (0x7U << CRU_CLKSEL_CON28_CCLK_EMMC_SEL_SHIFT)
#define CRU_CLKSEL_CON28_BCLK_EMMC_SEL_SHIFT     8
#define CRU_CLKSEL_CON28_BCLK_EMMC_SEL_MASK      (0x3U << CRU_CLKSEL_CON28_BCLK_EMMC_SEL_SHIFT)

/* CLKSEL_CON30 fields */
#define CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_SHIFT    12
#define CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_MASK     (0x7U << CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_SHIFT)
#define CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_SHIFT    8
#define CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_MASK     (0x7U << CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_SHIFT)

/* CLKSEL_CON32 fields */
#define CRU_CLKSEL_CON32_CLK_SDMMC2_SEL_SHIFT    8
#define CRU_CLKSEL_CON32_CLK_SDMMC2_SEL_MASK     (0x7U << CRU_CLKSEL_CON32_CLK_SDMMC2_SEL_SHIFT)


/* GATE registers */
#define CRU_GATE_CON(n)     (CRU_BASE + (n) * 0x4 + 0x0300)

/* SOFTRST registers */
#define CRU_SOFTRST_CON(n)  (CRU_BASE + (n) * 0x4 + 0x0400)

/* PMU PLL registers */
#define PMUCRU_PLL_CON0(n)  (PMUCRU_BASE + (n) * 0x40 + 0x0)
#define PMUCRU_PLL_CON1(n)  (PMUCRU_BASE + (n) * 0x40 + 0x4)
#define PMUCRU_PLL_CON2(n)  (PMUCRU_BASE + (n) * 0x40 + 0x8)
#define PMUCRU_PLL_CON3(n)  (PMUCRU_BASE + (n) * 0x40 + 0xc)
#define PMUCRU_PLL_CON4(n)  (PMUCRU_BASE + (n) * 0x40 + 0x10)

/* PMU MODE registers */
#define PMUCRU_MODE_CON00   (PMUCRU_BASE + 0x0080)

/* PMUCRU_MODE_CON00 registers */
#define PMUCRU_MODE_CON00_CLK_PLL_MODE_SHIFT(n)       ((n) * 2)
#define PMUCRU_MODE_CON00_CLK_PLL_MODE_MASK(n)        (0x3U << PMUCRU_MODE_CON00_CLK_PLL_MODE_SHIFT(n))

/* PMUCLKSEL registers */
#define PMUCRU_PMUCLKSEL_CON(n)   (PMUCRU_BASE + (n) * 0x4 + 0x0100)

/* PMUCLKSEL_CON09 fields */
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_SHIFT(n)   ((n) * 4)
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_MASK(n)    (0x7U << PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_SHIFT(n))
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_SHIFT(n)   ((n) * 4 + 3)
#define PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_MASK(n)    (0x1U << PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_SHIFT(n))

/* PMUCRU_PMUGATE registers */
#define PMUCRU_PMUGATE_CON(n)       (PMUCRU_BASE + (n) * 4 + 0x0180)

#endif /* RK356XCRU_H__ */
