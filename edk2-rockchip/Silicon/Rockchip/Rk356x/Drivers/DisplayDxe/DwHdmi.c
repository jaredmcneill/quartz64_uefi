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
#include <Library/CruLib.h>
#include <Library/GpioLib.h>

/* Maximum number of EDID extension blocks */
#define MAX_EDID_EXTENSION_BLOCKS   5

/* RK356x specific GRF registers */
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
        { 600000,       0x1a40, 0x0003, 0x0000 },
        { 0,            0x0051, 0x0003, 0x0000 },
};

DWHDMI_PHY_CONFIG mDwHdmiPhyConfig[] = {
        { 74250,        0x8009, 0x0004, 0x0272 },
        { 148500,       0x802b, 0x0004, 0x028d },
        { 297000,       0x8039, 0x0005, 0x028d },
        { 594000,       0x8039, 0x0000, 0x019d },
        { 0,            0x0000, 0x0000, 0x0000 }
};

STATIC EFI_EDID_DISCOVERED_PROTOCOL mEdidDiscovered;
STATIC EFI_EDID_ACTIVE_PROTOCOL mEdidActive;

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

    /* hdmitxm0_cec */
    GpioPinSetFunction (4, GPIO_PIN_PD1, IOMUX_HDMITX_FUNC);
    GpioPinSetPull (4, GPIO_PIN_PD1, GPIO_PIN_PULL_NONE);

    /* hdmitx_scl */
    GpioPinSetFunction (4, GPIO_PIN_PC7, IOMUX_HDMITX_FUNC);
    GpioPinSetPull (4, GPIO_PIN_PC7, GPIO_PIN_PULL_NONE);

    /* hdmitx_sda */
    GpioPinSetFunction (4, GPIO_PIN_PD0, IOMUX_HDMITX_FUNC);
    GpioPinSetPull (4, GPIO_PIN_PD0, GPIO_PIN_PULL_NONE);
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

BOOLEAN
DwHdmiIsEdidValid (
    IN UINT8 *Edid
    )
{
    UINT8 Sum;
    UINT32 Index;

    for (Sum = 0, Index = 0; Index < 8; Index++) {
        if (Index == 0 || Index == 7) {
            if (Edid[Index] != 0x00) {
                return FALSE;
            }
        } else if (Edid[Index] != 0xFF) {
            return FALSE;
        }
        Sum += Edid[Index];
    }
    for (; Index < 128; Index++) {
        Sum += Edid[Index];
    }

    return Sum == 0;
}

STATIC BOOLEAN
DwHdmiParseDetailedTimingBlock (
    IN UINT8 *Data,
    OUT HDMI_DISPLAY_TIMING *Timings
    )
{
    UINT16 PixelClock;

    PixelClock = Data[0] | (Data[1] << 8);
    if (PixelClock == 0) {
        return FALSE;
    }

    /*
     * Driver doesn't know how to drive all modes yet. Restrict to known modes.
     */
    switch (PixelClock * 10) {
    case 594000:
    case 297000:
    case 148500:
    case 74250:
        break;
    default:
        DEBUG ((DEBUG_WARN, "HDMI: Unsupported pixel clock %u kHz for %ux%u mode.\n",
                PixelClock * 10,
                Data[2] | ((Data[4] & 0xF0) << 4),
                Data[5] | ((Data[7] & 0xF0) << 4)));
        return FALSE;
    }

    Timings->Vic        = 0;   // Default value; will try to get from extension block later
    Timings->FrequencyKHz = PixelClock * 10;
    Timings->HDisplay   = Data[2] | ((Data[4] & 0xF0) << 4);
    Timings->HSyncStart = Timings->HDisplay +
                          (Data[8] | ((Data[11] & 0xC0) << 2));
    Timings->HSyncEnd   = Timings->HSyncStart +
                          (Data[9] | ((Data[11] & 0x30) << 4));
    Timings->HTotal     = Timings->HDisplay +
                          (Data[3] | ((Data[4] & 0x0F) << 8));
    Timings->VDisplay   = Data[5] | ((Data[7] & 0xF0) << 4);
    Timings->VSyncStart = Timings->VDisplay +
                          ((Data[10] >> 4) | ((Data[11] & 0x0C) << 2));
    Timings->VSyncEnd   = Timings->VSyncStart +
                          ((Data[10] & 0x0F) | ((Data[11] & 0x03) << 4));
    Timings->VTotal     = Timings->VDisplay +
                          (Data[6] | ((Data[7] & 0x0F) << 8));
    Timings->HSyncPol   = (Data[17] & BIT1) != 0;
    Timings->VSyncPol   = (Data[17] & BIT2) != 0;

    DEBUG ((DEBUG_INFO, "HDMI: EDID preferred mode (VIC %u):\n", Timings->Vic));
    DEBUG ((DEBUG_INFO, "      DotClock %u kHz\n", Timings->FrequencyKHz));
    DEBUG ((DEBUG_INFO, "      HDisplay %u HSyncStart %u HSyncEnd %u HTotal %u HSyncPol %c\n",
            Timings->HDisplay, Timings->HSyncStart, Timings->HSyncEnd, Timings->HTotal, Timings->HSyncPol ? '+' : '-'));
    DEBUG ((DEBUG_INFO, "      VDisplay %u VSyncStart %u VSyncEnd %u VTotal %u VSyncPol %c\n",
            Timings->VDisplay, Timings->VSyncStart, Timings->VSyncEnd, Timings->VTotal, Timings->VSyncPol ? '+' : '-'));

    return TRUE;
}

STATIC VOID
DwHdmiParseCea861ExtentionBlock (
    IN UINT8 *Edid,
    IN UINT8 BlockNo,
    IN UINT8 Rev,
    IN UINT8 Off,
    OUT HDMI_DISPLAY_TIMING *Timings
    )
{
    if (Rev != 3) {
        return;
    }
    
    for (UINT32 Data = 4; Data < Off;) {
        CONST UINT8 BlockTag = (Edid[Data] >> 5) & 0x7;
        CONST UINT8 BlockLen = Edid[Data] & 0x1F;
        DEBUG ((DEBUG_INFO, "HDMI: [%02X] CEA data block @ 0x%X, Tag 0x%X, Len 0x%X\n", BlockNo, BlockTag, BlockLen));
        if (Data + BlockLen + 1 > Off) {
            break;
        }
        switch (BlockTag) {
        case 2: // video
            for (UINT32 Entry = 1; Entry <= BlockLen; Entry++) {
                UINT8 Svd = Edid[Data + Entry];
                DEBUG ((DEBUG_INFO, "HDMI: [%02X] SVD %u%s\n", BlockNo, Svd & 0x7F, (Svd & 0x80) ? " (native)" : ""));
                if (Timings->Vic == 0 && (Svd & 0x80) != 0) {
                    Timings->Vic = Svd & 0x7F;
                }
            }
            break;
        }
        Data += (1 + BlockLen);
    }
}

STATIC UINT8
DwHdmiGuessVic (
    IN HDMI_DISPLAY_TIMING *Timings
    )
{
    ASSERT (Timings->Vic == 0);

    if (Timings->HDisplay == 3840 && Timings->VDisplay == 2160) {
        if (Timings->FrequencyKHz == 594000) {
            return 97;  // 16:9 2160p60
        } else if (Timings->FrequencyKHz == 297000) {
            return 95;  // 16:9 2160p30
        }
    }

    return 0;
}

BOOLEAN
DwHdmiParseEdid (
    IN UINT8 *Edid,
    OUT HDMI_DISPLAY_TIMING *Timings,
    IN UINT8 NumExt
    )
{
    BOOLEAN Valid;

    for (UINT32 Index = 0; Index < 128 * (1 + NumExt); Index += 16) {
        DEBUG ((DEBUG_WARN, "EDID: [%02X] +%04X: %02X %02X %02X %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X %02X %02X %02X\n",
                Index / 128, Index,
                Edid[Index + 0], Edid[Index + 1], Edid[Index + 2], Edid[Index + 3],
                Edid[Index + 4], Edid[Index + 5], Edid[Index + 6], Edid[Index + 7],
                Edid[Index + 8], Edid[Index + 9], Edid[Index + 10], Edid[Index + 11],
                Edid[Index + 12], Edid[Index + 13], Edid[Index + 14], Edid[Index + 15]));
    }

    if (!DwHdmiIsEdidValid (Edid)) {
        DEBUG ((DEBUG_WARN, "HDMI: EDID is not valid\n"));
        return FALSE;
    }

    Valid = DwHdmiParseDetailedTimingBlock (Edid + 0x36, Timings);
    if (!Valid) {
        DEBUG ((DEBUG_WARN, "HDMI: Detailed timing block not usable.\n"));
        return FALSE;
    }

    for (UINT32 Index = 128; Index < 128 * (1 + NumExt); Index += 128) {
        CONST UINT8 Tag = Edid[Index + 0];
        CONST UINT8 Rev = Edid[Index + 1];
        CONST UINT8 Off = Edid[Index + 2];
        DEBUG ((DEBUG_INFO, "HDMI: [%02X] EDID Extension 0x%02X, Rev 0x%02X, Off 0x%02X", Index / 128, Tag, Rev, Off));
        switch (Tag) {
        case 0x02:  // CEA EDID
            DwHdmiParseCea861ExtentionBlock (&Edid[Index], Index / 128, Rev, Off, Timings);
            break;
        }
    }

    if (Timings->Vic == 0) {
        Timings->Vic = DwHdmiGuessVic (Timings);
        if (Timings->Vic != 0) {
            DEBUG ((DEBUG_INFO, "HDMI: Couldn't determine VIC, guessing %u\n", Timings->Vic));
        }
    }

    return TRUE;
}

BOOLEAN
DwHdmiDetect (
    OUT HDMI_DISPLAY_TIMING *Timings
    )
{
    BOOLEAN Hpd;
    EFI_STATUS Status;
    UINT8 Buf[128 * (1 + MAX_EDID_EXTENSION_BLOCKS)];
    EFI_HANDLE Handle;
    UINT8 Retry;
    UINT8 NumExt;

    /* Configure IOMUX */
    DwHdmiIomuxSetup ();

    /* Init DW HDMI */
    DwHdmiInit ();
    DwHdmiPhyInit (NULL);

    Hpd = DwHdmiPhyDetect ();
    DEBUG ((DEBUG_INFO, "HDMI: Plug %adetected\n", Hpd ? "" : "not "));
    if (!Hpd) {
        return FALSE;
    }

    for (Retry = 0; Retry < 5; Retry++) {
        Status = DwHdmiEdidRead (0, &Buf[0], 128);
        if (Status == EFI_SUCCESS) {
            break;
        }
    }
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "HDMI: EDID DDC read failed: %r\n", Status));
        return FALSE;
    }

    for (NumExt = 0; NumExt < MIN (MAX_EDID_EXTENSION_BLOCKS, Buf[126]); NumExt++) {
        Status = DwHdmiEdidRead (1 + NumExt, &Buf[128 + (1 + NumExt)], 128);
        if (Status != EFI_SUCCESS) {
            break;
        }
    }

    DEBUG ((DEBUG_INFO, "HDMI: Read %u extention blocks (of possible %u)\n", NumExt, Buf[126]));
    if (DwHdmiParseEdid (Buf, Timings, NumExt) == FALSE) {
        // There was something we didn't like about the EDID, but return TRUE anyway so we can just
        // use the default display mode.
        return TRUE;
    }

    if (mEdidDiscovered.Edid != NULL) {
        FreePool (mEdidDiscovered.Edid);
    }
    mEdidDiscovered.SizeOfEdid = mEdidActive.SizeOfEdid = 128;
    mEdidDiscovered.Edid = mEdidActive.Edid = AllocateCopyPool (mEdidDiscovered.SizeOfEdid, Buf);
    ASSERT (mEdidDiscovered.Edid != NULL);

    Handle = NULL;
    Status = gBS->InstallMultipleProtocolInterfaces (
      &Handle,
      &gEfiEdidDiscoveredProtocolGuid,
      &mEdidDiscovered,
      &gEfiEdidActiveProtocolGuid,
      &mEdidActive,
      NULL);
    ASSERT (Status == EFI_SUCCESS);

    return TRUE;
}

VOID
DwHdmiEnable (
    IN HDMI_DISPLAY_TIMING *Timings
    )
{
    DwHdmiBridgeEnable (Timings);
    DwHdmiPhyInit (Timings);
}
