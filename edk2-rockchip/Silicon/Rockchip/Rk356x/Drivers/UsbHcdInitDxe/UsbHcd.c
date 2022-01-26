/** @file

  Copyright 2017, 2020 NXP
  Copyright 2021, Jared McNeill <jmcneill@invisible.ca>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "UsbHcd.h"
#include "UsbPhy.h"

STATIC
VOID
XhciSetBeatBurstLength (
  IN  UINTN  UsbReg
  )
{
  DWC3       *Dwc3Reg;

  Dwc3Reg = (VOID *)(UsbReg + DWC3_REG_OFFSET);

  MmioAndThenOr32 ((UINTN)&Dwc3Reg->GSBusCfg0, ~USB3_ENABLE_BEAT_BURST_MASK,
    USB3_ENABLE_BEAT_BURST);

  MmioOr32 ((UINTN)&Dwc3Reg->GSBusCfg1, USB3_SET_BEAT_BURST_LIMIT);
}

STATIC
VOID
Dwc3SetFladj (
  IN  DWC3   *Dwc3Reg,
  IN  UINT32 Val
  )
{
  MmioOr32 ((UINTN)&Dwc3Reg->GFLAdj, GFLADJ_30MHZ_REG_SEL |
    GFLADJ_30MHZ (Val));
}

STATIC
VOID
Dwc3SetMode (
  IN  DWC3   *Dwc3Reg,
  IN  UINT32 Mode
  )
{
  MmioAndThenOr32 ((UINTN)&Dwc3Reg->GCtl,
    ~(DWC3_GCTL_PRTCAPDIR (DWC3_GCTL_PRTCAP_OTG)),
    DWC3_GCTL_PRTCAPDIR (Mode));
}

/**
  This function issues phy reset and core soft reset

  @param  Dwc3Reg      Pointer to DWC3 register.

**/
STATIC
VOID
Dwc3CoreSoftReset (
  IN  DWC3   *Dwc3Reg
  )
{
  //
  // Put core in reset before resetting PHY
  //
  MmioOr32 ((UINTN)&Dwc3Reg->GCtl, DWC3_GCTL_CORESOFTRESET);

  //
  // Assert USB2 PHY reset
  //
  MmioOr32 ((UINTN)&Dwc3Reg->GUsb3PipeCtl[0], DWC3_GUSB3PIPECTL_PHYSOFTRST);

  //
  // Assert USB3 PHY reset
  //
  MmioOr32 ((UINTN)&Dwc3Reg->GUsb2PhyCfg[0], DWC3_GUSB2PHYCFG_PHYSOFTRST);

  MemoryFence ();

  //
  // Clear USB3 PHY reset
  //
  MmioAnd32 ((UINTN)&Dwc3Reg->GUsb3PipeCtl[0], ~DWC3_GUSB3PIPECTL_PHYSOFTRST);

  //
  // Clear USB2 PHY reset
  //
  MmioAnd32 ((UINTN)&Dwc3Reg->GUsb2PhyCfg[0], ~DWC3_GUSB2PHYCFG_PHYSOFTRST);

  MemoryFence ();

  //
  // Take core out of reset, PHYs are stable now
  //
  MmioAnd32 ((UINTN)&Dwc3Reg->GCtl, ~DWC3_GCTL_CORESOFTRESET);
}

/**
  This function performs low-level initialization of DWC3 Core

  @param  Dwc3Reg      Pointer to DWC3 register.

**/
STATIC
EFI_STATUS
Dwc3CoreInit (
  IN  DWC3   *Dwc3Reg
  )
{
  UINT32     Revision;
  UINT32     Reg;
  UINTN      Dwc3Hwparams1;

  Revision = MmioRead32 ((UINTN)&Dwc3Reg->GSnpsId);
  //
  // This should read as 0x5533, ascii of U3(DWC_usb3) followed by revision num
  //
  if ((Revision & DWC3_GSNPSID_MASK) != DWC3_SYNOPSYS_ID) {
    DEBUG ((DEBUG_ERROR,"This is not a DesignWare USB3 DRD Core.\n"));
    return EFI_NOT_FOUND;
  }

  Dwc3CoreSoftReset (Dwc3Reg);

  Reg = MmioRead32 ((UINTN)&Dwc3Reg->GCtl);
  Reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
  Reg &= ~DWC3_GCTL_DISSCRAMBLE;

  Dwc3Hwparams1 = MmioRead32 ((UINTN)&Dwc3Reg->GHwParams1);

  if (DWC3_GHWPARAMS1_EN_PWROPT (Dwc3Hwparams1) ==
      DWC3_GHWPARAMS1_EN_PWROPT_CLK) {
    Reg &= ~DWC3_GCTL_DSBLCLKGTNG;
  } else {
    DEBUG ((DEBUG_WARN,"No power optimization available.\n"));
  }

  if ((Revision & DWC3_RELEASE_MASK) < DWC3_RELEASE_190a) {
    Reg |= DWC3_GCTL_U2RSTECN;
  }

  MmioWrite32 ((UINTN)&Dwc3Reg->GCtl, Reg);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
XhciCoreInit (
  IN  UINTN  UsbReg
  )
{
  EFI_STATUS Status;
  DWC3       *Dwc3Reg;

  Dwc3Reg = (VOID *)(UsbReg + DWC3_REG_OFFSET);

  Status = Dwc3CoreInit (Dwc3Reg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Dwc3CoreInit Failed for controller 0x%x (0x%r) \n",
      UsbReg, Status));

    return Status;
  }

  Dwc3SetMode (Dwc3Reg, DWC3_GCTL_PRTCAP_HOST);

  Dwc3SetFladj (Dwc3Reg, GFLADJ_30MHZ_DEFAULT);

  /* UTMI+ mode */
  MmioAndThenOr32 ((UINTN)&Dwc3Reg->GUsb2PhyCfg[0], ~DWC3_GUSB2PHYCFG_USBTRDTIM_MASK, DWC3_GUSB2PHYCFG_USBTRDTIM(5));
  MmioOr32 ((UINTN)&Dwc3Reg->GUsb2PhyCfg[0], DWC3_GUSB2PHYCFG_PHYIF);

  /* snps,dis_enblslpm_quirk */
  MmioAndThenOr32 ((UINTN)&Dwc3Reg->GUsb2PhyCfg[0], ~DWC3_GUSB2PHYCFG_ENBLSLPM, 0);
  /* snps,dis-u2-freeclk-exists-quirk */
  MmioAndThenOr32 ((UINTN)&Dwc3Reg->GUsb2PhyCfg[0], ~DWC3_GUSB2PHYCFG_U2_FREECLK_EXISTS, 0);
  /* snps,dis_u2_susphy_quirk */
  MmioAndThenOr32 ((UINTN)&Dwc3Reg->GUsb2PhyCfg[0], ~DWC3_GUSB2PHYCFG_SUSPHY, 0);
  /* snps,dis-del-phy-power-chg-quirk */
  MmioAndThenOr32 ((UINTN)&Dwc3Reg->GUsb3PipeCtl[0], ~DWC3_GUSB3PIPECTL_DEPOCHANGE, 0);
  /* snps,dis-tx-ipgap-linecheck-quirk */
  MmioOr32 ((UINTN)&Dwc3Reg->GUctl1, DWC3_GUCTL1_TX_IPGAP_LINECHECK_DIS);

  /* Set max speed */
  MmioAndThenOr32 ((UINTN)&Dwc3Reg->DCfg, ~DCFG_SPEED_MASK, DCFG_SPEED_SS);

  return Status;
}

EFIAPI
EFI_STATUS
InitializeXhciController (
  IN  NON_DISCOVERABLE_DEVICE *This
  )
{
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS UsbReg = This->Resources->AddrRangeMin;

  DEBUG ((DEBUG_ERROR, "XHCI: Initialize DWC3 at 0x%lX\n", UsbReg));

  Status = XhciCoreInit (UsbReg);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XHCI: Controller init Failed for 0x%lX (0x%r)\n",
      UsbReg, Status));
    return EFI_DEVICE_ERROR;
  }

  //
  // Change beat burst and outstanding pipelined transfers requests
  //
  XhciSetBeatBurstLength (UsbReg);

  return EFI_SUCCESS;
}

STATIC
VOID
UsbPhyEnable (
  VOID
  )
{
  USBPHY_GRF *GrfReg;
  
  /* Take USB3 OTG PHY out of suspend mode */
  GrfReg = (VOID *) PcdGet64 (PcdUsbPhyGrfBaseAddr);
  MmioWrite32 ((UINTN)&GrfReg->Con1, 0x01ff01d2);

  /* Take USB2 HOST0 and HOST1 PHYs out of suspend mode */
  GrfReg = (VOID *) PcdGet64 (PcdUsbPhyGrfBaseAddr) + 0x8000;
  MmioWrite32 ((UINTN)&GrfReg->Con0, 0x01ff01d2);
  MmioWrite32 ((UINTN)&GrfReg->Con1, 0x01ff01d2);
}

/**
  This function gets registered as a callback to perform USB controller intialization

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context.

**/
VOID
EFIAPI
UsbEndOfDxeCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS    Status;
  UINT32        NumUsb2Controller;
  UINT32        NumUsb3Controller;
  UINT32        XhciControllerAddr;
  UINT32        EhciControllerAddr;
  UINT32        OhciControllerAddr;
  UINT32        Index;

  gBS->CloseEvent (Event);

  NumUsb3Controller = PcdGet32 (PcdNumUsb3Controller);
  NumUsb2Controller = PcdGet32 (PcdNumUsb2Controller);

  /* Enable USB PHYs */
  UsbPhyEnable ();

  /* Register USB3 controllers */
  for (Index = 0; Index < NumUsb3Controller; Index++) {
    if ((Index == 0 && FixedPcdGet8(PcdXhc0Status) == 0x0) ||
        (Index == 1 && FixedPcdGet8(PcdXhc1Status) == 0x0)) {
      continue;
    }

    XhciControllerAddr = PcdGet64 (PcdUsb3BaseAddr) +
                          (Index * PcdGet32 (PcdUsb3Size));

    Status = RegisterNonDiscoverableMmioDevice (
               NonDiscoverableDeviceTypeXhci,
               NonDiscoverableDeviceDmaTypeNonCoherent,
               InitializeXhciController,
               NULL,
               1,
               XhciControllerAddr, PcdGet32 (PcdUsb3Size)
             );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to register XHCI device 0x%x, error 0x%r \n",
        XhciControllerAddr, Status));
    }
  }

  /* Register USB2 controllers */
  for (Index = 0; Index < NumUsb2Controller; Index++) {
    if ((Index == 0 && FixedPcdGet8(PcdEhc0Status) == 0x0) ||
        (Index == 1 && FixedPcdGet8(PcdEhc1Status) == 0x0)) {
      continue;
    }

    EhciControllerAddr = PcdGet64 (PcdUsb2BaseAddr) +
                          (Index * PcdGet32 (PcdUsb2Size));

    Status = RegisterNonDiscoverableMmioDevice (
               NonDiscoverableDeviceTypeEhci,
               NonDiscoverableDeviceDmaTypeNonCoherent,
               NULL,
               NULL,
               1,
               EhciControllerAddr, 0x10000
             );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to register EHCI device 0x%x, error 0x%r \n",
        EhciControllerAddr, Status));
    }
  }

  for (Index = 0; Index < NumUsb2Controller; Index++) {
    if ((Index == 0 && FixedPcdGet8(PcdOhc0Status) == 0x0) ||
        (Index == 1 && FixedPcdGet8(PcdOhc1Status) == 0x0)) {
      continue;
    }

    OhciControllerAddr = PcdGet64 (PcdUsb2BaseAddr) +
                          (Index * PcdGet32 (PcdUsb2Size)) +
                          0x10000;

    Status = RegisterNonDiscoverableMmioDevice (
               NonDiscoverableDeviceTypeOhci,
               NonDiscoverableDeviceDmaTypeNonCoherent,
               NULL,
               NULL,
               1,
               OhciControllerAddr, 0x10000
             );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to register OHCI device 0x%x, error 0x%r \n",
        OhciControllerAddr, Status));
    }
  }
}

/**
  The Entry Point of module. It follows the standard UEFI driver model.

  @param[in] ImageHandle   The firmware allocated handle for the EFI image.
  @param[in] SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The entry point is executed successfully.
  @retval other            Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUsbHcd (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
{
  EFI_STATUS               Status;
  EFI_EVENT                EndOfDxeEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  UsbEndOfDxeCallback,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );

  return Status;
}
