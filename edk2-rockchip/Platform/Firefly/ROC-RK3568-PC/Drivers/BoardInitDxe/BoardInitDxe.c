/** @file
 *
 *  Board init for the ROC-RK3568-PC platform
 *
 *  Copyright (c) 2021-2022, Jared McNeill <jmcneill@invisible.ca>
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
#include <Library/BaseCryptLib.h>

#include <IndustryStandard/Rk356x.h>
#include <IndustryStandard/Rk356xCru.h>

#include "EthernetPhy.h"

/*
 * GMAC registers
 */
#define GMAC0_MAC_ADDRESS0_LOW  (GMAC0_BASE + 0x0304)
#define GMAC0_MAC_ADDRESS0_HIGH (GMAC0_BASE + 0x0300)
#define GMAC1_MAC_ADDRESS0_LOW  (GMAC1_BASE + 0x0304)
#define GMAC1_MAC_ADDRESS0_HIGH (GMAC1_BASE + 0x0300)

#define GRF_MAC0_CON0           (SYS_GRF + 0x0380)
#define GRF_MAC1_CON0           (SYS_GRF + 0x0388)
#define  CLK_RX_DL_CFG_SHIFT    8
#define  CLK_TX_DL_CFG_SHIFT    0
#define GRF_MAC0_CON1           (SYS_GRF + 0x0384)
#define GRF_MAC1_CON1           (SYS_GRF + 0x038C)
#define  PHY_INTF_SEL_SHIFT     4
#define  PHY_INTF_SEL_MASK      (0x7U << PHY_INTF_SEL_SHIFT)
#define  PHY_INTF_SEL_RGMII     (1U << PHY_INTF_SEL_SHIFT)
#define  FLOWCTRL               BIT3
#define  MAC_SPEED              BIT2
#define  RXCLK_DLY_ENA          BIT1
#define  TXCLK_DLY_ENA          BIT0

#define TX_DELAY_GMAC0          0x3C
#define RX_DELAY_GMAC0          0x2F
#define TX_DELAY_GMAC1          0x4F
#define RX_DELAY_GMAC1          0x26

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
#define GRF_IOFUNC_SEL0                       (SYS_GRF + 0x0300)
#define  GMAC1_IOMUX_SEL                      BIT8
#define GRF_IOFUNC_SEL5                       (SYS_GRF + 0x0314)
#define  PCIE30X2_IOMUX_SEL_MASK              (BIT7|BIT6)
#define  PCIE30X2_IOMUX_SEL_M1                BIT6

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

STATIC CONST GPIO_IOMUX_CONFIG mGmac1IomuxConfig[] = {
  { "gmac1_mdcm1",        4, GPIO_PIN_PB6, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_mdiom1",       4, GPIO_PIN_PB7, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_txd0m1",       4, GPIO_PIN_PA4, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac1_txd1m1",       4, GPIO_PIN_PA5, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac1_txenm1",       4, GPIO_PIN_PA6, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd0m1",       4, GPIO_PIN_PA7, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd1m1",       4, GPIO_PIN_PB0, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxdvcrsm1",    4, GPIO_PIN_PB1, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxclkm1",      4, GPIO_PIN_PA3, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_txclkm1",      4, GPIO_PIN_PA0, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_1 },
  { "gmac1_mclkinoutm1",  4, GPIO_PIN_PC1, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd2m1",       4, GPIO_PIN_PA1, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_rxd3m1",       4, GPIO_PIN_PA2, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "gmac1_txd2m1",       3, GPIO_PIN_PD6, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
  { "gmac1_txd3m1",       3, GPIO_PIN_PD7, 3, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_2 },
};

STATIC CONST GPIO_IOMUX_CONFIG mSdmmc2IomuxConfig[] = {
  { "sdmmc2_d0m0",        3, GPIO_PIN_PC6, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_d1m0",        3, GPIO_PIN_PC7, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_d2m0",        3, GPIO_PIN_PD0, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_d3m0",        3, GPIO_PIN_PD1, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_cmdm0",       3, GPIO_PIN_PD2, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
  { "sdmmc2_clkm0",       3, GPIO_PIN_PD3, 3, GPIO_PIN_PULL_UP,   GPIO_PIN_DRIVE_2 },
};

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
  UINT8 OtpData[32];
  UINT8 Hash[SHA256_DIGEST_SIZE];
  UINT32 MacLo, MacHi;

  /* Assert reset */
  CruAssertSoftReset (13, 7);  // GMAC0
  CruAssertSoftReset (14, 12); // GMAC1

  /* Select M1 mux solution for GMAC1 */
  MmioWrite32 (GRF_IOFUNC_SEL0, (GMAC1_IOMUX_SEL << 16) | GMAC1_IOMUX_SEL);

  /* Configure pins */
  GpioSetIomuxConfig (mGmac0IomuxConfig, ARRAY_SIZE (mGmac0IomuxConfig));
  GpioSetIomuxConfig (mGmac1IomuxConfig, ARRAY_SIZE (mGmac1IomuxConfig));

  /* Setup GMAC0 clocks */
  MmioWrite32 (CRU_CLKSEL_CON (31), 0x00370004);  // Set rmii1_mode to rgmii mode
                                                  // Set rgmii1_clk_sel to 125M
                                                  // Set rmii1_extclk_sel to mac1 clock from IO
  /* Setup GMAC1 clocks */
  MmioWrite32 (CRU_CLKSEL_CON (33), 0x00370004);  // Set rmii1_mode to rgmii mode
                                                  // Set rgmii1_clk_sel to 125M
                                                  // Set rmii1_extclk_sel to mac1 clock from IO

  /* Configure GMAC1 */
  MmioWrite32 (GRF_MAC0_CON0,
               0x7F7F0000U |
               (TX_DELAY_GMAC0 << CLK_TX_DL_CFG_SHIFT) |
               (RX_DELAY_GMAC0 << CLK_RX_DL_CFG_SHIFT));
  MmioWrite32 (GRF_MAC0_CON1,
               ((PHY_INTF_SEL_MASK | TXCLK_DLY_ENA | RXCLK_DLY_ENA) << 16) |
               PHY_INTF_SEL_RGMII |
               TXCLK_DLY_ENA |
               RXCLK_DLY_ENA);

  /* Configure GMAC1 */
  MmioWrite32 (GRF_MAC1_CON0,
               0x7F7F0000U |
               (TX_DELAY_GMAC1 << CLK_TX_DL_CFG_SHIFT) |
               (RX_DELAY_GMAC1 << CLK_RX_DL_CFG_SHIFT));
  MmioWrite32 (GRF_MAC1_CON1,
               ((PHY_INTF_SEL_MASK | TXCLK_DLY_ENA | RXCLK_DLY_ENA) << 16) |
               PHY_INTF_SEL_RGMII |
               TXCLK_DLY_ENA |
               RXCLK_DLY_ENA);

  /* Reset GMAC0 PHY */
  GpioPinSetDirection (2, GPIO_PIN_PD3, GPIO_PIN_OUTPUT);
  MicroSecondDelay (1000);
  GpioPinWrite (2, GPIO_PIN_PD3, 0);
  MicroSecondDelay (20000);
  GpioPinWrite (2, GPIO_PIN_PD3, 1);
  MicroSecondDelay (100000);

  /* Reset GMAC1 PHY */
  GpioPinSetDirection (2, GPIO_PIN_PD1, GPIO_PIN_OUTPUT);
  MicroSecondDelay (1000);
  GpioPinWrite (2, GPIO_PIN_PD1, 0);
  MicroSecondDelay (20000);
  GpioPinWrite (2, GPIO_PIN_PD1, 1);
  MicroSecondDelay (100000);

  /* Deassert reset */
  CruDeassertSoftReset (13, 7);  // GMAC0
  CruDeassertSoftReset (14, 12); // GMAC1

  /* Generate MAC addresses from the first 32 bytes in the OTP and write it to GMAC0 and GMAC1 */
  OtpRead (0x00, sizeof (OtpData), OtpData);
  Sha256HashAll (OtpData, sizeof (OtpData), Hash);
  Hash[0] &= 0xFE;
  Hash[0] |= 0x02;

  /* Use sequential MAC addresses. Last byte is even for GMAC0, and odd for GMAC1. */
  Hash[5] &= ~1;
  DEBUG ((DEBUG_INFO, "BOARD: GMAC0 MAC address %02X:%02X:%02X:%02X:%02X:%02X\n",
          Hash[0], Hash[1], Hash[2],
          Hash[3], Hash[4], Hash[5]));
  MacLo = Hash[3] | (Hash[2] << 8) | (Hash[1] << 16) | (Hash[0] << 24);
  MacHi = Hash[5] | (Hash[4] << 8);
  MmioWrite32 (GMAC0_MAC_ADDRESS0_LOW, MacLo);
  MmioWrite32 (GMAC0_MAC_ADDRESS0_HIGH, MacHi);

  Hash[5] |= 1;
  DEBUG ((DEBUG_INFO, "BOARD: GMAC1 MAC address %02X:%02X:%02X:%02X:%02X:%02X\n",
          Hash[0], Hash[1], Hash[2],
          Hash[3], Hash[4], Hash[5]));
  MacLo = Hash[3] | (Hash[2] << 8) | (Hash[1] << 16) | (Hash[0] << 24);
  MacHi = Hash[5] | (Hash[4] << 8);
  MmioWrite32 (GMAC1_MAC_ADDRESS0_LOW, MacLo);
  MmioWrite32 (GMAC1_MAC_ADDRESS0_HIGH, MacHi);

  EthernetPhyInit (GMAC0_BASE);
  EthernetPhyInit (GMAC1_BASE);
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

  /* Initialize PMIC */
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
  PmicWrite (PMIC_POWER_EN3, 0x55); /* LDO9, SW1 */
}

STATIC
VOID
BoardInitWiFi (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BOARD: WiFi init\n"));

  CruSetSdmmcClockRate (2, 100000000UL);

  /* Configure pins */
  GpioSetIomuxConfig (mSdmmc2IomuxConfig, ARRAY_SIZE (mSdmmc2IomuxConfig));

  /* Set GPIO3 PD5 (WIFI_REG_ON) output high to enable WiFi */
  GpioPinSetDirection (3, GPIO_PIN_PD5, GPIO_PIN_OUTPUT);
  MicroSecondDelay (1000);
  GpioPinWrite (3, GPIO_PIN_PD5, FALSE);
  MicroSecondDelay (500000);
  GpioPinWrite (3, GPIO_PIN_PD5, TRUE);
  MicroSecondDelay (100000);
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
  GpioPinSetPull (3, GPIO_PIN_PB3, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (3, GPIO_PIN_PB3, GPIO_PIN_INPUT_SCHMITT);
  GpioPinSetFunction (3, GPIO_PIN_PB3, 4);
  GpioPinSetPull (3, GPIO_PIN_PB4, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (3, GPIO_PIN_PB4, GPIO_PIN_INPUT_SCHMITT);
  GpioPinSetFunction (3, GPIO_PIN_PB4, 4);

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
  /* Configure MULTI-PHY 2 for SATA mode */
  MultiPhySetMode (2, MULTIPHY_MODE_SATA);

  /* Set GPIO0 PA6 (USB_HOST5V_EN) output high to power USB ports */
  GpioPinSetDirection (0, GPIO_PIN_PA6, GPIO_PIN_OUTPUT);
  GpioPinWrite (0, GPIO_PIN_PA6, TRUE);

  /* Set GPIO1 PA4 (HUB_RST_GPIO1_A4_d) output high to take USB 2.0 hub out of reset */
  GpioPinSetDirection (1, GPIO_PIN_PA4, GPIO_PIN_OUTPUT);
  GpioPinWrite (1, GPIO_PIN_PA4, TRUE);

  /* PCIe setup */
  BoardInitPcie ();

  /* GMAC setup */
  BoardInitGmac ();

  /* WiFi setup */
  BoardInitWiFi ();

  return EFI_SUCCESS;
}
