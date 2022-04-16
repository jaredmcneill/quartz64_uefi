/** @file
 *
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <PiDxe.h>
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/I2cLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>


#define RTC_I2C_ADDR            FixedPcdGet8 (PcdRtcI2cAddr)

#define RTC_CS1                 0x00
#define RTC_CS2                 0x01
#define RTC_SECONDS             0x02
#define  RTC_SECONDS_DL         BIT7
#define  RTC_SECONDS_MASK       0x7F
#define RTC_MINUTES             0x03
#define  RTC_MINUTES_MASK       0x7F
#define RTC_HOURS               0x04
#define  RTC_HOURS_MASK         0x3F
#define RTC_DAYS                0x05
#define  RTC_DAYS_MASK          0x3F
#define RTC_WEEKDAYS            0x06
#define  RTC_WEEKDAYS_MASK      0x07
#define RTC_MON_CENTURY         0x07
#define  RTC_MON_MASK           0x1F
#define  RTC_CENTURY_MASK       0x80
#define RTC_YEARS               0x08

STATIC UINTN                    mRtcI2cBusBase = FixedPcdGet32 (PcdRtcI2cBusBase);
STATIC EFI_EVENT                mVirtualAddressChangeEvent = NULL;
STATIC BOOLEAN                  mRuntimeEnable = FALSE;

STATIC
EFI_STATUS
RtcRead (
    IN UINT8    Register,
    OUT UINT8   *Value
    )
{
    return I2cRead (mRtcI2cBusBase, RTC_I2C_ADDR,
                    &Register, sizeof (Register),
                    Value, sizeof (*Value));
}

STATIC
EFI_STATUS
RtcWrite (
    IN UINT8    Register,
    IN UINT8    Value
    )
{
    return I2cWrite (mRtcI2cBusBase, RTC_I2C_ADDR,
                     &Register, sizeof (Register),
                     &Value, sizeof (Value));
}

EFI_STATUS
EFIAPI
LibGetTime (
    OUT EFI_TIME                *Time,
    OUT EFI_TIME_CAPABILITIES   *Capabilities
    )
{
    EFI_STATUS Status;
    UINT8 Seconds, Minutes, Hours, Days, MonCentury, Years;

    Status = RtcRead (RTC_SECONDS, &Seconds);
    if (!EFI_ERROR (Status)) {
        Status = RtcRead (RTC_MINUTES, &Minutes);
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcRead (RTC_HOURS, &Hours);
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcRead (RTC_DAYS, &Days);
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcRead (RTC_MON_CENTURY, &MonCentury);
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcRead (RTC_YEARS, &Years);
    }
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "RTC read failed: %r\n", Status));
        return EFI_DEVICE_ERROR;
    }

    if ((Seconds & RTC_SECONDS_DL) != 0) {
        DEBUG ((DEBUG_WARN, "RTC voltage-low detect bit set; clock integrity not guaranteed.\n"));
        RtcWrite (RTC_SECONDS, Seconds & ~RTC_SECONDS_DL);
        return EFI_DEVICE_ERROR;
    }

    Time->Year = 2000 + BcdToDecimal8 (Years);
    Time->Month = BcdToDecimal8 (MonCentury & RTC_MON_MASK);
    Time->Day = BcdToDecimal8 (Days & RTC_DAYS_MASK);
    Time->Hour = BcdToDecimal8 (Hours & RTC_HOURS_MASK);
    Time->Minute = BcdToDecimal8 (Minutes & RTC_MINUTES_MASK);
    Time->Second = BcdToDecimal8 (Seconds & RTC_SECONDS_MASK);
    Time->TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    Time->Daylight = 0;

    if (Time->Month == 0) {
        Time->Month = 1;
    }
    if (Time->Day == 0) {
        Time->Day = 1;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LibSetTime (
    IN EFI_TIME     *Time
    )
{
    EFI_STATUS Status;

    DEBUG ((DEBUG_INFO, "LibSetTime() called\n"));

    if (Time->Year < 2000 || Time->Year > 2099) {
        DEBUG ((DEBUG_ERROR, "WARNING: Year should be between 2000 and 2099\n"));
        return EFI_INVALID_PARAMETER;
    }

    Status = RtcWrite (RTC_YEARS, DecimalToBcd8 (Time->Year % 100));
    if (!EFI_ERROR (Status)) {
        Status = RtcWrite (RTC_MON_CENTURY, DecimalToBcd8 (Time->Month));
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcWrite (RTC_DAYS, DecimalToBcd8 (Time->Day));
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcWrite (RTC_HOURS, DecimalToBcd8 (Time->Hour));
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcWrite (RTC_MINUTES, DecimalToBcd8 (Time->Minute));
    }
    if (!EFI_ERROR (Status)) {
        Status = RtcWrite (RTC_SECONDS, DecimalToBcd8 (Time->Second));
    }
    if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LibGetWakeupTime (
    OUT BOOLEAN     *Enabled,
    OUT BOOLEAN     *Pending,
    OUT EFI_TIME    *Time
    )
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LibSetWakeupTime (
    IN BOOLEAN      Enabled,
    IN EFI_TIME     *Time
    )
{
    return EFI_UNSUPPORTED;
}

VOID
EFIAPI
RtcVirtualAddressChangeEvent (
    IN EFI_EVENT    Event,
    IN VOID         *Context
    )
{
    if (!mRuntimeEnable) {
        return;
    }
    EfiConvertPointer (0x0, (VOID **)&mRtcI2cBusBase);
}

EFI_STATUS
EFIAPI
LibRtcInitialize (
    IN EFI_HANDLE            ImageHandle,
    IN EFI_SYSTEM_TABLE      *SystemTable
    )
{
    return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
RtcLibConstructor (
    VOID
    )
{
    EFI_STATUS Status = EFI_SUCCESS;

    ASSERT ((mRtcI2cBusBase != 0));
    ASSERT ((RTC_I2C_ADDR != 0));

    if (mVirtualAddressChangeEvent == NULL) {
        Status = gBS->CreateEventEx (
                        EVT_NOTIFY_SIGNAL,
                        TPL_NOTIFY,
                        RtcVirtualAddressChangeEvent,
                        NULL,
                        &gEfiEventVirtualAddressChangeGuid,
                        &mVirtualAddressChangeEvent
                        );
        ASSERT_EFI_ERROR (Status);
    }

    Status = gDS->AddMemorySpace (
        EfiGcdMemoryTypeMemoryMappedIo,
        mRtcI2cBusBase, SIZE_64KB,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
        );
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add memory space: %r\n", Status));
        return Status;
    }

    Status = gDS->SetMemorySpaceAttributes (
        mRtcI2cBusBase, SIZE_64KB,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
        );
    ASSERT_EFI_ERROR (Status);

    mRuntimeEnable = TRUE;

    return Status;
}