/** @file
 *
 *  RK3566/RK3568 OTP Library.
 * 
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/OtpLib.h>
#include <IndustryStandard/Rk356x.h>

#define OTPC_SBPI_CTRL                (OTP_BASE + 0x0020)
#define  SBPI_DEVICE_ID_SHIFT         8
#define  SBPI_DEVICE_ID_MASK          (0xFFU << SBPI_DEVICE_ID_SHIFT)
#define  SBPI_ENABLE                  BIT0
#define OTPC_SBPI_CMD_VALID_PRELOAD   (OTP_BASE + 0x0024)
#define OTPC_USER_CTRL                (OTP_BASE + 0x0100)
#define  USER_DCTRL                   BIT0
#define OTPC_USER_ADDR                (OTP_BASE + 0x0104)
#define OTPC_USER_ENABLE              (OTP_BASE + 0x0108)
#define OTPC_USER_Q                   (OTP_BASE + 0x0124)
#define OTPC_INT_STATUS               (OTP_BASE + 0x0304)
#define  USER_DONE_INT_STATUS         BIT2
#define  SBPI_DONE_INT_STATUS         BIT1
#define OTPC_SBPI_CMD_BASE(n)         (OTP_BASE + 0x1000 + 0x4 * (n))

STATIC
VOID
OtpDisableEcc (
  VOID
  )
{
  UINTN Retry = 100000;

  MmioWrite32 (OTPC_SBPI_CTRL, (SBPI_DEVICE_ID_MASK << 16) | (2 << SBPI_DEVICE_ID_SHIFT));
  MmioWrite32 (OTPC_SBPI_CMD_VALID_PRELOAD, (0xFFFFU << 16) | 1);
  MmioWrite32 (OTPC_SBPI_CMD_BASE (0), 0xFA);
  MmioWrite32 (OTPC_SBPI_CMD_BASE (1), 0x09);
  MmioWrite32 (OTPC_SBPI_CTRL, (SBPI_ENABLE << 16) | SBPI_ENABLE);
  while ((MmioRead32 (OTPC_INT_STATUS) & SBPI_DONE_INT_STATUS) == 0) {
      MicroSecondDelay (1);
      if (--Retry == 0) {
        DEBUG ((DEBUG_WARN, "OTP ECC disable timeout!\n"));
        break;
      }
    }
    MmioWrite32 (OTPC_INT_STATUS, SBPI_DONE_INT_STATUS);
}

VOID
OtpRead (
  IN UINT16 Offset,
  IN UINT16 Length,
  OUT UINT8 *Data
  )
{
  UINT16 Addr;
  UINT16 BytesRemaining;
  UINT32 Value;
  UINTN Retry = 100000;

  OtpDisableEcc ();

  MmioWrite32 (OTPC_USER_CTRL, (USER_DCTRL << 16) | USER_DCTRL);
  MicroSecondDelay (5);

  Addr = Offset / 2;
  BytesRemaining = Length;
  while (BytesRemaining > 0) {
    MmioWrite32 (OTPC_USER_ADDR, (0xFFFFU << 16) | Addr);
    MmioWrite32 (OTPC_USER_ENABLE, (1 << 16) | 1);
    while ((MmioRead32 (OTPC_INT_STATUS) & USER_DONE_INT_STATUS) == 0) {
      MicroSecondDelay (1);
      if (--Retry == 0) {
        DEBUG ((DEBUG_WARN, "OTP read timeout!\n"));
        break;
      }
    }
    MmioWrite32 (OTPC_INT_STATUS, USER_DONE_INT_STATUS);

    Value = MmioRead32 (OTPC_USER_Q);

    if (BytesRemaining == Length && (Offset & 1) != 0) {
      *Data++ = (Value >> 8) & 0xFF;
      BytesRemaining--;
    } else if (BytesRemaining == 1) {
      *Data++ = Value & 0xFF;
      BytesRemaining--;
    } else {
      *Data++ = Value & 0xFF;
      *Data++ = (Value >> 8) & 0xFF;
      BytesRemaining -= 2;
    }
    Addr++;
  }

  MmioWrite32 (OTPC_USER_CTRL, USER_DCTRL << 16);
}