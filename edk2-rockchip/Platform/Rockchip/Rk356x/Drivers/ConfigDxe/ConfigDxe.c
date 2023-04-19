/** @file
 *
 *  Copyright (c) 2023, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2019 - 2020, ARM Limited. All rights reserved.
 *  Copyright (c) 2018 - 2020, Andrei Warkentin <andrey.warkentin@gmail.com>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/CpuVoltageLib.h>
#include <Library/GpioLib.h>
#include <Protocol/ArmScmi.h>
#include <Protocol/ArmScmiClockProtocol.h>
#include <ConfigVars.h>
#include "ConfigDxeFormSetGuid.h"
#include "ConfigDxe.h"

#define CLOCK_ID_CLK_SCMI_CPU     0
#define FREQ_1_MHZ                1000000
#define FAN_GPIO_BANK             FixedPcdGet8 (PcdFanGpioBank)
#define FAN_GPIO_PIN              FixedPcdGet8 (PcdFanGpioPin)
#define FAN_GPIO_ENABLE_VALUE     FixedPcdGetBool (PcdFanGpioActiveHigh)

extern UINT8 ConfigDxeHiiBin[];
extern UINT8 ConfigDxeStrings[];

typedef struct {
  VENDOR_DEVICE_PATH VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL End;
} HII_VENDOR_DEVICE_PATH;

STATIC HII_VENDOR_DEVICE_PATH mVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    CONFIGDXE_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

STATIC EFI_STATUS
InstallHiiPages (
  VOID
  )
{
  EFI_STATUS     Status;
  EFI_HII_HANDLE HiiHandle;
  EFI_HANDLE     DriverHandle;

  DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (&DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mVendorDevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiHandle = HiiAddPackages (&gConfigDxeFormSetGuid,
                DriverHandle,
                ConfigDxeStrings,
                ConfigDxeHiiBin,
                NULL);

  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mVendorDevicePath,
           NULL);
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetMinMaxCpuSpeed (
  IN BOOLEAN             MinSpeed,
  OUT UINT64             *Speed
  )
{
  EFI_STATUS             Status;
  SCMI_CLOCK_PROTOCOL    *ClockProtocol;
  EFI_GUID               ClockProtocolGuid = ARM_SCMI_CLOCK_PROTOCOL_GUID;
  UINT32                 TotalRates = 0;
  UINT32                 ClockRateSize;
  SCMI_CLOCK_RATE        *ClockRate;
  SCMI_CLOCK_RATE_FORMAT ClockRateFormat;
  UINT32                 ClockId = CLOCK_ID_CLK_SCMI_CPU;

  Status = gBS->LocateProtocol (
                  &ClockProtocolGuid,
                  NULL,
                  (VOID**)&ClockProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  TotalRates = 0;
  ClockRateSize = 0;
  Status = ClockProtocol->DescribeRates (
                            ClockProtocol,
                            ClockId,
                            &ClockRateFormat,
                            &TotalRates,
                            &ClockRateSize,
                            ClockRate
                            );
  if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  ASSERT (TotalRates > 0);
  ASSERT (ClockRateFormat == ScmiClockRateFormatDiscrete);
  if (Status != EFI_BUFFER_TOO_SMALL ||
      TotalRates == 0 ||
      ClockRateFormat != ScmiClockRateFormatDiscrete) {
    return EFI_DEVICE_ERROR;
  }
  
  ClockRateSize = sizeof (*ClockRate) * TotalRates;
  ClockRate = AllocatePool (ClockRateSize);
  ASSERT (ClockRate != NULL);
  if (ClockRate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = ClockProtocol->DescribeRates (
                            ClockProtocol,
                            ClockId,
                            &ClockRateFormat,
                            &TotalRates,
                            &ClockRateSize,
                            ClockRate
                            );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    FreePool (ClockRate);
    return Status;
  }

  for (UINTN x = 0; x < TotalRates; x++) {
    DEBUG ((DEBUG_INFO, "SCMI: clock %u: Available rate %u is %uHz\n", ClockId, x, ClockRate[x].DiscreteRate.Rate));
  }

  *Speed = ClockRate[MinSpeed ? 0 : TotalRates - 1].DiscreteRate.Rate;
  FreePool (ClockRate);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetCpuSpeed (
  IN UINT64 CpuRate
  )
{
  EFI_STATUS             Status;
  SCMI_CLOCK_PROTOCOL    *ClockProtocol;
  EFI_GUID               ClockProtocolGuid = ARM_SCMI_CLOCK_PROTOCOL_GUID;
  UINT32                 ClockId = CLOCK_ID_CLK_SCMI_CPU;

  Status = gBS->LocateProtocol (
                  &ClockProtocolGuid,
                  NULL,
                  (VOID**)&ClockProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "SCMI: clock %u: New rate is %uHz\n", ClockId, CpuRate));

  Status = ClockProtocol->RateSet (
                            ClockProtocol,
                            ClockId,
                            CpuRate
                            );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = ClockProtocol->RateGet (ClockProtocol, ClockId, &CpuRate);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "SCMI: clock %u: Current rate is %uHz\n", ClockId, CpuRate));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetCpuSpeed (
  OUT UINT64 *CpuRate
  )
{
  EFI_STATUS             Status;
  SCMI_CLOCK_PROTOCOL    *ClockProtocol;
  EFI_GUID               ClockProtocolGuid = ARM_SCMI_CLOCK_PROTOCOL_GUID;
  UINT32                 ClockId = CLOCK_ID_CLK_SCMI_CPU;

  Status = gBS->LocateProtocol (
                  &ClockProtocolGuid,
                  NULL,
                  (VOID**)&ClockProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = ClockProtocol->RateGet (ClockProtocol, ClockId, CpuRate);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "SCMI: clock %u: Current rate is %uHz\n", ClockId, *CpuRate));

  return EFI_SUCCESS;
}

STATIC EFI_STATUS
SetupVariables (
  VOID
  )
{
  UINTN      Size;
  UINT32     Var32;
#if FAN_GPIO_BANK != 0xFF
  BOOLEAN    VarBool;
#endif
  EFI_STATUS Status;

  /*
   * Create the vars with default value.
   * If we don't, forms won't be able to update.
   */

  Size = sizeof (UINT32);
  Status = gRT->GetVariable (L"SystemTableMode",
                  &gConfigDxeFormSetGuid,
                  NULL, &Size, &Var32);
  if (EFI_ERROR (Status)) {
    Status = PcdSet32S (PcdSystemTableMode, PcdGet32 (PcdSystemTableMode));
    ASSERT_EFI_ERROR (Status);
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable (L"CpuClock",
                  &gConfigDxeFormSetGuid,
                  NULL, &Size, &Var32);
  if (EFI_ERROR (Status)) {
    Status = PcdSet32S (PcdCpuClock, PcdGet32 (PcdCpuClock));
    ASSERT_EFI_ERROR (Status);
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable (L"CustomCpuClock",
                             &gConfigDxeFormSetGuid,
                             NULL, &Size, &Var32);
  if (EFI_ERROR (Status)) {
    Status = PcdSet32S (PcdCustomCpuClock, PcdGet32 (PcdCustomCpuClock));
    ASSERT_EFI_ERROR (Status);
  }

#ifdef QUARTZ64
  Size = sizeof (UINT32);
  Status = gRT->GetVariable (L"MultiPhy1Mode",
                             &gConfigDxeFormSetGuid,
                             NULL, &Size, &Var32);
  if (EFI_ERROR (Status)) {
    Status = PcdSet32S (PcdMultiPhy1Mode, PcdGet32 (PcdMultiPhy1Mode));
    ASSERT_EFI_ERROR (Status);
  }
#endif

#if FAN_GPIO_BANK != 0xFF
  ASSERT (FAN_GPIO_PIN != 0xFF);
  Size = sizeof (BOOLEAN);
  Status = gRT->GetVariable (L"FanMode",
                             &gConfigDxeFormSetGuid,
                             NULL, &Size, &VarBool);
  if (EFI_ERROR (Status)) {
    Status = PcdSetBoolS (PcdFanMode, PcdGetBool (PcdFanMode));
    ASSERT_EFI_ERROR (Status);
  }
#endif

  return EFI_SUCCESS;
}


STATIC VOID
ApplyVariables (
  VOID
  )
{
  EFI_STATUS Status;
  UINT32     CpuClock = PcdGet32 (PcdCpuClock);
  UINT32     CustomCpuClock = PcdGet32 (PcdCustomCpuClock);
  UINT64     SpeedHz;
  UINT64     CurSpeedHz;

#if FAN_GPIO_BANK != 0xFF
  /*
   * Fan settings
   */
  if (PcdGetBool (PcdFanMode)) {
    GpioPinSetDirection (FAN_GPIO_BANK, FAN_GPIO_PIN, GPIO_PIN_OUTPUT);
    GpioPinWrite (FAN_GPIO_BANK, FAN_GPIO_PIN, FAN_GPIO_ENABLE_VALUE);
  }
#endif

  /*
   * CPU clock settings
   */
  switch (CpuClock) {
  case CPUCLOCK_DEFAULT:
    SpeedHz = 0;
    break;
  case CPUCLOCK_LOW:
  case CPUCLOCK_MAX:
    Status = GetMinMaxCpuSpeed (CpuClock == CPUCLOCK_LOW, &SpeedHz);
    if (EFI_ERROR (Status)) {
      SpeedHz = 0;
    }
    break;
  case CPUCLOCK_CUSTOM:
    SpeedHz = (UINT64)CustomCpuClock * FREQ_1_MHZ;
    break;
  }

  Status = GetCpuSpeed (&CurSpeedHz);
  if (EFI_ERROR (Status)) {
    SpeedHz = 0;
  }

  if (SpeedHz != 0) {
    if (CurSpeedHz < SpeedHz) {
      Status = CpuVoltageSet (SpeedHz);
    }
    SetCpuSpeed (SpeedHz);
    if (CurSpeedHz > SpeedHz) {
      Status = CpuVoltageSet (SpeedHz);
    }
  }
}


STATIC
VOID
EFIAPI
RemoveTables (
  EFI_EVENT           Event,
  VOID                *Context
  )
{
  switch (PcdGet32 (PcdSystemTableMode)) {
  case SYSTEM_TABLE_MODE_BOTH:
  case SYSTEM_TABLE_MODE_ACPI:
    break;
  default:
    DEBUG ((DEBUG_ERROR, "Removing ACPI tables (user config)\n"));
    gBS->InstallConfigurationTable (&gEfiAcpiTableGuid, NULL);
    gBS->InstallConfigurationTable (&gEfiAcpi10TableGuid, NULL);
    break;
  }  
}


EFI_STATUS
EFIAPI
ConfigInitialize (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_EVENT                       EndOfDxeEvent;

  Status = SetupVariables ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Couldn't not setup NV vars: %r\n", Status));
  }

  ApplyVariables ();

  Status = InstallHiiPages ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Couldn't install ConfigDxe configuration pages: %r\n", Status));
  }

  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, RemoveTables,
                               NULL, &gEfiEndOfDxeEventGroupGuid, &EndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, RemoveTables,
                               NULL, &gEfiAcpiTableGuid, &EndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);
  
  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, RemoveTables,
                               NULL, &gEfiAcpi10TableGuid, &EndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
