/** @file
 *
 *  RK3566/RK3568 Clock and Reset Unit Library.
 * 
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/CruLib.h>
#include <IndustryStandard/Rk356x.h>
#include <IndustryStandard/Rk356xCru.h>

typedef struct {
    UINTN Rate;
    UINT32 RefDiv;
    UINT32 FbDiv;
    UINT32 PostDiv1;
    UINT32 PostDiv2;
    UINT32 Dsmpd;
    UINT32 Frac;
} CRU_PLL_RATE;

STATIC CRU_PLL_RATE CruPllRates[] = {
    { .Rate = 594000000, .RefDiv = 1, .FbDiv = 99, .PostDiv1 = 4, .PostDiv2 = 1, .Dsmpd = 1, .Frac = 0 },
    { .Rate = 297000000, .RefDiv = 1, .FbDiv = 99, .PostDiv1 = 4, .PostDiv2 = 2, .Dsmpd = 1, .Frac = 0 },
    { .Rate = 200000000, .RefDiv = 1, .FbDiv = 100, .PostDiv1 = 3, .PostDiv2 = 4, .Dsmpd = 1, .Frac = 0 },
    { .Rate = 148500000, .RefDiv = 1, .FbDiv = 99, .PostDiv1 = 4, .PostDiv2 = 4, .Dsmpd = 1, .Frac = 0 },
    { .Rate = 74250000,  .RefDiv = 2, .FbDiv = 99, .PostDiv1 = 4, .PostDiv2 = 4, .Dsmpd = 1, .Frac = 0 },
    { .Rate = 0 }
};

STATIC UINTN
CruGetPllRate (
  IN UINT32 PllNumber
  )
{
    UINT32 Con0, Con1, Con2;
    UINTN FOutVco, PostDiv1, PostDiv2;
    UINTN RefDiv, FbDiv, FracDiv;

    Con0 = MmioRead32 (CRU_PLL_CON0 (PllNumber));
    if ((Con0 & CRU_PLL_CON0_BYPASS) != 0) {
        return CRU_CLKREF_RATE;
    }
    FbDiv = Con0 & CRU_PLL_CON0_FBDIV_MASK;
    PostDiv1 = (Con0 & CRU_PLL_CON0_POSTDIV1_MASK) >> CRU_PLL_CON0_POSTDIV1_SHIFT;

    Con1 = MmioRead32 (CRU_PLL_CON1 (PllNumber));
    RefDiv = Con1 & CRU_PLL_CON1_REFDIV_MASK;
    PostDiv2 = (Con1 & CRU_PLL_CON1_POSTDIV2_MASK) >> CRU_PLL_CON1_POSTDIV2_SHIFT;

    FOutVco = CRU_CLKREF_RATE / RefDiv * FbDiv;

    if ((Con1 & CRU_PLL_CON1_DSMPD) == 0) {
        /* Fractional mode */
        Con2 = MmioRead32 (CRU_PLL_CON2 (PllNumber));
        FracDiv = Con2 & CRU_PLL_CON2_FRACDIV_MASK;
        FOutVco += ((CRU_CLKREF_RATE * FracDiv) >> 24);
    }

    DEBUG ((DEBUG_INFO, "CruGetPllRate(): PllNumber = %u, Rate = %lu\n", PllNumber, FOutVco / PostDiv1 / PostDiv2));

    return FOutVco / PostDiv1 / PostDiv2;
}


STATIC UINTN
PmuCruGetPllRate (
  IN UINT32 PllNumber
  )
{
    UINT32 Con0, Con1, Con2;
    UINTN FOutVco, PostDiv1, PostDiv2;
    UINTN RefDiv, FbDiv, FracDiv;

    Con0 = MmioRead32 (PMUCRU_PLL_CON0 (PllNumber));
    if ((Con0 & CRU_PLL_CON0_BYPASS) != 0) {
        return CRU_CLKREF_RATE;
    }
    FbDiv = Con0 & CRU_PLL_CON0_FBDIV_MASK;
    PostDiv1 = (Con0 & CRU_PLL_CON0_POSTDIV1_MASK) >> CRU_PLL_CON0_POSTDIV1_SHIFT;

    Con1 = MmioRead32 (PMUCRU_PLL_CON1 (PllNumber));
    RefDiv = Con1 & CRU_PLL_CON1_REFDIV_MASK;
    PostDiv2 = (Con1 & CRU_PLL_CON1_POSTDIV2_MASK) >> CRU_PLL_CON1_POSTDIV2_SHIFT;

    FOutVco = CRU_CLKREF_RATE / RefDiv * FbDiv;

    if ((Con1 & CRU_PLL_CON1_DSMPD) == 0) {
        /* Fractional mode */
        Con2 = MmioRead32 (PMUCRU_PLL_CON2 (PllNumber));
        FracDiv = Con2 & CRU_PLL_CON2_FRACDIV_MASK;
        FOutVco += ((CRU_CLKREF_RATE * FracDiv) >> 24);
    }

    DEBUG ((DEBUG_INFO, "PmuCruGetPllRate(): PllNumber = %u, Rate = %lu\n", PllNumber, FOutVco / PostDiv1 / PostDiv2));

    return FOutVco / PostDiv1 / PostDiv2;
}

STATIC VOID
PmuCruSetPllRate (
  IN UINT32 PllNumber,
  IN CRU_PLL_RATE *Rate
  )
{
    UINT32 PllLockRetry = 0;

    MmioWrite32 (PMUCRU_MODE_CON00,
                 (PMUCRU_MODE_CON00_CLK_PLL_MODE_MASK (PllNumber) << 16) |
                 (0U << PMUCRU_MODE_CON00_CLK_PLL_MODE_SHIFT (PllNumber)));

    MmioWrite32 (PMUCRU_PLL_CON0 (PllNumber),
                 ((CRU_PLL_CON0_FBDIV_MASK | CRU_PLL_CON0_POSTDIV1_MASK | CRU_PLL_CON0_BYPASS) << 16) |
                 (Rate->PostDiv1 << CRU_PLL_CON0_POSTDIV1_SHIFT) |
                 Rate->FbDiv);

    MmioWrite32 (PMUCRU_PLL_CON1 (PllNumber),
                 ((CRU_PLL_CON1_REFDIV_MASK | CRU_PLL_CON1_POSTDIV2_MASK | CRU_PLL_CON1_DSMPD) << 16) |
                 (Rate->Dsmpd ? CRU_PLL_CON1_DSMPD : 0) |
                 (Rate->PostDiv2 << CRU_PLL_CON1_POSTDIV2_SHIFT) |
                 Rate->RefDiv);

    MmioAndThenOr32 (PMUCRU_PLL_CON2 (PllNumber), ~CRU_PLL_CON2_FRACDIV_MASK, Rate->Frac);

    do {
        UINT32 Val = MmioRead32 (PMUCRU_PLL_CON1 (PllNumber));
        if ((Val & CRU_PLL_CON1_LOCK_STATUS) != 0) {
            break;
        }
        MicroSecondDelay (1000);
    } while (++PllLockRetry < 100);
    ASSERT (PllLockRetry < 100);

    MmioWrite32 (PMUCRU_MODE_CON00,
                 (PMUCRU_MODE_CON00_CLK_PLL_MODE_MASK (PllNumber) << 16) |
                 (1U << PMUCRU_MODE_CON00_CLK_PLL_MODE_SHIFT (PllNumber)));
}

STATIC
CRU_PLL_RATE *
CruFindPllRate (
  IN UINTN Rate
  )
{
    UINT32 Index;
    CRU_PLL_RATE *PllRate = NULL;

    for (Index = 0; CruPllRates[Index].Rate != 0; Index++) {
        if (CruPllRates[Index].Rate == Rate) {
            PllRate = &CruPllRates[Index];
            break;
        }
    }

    return PllRate;
}

UINTN
CruGetCoreClockRate (
  VOID
  )
{
    UINT32 Val;
    UINT32 Sel;
    UINTN PllRate, Div;

    Val = MmioRead32 (CRU_CLKSEL_CON (0));
    Sel = (Val & CRU_CLKSEL_CON00_CLK_CORE_I_SEL_MASK) >> CRU_CLKSEL_CON00_CLK_CORE_I_SEL_SHIFT;
    Div = Val & CRU_CLKSEL_CON00_CLK_CORE0_DIV_MASK;

    if (Sel == CRU_CLKSEL_CON00_CLK_CORE_I_SEL_APLL) {
        PllRate = CruGetPllRate (CRU_APLL);
    } else {
        PllRate = CruGetPllRate (CRU_GPLL);
    }

    return PllRate / (Div + 1);
}


UINTN
CruGetSdmmcClockRate (
    IN UINT8 Index
    )
{
    UINT32 Val;
    UINT32 Sel;
    UINT32 Shift, Mask;
    UINTN Rate;
    
    ASSERT (Index <= 1);

    if (Index == 0) {
        Shift = CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_SHIFT;
        Mask = CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_MASK;
    } else {
        Shift = CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_SHIFT;
        Mask = CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_MASK;
    }

    Val = MmioRead32 (CRU_CLKSEL_CON (30));
    DEBUG ((DEBUG_INFO, "CruGetSdmmcClockRate(): CRU_CLKSEL_CON30 = %08X\n", Val));
    Sel = (Val & Mask) >> Shift;

    switch (Sel) {
    case 0:
        Rate = CRU_CLKREF_RATE;
        break;
    case 1:
        Rate = 400000000U;
        break;
    case 2:
        Rate = 300000000U;
        break;
    case 3:
        Rate = 100000000U;
        break;
    case 4:
        Rate = 50000000U;
        break;
    case 5:
        Rate = 750000U;
        break;
    default:
        ASSERT (FALSE);
        Rate = 0;
    }

    return Rate;
}

VOID
CruSetSdmmcClockRate (
  IN UINT8 Index,
  IN UINTN Rate
  )
{
    UINT32 Val;
    UINT32 Sel;
    UINT32 Shift, Mask;
    EFI_PHYSICAL_ADDRESS Reg;
    
    ASSERT (Index <= 2);

    if (Index == 0) {
        Reg = CRU_CLKSEL_CON (30);
        Shift = CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_SHIFT;
        Mask = CRU_CLKSEL_CON30_CLK_SDMMC0_SEL_MASK;
    } else if (Index == 1) {
        Reg = CRU_CLKSEL_CON (30);
        Shift = CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_SHIFT;
        Mask = CRU_CLKSEL_CON30_CLK_SDMMC1_SEL_MASK;
    } else {
        Reg = CRU_CLKSEL_CON (32);
        Shift = CRU_CLKSEL_CON32_CLK_SDMMC2_SEL_SHIFT;
        Mask = CRU_CLKSEL_CON32_CLK_SDMMC2_SEL_MASK;
    }

    if (Rate <= 750000U) {
        Sel = 5;
    } else if (Rate <= 24000000U) {
        Sel = 0;
    } else if (Rate <= 50000000U) {
        Sel = 4;
    } else if (Rate <= 100000000U) {
        Sel = 3;
    } else if (Rate <= 300000000U) {
        Sel = 2;
    } else if (Rate <= 400000000U) {
        Sel = 1;
    } else {
        ASSERT (FALSE);
    }

    Val = Mask << 16;
    Val |= Sel << Shift;
    MmioWrite32 (Reg, Val);

    DEBUG ((DEBUG_INFO, "CruSetSdmmcClockRate(%u, %lu): 0x%08X = %08X (wrote %08X)\n", Index, Rate, (UINT32)Reg, MmioRead32 (Reg), Val));
}

VOID
CruSetEmmcClockRate (
  IN UINTN Rate
  )
{
    UINT32 Val;
    UINT32 Sel;

    if (Rate >= 200000000U) {
      Sel = 1;
    } else if (Rate >= 150000000U) {
      Sel = 2;
    } else if (Rate >= 100000000U) {
      Sel = 3;
    } else if (Rate >= 50000000U) {
      Sel = 4;
    } else if (Rate >= 24000000U) {
      Sel = 0;
    } else {
      Sel = 5;
    }

    Val = CRU_CLKSEL_CON28_BCLK_EMMC_SEL_MASK << 16;
    Val |= 0 << CRU_CLKSEL_CON28_BCLK_EMMC_SEL_SHIFT;
    Val |= CRU_CLKSEL_CON28_CCLK_EMMC_SEL_MASK << 16;
    Val |= Sel << CRU_CLKSEL_CON28_CCLK_EMMC_SEL_SHIFT;
    MmioWrite32 (CRU_CLKSEL_CON (28), Val);

    DEBUG ((DEBUG_INFO, "CruSetEmmcClockRate(%lu): CRU_CLKSEL_CON28 = %08X (wrote %08X)\n", Rate, MmioRead32 (CRU_CLKSEL_CON (28)), Val));
}

VOID
CruSetPciePhySource (
  IN UINT8 Index,
  IN UINT8 Source
  )
{
    ASSERT (Index <= 2);
    ASSERT (Source <= 1);

    MmioWrite32 (PMUCRU_PMUCLKSEL_CON (9),
                 PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_MASK (Index) << 16 |
                 ((UINT32)Source << PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_SHIFT (Index)));    
}

UINTN
CruGetPciePhyClockRate (
  IN UINT8 Index
  )
{
    UINT32 Val;
    UINT32 Sel;
    UINT32 Div;
    UINTN Rate;

    ASSERT (Index <= 2);

    Val = MmioRead32 (PMUCRU_PMUCLKSEL_CON (9));
    Sel = (Val & PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_MASK (Index)) >> PMUCRU_PMUCLKSEL_CON09_PCIEPHY_SEL_SHIFT (Index);

    if (Sel == 0) {
        Rate = CRU_CLKREF_RATE;
    } else {
        Div = (Val & PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_MASK (Index)) >> PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_SHIFT (Index);
        Rate = PmuCruGetPllRate (PMUCRU_PPLL) / 2 / (Div + 1);
    }

    DEBUG ((DEBUG_INFO, "CruGetPciePhyClockRate(): Index = %u, Sel = %u, Rate = %lu Hz\n", Index, Sel, Rate));

    return Rate;
}


VOID
CruSetPciePhyClockRate (
  IN UINT8 Index,
  IN UINTN Rate
  )
{
    CRU_PLL_RATE *PllRate;
    UINT32 Mask;
    UINT32 Value;

    DEBUG ((DEBUG_INFO, "CruSetPpllRate(): Rate = %lu Hz\n", Rate));
    if (Rate == 100000000) {
        PllRate = CruFindPllRate (Rate * 2);
        ASSERT (PllRate != NULL);

        PmuCruSetPllRate (PMUCRU_PPLL, PllRate);

        Mask = PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_MASK (Index) << 16;
        Value = 0U << PMUCRU_PMUCLKSEL_CON09_PCIEPHY_DIV_SHIFT (Index);
        MmioWrite32 (PMUCRU_PMUCLKSEL_CON (9), Mask | Value);

        CruSetPciePhySource (Index, 1);
    } else if (Rate == 24000000) {
        CruSetPciePhySource (Index, 0);
    } else {
        ASSERT (FALSE);
    }
}

UINTN
CruGetHdmiClockRate (
  VOID
  )
{
    UINTN Rate;

    Rate = PmuCruGetPllRate (PMUCRU_HPLL);

    DEBUG ((DEBUG_INFO, "CruGetHdmiClockRate(): Rate = %lu Hz\n", Rate));

    return Rate;
}

VOID
CruSetHdmiClockRate (
  IN UINTN Rate
  )
{
    CRU_PLL_RATE *PllRate;

    DEBUG ((DEBUG_INFO, "CruSetHdmiClockRate(): Rate = %lu Hz\n", Rate));

    PllRate = CruFindPllRate (Rate);
    ASSERT (PllRate != NULL);

    if (PllRate != NULL) {
        PmuCruSetPllRate (PMUCRU_HPLL, PllRate);
    }
}

VOID
CruEnableClock (
  IN UINT32 Index,
  IN UINT8 Bit
  )
{
    MmioWrite32 (CRU_GATE_CON (Index), 1U << (Bit + 16));
}

VOID
CruAssertSoftReset (
  IN UINT32 Index,
  IN UINT8 Bit
  )
{
    MmioWrite32 (CRU_SOFTRST_CON (Index), (1U << (Bit + 16)) | (1U << Bit));
}

VOID
CruDeassertSoftReset (
  IN UINT32 Index,
  IN UINT8 Bit
  )
{
    MmioWrite32 (CRU_SOFTRST_CON (Index), 1U << (Bit + 16));
}
