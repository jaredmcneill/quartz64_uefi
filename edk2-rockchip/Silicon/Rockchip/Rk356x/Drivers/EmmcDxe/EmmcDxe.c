 /** @file
  Emmc DXE platform driver - eMMC support

  Copyright (c) 2022, Patrick Wildt <patrick@blueri.se>
  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/CruLib.h>
#include <Library/GpioLib.h>

#include <Protocol/NonDiscoverableDevice.h>
#include <Protocol/SdMmcOverride.h>

#include "Emmc.h"

#define EMMC_FORCE_HIGH_SPEED   FixedPcdGetBool(PcdEmmcForceHighSpeed)

typedef struct {
  UINT32    TimeoutFreq   : 6; // bit 0:5
  UINT32    Reserved      : 1; // bit 6
  UINT32    TimeoutUnit   : 1; // bit 7
  UINT32    BaseClkFreq   : 8; // bit 8:15
  UINT32    MaxBlkLen     : 2; // bit 16:17
  UINT32    BusWidth8     : 1; // bit 18
  UINT32    Adma2         : 1; // bit 19
  UINT32    Reserved2     : 1; // bit 20
  UINT32    HighSpeed     : 1; // bit 21
  UINT32    Sdma          : 1; // bit 22
  UINT32    SuspRes       : 1; // bit 23
  UINT32    Voltage33     : 1; // bit 24
  UINT32    Voltage30     : 1; // bit 25
  UINT32    Voltage18     : 1; // bit 26
  UINT32    SysBus64V4    : 1; // bit 27
  UINT32    SysBus64V3    : 1; // bit 28
  UINT32    AsyncInt      : 1; // bit 29
  UINT32    SlotType      : 2; // bit 30:31
  UINT32    Sdr50         : 1; // bit 32
  UINT32    Sdr104        : 1; // bit 33
  UINT32    Ddr50         : 1; // bit 34
  UINT32    Reserved3     : 1; // bit 35
  UINT32    DriverTypeA   : 1; // bit 36
  UINT32    DriverTypeC   : 1; // bit 37
  UINT32    DriverTypeD   : 1; // bit 38
  UINT32    DriverType4   : 1; // bit 39
  UINT32    TimerCount    : 4; // bit 40:43
  UINT32    Reserved4     : 1; // bit 44
  UINT32    TuningSDR50   : 1; // bit 45
  UINT32    RetuningMod   : 2; // bit 46:47
  UINT32    ClkMultiplier : 8; // bit 48:55
  UINT32    Reserved5     : 7; // bit 56:62
  UINT32    Hs400         : 1; // bit 63
} SD_MMC_HC_SLOT_CAP;

STATIC CONST GPIO_IOMUX_CONFIG mEmmcIomuxConfig[] = {
  { "emmc_d0",            1, GPIO_PIN_PB4, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_d1",            1, GPIO_PIN_PB5, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_d2",            1, GPIO_PIN_PB6, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_d3",            1, GPIO_PIN_PB7, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_d4",            1, GPIO_PIN_PC0, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_d5",            1, GPIO_PIN_PC1, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_d6",            1, GPIO_PIN_PC2, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_d7",            1, GPIO_PIN_PC3, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_cmd",           1, GPIO_PIN_PC4, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_clkout",        1, GPIO_PIN_PC5, 1, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "emmc_datastrobe",    1, GPIO_PIN_PC6, 1, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
};

STATIC EFI_HANDLE mSdMmcControllerHandle;

/**
  Override function for SDHCI capability bits

  @param[in]      ControllerHandle      The EFI_HANDLE of the controller.
  @param[in]      Slot                  The 0 based slot index.
  @param[in,out]  SdMmcHcSlotCapability The SDHCI capability structure.
  @param[in,out]  BaseClkFreq           The base clock frequency value that
                                        optionally can be updated.

  @retval EFI_SUCCESS           The override function completed successfully.
  @retval EFI_NOT_FOUND         The specified controller or slot does not exist.
  @retval EFI_INVALID_PARAMETER SdMmcHcSlotCapability is NULL

**/
STATIC
EFI_STATUS
EFIAPI
EmmcSdMmcCapability (
  IN      EFI_HANDLE                      ControllerHandle,
  IN      UINT8                           Slot,
  IN OUT  VOID                            *SdMmcHcSlotCapability,
  IN OUT  UINT32                          *BaseClkFreq
  )
{
  SD_MMC_HC_SLOT_CAP *Capability = SdMmcHcSlotCapability;

  if (SdMmcHcSlotCapability == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (ControllerHandle != mSdMmcControllerHandle) {
    return EFI_NOT_FOUND;
  }

  if (EMMC_FORCE_HIGH_SPEED) {
    Capability->BaseClkFreq = 52;
    Capability->Sdr50 = 0;
    Capability->Ddr50 = 0;
    Capability->Sdr104 = 0;
    Capability->Hs400 = 0;
  }

  return EFI_SUCCESS;
}

/**

  Override function for SDHCI controller operations

  @param[in]      ControllerHandle      The EFI_HANDLE of the controller.
  @param[in]      Slot                  The 0 based slot index.
  @param[in]      PhaseType             The type of operation and whether the
                                        hook is invoked right before (pre) or
                                        right after (post)
  @param[in,out]  PhaseData             The pointer to a phase-specific data.

  @retval EFI_SUCCESS           The override function completed successfully.
  @retval EFI_NOT_FOUND         The specified controller or slot does not exist.
  @retval EFI_INVALID_PARAMETER PhaseType is invalid

**/
STATIC
EFI_STATUS
EFIAPI
EmmcSdMmcNotifyPhase (
  IN      EFI_HANDLE                      ControllerHandle,
  IN      UINT8                           Slot,
  IN      EDKII_SD_MMC_PHASE_TYPE         PhaseType,
  IN OUT  VOID                           *PhaseData
  )
{
  SD_MMC_BUS_MODE                 *Timing;
  UINTN                            MaxClockFreq;
  UINT32                           Value, i;

  DEBUG ((DEBUG_INFO, "EmmcSdMmcNotifyPhase()\n"));

  if (ControllerHandle != mSdMmcControllerHandle) {
    return EFI_SUCCESS;
  }

  ASSERT (Slot == 0);

  switch (PhaseType) {
  case EdkiiSdMmcInitHostPost:
    /*
     * Just before this Notification POWER_CTRL is toggled to power off
     * and on the card.  On this controller implementation, toggling
     * power off also removes SDCLK_ENABLE (BIT2) from from CLOCK_CTRL.
     * Since the clock has already been set up prior to the power toggle,
     * re-add the SDCLK_ENABLE bit to start the clock.
     */
    MmioOr16((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) +
      SD_MMC_HC_CLOCK_CTRL, BIT2);

    /*
     * Enable back-end power supply to card.
     */
    MmioOr32 (SDMMC_BACKEND_POWER, BIT0);
    break;

  case EdkiiSdMmcUhsSignaling:
    if (PhaseData == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Timing = (SD_MMC_BUS_MODE *)PhaseData;
    if (*Timing == SdMmcMmcHs400) {
      /* HS400 uses a non-standard setting */
      MmioOr16((UINT32)PcdGet32 (PcdEmmcDxeBaseAddress) +
        SD_MMC_HC_HOST_CTRL2, BIT2|BIT1|BIT0);
    }
    break;

  case EdkiiSdMmcSwitchClockFreqPost:
    if (PhaseData == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Timing = (SD_MMC_BUS_MODE *)PhaseData;
    switch (*Timing) {
    case SdMmcMmcHs400:
    case SdMmcMmcHs200:
      MaxClockFreq = 200000000UL;
      break;
    case SdMmcMmcHsSdr:
    case SdMmcMmcHsDdr:
      MaxClockFreq = 52000000UL;
      break;
    default:
      MaxClockFreq = 26000000UL;
      break;
    }

    CruSetEmmcClockRate(MaxClockFreq);

    if (MaxClockFreq <= 52000000UL) {
      MmioWrite32 (EMMC_DLL_CTRL, 0);
      MmioWrite32 (EMMC_DLL_RXCLK, BIT29);
      MmioWrite32 (EMMC_DLL_TXCLK, 0);
      MmioWrite32 (EMMC_DLL_STRBIN, 0);
      break;
    }

    MmioWrite32(EMMC_DLL_CTRL, EMMC_DLL_CTRL_START);
    gBS->Stall (1);
    MmioWrite32(EMMC_DLL_CTRL, 0);

    MmioWrite32(EMMC_DLL_CTRL, EMMC_DLL_CTRL_START_POINT_DEFAULT |
      EMMC_DLL_CTRL_INCREMENT_DEFAULT | EMMC_DLL_CTRL_START);

    for (i = 0; i < 500; i++) {
      Value = MmioRead32(EMMC_DLL_STATUS0);
      if (Value & EMMC_DLL_STATUS0_DLL_LOCK &&
          !(Value & EMMC_DLL_STATUS0_DLL_TIMEOUT))
        break;
      gBS->Stall (1);
    }

    MmioWrite32(EMMC_DLL_RXCLK, EMMC_DLL_RXCLK_DLYENA |
      EMMC_DLL_RXCLK_NO_INVERTER);
    MmioWrite32(EMMC_DLL_TXCLK, EMMC_DLL_TXCLK_DLYENA |
      EMMC_DLL_TXCLK_TAPNUM_DEFAULT | EMMC_DLL_TXCLK_TAPNUM_FROM_SW);
    MmioWrite32(EMMC_DLL_STRBIN, EMMC_DLL_TXCLK_DLYENA |
      EMMC_DLL_STRBIN_TAPNUM_DEFAULT);
    break;

  default:
    break;
  }
  return EFI_SUCCESS;
}

STATIC EDKII_SD_MMC_OVERRIDE mSdMmcOverride = {
  EDKII_SD_MMC_OVERRIDE_PROTOCOL_VERSION,
  EmmcSdMmcCapability,
  EmmcSdMmcNotifyPhase,
};

EFI_STATUS
EFIAPI
EmmcDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      Handle;

  /* Start card on 375 kHz */
  CruSetEmmcClockRate (375000UL);

  DEBUG ((DEBUG_BLKIO, "EmmcDxeInitialize()\n"));

  /* Configure pins */
  GpioSetIomuxConfig (mEmmcIomuxConfig, ARRAY_SIZE (mEmmcIomuxConfig));

  /* Disable Command Conflict Check */
  MmioWrite32 (DWCMSHC_HOST_CTRL3, 0);

  /* Switch to eMMC mode */
  MmioOr32 (EMMC_EMMC_CTRL, BIT0);

  /* Disable DLL for identification */
  MmioWrite32 (EMMC_DLL_CTRL, 0);
  MmioWrite32 (EMMC_DLL_RXCLK, BIT29);
  MmioWrite32 (EMMC_DLL_TXCLK, 0);
  MmioWrite32 (EMMC_DLL_STRBIN, 0);

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeSdhci,
             NonDiscoverableDeviceDmaTypeNonCoherent,
             NULL,
             &mSdMmcControllerHandle,
             1,
             PcdGet32 (PcdEmmcDxeBaseAddress), 0x10000);
  ASSERT_EFI_ERROR (Status);

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (&Handle,
                  &gEdkiiSdMmcOverrideProtocolGuid,
                  EFI_NATIVE_INTERFACE, (VOID **)&mSdMmcOverride);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
