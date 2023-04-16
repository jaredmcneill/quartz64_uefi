/** @file
 *
 *  Copyright (c) 2023, Jared McNeill <jmcneill@invisible.ca>
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
#include <Library/CpuVoltageLib.h>
#include <Library/GpioLib.h>
#include <IndustryStandard/Rk356x.h>

#define I2C_BUS_BASE            FixedPcdGet32 (PcdCpuVoltageI2cBusBase)
#define I2C_ADDR                FixedPcdGet8 (PcdCpuVoltageI2cAddr)
#define VSEL_REG                FixedPcdGet8 (PcdCpuVoltageVselReg)
#define VOLTAGE_BASE            FixedPcdGet32 (PcdCpuVoltageUVolBase)
#define VOLTAGE_STEP            FixedPcdGet32 (PcdCpuVoltageUVolStep)
#define RAMP_DELAY              FixedPcdGet32 (PcdCpuVoltageRampDelay)

#define ID1_REG                 0x03
#define  ID1_VENDOR_SHIFT       5
#define  ID1_VENDOR_MASK        (0x7 << ID1_VENDOR_SHIFT)

#define VSEL_NSEL_MASK          0x3f
#define VSEL_MODE               BIT6
#define VSEL_BUCK_EN            BIT7

typedef struct CpuVoltageOpp {
    UINT64  Hz;
    UINT64  UVol;
} CpuVoltageOpp;

STATIC CpuVoltageOpp mCpuVoltageOpp[] = {
    { 1200000000,   825000 },
    { 1416000000,   900000 },
    { 1608000000,   975000 },
    { 1800000000,   1050000 },
    { 1992000000,   1150000 },
    { 0 }
};

STATIC
EFI_STATUS
CpuVoltageReadReg (
    IN UINT8    Register,
    OUT UINT8   *Value
    )
{
    return I2cRead (I2C_BUS_BASE, I2C_ADDR,
                    &Register, sizeof (Register),
                    Value, sizeof (*Value));
}

STATIC
EFI_STATUS
CpuVoltageWriteReg (
    IN UINT8    Register,
    IN UINT8    Value
    )
{
    return I2cWrite (I2C_BUS_BASE, I2C_ADDR,
                     &Register, sizeof (Register),
                     &Value, sizeof (Value));
}

STATIC
EFI_STATUS
CpuVoltageHzToUVol (
    IN UINT64   Hz,
    OUT UINT32  *UVol
    )
{
    for (UINTN Index = 0; mCpuVoltageOpp[Index].Hz; Index++) {
        if (Hz <= mCpuVoltageOpp[Index].Hz) {
            *UVol = mCpuVoltageOpp[Index].UVol;
            return EFI_SUCCESS;
        }
    }

    /* Not found */
    return EFI_UNSUPPORTED;
}

EFI_STATUS
CpuVoltageSet (
    IN UINT64 Hz
    )
{
    EFI_STATUS  Status;
    UINT8       Value, OldValue;
    UINT32      CurUVol;
    UINT32      NewUVol;

    Status = CpuVoltageHzToUVol (Hz, &NewUVol);
    if (Status != EFI_SUCCESS) {
        DEBUG ((DEBUG_ERROR, "CpuVoltageSet: Unsupported rate %lu Hz\n", Hz));
        return Status;
    }

    Status = CpuVoltageReadReg (VSEL_REG, &OldValue);
    ASSERT (Status == EFI_SUCCESS);

    CurUVol = (OldValue & VSEL_NSEL_MASK) * VOLTAGE_STEP + VOLTAGE_BASE;
    DEBUG ((DEBUG_INFO, "CpuVoltageSet: %u uV -> %u uV\n", CurUVol, NewUVol));

    if (CurUVol == NewUVol) {
        return EFI_SUCCESS;
    }

    Value = OldValue & ~VSEL_NSEL_MASK;
    Value |= (NewUVol - VOLTAGE_BASE) / VOLTAGE_STEP;
    Status = CpuVoltageWriteReg (VSEL_REG, Value);

    if (RAMP_DELAY) {
        if (CurUVol > NewUVol) {
            gBS->Stall (((CurUVol - NewUVol) / VOLTAGE_STEP) * RAMP_DELAY);
        } else {
            gBS->Stall (((NewUVol - CurUVol) / VOLTAGE_STEP) * RAMP_DELAY);
        }
    }

    return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
CpuVoltageLibConstructor (
    VOID
    )
{
    ASSERT (I2C_BUS_BASE != 0);
    ASSERT (I2C_ADDR != 0);

    /* XXX */
    ASSERT (I2C_BUS_BASE == I2C0_BASE);
    GpioPinSetPull (0, GPIO_PIN_PB1, GPIO_PIN_PULL_NONE);
    GpioPinSetInput (0, GPIO_PIN_PB1, GPIO_PIN_INPUT_SCHMITT);
    GpioPinSetFunction (0, GPIO_PIN_PB1, 1);
    GpioPinSetPull (0, GPIO_PIN_PB2, GPIO_PIN_PULL_NONE);
    GpioPinSetInput (0, GPIO_PIN_PB2, GPIO_PIN_INPUT_SCHMITT);
    GpioPinSetFunction (0, GPIO_PIN_PB2, 1);

    return EFI_SUCCESS;
}