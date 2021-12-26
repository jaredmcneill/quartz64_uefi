/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef _DWHDMI_H_
#define _DWHDMI_H_

typedef struct {
	UINT32			PixelClock;
	UINT32		    Sym;
	UINT32		    Term;
	UINT32		    Vlev;
} DWHDMI_PHY_CONFIG;

typedef struct {
	UINT32			PixelClock;
	UINT32  		Cpce;
	UINT32  		Gmp;
	UINT32  		Curr;
} DWHDMI_MPLL_CONFIG;

extern DWHDMI_PHY_CONFIG mDwHdmiPhyConfig[];
extern DWHDMI_MPLL_CONFIG mDwHdmiMpllConfig[];
extern UINT16 mDwHdmiVersion;
extern UINT8 mDwHdmiPhyType;
extern BOOLEAN mDwHdmiMonitorAudio;

UINT8
DwHdmiRead (
    IN UINTN Reg
    );

VOID
DwHdmiWrite (
    IN UINTN Reg,
    IN UINT8 Val
    );

VOID
DwHdmiEnable (
    IN HDMI_DISPLAY_TIMING *Timings
    );

VOID
DwHdmiInit (
	VOID
	);

VOID
DwHdmiBridgeEnable (
	HDMI_DISPLAY_TIMING *Timings
	);

BOOLEAN
DwHdmiPhyDetect (
	VOID
	);

VOID
DwHdmiPhyInit (
	HDMI_DISPLAY_TIMING *Timings
	);

EFI_STATUS
DwHdmiEdidRead (
	IN UINT8 block,
	OUT UINT8 *buf,
	IN UINTN len);

EFI_STATUS
DwHdmiScdcRead (
	IN UINT8 Register,
	OUT UINT8 *Value
	);

EFI_STATUS
DwHdmiScdcWrite (
	IN UINT8 Register,
	IN UINT8 Value
	);

BOOLEAN
DwHdmiDetect (
    OUT HDMI_DISPLAY_TIMING *Timings
    );

#endif /* _DWHDMI_H_ */