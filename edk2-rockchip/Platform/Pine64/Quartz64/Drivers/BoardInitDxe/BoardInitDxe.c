/** @file
 *
 *  Board init for the Quartz64 platform
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/CruLib.h>
#include <Library/GpioLib.h>
#include <Library/MultiPhyLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ArmScmi.h>
#include <Protocol/ArmScmiClockProtocol.h>

#include <IndustryStandard/Rk356x.h>
#include <IndustryStandard/Rk356xCru.h>

#define GRF_MAC1_CON0           (SYS_GRF + 0x0388)
#define  CLK_RX_DL_CFG_SHIFT    8
#define  CLK_TX_DL_CFG_SHIFT    0
#define GRF_MAC1_CON1           (SYS_GRF + 0x038C)
#define  PHY_INTF_SEL_SHIFT     4
#define  PHY_INTF_SEL_MASK      (0x7U << PHY_INTF_SEL_SHIFT)
#define  PHY_INTF_SEL_RGMII     (1U << PHY_INTF_SEL_SHIFT)
#define  FLOWCTRL               BIT3
#define  MAC_SPEED              BIT2
#define  RXCLK_DLY_ENA          BIT1
#define  TXCLK_DLY_ENA          BIT0

#define TX_DELAY                0x30
#define RX_DELAY                0x10

typedef struct {
  CONST char *Name;
  UINT8 Group;
  UINT8 Pin;
  UINT8 Function;
  GPIO_PIN_PULL Pull;
  GPIO_PIN_DRIVE Drive;
} BOARD_IOMUX_CONFIG;

STATIC CONST BOARD_IOMUX_CONFIG mGmac1IomuxConfig[] = {
  { "gmac1_mdcm0",        3, GPIO_PIN_PC4, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_mdiom0",       3, GPIO_PIN_PC5, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_txd0m0",       3, GPIO_PIN_PB5, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac1_txd1m0",       3, GPIO_PIN_PB6, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac1_txenm0",       3, GPIO_PIN_PB7, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd0m0",       3, GPIO_PIN_PB1, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd1m0",       3, GPIO_PIN_PB2, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxdvcrsm0",    3, GPIO_PIN_PB3, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxclkm0",      3, GPIO_PIN_PA7, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_txclkm0",      3, GPIO_PIN_PA6, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_1 },
  { "gmac1_mclkinoutm0",  3, GPIO_PIN_PC0, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd2m0",       3, GPIO_PIN_PA4, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd3m0",       3, GPIO_PIN_PA5, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_txd2m0",       3, GPIO_PIN_PA2, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac1_txd3m0",       3, GPIO_PIN_PA3, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
};

STATIC
EFI_STATUS
BoardInitSetCpuSpeed (
  VOID
  )
{
  EFI_STATUS             Status;
  SCMI_CLOCK_PROTOCOL    *ClockProtocol;
  EFI_GUID               ClockProtocolGuid = ARM_SCMI_CLOCK_PROTOCOL_GUID;
  UINT64                 CpuRate;
  UINT32                 ClockId;
  UINT32                 ClockProtocolVersion;
  BOOLEAN                Enabled;
  CHAR8                  ClockName[SCMI_MAX_STR_LEN];
  UINT32                 TotalRates = 0;
  UINT32                 ClockRateSize;
  SCMI_CLOCK_RATE        *ClockRate;
  SCMI_CLOCK_RATE_FORMAT ClockRateFormat;

  Status = gBS->LocateProtocol (
                  &ClockProtocolGuid,
                  NULL,
                  (VOID**)&ClockProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = ClockProtocol->GetVersion (ClockProtocol, &ClockProtocolVersion);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }
  DEBUG ((DEBUG_ERROR, "SCMI clock management protocol version = %x\n",
    ClockProtocolVersion));

  ClockId = 0;

  Status = ClockProtocol->GetClockAttributes (
                            ClockProtocol,
                            ClockId,
                            &Enabled,
                            ClockName
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

  DEBUG ((DEBUG_INFO, "SCMI: %a: Current rate is %uHz\n", ClockName, CpuRate));

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

  CpuRate = ClockRate[TotalRates - 1].DiscreteRate.Rate;
  FreePool (ClockRate);

  DEBUG ((DEBUG_INFO, "SCMI: %a: New rate is %uHz\n", ClockName, CpuRate));

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

  DEBUG ((DEBUG_INFO, "SCMI: %a: Current rate is %uHz\n", ClockName, CpuRate));

  return EFI_SUCCESS;
}

STATIC
VOID
BoardInitGmac (
  VOID
  )
{
  UINT32 Index;

  /* Assert reset */
  CruAssertSoftReset (14, 12);

  /* Configure pins */
  for (Index = 0; Index < ARRAY_SIZE (mGmac1IomuxConfig); Index++) {
    CONST BOARD_IOMUX_CONFIG *Mux = &mGmac1IomuxConfig[Index];
    DEBUG ((DEBUG_INFO, "BOARD: Configuring pin '%a'\n", Mux->Name));
    GpioPinSetFunction (Mux->Group, Mux->Pin, Mux->Function);
    GpioPinSetPull (Mux->Group, Mux->Pin, Mux->Pull);
    if (Mux->Drive != GPIO_PIN_DRIVE_DEFAULT) {
      GpioPinSetDrive (Mux->Group, Mux->Pin, Mux->Drive);
    }
  }

  /* Setup clocks */
  MmioWrite32 (CRU_CLKSEL_CON (33), 0x00370004);  // Set rmii1_mode to rgmii mode
                                                  // Set rgmii1_clk_sel to 125M
                                                  // Set rmii1_extclk_sel to mac1 clock from IO

  /* Configure GMAC1 */
  MmioWrite32 (GRF_MAC1_CON0,
               0x7F7F0000U |
               (TX_DELAY << CLK_TX_DL_CFG_SHIFT) |
               (RX_DELAY << CLK_RX_DL_CFG_SHIFT));
  MmioWrite32 (GRF_MAC1_CON1,
               ((PHY_INTF_SEL_MASK | TXCLK_DLY_ENA | RXCLK_DLY_ENA) << 16) |
               PHY_INTF_SEL_RGMII |
               TXCLK_DLY_ENA |
               RXCLK_DLY_ENA);

  /* Reset PHY */
  GpioPinSetDirection (0, GPIO_PIN_PC3, GPIO_PIN_OUTPUT);
  MicroSecondDelay (1000);
  GpioPinWrite (0, GPIO_PIN_PC3, 0);
  MicroSecondDelay (20000);
  GpioPinWrite (0, GPIO_PIN_PC3, 1);
  MicroSecondDelay (100000);

  /* Deassert reset */
  CruDeassertSoftReset (14, 12);
}


EFI_STATUS
EFIAPI
BoardInitDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  DEBUG ((DEBUG_INFO, "BoardInitDriverEntryPoint() called\n"));

  /* Set GPIO0 PD3 (WORK_LED) output high to enable LED */
  GpioPinSetDirection (0, GPIO_PIN_PD3, GPIO_PIN_OUTPUT);
  GpioPinWrite (0, GPIO_PIN_PD3, TRUE);

  /* Update CPU speed */
  BoardInitSetCpuSpeed ();

  /* Set core_pvtpll ring length */
  MmioWrite32 (0xFDC30010U, 0x00ff002b);

  /* Configure MULTI-PHY 0 and 1 for USB3 mode */
  MultiPhySetMode (0, MULTIPHY_MODE_USB3);
  MultiPhySetMode (1, MULTIPHY_MODE_USB3);

  /* Set GPIO4 PB5 (USB_HOST_PWREN) output high to power USB ports */
  GpioPinSetDirection (4, GPIO_PIN_PB5, GPIO_PIN_OUTPUT);
  GpioPinWrite (4, GPIO_PIN_PB5, TRUE);

  /* GMAC setup */
  BoardInitGmac ();

  return EFI_SUCCESS;
}
