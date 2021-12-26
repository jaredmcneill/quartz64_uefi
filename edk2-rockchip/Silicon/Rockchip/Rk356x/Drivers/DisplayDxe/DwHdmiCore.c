/* $NetBSD: dw_hdmi.c,v 1.9 2021/12/19 11:01:11 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <Base.h>
#include "DisplayDxe.h"
#include "Vop2.h"
#include "DwHdmi.h"
#include "BitHelpers.h"

#define	DDC_ADDR			0x50
#define	DDC_SEGMENT_ADDR	0x30
#define SCDC_ADDR			0x54

#define	HDMI_DESIGN_ID		0x0000
#define	HDMI_REVISION_ID	0x0001
#define	HDMI_CONFIG0_ID		0x0004
#define	 HDMI_CONFIG0_ID_AUDI2S			__BIT(4)
#define	HDMI_CONFIG2_ID		0x0006

#define	HDMI_IH_I2CM_STAT0	0x0105
#define	 HDMI_IH_I2CM_STAT0_DONE		__BIT(1)
#define	 HDMI_IH_I2CM_STAT0_ERROR		__BIT(0)
#define	HDMI_IH_MUTE		0x01ff
#define	 HDMI_IH_MUTE_WAKEUP_INTERRUPT		__BIT(1)
#define	 HDMI_IH_MUTE_ALL_INTERRUPT		__BIT(0)

#define	HDMI_TX_INVID0		0x0200
#define	 HDMI_TX_INVID0_VIDEO_MAPPING		__BITS(4,0)
#define	  HDMI_TX_INVID0_VIDEO_MAPPING_DEFAULT	1
#define	HDMI_TX_INSTUFFING	0x0201
#define	 HDMI_TX_INSTUFFING_BCBDATA_STUFFING	__BIT(2)
#define	 HDMI_TX_INSTUFFING_RCRDATA_STUFFING	__BIT(1)
#define	 HDMI_TX_INSTUFFING_GYDATA_STUFFING	__BIT(0)
#define	HDMI_TX_GYDATA0		0x0202
#define	HDMI_TX_GYDATA1		0x0203
#define	HDMI_TX_RCRDATA0	0x0204
#define	HDMI_TX_RCRDATA1	0x0205
#define	HDMI_TX_BCBDATA0	0x0206
#define	HDMI_TX_BCBDATA1	0x0207

#define	HDMI_VP_STATUS		0x0800
#define	HDMI_VP_PR_CD		0x0801
#define	 HDMI_VP_PR_CD_COLOR_DEPTH		__BITS(7,4)
#define	  HDMI_VP_PR_CD_COLOR_DEPTH_24		0
#define	 HDMI_VP_PR_CD_DESIRED_PR_FACTOR	__BITS(3,0)
#define	  HDMI_VP_PR_CD_DESIRED_PR_FACTOR_NONE	0
#define	HDMI_VP_STUFF		0x0802
#define	 HDMI_VP_STUFF_IDEFAULT_PHASE		__BIT(5)
#define	 HDMI_VP_STUFF_YCC422_STUFFING		__BIT(2)
#define	 HDMI_VP_STUFF_PP_STUFFING		__BIT(1)
#define	 HDMI_VP_STUFF_PR_STUFFING		__BIT(0)
#define	HDMI_VP_REMAP		0x0803
#define	 HDMI_VP_REMAP_YCC422_SIZE		__BITS(1,0)
#define	  HDMI_VP_REMAP_YCC422_SIZE_16		0
#define	HDMI_VP_CONF		0x0804
#define	 HDMI_VP_CONF_BYPASS_EN			__BIT(6)
#define	 HDMI_VP_CONF_BYPASS_SELECT		__BIT(2)
#define	 HDMI_VP_CONF_OUTPUT_SELECT		__BITS(1,0)
#define	  HDMI_VP_CONF_OUTPUT_SELECT_BYPASS	2
#define	HDMI_VP_STAT		0x0805
#define	HDMI_VP_INT		0x0806
#define	HDMI_VP_MASK		0x0807
#define	HDMI_VP_POL		0x0808

#define	HDMI_FC_INVIDCONF	0x1000
#define	 HDMI_FC_INVIDCONF_HDCP_KEEPOUT			__BIT(7)
#define	 HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY	__BIT(6)
#define	 HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY	__BIT(5)
#define	 HDMI_FC_INVIDCONF_DE_IN_POLARITY	__BIT(4)
#define	 HDMI_FC_INVIDCONF_DVI_MODE_HDMI    __BIT(3)
#define	 HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC	__BIT(1)
#define	 HDMI_FC_INVIDCONF_IN_I_P		__BIT(0)
#define	HDMI_FC_INHACTIV0	0x1001
#define	HDMI_FC_INHACTIV1	0x1002
#define	HDMI_FC_INHBLANK0	0x1003
#define	HDMI_FC_INHBLANK1	0x1004
#define	HDMI_FC_INVACTIV0	0x1005
#define	HDMI_FC_INVACTIV1	0x1006
#define	HDMI_FC_INVBLANK	0x1007
#define	HDMI_FC_HSYNCINDELAY0	0x1008
#define	HDMI_FC_HSYNCINDELAY1	0x1009
#define	HDMI_FC_HSYNCINWIDTH0	0x100a
#define	HDMI_FC_HSYNCINWIDTH1	0x100b
#define	HDMI_FC_VSYNCINDELAY	0x100c
#define	HDMI_FC_VSYNCINWIDTH	0x100d
#define	HDMI_FC_CTRLDUR		0x1011
#define	 HDMI_FC_CTRLDUR_DEFAULT		12
#define	HDMI_FC_EXCTRLDUR	0x1012
#define	 HDMI_FC_EXCTRLDUR_DEFAULT		32
#define	HDMI_FC_EXCTRLSPAC	0x1013
#define	 HDMI_FC_EXCTRLSPAC_DEFAULT		1
#define	HDMI_FC_CH0PREAM	0x1014
#define	 HDMI_FC_CH0PREAM_DEFAULT		0x0b
#define	HDMI_FC_CH1PREAM	0x1015
#define	 HDMI_FC_CH1PREAM_DEFAULT		0x16
#define	HDMI_FC_CH2PREAM	0x1016
#define	 HDMI_FC_CH2PREAM_DEFAULT		0x21
#define	HDMI_FC_AVICONF0	0x1019
#define	HDMI_FC_AVIVID		0x101c
#define	HDMI_FC_AUDCONF0	0x1025
#define	HDMI_FC_AUDCONF1	0x1026
#define	HDMI_FC_AUDCONF2	0x1027
#define	HDMI_FC_AUDCONF3	0x1028
#define	HDMI_FC_VSDIEEEID2	0x1029
#define	HDMI_FC_VSDSIZE		0x102a
#define	HDMI_FC_VSDIEEEID1	0x1030
#define	HDMI_FC_VSDIEEEID0	0x1031
#define	HDMI_FC_VSDPAYLOAD(n) (0x1032 + (n))
#define	HDMI_FC_DATAUTO0	0x10b3
#define	 HDMI_FC_DATAUTO0_VSD_AUTO	__BIT(3)
#define	HDMI_FC_DATAUTO1	0x10b4
#define	HDMI_FC_DATAUTO2	0x10b5
#define HDMI_FC_SCRAMBLER_CTRL	0x10e1

#define	HDMI_PHY_CONF0		0x3000
#define	 HDMI_PHY_CONF0_PDZ			__BIT(7)
#define	 HDMI_PHY_CONF0_ENTMDS			__BIT(6)
#define	 HDMI_PHY_CONF0_SVSRET			__BIT(5)
#define	 HDMI_PHY_CONF0_PDDQ			__BIT(4)
#define	 HDMI_PHY_CONF0_TXPWRON			__BIT(3)
#define	 HDMI_PHY_CONF0_ENHPDRXSENSE		__BIT(2)
#define	 HDMI_PHY_CONF0_SELDATAENPOL		__BIT(1)
#define	 HDMI_PHY_CONF0_SELDIPIF		__BIT(0)
#define	HDMI_PHY_STAT0		0x3004
#define	 HDMI_PHY_STAT0_RX_SENSE_3		__BIT(7)
#define	 HDMI_PHY_STAT0_RX_SENSE_2		__BIT(6)
#define	 HDMI_PHY_STAT0_RX_SENSE_1		__BIT(5)
#define	 HDMI_PHY_STAT0_RX_SENSE_0		__BIT(4)
#define	 HDMI_PHY_STAT0_HPD			__BIT(1)
#define	 HDMI_PHY_STAT0_TX_PHY_LOCK		__BIT(0)

#define	HDMI_AUD_CONF0		0x3100
#define	 HDMI_AUD_CONF0_SW_AUDIO_FIFO_RST	__BIT(7)
#define	 HDMI_AUD_CONF0_I2S_SELECT		__BIT(5)
#define	 HDMI_AUD_CONF0_I2S_IN_EN		__BITS(3,0)
#define	HDMI_AUD_CONF1		0x3101
#define	 HDMI_AUD_CONF1_I2S_WIDTH		__BITS(4,0)
#define	HDMI_AUD_INT		0x3102
#define	HDMI_AUD_CONF2		0x3103
#define	 HDMI_AUD_CONF2_INSERT_PCUV		__BIT(2)
#define	 HDMI_AUD_CONF2_NLPCM			__BIT(1)
#define	 HDMI_AUD_CONF2_HBR			__BIT(0)
#define	HDMI_AUD_INT1		0x3104

#define	HDMI_AUD_N1		0x3200
#define	HDMI_AUD_N2		0x3201
#define	HDMI_AUD_N3		0x3202
#define	HDMI_AUD_CTS1		0x3203
#define	HDMI_AUD_CTS2		0x3204
#define	HDMI_AUD_CTS3		0x3205
#define	HDMI_AUD_INPUTCLKFS	0x3206
#define	 HDMI_AUD_INPUTCLKFS_IFSFACTOR		__BITS(2,0)

#define	HDMI_MC_CLKDIS		0x4001
#define	 HDMI_MC_CLKDIS_HDCPCLK_DISABLE		__BIT(6)
#define	 HDMI_MC_CLKDIS_CECCLK_DISABLE		__BIT(5)
#define	 HDMI_MC_CLKDIS_CSCCLK_DISABLE		__BIT(4)
#define	 HDMI_MC_CLKDIS_AUDCLK_DISABLE		__BIT(3)
#define	 HDMI_MC_CLKDIS_PREPCLK_DISABLE		__BIT(2)
#define	 HDMI_MC_CLKDIS_TMDSCLK_DISABLE		__BIT(1)
#define	 HDMI_MC_CLKDIS_PIXELCLK_DISABLE	__BIT(0)
#define	HDMI_MC_SWRSTZREQ	0x4002
#define	 HDMI_MC_SWRSTZREQ_CECSWRST_REQ		__BIT(6)
#define	 HDMI_MC_SWRSTZREQ_PREPSWRST_REQ	__BIT(2)
#define	 HDMI_MC_SWRSTZREQ_TMDSSWRST_REQ	__BIT(1)
#define	 HDMI_MC_SWRSTZREQ_PIXELSWRST_REQ	__BIT(0)
#define	HDMI_MC_FLOWCTRL	0x4004
#define	HDMI_MC_PHYRSTZ		0x4005
#define	 HDMI_MC_PHYRSTZ_ASSERT			__BIT(0)
#define	 HDMI_MC_PHYRSTZ_DEASSERT		0
#define	HDMI_MC_LOCKONCLOCK	0x4006
#define	HDMI_MC_HEACPHY_RST	0x4007

#define	HDMI_A_HDCPCFG0		0x5000
#define	 HDMI_A_HDCPCFG0_HDMIDVI		__BIT(0)

#define	HDMI_I2CM_SLAVE		0x7e00
#define	HDMI_I2CM_ADDRESS	0x7e01
#define	HDMI_I2CM_DATAO		0x7e02
#define	HDMI_I2CM_DATAI		0x7e03
#define	HDMI_I2CM_OPERATION	0x7e04
#define	 HDMI_I2CM_OPERATION_WR			__BIT(4)
#define	 HDMI_I2CM_OPERATION_RD_EXT		__BIT(1)
#define	 HDMI_I2CM_OPERATION_RD			__BIT(0)
#define	HDMI_I2CM_INT		0x7e05
#define	 HDMI_I2CM_INT_DONE_POL			__BIT(3)
#define	 HDMI_I2CM_INT_DONE_MASK		__BIT(2)
#define	 HDMI_I2CM_INT_DONE_INTERRUPT		__BIT(1)
#define	 HDMI_I2CM_INT_DONE_STATUS		__BIT(0)
#define	 HDMI_I2CM_INT_DEFAULT			\
	(HDMI_I2CM_INT_DONE_POL|		\
	 HDMI_I2CM_INT_DONE_INTERRUPT|		\
	 HDMI_I2CM_INT_DONE_STATUS)
#define	HDMI_I2CM_CTLINT	0x7e06
#define	 HDMI_I2CM_CTLINT_NACK_POL		__BIT(7)
#define	 HDMI_I2CM_CTLINT_NACK_MASK		__BIT(6)
#define	 HDMI_I2CM_CTLINT_NACK_INTERRUPT	__BIT(5)
#define	 HDMI_I2CM_CTLINT_NACK_STATUS		__BIT(4)
#define	 HDMI_I2CM_CTLINT_ARB_POL		__BIT(3)
#define	 HDMI_I2CM_CTLINT_ARB_MASK		__BIT(2)
#define	 HDMI_I2CM_CTLINT_ARB_INTERRUPT		__BIT(1)
#define	 HDMI_I2CM_CTLINT_ARB_STATUS		__BIT(0)
#define	 HDMI_I2CM_CTLINT_DEFAULT		\
	(HDMI_I2CM_CTLINT_NACK_POL|		\
	 HDMI_I2CM_CTLINT_NACK_INTERRUPT|	\
	 HDMI_I2CM_CTLINT_NACK_STATUS|		\
	 HDMI_I2CM_CTLINT_ARB_POL|		\
	 HDMI_I2CM_CTLINT_ARB_INTERRUPT|	\
	 HDMI_I2CM_CTLINT_ARB_STATUS)
#define	HDMI_I2CM_DIV		0x7e07
#define	 HDMI_I2CM_DIV_FAST_STD_MODE		__BIT(3)
#define	HDMI_I2CM_SEGADDR	0x7e08
#define	 HDMI_I2CM_SEGADDR_SEGADDR		__BITS(6,0)
#define	HDMI_I2CM_SOFTRSTZ	0x7e09
#define	 HDMI_I2CM_SOFTRSTZ_I2C_SOFTRST		__BIT(0)
#define	HDMI_I2CM_SEGPTR	0x7e0a
#define	HDMI_I2CM_SS_SCL_HCNT_0_ADDR 0x730c
#define	HDMI_I2CM_SS_SCL_LCNT_0_ADDR 0x730e
#define HDMI_I2CM_SDA_HOLD	0x7e13

#define	SCDC_SINK_VERSION	0x01
#define	SCDC_SOURCE_VERSION	0x02
#define	SCDC_TMDS_CONFIG	0x20
#define  SCDC_TMDS_CONFIG_SCRAMBLING_ENABLE		__BIT(0)

UINT16 mDwHdmiVersion;
UINT8 mDwHdmiPhyType;
BOOLEAN mDwHdmiMonitorAudio;

EFI_STATUS
DwHdmiEdidRead (
	IN UINT8 block,
	OUT UINT8 *buf,
	IN UINTN len)
{
	UINT8 operation, val;
	UINT8 *pbuf = buf;
	int off, n, retry;

	ASSERT (buf != NULL);
	ASSERT (len > 0);
	ASSERT (len <= 256);

	DwHdmiWrite (HDMI_I2CM_SOFTRSTZ, 0);
	DwHdmiWrite (HDMI_IH_I2CM_STAT0, DwHdmiRead (HDMI_IH_I2CM_STAT0));
	DwHdmiWrite (HDMI_I2CM_SDA_HOLD, 0x48);
	DwHdmiWrite (HDMI_I2CM_SS_SCL_HCNT_0_ADDR, 0x71);
	DwHdmiWrite (HDMI_I2CM_SS_SCL_LCNT_0_ADDR, 0x76);
	DwHdmiWrite (HDMI_I2CM_DIV, 0);
	DwHdmiWrite (HDMI_I2CM_SLAVE, DDC_ADDR);
	DwHdmiWrite (HDMI_I2CM_SEGADDR, DDC_SEGMENT_ADDR);

	operation = block ? HDMI_I2CM_OPERATION_RD_EXT : HDMI_I2CM_OPERATION_RD;
	off = (block & 1) ? 128 : 0;

	DwHdmiWrite (HDMI_I2CM_SEGPTR, block >> 1);

	for (n = 0; n < len; n++) {
		DwHdmiWrite (HDMI_I2CM_ADDRESS, n + off);
		DwHdmiWrite (HDMI_I2CM_OPERATION, operation);
		for (retry = 10000; retry > 0; retry--) {
			MicroSecondDelay (1000);
			val = DwHdmiRead (HDMI_IH_I2CM_STAT0);
			if (val & HDMI_IH_I2CM_STAT0_ERROR) {
				DEBUG ((DEBUG_WARN, "DwHdmiDdcExec: Error! I2CM_STAT0 = 0x%X\n", val));
				return EFI_DEVICE_ERROR;
			}
			if (val & HDMI_IH_I2CM_STAT0_DONE) {
				DwHdmiWrite (HDMI_IH_I2CM_STAT0, val);
				break;
			}
		}
		if (retry == 0) {
			DEBUG ((DEBUG_WARN, "DwHdmiDdcExec: Timeout waiting for xfer, stat0=0x%X\n", DwHdmiRead (HDMI_IH_I2CM_STAT0)));
			return EFI_TIMEOUT;
		}

		pbuf[n] = DwHdmiRead (HDMI_I2CM_DATAI);
	}

	return EFI_SUCCESS;
}

EFI_STATUS
DwHdmiScdcRead (
	IN UINT8 Register,
	OUT UINT8 *Value
	)
{
	int Retry;
	UINT8 Val;

	ASSERT (Value != NULL);

	DwHdmiWrite (HDMI_I2CM_SOFTRSTZ, 0);
	DwHdmiWrite (HDMI_IH_I2CM_STAT0, DwHdmiRead (HDMI_IH_I2CM_STAT0));
	DwHdmiWrite (HDMI_I2CM_SDA_HOLD, 0x48);
	DwHdmiWrite (HDMI_I2CM_SS_SCL_HCNT_0_ADDR, 0x71);
	DwHdmiWrite (HDMI_I2CM_SS_SCL_LCNT_0_ADDR, 0x76);
	DwHdmiWrite (HDMI_I2CM_DIV, 0);
	DwHdmiWrite (HDMI_I2CM_SLAVE, SCDC_ADDR);

	DwHdmiWrite (HDMI_I2CM_ADDRESS, Register);
	DwHdmiWrite (HDMI_I2CM_OPERATION, HDMI_I2CM_OPERATION_RD);
	for (Retry = 10000; Retry > 0; Retry--) {
		MicroSecondDelay (1000);
		Val = DwHdmiRead (HDMI_IH_I2CM_STAT0);
		if (Val & HDMI_IH_I2CM_STAT0_ERROR) {
			DEBUG ((DEBUG_WARN, "DwHdmiScdcRead: Error! I2CM_STAT0 = 0x%X\n", Val));
			return EFI_DEVICE_ERROR;
		}
		if (Val & HDMI_IH_I2CM_STAT0_DONE) {
			DwHdmiWrite (HDMI_IH_I2CM_STAT0, Val);
			break;
		}
	}
	if (Retry == 0) {
		DEBUG ((DEBUG_WARN, "DwHdmiScdcRead: Timeout waiting for xfer, stat0=0x%X\n", DwHdmiRead (HDMI_IH_I2CM_STAT0)));
		return EFI_TIMEOUT;
	}

	*Value = DwHdmiRead (HDMI_I2CM_DATAI);

	return EFI_SUCCESS;
}

EFI_STATUS
DwHdmiScdcWrite (
	IN UINT8 Register,
	IN UINT8 Value
	)
{
	int Retry;
	UINT8 Val;

	DwHdmiWrite (HDMI_I2CM_SOFTRSTZ, 0);
	DwHdmiWrite (HDMI_IH_I2CM_STAT0, DwHdmiRead (HDMI_IH_I2CM_STAT0));
	DwHdmiWrite (HDMI_I2CM_SDA_HOLD, 0x48);
	DwHdmiWrite (HDMI_I2CM_SS_SCL_HCNT_0_ADDR, 0x71);
	DwHdmiWrite (HDMI_I2CM_SS_SCL_LCNT_0_ADDR, 0x76);
	DwHdmiWrite (HDMI_I2CM_DIV, 0);
	DwHdmiWrite (HDMI_I2CM_DATAO, Value);
	DwHdmiWrite (HDMI_I2CM_SLAVE, SCDC_ADDR);

	DwHdmiWrite (HDMI_I2CM_ADDRESS, Register);
	DwHdmiWrite (HDMI_I2CM_OPERATION, HDMI_I2CM_OPERATION_WR);
	for (Retry = 10000; Retry > 0; Retry--) {
		MicroSecondDelay (1000);
		Val = DwHdmiRead (HDMI_IH_I2CM_STAT0);
		if (Val & HDMI_IH_I2CM_STAT0_ERROR) {
			DEBUG ((DEBUG_WARN, "DwHdmiScdcWrite: Error! I2CM_STAT0 = 0x%X\n", Val));
			return EFI_DEVICE_ERROR;
		}
		if (Val & HDMI_IH_I2CM_STAT0_DONE) {
			DwHdmiWrite (HDMI_IH_I2CM_STAT0, Val);
			break;
		}
	}
	if (Retry == 0) {
		DEBUG ((DEBUG_WARN, "DwHdmiScdcWrite: Timeout waiting for xfer, stat0=0x%X\n", DwHdmiRead (HDMI_IH_I2CM_STAT0)));
		return EFI_TIMEOUT;
	}

	return EFI_SUCCESS;
}

STATIC
VOID
DwHdmiVpInit (
	VOID
	)
{
	UINT8 val;

	/* Select 24-bits per pixel video, 8-bit packing mode and disable pixel repetition */
	val = __SHIFTIN(HDMI_VP_PR_CD_COLOR_DEPTH_24, HDMI_VP_PR_CD_COLOR_DEPTH) |
	      __SHIFTIN(HDMI_VP_PR_CD_DESIRED_PR_FACTOR_NONE, HDMI_VP_PR_CD_DESIRED_PR_FACTOR);
	DwHdmiWrite (HDMI_VP_PR_CD, val);

	/* Configure stuffing */
	val = HDMI_VP_STUFF_IDEFAULT_PHASE |
	      HDMI_VP_STUFF_YCC422_STUFFING |
	      HDMI_VP_STUFF_PP_STUFFING |
	      HDMI_VP_STUFF_PR_STUFFING;
	DwHdmiWrite (HDMI_VP_STUFF, val);

	/* Set YCC422 remap to 16-bit input video */
	val = __SHIFTIN(HDMI_VP_REMAP_YCC422_SIZE_16, HDMI_VP_REMAP_YCC422_SIZE);
	DwHdmiWrite (HDMI_VP_REMAP, val);

	/* Configure video packetizer */
	val = HDMI_VP_CONF_BYPASS_EN |
	      HDMI_VP_CONF_BYPASS_SELECT |
	      __SHIFTIN(HDMI_VP_CONF_OUTPUT_SELECT_BYPASS, HDMI_VP_CONF_OUTPUT_SELECT);
	DwHdmiWrite (HDMI_VP_CONF, val);
}

STATIC
VOID
DwHdmiTxInit (
	VOID
	)
{
	UINT8 val;

	/* Disable internal data enable generator and set default video mapping */
	val = __SHIFTIN(HDMI_TX_INVID0_VIDEO_MAPPING_DEFAULT, HDMI_TX_INVID0_VIDEO_MAPPING);
	DwHdmiWrite (HDMI_TX_INVID0, val);

	/* Enable video sampler stuffing */
	val = HDMI_TX_INSTUFFING_BCBDATA_STUFFING |
	      HDMI_TX_INSTUFFING_RCRDATA_STUFFING |
	      HDMI_TX_INSTUFFING_GYDATA_STUFFING;
	DwHdmiWrite (HDMI_TX_INSTUFFING, val);
}

STATIC
BOOLEAN
DwHdmiCeaModeUsesFractionalVBlank (
	IN UINT8 Vic
	)
{
	const UINT8 Match[] = { 5, 6, 7, 10, 11, 20, 21, 22 };
	UINT32 Index;

	for (Index = 0; Index < ARRAY_SIZE(Match); Index++) {
		if (Match[Index] == Vic) {
			return TRUE;
		}
	}

	return FALSE;
}

STATIC
VOID
DwHdmiFcInit (
	HDMI_DISPLAY_TIMING *Timings
	)
{
	BOOLEAN scramble = Timings->FrequencyKHz > 340000;
	UINT8 val;

	const UINT8 vic = Timings->Vic;
	const UINT16 inhactiv = Timings->HDisplay;
	const UINT16 inhblank = Timings->HTotal - Timings->HDisplay;
	const UINT16 invactiv = Timings->VDisplay;
	const UINT8 invblank = Timings->VTotal - Timings->VDisplay;
	const UINT16 hsyncindelay = Timings->HSyncStart - Timings->HDisplay;
	const UINT16 hsyncinwidth = Timings->HSyncEnd - Timings->HSyncStart;
	const UINT8 vsyncindelay = Timings->VSyncStart - Timings->VDisplay;
	const UINT8 vsyncinwidth = Timings->VSyncEnd - Timings->VSyncStart;

	/* Input video configuration for frame composer */
	val = HDMI_FC_INVIDCONF_DE_IN_POLARITY;
	if (Timings->VSyncPol)
		val |= HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY;
	if (Timings->HSyncPol)
		val |= HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY;
#if 0
	if ((mode->flags & DRM_MODE_FLAG_INTERLACE) != 0)
		val |= HDMI_FC_INVIDCONF_IN_I_P;
#endif

	if (vic) {
		val |= HDMI_FC_INVIDCONF_DVI_MODE_HDMI;
	}
	if (Timings->FrequencyKHz > 340000) {
		val |= HDMI_FC_INVIDCONF_HDCP_KEEPOUT;
	}

	if (DwHdmiCeaModeUsesFractionalVBlank(vic))
		val |= HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC;
	DwHdmiWrite (HDMI_FC_INVIDCONF, val);

	/* Scrambling control */
	if (scramble) {
		/* XXX check to see if sink supports scrambling first */
		DwHdmiScdcRead (SCDC_SINK_VERSION, &val);	// Get sink version
		DwHdmiScdcWrite (SCDC_SOURCE_VERSION, val);	// Set source version

		DwHdmiScdcRead (SCDC_TMDS_CONFIG, &val);
		val |= SCDC_TMDS_CONFIG_SCRAMBLING_ENABLE;
		DwHdmiScdcWrite (SCDC_TMDS_CONFIG, val);

		/* Soft reset TMDS */
		val = 0xff & ~HDMI_MC_SWRSTZREQ_TMDSSWRST_REQ;
		DwHdmiWrite (HDMI_MC_SWRSTZREQ, val);

		DwHdmiWrite (HDMI_FC_SCRAMBLER_CTRL, 1);
	}

	/* Input video mode timings */
	DwHdmiWrite (HDMI_FC_INHACTIV0, inhactiv & 0xff);
	DwHdmiWrite (HDMI_FC_INHACTIV1, inhactiv >> 8);
	DwHdmiWrite (HDMI_FC_INHBLANK0, inhblank & 0xff);
	DwHdmiWrite (HDMI_FC_INHBLANK1, inhblank >> 8);
	DwHdmiWrite (HDMI_FC_INVACTIV0, invactiv & 0xff);
	DwHdmiWrite (HDMI_FC_INVACTIV1, invactiv >> 8);
	DwHdmiWrite (HDMI_FC_INVBLANK, invblank);
	DwHdmiWrite (HDMI_FC_HSYNCINDELAY0, hsyncindelay & 0xff);
	DwHdmiWrite (HDMI_FC_HSYNCINDELAY1, hsyncindelay >> 8);
	DwHdmiWrite (HDMI_FC_HSYNCINWIDTH0, hsyncinwidth & 0xff);
	DwHdmiWrite (HDMI_FC_HSYNCINWIDTH1, hsyncinwidth >> 8);
	DwHdmiWrite (HDMI_FC_VSYNCINDELAY, vsyncindelay);
	DwHdmiWrite (HDMI_FC_VSYNCINWIDTH, vsyncinwidth);

	/* Setup control period minimum durations */
	DwHdmiWrite (HDMI_FC_CTRLDUR, HDMI_FC_CTRLDUR_DEFAULT);
	DwHdmiWrite (HDMI_FC_EXCTRLDUR, HDMI_FC_EXCTRLDUR_DEFAULT);
	DwHdmiWrite (HDMI_FC_EXCTRLSPAC, HDMI_FC_EXCTRLSPAC_DEFAULT);

	/* Setup channel preamble filters */
	DwHdmiWrite (HDMI_FC_CH0PREAM, HDMI_FC_CH0PREAM_DEFAULT);
	DwHdmiWrite (HDMI_FC_CH1PREAM, HDMI_FC_CH1PREAM_DEFAULT);
	DwHdmiWrite (HDMI_FC_CH2PREAM, HDMI_FC_CH2PREAM_DEFAULT);

	/* Setup HDMI mode */
	if (vic) {
		DEBUG ((DEBUG_INFO, "HDMI: Setting up HDMI mode for VIC %u\n", vic));

		/* AVI infoframe */
		DwHdmiWrite (HDMI_FC_AVICONF0, 1 << 6);
		DwHdmiWrite (HDMI_FC_AVIVID, vic);		

		/* Disable VSD automatic packet scheduling */
		val = DwHdmiRead (HDMI_FC_DATAUTO0);
		val &= ~HDMI_FC_DATAUTO0_VSD_AUTO;
		DwHdmiWrite (HDMI_FC_DATAUTO0, val);

		/* Setup the infoframe itself */
		DwHdmiWrite (HDMI_FC_VSDSIZE, 9);
		DwHdmiWrite (HDMI_FC_VSDIEEEID0, 0x03);
		DwHdmiWrite (HDMI_FC_VSDIEEEID1, 0x0c);
		DwHdmiWrite (HDMI_FC_VSDIEEEID2, 0x00);
		DwHdmiWrite (HDMI_FC_VSDPAYLOAD (0), 1 << 5);
		DwHdmiWrite (HDMI_FC_VSDPAYLOAD (1), vic);

		/* Enable VSD automatic packet scheduling */
		val = DwHdmiRead (HDMI_FC_DATAUTO0);
		val |= HDMI_FC_DATAUTO0_VSD_AUTO;
		DwHdmiWrite (HDMI_FC_DATAUTO0, val);
		DwHdmiWrite (HDMI_FC_DATAUTO1, 0x01);
		DwHdmiWrite (HDMI_FC_DATAUTO2, 0x11);

		/* Enable HDMI HDCP */
		val = DwHdmiRead (HDMI_A_HDCPCFG0);
		val |= HDMI_A_HDCPCFG0_HDMIDVI;
		DwHdmiWrite (HDMI_A_HDCPCFG0, val);
	}
}

STATIC
VOID
DwHdmiMcInit (
	VOID
	)
{
	UINT8 val;
	UINT32 n, iter;

	/* Bypass colour space converter */
	DwHdmiWrite (HDMI_MC_FLOWCTRL, 0);

	/* Enable TMDS, pixel, and (if required) audio sampler clocks */
	val = HDMI_MC_CLKDIS_HDCPCLK_DISABLE |
	      HDMI_MC_CLKDIS_CECCLK_DISABLE |
	      HDMI_MC_CLKDIS_CSCCLK_DISABLE |
	      HDMI_MC_CLKDIS_PREPCLK_DISABLE;
	DwHdmiWrite (HDMI_MC_CLKDIS, val);

	/* Soft reset TMDS */
	val = 0xff & ~HDMI_MC_SWRSTZREQ_TMDSSWRST_REQ;
	DwHdmiWrite (HDMI_MC_SWRSTZREQ, val);

	iter = mDwHdmiVersion == 0x130A ? 4 : 1;

	val = DwHdmiRead (HDMI_FC_INVIDCONF);
	for (n = 0; n < iter; n++)
		DwHdmiWrite (HDMI_FC_INVIDCONF, val);
}

STATIC
VOID
DwHdmiAudioInit (
	HDMI_DISPLAY_TIMING *Timings
	)
{
	UINT8 val;
	UINT32 n;

	/* The following values are for 48 kHz */
	switch (Timings->FrequencyKHz) {
	case 25170:
		n = 6864;
		break;
	case 74170:
		n = 11648;
		break;
	case 148350:
		n = 5824;
		break;
	default:
		n = 6144;
		break;
	}

	/* Use automatic CTS generation */
	DwHdmiWrite (HDMI_AUD_CTS1, 0);
	DwHdmiWrite (HDMI_AUD_CTS2, 0);
	DwHdmiWrite (HDMI_AUD_CTS3, 0);

	/* Set N factor for audio clock regeneration */
	DwHdmiWrite (HDMI_AUD_N1, n & 0xff);
	DwHdmiWrite (HDMI_AUD_N2, (n >> 8) & 0xff);
	DwHdmiWrite (HDMI_AUD_N3, (n >> 16) & 0xff);

	val = DwHdmiRead (HDMI_AUD_CONF0);
	val |= HDMI_AUD_CONF0_I2S_SELECT;		/* XXX i2s mode */
	val &= ~HDMI_AUD_CONF0_I2S_IN_EN;
	val |= __SHIFTIN(1, HDMI_AUD_CONF0_I2S_IN_EN);	/* XXX 2ch */
	DwHdmiWrite (HDMI_AUD_CONF0, val);
	
	val = __SHIFTIN(16, HDMI_AUD_CONF1_I2S_WIDTH);
	DwHdmiWrite (HDMI_AUD_CONF1, val);

	DwHdmiWrite (HDMI_AUD_INPUTCLKFS, 4);	/* XXX 64 FS */

	DwHdmiWrite (HDMI_FC_AUDCONF0, 1 << 4);	/* XXX 2ch */
	DwHdmiWrite (HDMI_FC_AUDCONF1, 0);
	DwHdmiWrite (HDMI_FC_AUDCONF2, 0);
	DwHdmiWrite (HDMI_FC_AUDCONF3, 0);

	val = DwHdmiRead (HDMI_MC_CLKDIS);
	val &= ~HDMI_MC_CLKDIS_PREPCLK_DISABLE;
	DwHdmiWrite (HDMI_MC_CLKDIS, val);
}

VOID
DwHdmiBridgeEnable (
	HDMI_DISPLAY_TIMING *Timings
	)
{
	DwHdmiVpInit ();
	DwHdmiFcInit (Timings);

#if 0
	if (sc->sc_enable)
		sc->sc_enable(sc);
#endif

	DwHdmiTxInit ();
	DwHdmiMcInit ();

	if (mDwHdmiMonitorAudio) {
		DwHdmiAudioInit (Timings);
	}
}

VOID
DwHdmiInit (
	VOID
	)
{
	UINT8 val;

	mDwHdmiMonitorAudio = FALSE; // TODO

	mDwHdmiVersion = DwHdmiRead (HDMI_DESIGN_ID);
	mDwHdmiVersion <<= 8;
	mDwHdmiVersion |= DwHdmiRead (HDMI_REVISION_ID);

	mDwHdmiPhyType = DwHdmiRead (HDMI_CONFIG2_ID);

	DEBUG ((DEBUG_INFO, "HDMI: version %x.%03X, phytype 0x%02X\n",
			mDwHdmiVersion >> 12, mDwHdmiVersion & 0xFFF, mDwHdmiPhyType));

	/*
	 * Enable HPD on internal PHY
	 */
	val = DwHdmiRead (HDMI_PHY_CONF0);
	val |= HDMI_PHY_CONF0_ENHPDRXSENSE;
	DwHdmiWrite (HDMI_PHY_CONF0, val);
}
