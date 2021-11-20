/** @file

  Copyright 2021, Jared McNeill <jmcneill@invisible.ca>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef USBPHY_H_
#define USBPHY_H_

#include <Base.h>

/* USBPHY_U2_GRF_CON0 Registers */
#define USBPHY_U2_GRF_CON0_USB2HOST0_UTMI_SUSPEND_N     BIT1
#define USBPHY_U2_GRF_CON0_USB2HOST0_UTMI_SEL           BIT0

/* USBPHY_U2_GRF_CON1 Registers */
#define USBPHY_U2_GRF_CON1_USB2HOST1_UTMI_SUSPEND_N     BIT1
#define USBPHY_U2_GRF_CON1_USB2HOST1_UTMI_SEL           BIT0

/* USBPHY_U2_GRF_CON2 Registers */
#define USBPHY_U2_GRF_CON2_USBPHY_COMMONONN             BIT4

typedef struct {
  UINT32 Con0;
  UINT32 Con1;
  UINT32 Con2;
  UINT32 Con3;
  UINT32 Res1[12];
  UINT32 LsCon;
  UINT32 DisCon;
  UINT32 BValidCon;
  UINT32 IdCon;
  UINT32 Res2[12];
  UINT32 IntMask;
  UINT32 IntStatus;
  UINT32 IntStatusClr;
  UINT32 Status;
} USBPHY_GRF;

#endif