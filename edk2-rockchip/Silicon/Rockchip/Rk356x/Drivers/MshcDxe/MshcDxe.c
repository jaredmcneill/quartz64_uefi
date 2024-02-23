/** @file
  This file implement the MMC Host Protocol for the DesignWare eMMC.

  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/CruLib.h>
#include <Library/GpioLib.h>

#include <Protocol/MmcHost.h>

#include "Mshc.h"

#define DW_DBG    DEBUG_INFO

#define DWEMMC_DESC_PAGE                1
#define DWEMMC_BLOCK_SIZE               512
#define DWEMMC_DMA_BUF_SIZE             (512 * 8)
#define DWEMMC_MAX_DESC_PAGES           512

STATIC CONST GPIO_IOMUX_CONFIG mSdmmc0IomuxConfig[] = {
  { "sdmmc0_d0",          1, GPIO_PIN_PD5, 1, GPIO_PIN_PULL_UP, GPIO_PIN_DRIVE_2 },
  { "sdmmc0_d1",          1, GPIO_PIN_PD6, 1, GPIO_PIN_PULL_UP, GPIO_PIN_DRIVE_2 },
  { "sdmmc0_d2",          1, GPIO_PIN_PD7, 1, GPIO_PIN_PULL_UP, GPIO_PIN_DRIVE_2 },
  { "sdmmc0_d3",          2, GPIO_PIN_PA0, 1, GPIO_PIN_PULL_UP, GPIO_PIN_DRIVE_2 },
  { "sdmmc0_cmd",         2, GPIO_PIN_PA1, 1, GPIO_PIN_PULL_UP, GPIO_PIN_DRIVE_2 },
  { "sdmmc0_clk",         2, GPIO_PIN_PA2, 1, GPIO_PIN_PULL_UP, GPIO_PIN_DRIVE_2 },
  { "sdmmc0_det",         0, GPIO_PIN_PA4, 1, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
};
STATIC CONST GPIO_IOMUX_CONFIG mSdmmc0IomuxPwrEnDefaultConfig[] = {
  { "sdmmc0_pwren",       0, GPIO_PIN_PA5, 1, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
};
STATIC CONST GPIO_IOMUX_CONFIG mSdmmc0IomuxPwrEnInvertedConfig[] = {
  { "sdmmc0_pwren",       0, GPIO_PIN_PA5, 0, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
};

typedef struct {
  UINT32                        Des0;
  UINT32                        Des1;
  UINT32                        Des2;
  UINT32                        Des3;
} DWEMMC_IDMAC_DESCRIPTOR;

EFI_MMC_HOST_PROTOCOL     *gpMmcHost;
EFI_GUID mMshcDevicePathGuid = EFI_CALLER_ID_GUID;
STATIC UINT32 mMshcCommand;
STATIC UINT32 mMshcArgument;

EFI_STATUS
MshcReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  );

BOOLEAN
MshcIsPowerOn (
  VOID
  )
{
    return TRUE;
}

EFI_STATUS
MshcInitialize (
  VOID
  )
{
    DEBUG ((DEBUG_BLKIO, "MshcInitialize()"));
    return EFI_SUCCESS;
}

BOOLEAN
MshcIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  return TRUE;
}

BOOLEAN
MshcIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  return FALSE;
}

BOOLEAN
MshcIsDmaSupported (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  return TRUE;
}

EFI_STATUS
MshcBuildDevicePath (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN EFI_DEVICE_PATH_PROTOCOL   **DevicePath
  )
{
  MEMMAP_DEVICE_PATH *MemMap;

  MemMap = (MEMMAP_DEVICE_PATH*) CreateDeviceNode (HARDWARE_DEVICE_PATH, HW_MEMMAP_DP, sizeof (MEMMAP_DEVICE_PATH));
  MemMap->MemoryType = EfiMemoryMappedIO;
  MemMap->StartingAddress = PcdGet32 (PcdMshcDxeBaseAddress);
  MemMap->EndingAddress = MemMap->StartingAddress + 0x1000 - 1;

  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL*) MemMap;
  return EFI_SUCCESS;
}

EFI_STATUS
MshcUpdateClock (
  VOID
  )
{
  UINT32 Data;

  /* CMD_UPDATE_CLK */
  Data = BIT_CMD_WAIT_PRVDATA_COMPLETE | BIT_CMD_UPDATE_CLOCK_ONLY |
         BIT_CMD_START;
  MmioWrite32 (DWEMMC_CMD, Data);
  while (1) {
    Data = MmioRead32 (DWEMMC_CMD);
    if (!(Data & CMD_START_BIT)) {
      break;
    }
    Data = MmioRead32 (DWEMMC_RINTSTS);
    if (Data & DWEMMC_INT_HLE) {
      Print (L"failed to update mmc clock frequency\n");
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
MshcSetClock (
  IN UINTN                     ClockFreq
  )
{
  UINT32 Divider, Rate, Data;
  EFI_STATUS Status;
  BOOLEAN Found = FALSE;

  Rate = CruGetSdmmcClockRate (0);
  if (Rate == ClockFreq) {
    Divider = 0;
  } else {
    for (Divider = 1; Divider < 256; Divider++) {
      if ((Rate / (2 * Divider)) <= ClockFreq) {
        Found = TRUE;
        break;
      }
    }
    if (Found == FALSE) {
      return EFI_NOT_FOUND;
    }
  }

  DEBUG ((DEBUG_INFO, "MshcSetClock(): ClockFreq = %lu Hz, Rate = %lu Hz, Divider = %u\n", ClockFreq, Rate, Divider));

  // Wait until MMC is idle
  do {
    Data = MmioRead32 (DWEMMC_STATUS);
  } while (Data & DWEMMC_STS_DATA_BUSY);

  // Disable MMC clock first
  MmioWrite32 (DWEMMC_CLKENA, 0);
  Status = MshcUpdateClock ();
  ASSERT (!EFI_ERROR (Status));

  MmioWrite32 (DWEMMC_CLKDIV, Divider);
  Status = MshcUpdateClock ();
  ASSERT (!EFI_ERROR (Status));

  // Enable MMC clock
  MmioWrite32 (DWEMMC_CLKENA, 1);
  MmioWrite32 (DWEMMC_CLKSRC, 0);
  Status = MshcUpdateClock ();
  ASSERT (!EFI_ERROR (Status));
  return EFI_SUCCESS;
}

EFI_STATUS
MshcNotifyState (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_STATE                 State
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  switch (State) {
  case MmcInvalidState:
    return EFI_INVALID_PARAMETER;
  case MmcHwInitializationState:
    MmioWrite32 (DWEMMC_PWREN, 1);

    // If device already turn on then restart it
    Data = DWEMMC_CTRL_RESET_ALL;
    MmioWrite32 (DWEMMC_CTRL, Data);
    do {
      // Wait until reset operation finished
      Data = MmioRead32 (DWEMMC_CTRL);
    } while (Data & DWEMMC_CTRL_RESET_ALL);

    // Setup clock that could not be higher than 400KHz.
    Status = MshcSetClock (400000);
    ASSERT (!EFI_ERROR (Status));
    // Wait clock stable
    MicroSecondDelay (100);

    MmioWrite32 (DWEMMC_RINTSTS, ~0);
    MmioWrite32 (DWEMMC_INTMASK, 0);
    MmioWrite32 (DWEMMC_TMOUT, ~0);
    MmioWrite32 (DWEMMC_IDINTEN, 0);
    MmioWrite32 (DWEMMC_CTYPE, 0);
    MmioWrite32 (DWEMMC_BMOD, DWEMMC_IDMAC_SWRESET);

    MmioWrite32 (DWEMMC_BLKSIZ, DWEMMC_BLOCK_SIZE);
    do {
      Data = MmioRead32 (DWEMMC_BMOD);
    } while (Data & DWEMMC_IDMAC_SWRESET);
    break;
  case MmcIdleState:
    break;
  case MmcReadyState:
    break;
  case MmcIdentificationState:
    break;
  case MmcStandByState:
    break;
  case MmcTransferState:
    break;
  case MmcSendingDataState:
    break;
  case MmcReceiveDataState:
    break;
  case MmcProgrammingState:
    break;
  case MmcDisconnectState:
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

// Need to prepare DMA buffer first before sending commands to MMC card
BOOLEAN
IsPendingReadCommand (
  IN MMC_CMD                    MmcCmd
  )
{
  UINTN  Mask;

  Mask = BIT_CMD_DATA_EXPECTED | BIT_CMD_READ;
  if ((MmcCmd & Mask) == Mask) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN
IsPendingWriteCommand (
  IN MMC_CMD                    MmcCmd
  )
{
  UINTN  Mask;

  Mask = BIT_CMD_DATA_EXPECTED | BIT_CMD_WRITE;
  if ((MmcCmd & Mask) == Mask) {
    return TRUE;
  }
  return FALSE;
}

EFI_STATUS
SendCommand (
  IN MMC_CMD                    MmcCmd,
  IN UINT32                     Argument
  )
{
  UINT32      Data, ErrMask;
  EFI_STATUS  Status = EFI_SUCCESS;

  // Wait until MMC is idle
  do {
    Data = MmioRead32 (DWEMMC_STATUS);
  } while (Data & DWEMMC_STS_DATA_BUSY);

  MmioWrite32 (DWEMMC_RINTSTS, ~0);
  MmioWrite32 (DWEMMC_CMDARG, Argument);
  MmioWrite32 (DWEMMC_CMD, MmcCmd);

  ErrMask = DWEMMC_INT_EBE | DWEMMC_INT_HLE | DWEMMC_INT_RTO |
            DWEMMC_INT_RCRC | DWEMMC_INT_RE;
  ErrMask |= DWEMMC_INT_DCRC | DWEMMC_INT_DRT | DWEMMC_INT_SBE;
  do {
    MicroSecondDelay(1);
    Data = MmioRead32 (DWEMMC_RINTSTS);

    if (Status == EFI_SUCCESS && (Data & ErrMask) != 0) {
      DEBUG ((DEBUG_INFO, "%a(): EFI_DEVICE_ERROR DWEMMC_RINTSTS=0x%x MmcCmd 0x%x(%d),Argument 0x%x\n",
          __func__, Data, MmcCmd, MmcCmd&0x3f, Argument));
      Status = EFI_DEVICE_ERROR;
      // Keep spinning until CMD done bit is set.
    }
    if (Data & DWEMMC_INT_DTO) {     // Transfer Done
      break;
    }
  } while (!(Data & DWEMMC_INT_CMD_DONE));
  return Status;
}

EFI_STATUS
MshcSendCommand (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_CMD                    MmcCmd,
  IN UINT32                     Argument
  )
{
  UINT32       Cmd = 0;
  EFI_STATUS   Status = EFI_SUCCESS;

  switch (MMC_GET_INDX(MmcCmd)) {
  case MMC_INDX(0):
    Cmd = BIT_CMD_SEND_INIT;
    break;
  case MMC_INDX(1):
    Cmd = BIT_CMD_RESPONSE_EXPECT;
    break;
  case MMC_INDX(2):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_LONG_RESPONSE |
           BIT_CMD_CHECK_RESPONSE_CRC | BIT_CMD_SEND_INIT;
    break;
  case MMC_INDX(3):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_SEND_INIT;
    break;
  case MMC_INDX(6):
    if (((Argument >> 31) & 0x1) == 0x1 || Argument == 0x00FFFFF0) {
      Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
            BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
            BIT_CMD_WAIT_PRVDATA_COMPLETE;
    } else {
      Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    }
    break;
  case MMC_INDX(7):
    if (Argument) {
        Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    } else {
        Cmd = 0;
    }
    break;
  case MMC_INDX(8):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(9):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_LONG_RESPONSE;
    break;
  case MMC_INDX(12):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_STOP_ABORT_CMD;
    break;
  case MMC_INDX(13):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(16):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    break;
  case MMC_INDX(17):
  case MMC_INDX(18):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(24):
  case MMC_INDX(25):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED | BIT_CMD_WRITE |
           BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  case MMC_INDX(30):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
           BIT_CMD_DATA_EXPECTED;
    break;
  case MMC_INDX(41):
    Cmd = BIT_CMD_RESPONSE_EXPECT;
    break;
  case MMC_INDX(51):
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
          BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
          BIT_CMD_WAIT_PRVDATA_COMPLETE;
    break;
  default:
    Cmd = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
    break;
  }

  Cmd |= MMC_GET_INDX(MmcCmd) | BIT_CMD_USE_HOLD_REG | BIT_CMD_START;

  if (IsPendingReadCommand (Cmd) || IsPendingWriteCommand (Cmd)) {
    // DEBUG ((DEBUG_INFO, "%a(): Deferred Cmd=0x%08X(%u), Argument 0x%08X\n", __func__, MmcCmd, MMC_GET_INDX (MmcCmd), Argument));
    mMshcCommand = Cmd;
    mMshcArgument = Argument;
  } else {
    // DEBUG ((DEBUG_INFO, "%a(): Immediate Cmd=0x%08X(%u), Argument 0x%08X\n", __func__, MmcCmd, MMC_GET_INDX (MmcCmd)));
    Status = SendCommand (Cmd, Argument);
  }
  return Status;
}

EFI_STATUS
MshcReceiveResponse (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_RESPONSE_TYPE          Type,
  IN UINT32*                    Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (   (Type == MMC_RESPONSE_TYPE_R1)
      || (Type == MMC_RESPONSE_TYPE_R1b)
      || (Type == MMC_RESPONSE_TYPE_R3)
      || (Type == MMC_RESPONSE_TYPE_R6)
      || (Type == MMC_RESPONSE_TYPE_R7))
  {
    Buffer[0] = MmioRead32 (DWEMMC_RESP0);
  } else if (Type == MMC_RESPONSE_TYPE_R2) {
    Buffer[0] = MmioRead32 (DWEMMC_RESP0);
    Buffer[1] = MmioRead32 (DWEMMC_RESP1);
    Buffer[2] = MmioRead32 (DWEMMC_RESP2);
    Buffer[3] = MmioRead32 (DWEMMC_RESP3);
  }
  return EFI_SUCCESS;
}

VOID
MshcAdjustFifoThreshold (
  VOID
  )
{
  /* DMA multiple transaction size map to reg value as array index */
  CONST UINT32 BurstSize[] = {1, 4, 8, 16, 32, 64, 128, 256};
  UINT32 BlkDepthInFifo, FifoThreshold, FifoWidth, FifoDepth;
  UINT32 BlkSize = DWEMMC_BLOCK_SIZE, Idx = 0, RxWatermark = 1, TxWatermark, TxWatermarkInvers;

  /* Skip FIFO adjustment if we do not have platform FIFO depth info */
  FifoDepth = PcdGet32 (PcdMshcDxeFifoDepth);
  if (!FifoDepth) {
    return;
  }

  TxWatermark = FifoDepth / 2;
  TxWatermarkInvers = FifoDepth - TxWatermark;

  FifoWidth = DWEMMC_GET_HDATA_WIDTH (MmioRead32 (DWEMMC_HCON));
  if (!FifoWidth) {
    FifoWidth = 2;
  } else if (FifoWidth == 2) {
    FifoWidth = 8;
  } else {
    FifoWidth = 4;
  }

  BlkDepthInFifo = BlkSize / FifoWidth;

  Idx = ARRAY_SIZE (BurstSize) - 1;
  while (Idx && ((BlkDepthInFifo % BurstSize[Idx]) || (TxWatermarkInvers % BurstSize[Idx]))) {
    Idx--;
  }

  RxWatermark = BurstSize[Idx] - 1;
  FifoThreshold = DWEMMC_DMA_BURST_SIZE (Idx) | DWEMMC_FIFO_TWMARK (TxWatermark)
           | DWEMMC_FIFO_RWMARK (RxWatermark);
  MmioWrite32 (DWEMMC_FIFOTH, FifoThreshold);
}

EFI_STATUS
MshcReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                   Buffer
  )
{
  EFI_STATUS	Status;
  UINT32		DataLen = Length>>2; //byte to word
  EFI_STATUS	ret = EFI_SUCCESS;
  UINT32		Data;
  UINT32		TimeOut = 0;
  UINT32		value = 0;

  DEBUG ((DW_DBG, "%a():\n", __func__));

  ASSERT ((mMshcCommand & BIT_CMD_WRITE) == BIT_CMD_READ);

  if (mMshcCommand & BIT_CMD_WAIT_PRVDATA_COMPLETE) {
    do {
      Data = MmioRead32 (DWEMMC_STATUS);
    } while (Data & DWEMMC_STS_DATA_BUSY);
  }

  if ((mMshcCommand & BIT_CMD_STOP_ABORT_CMD) || (mMshcCommand & BIT_CMD_DATA_EXPECTED)) {
    if (!(MmioRead32 (DWEMMC_STATUS) & FIFO_EMPTY)) {
      Data = MmioRead32 (DWEMMC_CTRL);
      Data |= FIFO_RESET;
      MmioWrite32 (DWEMMC_CTRL, Data);

      TimeOut = 100000;
      while (((value = MmioRead32 (DWEMMC_CTRL)) & (FIFO_RESET)) && (TimeOut > 0)) {
        TimeOut--;
      }
      if (TimeOut == 0) {
        DEBUG ((DEBUG_ERROR, "%a():  CMD=%d SDC_SDC_ERROR\n", __func__, mMshcCommand&0x3f));
        return EFI_DEVICE_ERROR;
      }
    }
  }

  MmioWrite32 (DWEMMC_BLKSIZ, Length < 512 ? Length : 512);
  MmioWrite32 (DWEMMC_BYTCNT, Length);

  Status = SendCommand (mMshcCommand, mMshcArgument);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to read data, mMshcCommand:%x, mMshcArgument:%x, Status:%r\n", mMshcCommand, mMshcArgument, Status));
    return EFI_DEVICE_ERROR;
  }

  DEBUG((DW_DBG, "Sdmmc::SdmmcReadBlockData  DataLen=%d\n", DataLen));
  TimeOut = 1000000;
  while (DataLen) {
    if (MmioRead32(DWEMMC_RINTSTS) & (DWEMMC_INT_DRT | DWEMMC_INT_SBE | DWEMMC_INT_EBE | DWEMMC_INT_DCRC))  {
      DEBUG ((DEBUG_ERROR, "%a(): EFI_DEVICE_ERROR DWEMMC_RINTSTS=0x%x DataLen=%d\n",
        __func__, MmioRead32(DWEMMC_RINTSTS), DataLen));
      return EFI_DEVICE_ERROR;
    }

    while((!(MmioRead32(DWEMMC_STATUS) & FIFO_EMPTY)) && DataLen) {
      *Buffer++ = MmioRead32(DWEMMC_DATA);
      DataLen--;
      TimeOut = 1000000;
    }

    if (!DataLen) {
      ret = (MmioRead32(DWEMMC_RINTSTS) & (DWEMMC_INT_DRT | DWEMMC_INT_SBE | DWEMMC_INT_EBE | DWEMMC_INT_DCRC))? 
        EFI_DEVICE_ERROR : EFI_SUCCESS;
      DEBUG((DW_DBG, "%a(): DataLen end :%d\n", __func__, ret));
      break;
    }

    NanoSecondDelay(1);
    TimeOut--;
    if (TimeOut == 0) {
      ret = EFI_DEVICE_ERROR;
      DEBUG ((DEBUG_ERROR, "%a(): TimeOut! DataLen=%d\n", __func__, DataLen));
      break;
    }
  }

  return ret;
}

#define MMC_GET_FCNT(x)		        (((x)>>17) & 0x1FF)
#define INTMSK_HTO      (0x1<<10)

/* Common flag combinations */
#define MMC_DATA_ERROR_FLAGS (DWEMMC_INT_DRT | DWEMMC_INT_DCRC | DWEMMC_INT_FRUN | \
	DWEMMC_INT_HLE | INTMSK_HTO | DWEMMC_INT_SBE  | \
	DWEMMC_INT_EBE)

EFI_STATUS
MshcWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  UINT32 *DataBuffer = Buffer;
  UINTN Count=0;
  UINTN Size32 = Length / 4;
  UINT32 Mask;
  EFI_STATUS	Status;
  UINT32		Data;
  UINT32		TimeOut = 0;
  UINT32		value = 0;

  DEBUG ((DW_DBG, "%a():\n", __func__));

  ASSERT ((mMshcCommand & BIT_CMD_WRITE) == BIT_CMD_WRITE);

  if (mMshcCommand & BIT_CMD_WAIT_PRVDATA_COMPLETE) {
    TimeOut = 100000;
    do {
      Data = MmioRead32 (DWEMMC_STATUS);
      TimeOut--;
    } while ((Data & DWEMMC_STS_DATA_BUSY) && (TimeOut > 0));

    if (TimeOut == 0) {
        DEBUG ((DEBUG_ERROR, "%a():  CMD=%d Timeout waiting for DWEMMC_STATUS DWEMMC_STS_DATA_BUSY\n", __func__, mMshcCommand&0x3f));
        return EFI_DEVICE_ERROR;
    }
  }

  if (!(((mMshcCommand&0x3f) == 6) || ((mMshcCommand&0x3f) == 51))) {
    if ((mMshcCommand & BIT_CMD_STOP_ABORT_CMD) || (mMshcCommand & BIT_CMD_DATA_EXPECTED)) {
      if (!(MmioRead32 (DWEMMC_STATUS) & FIFO_EMPTY)) {
        Data = MmioRead32 (DWEMMC_CTRL);
        Data |= FIFO_RESET;
        MmioWrite32 (DWEMMC_CTRL, Data);

        TimeOut = 100000;
        while (((value = MmioRead32 (DWEMMC_CTRL)) & (FIFO_RESET)) && (TimeOut > 0)) {
          TimeOut--;
        }
        if (TimeOut == 0) {
          DEBUG ((DEBUG_ERROR, "%a():  CMD=%d SDC_SDC_ERROR\n", __func__, mMshcCommand&0x3f));
          return EFI_DEVICE_ERROR;
        }
      }
    }
  }

  MmioWrite32 (DWEMMC_BLKSIZ, 512);
  MmioWrite32 (DWEMMC_BYTCNT, Length);

  Status = SendCommand (mMshcCommand, mMshcArgument);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to write data, mMshcCommand:%x, mMshcArgument:%x, Status:%r\n", mMshcCommand, mMshcArgument, Status));
    return EFI_DEVICE_ERROR;
  }

  for (Count = 0; Count < Size32; Count++) {
    TimeOut = 100000;
    while((MMC_GET_FCNT(MmioRead32(DWEMMC_STATUS)) >32) && (TimeOut > 0))
    {
      MicroSecondDelay(1);
      TimeOut--;
    }
    if (TimeOut == 0) {
      DEBUG ((DEBUG_ERROR, "%a():  CMD=%d DWEMMC_STATUS timeout\n", __func__, mMshcCommand&0x3f));
      return EFI_DEVICE_ERROR;
    }
    MmioWrite32(DWEMMC_DATA, *DataBuffer++);
  }

  do {
    Mask = MmioRead32(DWEMMC_RINTSTS);
    if (Mask & (MMC_DATA_ERROR_FLAGS)) {
      DEBUG((DEBUG_ERROR, "SdmmcWriteData error, RINTSTS = 0x%08x\n", Mask));
      return EFI_DEVICE_ERROR;
    }	
  } while (!(Mask & DWEMMC_INT_DTO));

  return EFI_SUCCESS;
}

EFI_STATUS
MshcSetIos (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN  UINT32                    BusClockFreq,
  IN  UINT32                    BusWidth,
  IN  UINT32                    TimingMode
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32    Data;

  if ((PcdGet32 (PcdMshcDxeMaxClockFreqInHz) != 0) &&
      (BusClockFreq > PcdGet32 (PcdMshcDxeMaxClockFreqInHz))) {
    return EFI_UNSUPPORTED;
  }
  if (TimingMode != EMMCBACKWARD) {
    Data = MmioRead32 (DWEMMC_UHSREG);
    switch (TimingMode) {
    case EMMCHS52DDR1V2:
    case EMMCHS52DDR1V8:
      Data |= 1 << 16;
      break;
    case EMMCHS52:
    case EMMCHS26:
      Data &= ~(1 << 16);
      break;
    default:
      return EFI_UNSUPPORTED;
    }
    MmioWrite32 (DWEMMC_UHSREG, Data);
  }

  switch (BusWidth) {
  case 1:
    MmioWrite32 (DWEMMC_CTYPE, 0);
    break;
  case 4:
    MmioWrite32 (DWEMMC_CTYPE, 1);
    break;
  case 8:
    MmioWrite32 (DWEMMC_CTYPE, 1 << 16);
    break;
  default:
    return EFI_UNSUPPORTED;
  }
  if (BusClockFreq) {
    Status = MshcSetClock (BusClockFreq);
  }
  return Status;
}

BOOLEAN
MshcIsMultiBlock (
  IN EFI_MMC_HOST_PROTOCOL      *This
  )
{
  return TRUE;
}

EFI_MMC_HOST_PROTOCOL gMciHost = {
  MMC_HOST_PROTOCOL_REVISION,
  MshcIsCardPresent,
  MshcIsReadOnly,
  MshcBuildDevicePath,
  MshcNotifyState,
  MshcSendCommand,
  MshcReceiveResponse,
  MshcReadBlockData,
  MshcWriteBlockData,
  MshcSetIos,
  MshcIsMultiBlock
};

EFI_STATUS
MshcDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;

  Handle = NULL;

  CruSetSdmmcClockRate (0, 100000000UL);

  CruAssertSoftReset (13, 4);
  MicroSecondDelay (5);
  CruDeassertSoftReset (13, 4);

  DEBUG ((DEBUG_BLKIO, "MshcDxeInitialize()\n"));

  /* Configure pins */
  GpioSetIomuxConfig (mSdmmc0IomuxConfig, ARRAY_SIZE (mSdmmc0IomuxConfig));
  if (PcdGetBool (PcdMshcDxePwrEnUsed)) {
    if (!PcdGetBool (PcdMshcDxePwrEnInverted)) {
      GpioSetIomuxConfig (mSdmmc0IomuxPwrEnDefaultConfig, ARRAY_SIZE (mSdmmc0IomuxPwrEnDefaultConfig));
    } else {
      /*
      * This board has the PWREN signal from SDMMC0 inverted. Configure the
      * pin as GPIO and drive it low since there is no way with the device tree
      * bindings to tell the driver about this quirk.
      */
      DEBUG ((DEBUG_INFO, "MshcDxeInitialize(): Applying PWREN inverted workaround\n"));
      GpioSetIomuxConfig (mSdmmc0IomuxPwrEnInvertedConfig, ARRAY_SIZE (mSdmmc0IomuxPwrEnInvertedConfig));
      GpioPinSetDirection (0, GPIO_PIN_PA5, GPIO_PIN_OUTPUT);
      GpioPinWrite (0, GPIO_PIN_PA5, FALSE);
    }
  }

  MshcAdjustFifoThreshold ();

  //Publish Component Name, BlockIO protocol interfaces
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEmbeddedMmcHostProtocolGuid,    &gMciHost,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
