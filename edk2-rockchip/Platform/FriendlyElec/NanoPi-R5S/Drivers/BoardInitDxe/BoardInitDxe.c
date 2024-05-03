/** @file
 *
 *  Board init for the FriendlyELEC NanoPi-R5S board
 *
 *  Copyright (c) 2021-2023, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2022-2023, Sergey Tyuryukanov <s199p.wa1k9r@gmail.com>
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
#include <Library/I2cLib.h>
#include <Library/MultiPhyLib.h>
#include <Library/OtpLib.h>
#include <Library/SocLib.h>
#include <Library/Pcie30PhyLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <IndustryStandard/Rk356x.h>
#include <IndustryStandard/Rk356xCru.h>

#include "EthernetPhy.h"

/*
 * GMAC registers
 */
#define GMAC0_MAC_ADDRESS0_LOW  (GMAC0_BASE + 0x0304)
#define GMAC0_MAC_ADDRESS0_HIGH (GMAC0_BASE + 0x0300)

#define GRF_MAC0_CON0           (SYS_GRF + 0x0380)
#define  CLK_RX_DL_CFG_SHIFT    8
#define  CLK_TX_DL_CFG_SHIFT    0
#define GRF_MAC0_CON1           (SYS_GRF + 0x0384)
#define  PHY_INTF_SEL_SHIFT     4
#define  PHY_INTF_SEL_MASK      (0x7U << PHY_INTF_SEL_SHIFT)
#define  PHY_INTF_SEL_RGMII     (1U << PHY_INTF_SEL_SHIFT)
#define  FLOWCTRL               BIT3
#define  MAC_SPEED              BIT2
#define  RXCLK_DLY_ENA          BIT1
#define  TXCLK_DLY_ENA          BIT0

#define TX_DELAY_GMAC0          0x3C
#define RX_DELAY_GMAC0          0x2F

/*
 * PMIC registers
*/
#define PMIC_I2C_ADDR           0x20

#define PMIC_CHIP_NAME          0xed
#define PMIC_CHIP_VER           0xee
#define PMIC_POWER_EN1          0xb2
#define PMIC_POWER_EN2          0xb3
#define PMIC_POWER_EN3          0xb4
#define PMIC_LDO1_ON_VSEL       0xcc
#define PMIC_LDO2_ON_VSEL       0xce
#define PMIC_LDO3_ON_VSEL       0xd0
#define PMIC_LDO4_ON_VSEL       0xd2
#define PMIC_LDO6_ON_VSEL       0xd6
#define PMIC_LDO7_ON_VSEL       0xd8
#define PMIC_LDO8_ON_VSEL       0xda
#define PMIC_LDO9_ON_VSEL       0xdc

/*
 * CPU_GRF registers
*/
#define GRF_CPU_COREPVTPLL_CON0               (CPU_GRF + 0x0010)
#define  CORE_PVTPLL_RING_LENGTH_SEL_SHIFT    3
#define  CORE_PVTPLL_RING_LENGTH_SEL_MASK     (0x1FU << CORE_PVTPLL_RING_LENGTH_SEL_SHIFT)
#define  CORE_PVTPLL_OSC_EN                   BIT1
#define  CORE_PVTPLL_START                    BIT0

 /*
 * SYS_GRF registers
 */
#define GRF_IOFUNC_SEL5                       (SYS_GRF + 0x0314)
#define  PCIE20X1_IOMUX_SEL_MASK              (BIT3|BIT2)
#define  PCIE20X1_IOMUX_SEL_M1                BIT2
#define  PCIE20X1_IOMUX_SEL_M2                BIT3
#define  PCIE30X1_IOMUX_SEL_MASK              (BIT5|BIT4)
#define  PCIE30X1_IOMUX_SEL_M1                BIT4
#define  PCIE30X1_IOMUX_SEL_M2                BIT5
#define  PCIE30X2_IOMUX_SEL_MASK              (BIT7|BIT6)
#define  PCIE30X2_IOMUX_SEL_M1                BIT6
#define  PCIE30X2_IOMUX_SEL_M2                BIT7

/*
 * PMU registers
 */
#define PMU_NOC_AUTO_CON0                     (PMU_BASE + 0x0070)
#define PMU_NOC_AUTO_CON1                     (PMU_BASE + 0x0074)

STATIC CONST GPIO_IOMUX_CONFIG mGmac0IomuxConfig[] = {
  { "gmac0_mdcm",         2, GPIO_PIN_PC3, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_mdio",         2, GPIO_PIN_PC4, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_txd0",         2, GPIO_PIN_PB3, 1, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac0_txd1",         2, GPIO_PIN_PB4, 1, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac0_txen",         2, GPIO_PIN_PB5, 1, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_rxd0",         2, GPIO_PIN_PB6, 1, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_rxd1",         2, GPIO_PIN_PB7, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_rxdvcrs",      2, GPIO_PIN_PC0, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_rxclk",        2, GPIO_PIN_PA5, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_txclk",        2, GPIO_PIN_PB0, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_1 },
  { "gmac0_mclkinout",    2, GPIO_PIN_PC2, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_rxd2",         2, GPIO_PIN_PA3, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_rxd3",         2, GPIO_PIN_PA4, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac0_txd2",         2, GPIO_PIN_PA6, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac0_txd3",         2, GPIO_PIN_PA7, 2, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
};

STATIC CONST GPIO_IOMUX_CONFIG mSdmmc2IomuxConfig[] = {
  { "sdmmc2_d0m0",        3, GPIO_PIN_PC6, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_d1m0",        3, GPIO_PIN_PC7, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_d2m0",        3, GPIO_PIN_PD0, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_d3m0",        3, GPIO_PIN_PD1, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_cmdm0",       3, GPIO_PIN_PD2, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_clkm0",       3, GPIO_PIN_PD3, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
};

// PCIE30X2_CLKREQn_M1 - GPIO2_D4
// PCIE30X2_WAKEn_M1   - GPIO2_D5
// PCIE30X2_PERSTn_M1  - GPIO2_D6 - NVMe
STATIC CONST GPIO_IOMUX_CONFIG mPcie30x2IomuxConfig[] = {
  { "pcie30x2_clkreqnm1", 2, GPIO_PIN_PD4, 4, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "pcie30x2_perstnm1",  2, GPIO_PIN_PD6, 4, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "pcie30x2_wakenm1",   2, GPIO_PIN_PD5, 4, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
};

STATIC
VOID
BoardInitGmac (
  VOID
  )
{
  UINT32 MacLo, MacHi;

  /* Assert reset */
  CruAssertSoftReset (13, 7);	// GMAC0

  /* Configure pins */
  GpioSetIomuxConfig (mGmac0IomuxConfig, ARRAY_SIZE (mGmac0IomuxConfig));

  /* Setup GMAC0 clocks */
  MmioWrite32 (CRU_CLKSEL_CON (31), 0x00370004);  // Set rmii1_mode to rgmii mode
                                                  // Set rgmii1_clk_sel to 125M
                                                  // Set rmii1_extclk_sel to mac1 clock from IO

  /* Configure GMAC0 */
  MmioWrite32 (GRF_MAC0_CON0,
               0x7F7F0000U |
               (TX_DELAY_GMAC0 << CLK_TX_DL_CFG_SHIFT) |
               (RX_DELAY_GMAC0 << CLK_RX_DL_CFG_SHIFT));
  MmioWrite32 (GRF_MAC0_CON1,
               ((PHY_INTF_SEL_MASK | TXCLK_DLY_ENA | RXCLK_DLY_ENA) << 16) |
               PHY_INTF_SEL_RGMII |
               TXCLK_DLY_ENA |
               RXCLK_DLY_ENA);

  /* Reset GMAC0 PHY */
  GpioPinSetDirection (0, GPIO_PIN_PC5, GPIO_PIN_OUTPUT);
  MicroSecondDelay (1000);
  GpioPinWrite (0, GPIO_PIN_PC5, 0);
  MicroSecondDelay (20000);
  GpioPinWrite (0, GPIO_PIN_PC5, 1);
  MicroSecondDelay (100000);

  /* Deassert reset */
  CruDeassertSoftReset (13, 7);	// GMAC0

  /* Generate MAC addresses from the first 32 bytes in the OTP and write it to GMAC0 */
  OtpGetMacAddress (&MacLo, &MacHi);

  /* Use sequential MAC addresses. Last byte is even for GMAC0. */
  MacHi &= ~(1 << 8);
  MmioWrite32 (GMAC0_MAC_ADDRESS0_LOW, MacLo);
  MmioWrite32 (GMAC0_MAC_ADDRESS0_HIGH, MacHi);

  EthernetPhyInit (GMAC0_BASE);
}

STATIC
VOID
BoardInitPcie (
  VOID
  )
{
  GpioSetIomuxConfig (mPcie30x2IomuxConfig, ARRAY_SIZE (mPcie30x2IomuxConfig));

  /* PCIe30x2 IO mux selection - M1 */
  MmioWrite32 (GRF_IOFUNC_SEL5, (PCIE30X2_IOMUX_SEL_MASK << 16) | PCIE30X2_IOMUX_SEL_M1);

  /* PCIECLKIC_OE_H_GPIO3_A7 */
  GpioPinSetPull (3, GPIO_PIN_PA7, GPIO_PIN_PULL_NONE);
  GpioPinSetDirection (3, GPIO_PIN_PA7, GPIO_PIN_OUTPUT);
  GpioPinWrite (3, GPIO_PIN_PA7, FALSE);
}

STATIC
EFI_STATUS
PmicRead (
  IN UINT8 Register,
  OUT UINT8 *Value
  )
{
  return I2cRead (I2C0_BASE, PMIC_I2C_ADDR,
                  &Register, sizeof (Register),
                  Value, sizeof (*Value));
}

STATIC
EFI_STATUS
PmicWrite (
  IN UINT8 Register,
  IN UINT8 Value
  )
{
  return I2cWrite (I2C0_BASE, PMIC_I2C_ADDR,
                  &Register, sizeof (Register),
                  &Value, sizeof (Value));
}

STATIC
VOID
BoardInitPmic (
  VOID
  )
{
  EFI_STATUS Status;
  UINT16 ChipName;
  UINT8 ChipVer;
  UINT8 Value;

  DEBUG ((DEBUG_INFO, "BOARD: PMIC init\n"));

  GpioPinSetPull (0, GPIO_PIN_PB1, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (0, GPIO_PIN_PB1, GPIO_PIN_INPUT_SCHMITT);
  GpioPinSetFunction (0, GPIO_PIN_PB1, 1);
  GpioPinSetPull (0, GPIO_PIN_PB2, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (0, GPIO_PIN_PB2, GPIO_PIN_INPUT_SCHMITT);
  GpioPinSetFunction (0, GPIO_PIN_PB2, 1);

  Status = PmicRead (PMIC_CHIP_NAME, &Value);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to read PMIC chip name! %r\n", Status));
    ASSERT (FALSE);
  }
  ChipName = (UINT16)Value << 4;

  Status = PmicRead (PMIC_CHIP_VER, &Value);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to read PMIC chip version! %r\n", Status));
    ASSERT (FALSE);
  }
  ChipName |= (Value >> 4) & 0xF;
  ChipVer = Value & 0xF;

  DEBUG ((DEBUG_INFO, "PMIC: Detected RK%03X ver 0x%X\n", ChipName, ChipVer));
  ASSERT (ChipName == 0x809);

  /* Initialize PMIC for HDMI */
  PmicWrite (PMIC_LDO1_ON_VSEL, 0x0c);  /* 0.9V - vdda0v9_image */
  PmicWrite (PMIC_LDO2_ON_VSEL, 0x0c);  /* 0.9V - vdda_0v9 */
  PmicWrite (PMIC_LDO3_ON_VSEL, 0x0c);  /* 0.9V - vdd0v9_pmu */
  PmicWrite (PMIC_LDO4_ON_VSEL, 0x6c);  /* 3.3V - vccio_acodec */
  /* Skip LDO5 for now; 1.8V/3.3V - vccio_sd */
  PmicWrite (PMIC_LDO6_ON_VSEL, 0x6c);  /* 3.3V - vcc3v3_pmu */
  PmicWrite (PMIC_LDO7_ON_VSEL, 0x30);  /* 1.8V - vcca_1v8 */
  PmicWrite (PMIC_LDO8_ON_VSEL, 0x30);  /* 1.8V - vcca1v8_pmu */
  PmicWrite (PMIC_LDO9_ON_VSEL, 0x30);  /* 1.8V - vcca1v8_image */

  PmicWrite (PMIC_POWER_EN1, 0xff); /* LDO1, LDO2, LDO3, LDO4 */
  PmicWrite (PMIC_POWER_EN2, 0xee); /* LDO6, LDO7, LDO8 */
  PmicWrite (PMIC_POWER_EN3, 0x55); /* LDO9, SW1*/
}

EFI_STATUS
EFIAPI
BoardInitDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DEBUG ((DEBUG_INFO, "BOARD: BoardInitDriverEntryPoint() called\n"));

  SocSetDomainVoltage (PMUIO2, VCC_3V3);
  SocSetDomainVoltage (VCCIO1, VCC_3V3);
  SocSetDomainVoltage (VCCIO4, VCC_1V8);
  SocSetDomainVoltage (VCCIO5, VCC_3V3);
  SocSetDomainVoltage (VCCIO6, VCC_1V8);
  SocSetDomainVoltage (VCCIO7, VCC_3V3);

  BoardInitPmic ();

  /* I2C5 bus, used for RTC */
  //GpioPinSetPull (3, GPIO_PIN_PB3, GPIO_PIN_PULL_NONE);
  //GpioPinSetInput (3, GPIO_PIN_PB3, GPIO_PIN_INPUT_SCHMITT);
  //GpioPinSetFunction (3, GPIO_PIN_PB3, 4);
  //GpioPinSetPull (3, GPIO_PIN_PB4, GPIO_PIN_PULL_NONE);
  //GpioPinSetInput (3, GPIO_PIN_PB4, GPIO_PIN_INPUT_SCHMITT);
  //GpioPinSetFunction (3, GPIO_PIN_PB4, 4);

  /* Set GPIO4 PD2 (SYS_LED) output high to enable LED */
  GpioPinSetDirection (4, GPIO_PIN_PD2, GPIO_PIN_OUTPUT);
  GpioPinWrite (4, GPIO_PIN_PD2, TRUE);

  /* Enable automatic clock gating */
  MmioWrite32 (PMU_NOC_AUTO_CON0, 0xFFFFFFFFU);
  MmioWrite32 (PMU_NOC_AUTO_CON1, 0x000F000FU);

  /* Set core_pvtpll ring length */
  MmioWrite32 (GRF_CPU_COREPVTPLL_CON0,
               ((CORE_PVTPLL_RING_LENGTH_SEL_MASK | CORE_PVTPLL_OSC_EN | CORE_PVTPLL_START) << 16) |
               (5U << CORE_PVTPLL_RING_LENGTH_SEL_SHIFT) | CORE_PVTPLL_OSC_EN | CORE_PVTPLL_START);

  /* Configure MULTI-PHY 0 and 1 for USB3 mode */
  MultiPhySetMode (0, MULTIPHY_MODE_USB3);
  MultiPhySetMode (1, MULTIPHY_MODE_USB3);

  /* Set GPIO0 PA5 (USB_OTG5V_EN) output high to power USB ports */
  GpioPinSetDirection (0, GPIO_PIN_PA5, GPIO_PIN_OUTPUT);
  GpioPinWrite (0, GPIO_PIN_PA5, TRUE);
  /* Set GPIO0 PA6 (USB_HOST5V_EN) output high to power USB ports */
  GpioPinSetDirection (0, GPIO_PIN_PA6, GPIO_PIN_OUTPUT);
  GpioPinWrite (0, GPIO_PIN_PA6, TRUE);

  /* PCIe setup */
  BoardInitPcie ();

  /* GMAC setup */
  BoardInitGmac ();

  return EFI_SUCCESS;
}
