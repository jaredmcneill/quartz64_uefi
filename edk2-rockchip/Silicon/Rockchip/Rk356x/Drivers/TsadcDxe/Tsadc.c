/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/Rk356x.h>

/* SYS_GRF */
#define GRF_TSADC_CON               (SYS_GRF + 0x0600)
#define  TSADC_ANA_REG(n)           (1U << (n))
#define  TSADC_TSEN                 BIT8

/* TS-ADC */
#define TSADC_USER_CON              (TSADC_BASE + 0x0000)
#define  INTER_PD_SOC_SHIFT         6
#define TSADC_AUTO_CON              (TSADC_BASE + 0x0004)
#define  AUTO_EN                    BIT0
#define  TSADC_Q_SEL                BIT1
#define  SRC0_EN                    BIT4
#define  SRC1_EN                    BIT5
#define TSADC_HIGHT_INT_DEBOUNCE    (TSADC_BASE + 0x0060)
#define TSADC_HIGHT_TSHUT_DEBOUNCE  (TSADC_BASE + 0x0064)
#define TSADC_AUTO_PERIOD           (TSADC_BASE + 0x0068)
#define TSADC_AUTO_PERIOD_HT        (TSADC_BASE + 0x006C)

EFI_STATUS
EFIAPI
InitializeTsadc (
    IN EFI_HANDLE            ImageHandle,
    IN EFI_SYSTEM_TABLE      *SystemTable
    )
{
    UINT32 Index;

    MmioWrite32 (TSADC_USER_CON, 0x3F << INTER_PD_SOC_SHIFT);
    MmioWrite32 (TSADC_AUTO_PERIOD, 1622);
    MmioWrite32 (TSADC_AUTO_PERIOD_HT, 1622);
    MmioWrite32 (TSADC_HIGHT_INT_DEBOUNCE, 4);
    MmioWrite32 (TSADC_HIGHT_TSHUT_DEBOUNCE, 4);
    MmioWrite32 (TSADC_AUTO_CON, SRC0_EN | SRC1_EN);

    MmioWrite32 (GRF_TSADC_CON, (TSADC_TSEN << 16) | TSADC_TSEN);
    MicroSecondDelay (100);
    for (Index = 0; Index <= 2; Index++) {
        MmioWrite32 (GRF_TSADC_CON, (TSADC_ANA_REG (Index) << 16) | TSADC_ANA_REG (Index));
    }
    MicroSecondDelay (200);

    MmioOr32 (TSADC_AUTO_CON, AUTO_EN | TSADC_Q_SEL);
    MicroSecondDelay (100);

    return EFI_SUCCESS;
}