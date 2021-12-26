/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Base.h>
#include "DisplayDxe.h"
#include <IndustryStandard/Rk356x.h>
#include <IndustryStandard/Rk356xVop2.h>
#include <Library/CruLib.h>
#include "Vop2.h"
#include "DwHdmi.h"

STATIC BOOLEAN mVop2Initialized = FALSE;
STATIC HDMI_DISPLAY_TIMING *mCurrentTimings;

STATIC
VOID
Vop2DebugDumpFrame (
    const char *Name,
    UINTN Start,
    UINTN End
    )
{
    UINTN Reg;

    DEBUG ((DEBUG_INFO, " --- %a: ---\n", Name));
    for (Reg = Start; Reg < End; Reg += (4 * 8)) {
        DEBUG ((DEBUG_INFO, "   %08X: %08X %08X %08X %08X    %08X %08X %08X %08X\n",
                Reg,
                MmioRead32 (Reg +  0), MmioRead32 (Reg +  4), MmioRead32 (Reg +  8), MmioRead32 (Reg + 12),
                MmioRead32 (Reg + 16), MmioRead32 (Reg + 20), MmioRead32 (Reg + 24), MmioRead32 (Reg + 28)
                ));
    }
}

STATIC
VOID
Vop2DebugDump (
    VOID
    )
{
    Vop2DebugDumpFrame ("SYSREG ", VOP2_SYSREG_BASE, VOP2_SYSREG_BASE + 0x100);
    Vop2DebugDumpFrame ("OVERLAY", VOP2_OVERLAY_BASE, VOP2_OVERLAY_BASE + 0x100);
    Vop2DebugDumpFrame ("POST0",   VOP2_POSTn_BASE (0), VOP2_POSTn_BASE (0) + 0x100);
    Vop2DebugDumpFrame ("ESMART0", VOP2_ESMARTn_BASE (0), VOP2_ESMARTn_BASE (0) + 0x100);
}

STATIC
VOID
Vop2Initialize (
    VOID
    )
{
    DEBUG ((DEBUG_INFO, "Vop2Initialize() called\n"));

    MmioOr32 (VOP2_SYS_OTP_WIN_EN_IMD, VOP2_SYS_OTP_WIN_EN_IMD_OTP_EN);
    MmioOr32 (VOP2_OVERLAY_CTRL, VOP2_OVERLAY_CTRL_LAYER_SEL_REGDONE_IMD);
    MmioOr32 (VOP2_SYS_DSP_INFACE_POL, VOP2_SYS_DSP_INFACE_POL_REGDONE_IMD_EN);
#if 0
    MmioWrite32 (VOP2_LAYER_SEL, 0x00000002);   /* layer0_sel = esmart0 */
    MmioAndThenOr32 (VOP2_PORT_SEL, ~0xFFF, 0x80);    /* port_mux = 1 layer */
#else
    MmioWrite32 (VOP2_LAYER_SEL, 0x00107632);
    MmioAndThenOr32 (VOP2_PORT_SEL, ~0xFFF, 0x085);
#endif
    MmioWrite32 (VOP2_SYS_AUTO_GATING_CTRL_IMD, 0);
}

VOID
Vop2SetMode (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode
    )
{
    UINT32 Val;
    UINT32 HSyncLen, HActSt, HActEnd, HBackPorch;
    UINT32 VSyncLen, VActSt, VActEnd, VBackPorch;
    UINTN Rate;

    mCurrentTimings = &mPreferredTimings;

    ASSERT (mCurrentTimings != NULL);
    ASSERT (Mode->Info->HorizontalResolution == mCurrentTimings->HDisplay);
    ASSERT (Mode->Info->VerticalResolution == mCurrentTimings->VDisplay);
    ASSERT (Mode->FrameBufferBase != 0 && Mode->FrameBufferBase < SIZE_4GB);

    if (!mVop2Initialized) {
        Vop2Initialize ();
        mVop2Initialized = TRUE;
    }

    CruSetHdmiClockRate (mCurrentTimings->FrequencyKHz * 1000);

    gBS->Stall (100000); // XXX wait for lock

    Rate = CruGetHdmiClockRate ();
    DEBUG ((DEBUG_INFO, "Vop2SetMode(): HPLL rate %u Hz\n", Rate));

    /* Enable HDMI interface */
    MmioOr32 (VOP2_SYS_DSP_INFACE_EN, VOP2_SYS_DSP_INFACE_EN_HDMI_OUT_EN);
    Val = MmioRead32 (VOP2_SYS_DSP_INFACE_POL);
    Val |= VOP2_SYS_DSP_INFACE_POL_REGDONE_IMD_EN;
    Val |= VOP2_SYS_DSP_INFACE_POL_HDMI_DCLK_POL;
    Val &= ~(VOP2_SYS_DSP_INFACE_POL_HDMI_VSYNC_POL|VOP2_SYS_DSP_INFACE_POL_HDMI_HSYNC_POL|VOP2_SYS_DSP_INFACE_POL_HDMI_DEN_POL);
    if (mCurrentTimings->VSyncPol) {
        Val |= VOP2_SYS_DSP_INFACE_POL_HDMI_VSYNC_POL;
    }
    if (mCurrentTimings->HSyncPol) {
        Val |= VOP2_SYS_DSP_INFACE_POL_HDMI_HSYNC_POL;
    }
    MmioWrite32 (VOP2_SYS_DSP_INFACE_POL, Val);

    /* Setup post-process timings */
    HSyncLen = mCurrentTimings->HSyncEnd - mCurrentTimings->HSyncStart;
    HBackPorch = mCurrentTimings->HTotal - mCurrentTimings->HSyncEnd;
    HActSt = mCurrentTimings->HTotal - mCurrentTimings->HSyncStart;
    HActEnd = HActSt + mCurrentTimings->HDisplay;
    VSyncLen = mCurrentTimings->VSyncEnd - mCurrentTimings->VSyncStart;
    VBackPorch = mCurrentTimings->VTotal - mCurrentTimings->VSyncEnd;
    VActSt = mCurrentTimings->VTotal - mCurrentTimings->VSyncStart;
    VActEnd = VActSt + mCurrentTimings->VDisplay;

    MmioWrite32 (VOP2_POSTn_DSP_BG (0), 0x1FF);
    MmioWrite32 (VOP2_POSTn_PRE_SCAN_HTIMING (0), ((42 + (mCurrentTimings->HDisplay >> 1) - 1) << 16) | HSyncLen);
    MmioWrite32 (VOP2_POSTn_DSP_HACT_INFO (0), (HActSt << 16) | HActEnd);
    MmioWrite32 (VOP2_POSTn_DSP_VACT_INFO (0), (VActSt << 16) | VActEnd);
    MmioWrite32 (VOP2_POSTn_SCL_FACTOR_YRGB (0), 0x10001000);   /* No scaling */
    MmioWrite32 (VOP2_POSTn_DSP_HTOTAL_HS_END (0), (mCurrentTimings->HTotal << 16) | HSyncLen);
    MmioWrite32 (VOP2_POSTn_DSP_HACT_ST_END (0), (HActSt << 16) | HActEnd);
    MmioWrite32 (VOP2_POSTn_DSP_VTOTAL_VS_END (0), (mCurrentTimings->VTotal << 16) | VSyncLen);
    MmioWrite32 (VOP2_POSTn_DSP_VACT_ST_END (0), (VActSt << 16) | VActEnd);

    MmioAndThenOr32 (VOP2_DPn_BG_MIX_CTRL (0),
                     ~VOP2_DPn_BG_MIX_CTRL_DP_BG_DLY_NUM_MASK,
                     42 << VOP2_DPn_BG_MIX_CTRL_DP_BG_DLY_NUM_SHIFT);

    /* Setup layer */
    MmioWrite32 (VOP2_ESMART_CTRL0 (0), BIT0);
    MmioWrite32 (VOP2_ESMART_REGION0_VIR (0), Mode->Info->HorizontalResolution);
    MmioWrite32 (VOP2_ESMART_REGION0_MST_YRGB (0), (UINT32)Mode->FrameBufferBase);
    MmioWrite32 (VOP2_ESMART_REGION0_ACT_INFO (0), ((mCurrentTimings->VDisplay - 1) << 16) | (mCurrentTimings->HDisplay - 1));
    MmioWrite32 (VOP2_ESMART_REGION0_DSP_INFO (0), ((mCurrentTimings->VDisplay - 1) << 16) | (mCurrentTimings->HDisplay - 1));
    MmioWrite32 (VOP2_ESMART_REGION0_DSP_OFFSET (0), 0);
    MmioAndThenOr32 (VOP2_ESMART_REGION0_MST_CTL (0),
                     ~VOP2_ESMART_REGION0_MST_CTL_DATA_FMT_MASK,
                     VOP2_ESMART_REGION0_MST_CTL_DATA_FMT_ARGB8888 | VOP2_ESMART_REGION0_MST_CTL_MST_ENABLE);

    /* Set output mode and enable */
    Val = MmioRead32 (VOP2_POSTn_DSP_CTRL (0));
    Val &= ~VOP2_POSTn_DSP_CTRL_DSP_OUT_MODE;
    Val |= VOP2_POSTn_DSP_CTRL_DSP_OUT_MODE_30BIT;
    // Val |= VOP2_POSTn_DSP_CTRL_DSP_OUT_MODE_24BIT;
    Val &= ~VOP2_POSTn_DSP_CTRL_VOP_STANDBY_EN_IMD;
    MmioWrite32 (VOP2_POSTn_DSP_CTRL (0), Val);

    /* XXX color bars */
    // MmioWrite32 (VOP2_POSTn_COLOR_CTRL (0), 1);

    /* Commit settings */
    MmioWrite32 (VOP2_SYS_REG_CFG_DONE,
                 VOP2_SYS_REG_CFG_DONE_SW_GLOBAL_REGDONE_EN |
                 VOP2_SYS_REG_CFG_DONE_REG_LOAD_GLOBAL0_EN);
 
    /* Dump registers */
    Vop2DebugDump ();
}