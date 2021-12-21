/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Base.h>
#include "DisplayDxe.h"
#include "Vop2.h"
#include "DwHdmi.h"
#include <IndustryStandard/Rk356x.h>

/* RK356x specific GRF registers */
#define GRF_GPIO4C_IOMUX_H  (SYS_GRF + 0x0074)
#define  GPIO4C7_SEL_SHIFT  12
#define  GPIO4C7_SEL_MASK   (0x7U << GPIO4C7_SEL_SHIFT)
#define GRF_GPIO4D_IOMUX_L  (SYS_GRF + 0x0078)
#define  GPIO4D1_SEL_SHIFT  4
#define  GPIO4D1_SEL_MASK   (0x7U << GPIO4D1_SEL_SHIFT)
#define  GPIO4D0_SEL_SHIFT  0
#define  GPIO4D0_SEL_MASK   (0x7U << GPIO4D1_SEL_SHIFT)
#define GRF_VO_CON1         (SYS_GRF + 0x0364)
#define  HDMI_SDAIN_MSK     BIT15
#define  HDMI_SCLIN_MSK     BIT14

#define IOMUX_HDMITX_FUNC   1

/* HDMI TX registers */
#define HDMI_REG(x)         (HDMI_BASE + ((x) * 4))

DWHDMI_MPLL_CONFIG mDwHdmiMpllConfig[] = {
        { 40000,        0x00b3, 0x0000, 0x0018 },
        { 65000,        0x0072, 0x0001, 0x0028 },
        { 66000,        0x013e, 0x0003, 0x0038 },
        { 83500,        0x0072, 0x0001, 0x0028 },
        { 146250,       0x0051, 0x0002, 0x0038 },
        { 148500,       0x0051, 0x0003, 0x0000 },
        { 272000,       0x0040, 0x0003, 0x0000 },
        { 340000,       0x0040, 0x0003, 0x0000 },
        { 0,            0x0051, 0x0003, 0x0000 },
};

DWHDMI_PHY_CONFIG mDwHdmiPhyConfig[] = {
        { 74250,        0x8009, 0x0004, 0x0272 },
        { 148500,       0x802b, 0x0004, 0x028d },
        { 297000,       0x8039, 0x0005, 0x028d },
        { 594000,       0x8039, 0x0000, 0x019d },
        { 0,            0x0000, 0x0000, 0x0000 }
};


STATIC
VOID
DwHdmiIomuxSetup (
    VOID
    )
{
    UINT32 Mask, Val;

    Mask = (HDMI_SDAIN_MSK | HDMI_SCLIN_MSK) << 16;
    Val = HDMI_SDAIN_MSK | HDMI_SCLIN_MSK;
    MmioWrite32 (GRF_VO_CON1, Mask | Val);
    Mask = GPIO4C7_SEL_MASK << 16;
    Val = IOMUX_HDMITX_FUNC << GPIO4C7_SEL_SHIFT;
    MmioWrite32 (GRF_GPIO4C_IOMUX_H, Mask | Val);
    Mask = (GPIO4D0_SEL_MASK | GPIO4D1_SEL_MASK) << 16;
    Val = (IOMUX_HDMITX_FUNC << GPIO4D0_SEL_SHIFT) | (IOMUX_HDMITX_FUNC << GPIO4D1_SEL_SHIFT);
    MmioWrite32 (GRF_GPIO4D_IOMUX_L, Mask | Val);
}

UINT8
DwHdmiRead (
    IN UINTN Reg
    )
{
    return MmioRead32 (HDMI_REG (Reg)) & 0xFF;
}

VOID
DwHdmiWrite (
    IN UINTN Reg,
    IN UINT8 Val
    )
{
    MmioWrite32 (HDMI_REG (Reg), Val);
}

VOID
DwHdmiEnable (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode,
    IN HDMI_DISPLAY_TIMING *Timings
    )
{
    BOOLEAN Hpd;

    /* Configure IOMUX */
    DwHdmiIomuxSetup ();

    /* Init DW HDMI */
    DwHdmiInit ();
    Hpd = DwHdmiPhyDetect ();
    DEBUG ((DEBUG_INFO, "HDMI: Plug %adetected\n", Hpd ? "" : "not "));
    DwHdmiBridgeEnable (Timings);
    DwHdmiPhyInit (Timings);
}
