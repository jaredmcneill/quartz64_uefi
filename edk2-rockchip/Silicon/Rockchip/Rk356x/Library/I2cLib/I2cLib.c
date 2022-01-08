/** @file
 *
 *  RK3566/RK3568 I2C Library.
 * 
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/I2cLib.h>
#include <IndustryStandard/Rk356x.h>

#define I2C_REFCLK              24000000
#define I2C_BUSCLK              100000

#define RKI2C_CON               0x000
#define	 ACT2NAK	              BIT6
#define	 ACK		                BIT5
#define	 STOP		                BIT4
#define	 START	                BIT3
#define	 I2C_MODE_SHIFT         1
#define  I2C_MODE_MASK          (0x3U << I2C_MODE_SHIFT)
#define	 I2C_MODE_TX		        (0U << I2C_MODE_SHIFT)
#define	 I2C_MODE_RTX           (1U << I2C_MODE_SHIFT)
#define	 I2C_MODE_RX		        (2U << I2C_MODE_SHIFT)
#define	 I2C_MODE_RRX	          (3U << I2C_MODE_SHIFT)
#define  I2C_EN                 BIT0
#define RKI2C_CLKDIV            0x004
#define  CLKDIVH_SHIFT          16
#define RKI2C_MRXADDR           0x008
#define RKI2C_MRXRADDR          0x00C
#define  ADDHVLD                BIT26   /* for MRXADDR and MRXRADDR */
#define  ADDMVLD                BIT25   /* for MRXADDR and MRXRADDR */
#define  ADDLVLD                BIT24   /* for MRXADDR and MRXRADDR */
#define RKI2C_MTXCNT            0x010
#define	RKI2C_MRXCNT		        0x014
#define RKI2C_IEN               0x018
#define RKI2C_IPD               0x01C
#define	 NAKRCVIPD	            BIT6
#define	 STOPIPD	              BIT5
#define	 STARTIPD	              BIT4
#define	 MBRFIPD	              BIT3
#define	 MBTFIPD	              BIT2
#define	 BRFIPD	                BIT1
#define	 BTFIPD	                BIT0
#define	RKI2C_TXDATA(N)		      (0x100 + (N) * 4)
#define	RKI2C_RXDATA(N)		      (0x200 + (N) * 4)

STATIC
VOID
I2cInit (
  IN EFI_PHYSICAL_ADDRESS I2cBase
  )
{
  INT32 Div;
  UINT32 SclHigh;
  UINT32 SclLow;

  /*
   * SCL frequency is calculated by the following formula:
   *
   * SCL Divisor = 8 * (CLKDIVL + 1 + CLKDIVH + 1)
   * SCL = PCLK / SCLK Divisor
   */
  Div = ((I2C_REFCLK + I2C_BUSCLK * 8 - 1) / I2C_BUSCLK) - 2;
  ASSERT (Div >= 0);
  SclLow = Div / 2;
  if (Div % 2 == 0) {
    SclHigh = SclLow;
  } else {
    SclHigh = (Div + 1) / 2;
  }

  MmioWrite32 (I2cBase + RKI2C_CLKDIV,
               (SclHigh << CLKDIVH_SHIFT) | SclLow);

  MmioWrite32 (I2cBase + RKI2C_CON, 0);
  MmioWrite32 (I2cBase + RKI2C_IEN, 0);
  MmioWrite32 (I2cBase + RKI2C_IPD, MmioRead32 (I2cBase + RKI2C_IPD));
}

STATIC
EFI_STATUS
I2cWait (
  IN EFI_PHYSICAL_ADDRESS I2cBase,
  IN UINT32 Mask
  )
{
  UINT32 Timeout = 100000;
  UINT32 Value;

  CONST UINT32 IpdMask = Mask | NAKRCVIPD;
  do {
    Value = MmioRead32 (I2cBase + RKI2C_IPD);
    if ((Value & IpdMask) != 0) {
      break;
    }
    MicroSecondDelay (1);
  } while (--Timeout > 0);

  MmioWrite32 (I2cBase + RKI2C_IPD, Value & IpdMask);

  if ((Value & NAKRCVIPD) != 0) {
    return EFI_DEVICE_ERROR;
  }
  if ((Value & Mask) != 0) {
    return EFI_SUCCESS;
  }

  return EFI_TIMEOUT;
}

STATIC
EFI_STATUS
I2cStart (
  IN EFI_PHYSICAL_ADDRESS I2cBase
  )
{
  EFI_STATUS Status;

  /* Send start */
  MmioOr32 (I2cBase + RKI2C_CON, START);
  Status = I2cWait (I2cBase, STARTIPD);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "I2C: Timeout sending start: %r\n", Status));
    return Status;
  }
  MmioAnd32 (I2cBase + RKI2C_CON, ~START);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
I2cStop (
  IN EFI_PHYSICAL_ADDRESS I2cBase
  )
{
  EFI_STATUS Status;

  /* Send stop */
  MmioOr32 (I2cBase + RKI2C_CON, STOP);
  Status = I2cWait (I2cBase, STOPIPD);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "I2C: Timeout sending stop: %r\n", Status));
    return Status;
  }
  MmioAnd32 (I2cBase + RKI2C_CON, ~STOP);

  return EFI_SUCCESS;
}

EFI_STATUS
I2cRead (
  IN EFI_PHYSICAL_ADDRESS I2cBase,
  IN UINT8 Address,
  IN UINT8 *Register,
  IN UINT8 RegisterLength,
  OUT UINT8 *Value,
  IN UINT8 ValueLength
  )
{
  EFI_STATUS Status;
  UINT32 RxData[8];
  UINT32 Mrxraddr = 0;
  UINTN Index;

  ASSERT (RegisterLength < 4);
  ASSERT (ValueLength <= 32);

  I2cInit (I2cBase);

  MmioWrite32 (I2cBase + RKI2C_CON, I2C_EN | I2C_MODE_RTX);
  Status = I2cStart (I2cBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MmioWrite32 (I2cBase + RKI2C_MRXADDR, 1 | (Address << 1) | ADDLVLD);
  for (Index = 0; Index < RegisterLength; Index++) {
    Mrxraddr |= Register[Index] << (Index * 8);
    Mrxraddr |= (ADDLVLD << Index);
  }
  MmioWrite32 (I2cBase + RKI2C_MRXRADDR, Mrxraddr);
  MmioOr32 (I2cBase + RKI2C_CON, ACK);

  MmioWrite32 (I2cBase + RKI2C_MRXCNT, ValueLength);
  Status = I2cWait (I2cBase, MBRFIPD);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "I2C: Read error: %r\n", Status));
    return Status;
  }

  for (Index = 0; Index < ((ValueLength + 3) / 4 * 4); Index += 4) {
    RxData[Index / 4] = MmioRead32 (I2cBase + RKI2C_RXDATA (Index / 4));
  }
  CopyMem (Value, RxData, ValueLength);

  Status = I2cStop (I2cBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  MmioWrite32 (I2cBase + RKI2C_CON, 0);

  return EFI_SUCCESS;
}

EFI_STATUS
I2cWrite (
  IN EFI_PHYSICAL_ADDRESS I2cBase,
  IN UINT8 Address,
  IN UINT8 *Register,
  IN UINT8 RegisterLength,
  IN UINT8 *Value,
  IN UINT8 ValueLength
  )
{
  EFI_STATUS Status;
  union {
    UINT8 Data8[32];
    UINT32 Data32[8];
  } TxData;
  UINTN Index;
  UINT8 Length;

  ASSERT (RegisterLength < 4);

  Length = RegisterLength + ValueLength;
  ASSERT (Length < 32); /* reserve one byte for device address */

  I2cInit (I2cBase);

  MmioWrite32 (I2cBase + RKI2C_CON, I2C_EN | I2C_MODE_TX);
  Status = I2cStart (I2cBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /* Transmit data. Device address goes in the lower 8 bits of TXDATA0 */
  TxData.Data8[0] = Address << 1;
  CopyMem (&TxData.Data8[1], Register, RegisterLength);
  CopyMem (&TxData.Data8[1 + RegisterLength], Value, ValueLength);
  for (Index = 0; Index < ((Length + 1 + 3) / 4 * 4); Index += 4) {
    MmioWrite32 (I2cBase + RKI2C_TXDATA (Index / 4), TxData.Data32[Index / 4]);
  }
  MmioWrite32 (I2cBase + RKI2C_MTXCNT, Length + 1);

  Status = I2cWait (I2cBase, MBTFIPD);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "I2C: Write error: %r\n", Status));
    return Status;
  }

  Status = I2cStop (I2cBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  MmioWrite32 (I2cBase + RKI2C_CON, 0);

  return EFI_SUCCESS;
}