/** @file
 *
 *  Board init for the NanoPi RSC platform
 *
 *  Copyright (c) 2021-2022, Joel Winarske <joel.winarske@gmail.com>
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
#define PMIC_LDO5_ON_VSEL       0xd4
#define PMIC_LDO6_ON_VSEL       0xd6
#define PMIC_LDO7_ON_VSEL       0xd8
#define PMIC_LDO8_ON_VSEL       0xda
#define PMIC_LDO9_ON_VSEL       0xdc

//#define DUMP_PMIC_REGS

/*
 * RTC registers
*/
STATIC UINTN                                  mRtcI2cBusBase = FixedPcdGet32 (PcdRtcI2cBusBase);
STATIC UINT8                                  mRtcI2cAddress = FixedPcdGet8 (PcdRtcI2cAddr);
#define RTC_CS1                               0x00
#define RTC_CS2                               0x01
#define RTC_SECONDS                           0x02
#define  RTC_SECONDS_DL                       BIT7
#define RTC_CLK_OUT                           0x0D

/*
 * SAR ADC registers
*/
STATIC UINTN                                  mSarAdcBase = FixedPcdGet32 (PcdSarAdcBase);
#define SARADC_DATA		                        (mSarAdcBase + 0)
#define SARADC_CTRL		                        (mSarAdcBase + 8)
#define SARADC_VIN0_KEY_CHANNEL               0
#define SARADC_VIN1_HW_ID_CHANNEL             1
#define SARADC_VIN2_VUSBC_CHANNEL             2

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

#if defined(DUMP_PMIC_REGS)
STATIC
VOID
PrintPmicRegister (
  UINT8 Register
  )
{
  EFI_STATUS Status;
  UINT8 Value;

  Status = PmicRead (Register, &Value);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to read PMIC! %r\n", Status));
    ASSERT (FALSE);
  }
  DEBUG ((DEBUG_INFO, "PMIC: 0x%02X 0x%02X\n", Register, Value));
}
#endif

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

  /* i2c0_scl */
  GpioPinSetPull (0, GPIO_PIN_PB1, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (0, GPIO_PIN_PB1, GPIO_PIN_INPUT_SCHMITT);
  GpioPinSetFunction (0, GPIO_PIN_PB1, 1);

  /* i2c0_sda */
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
  PmicWrite (PMIC_LDO1_ON_VSEL, 0x0c);  /* 0.9V - VDDA0V9_IMAGE */
  PmicWrite (PMIC_LDO2_ON_VSEL, 0x0c);  /* 0.9V - VDDA_0V9 */
  PmicWrite (PMIC_LDO3_ON_VSEL, 0x0c);  /* 0.9V - VDDA0V9_PMU */
  PmicWrite (PMIC_LDO4_ON_VSEL, 0x6c);  /* 3.3V - VCCIO_ACODEC */
  PmicWrite (PMIC_LDO5_ON_VSEL, 0x6c);  /* 3.3V - VCCIO_SD */
  PmicWrite (PMIC_LDO6_ON_VSEL, 0x6c);  /* 3.3V - VCC3V3_PMU */
  PmicWrite (PMIC_LDO7_ON_VSEL, 0x30);  /* 1.8V - VCCA_1V8 */
  PmicWrite (PMIC_LDO8_ON_VSEL, 0x30);  /* 1.8V - VCCA1V8_PMU */
  PmicWrite (PMIC_LDO9_ON_VSEL, 0x30);  /* 1.8V - VCCA1V8_IMAGE */

  PmicWrite (PMIC_POWER_EN1, 0xff); /* LDO1, LDO2, LDO3, LDO4 */
  PmicWrite (PMIC_POWER_EN2, 0xee); /* LDO6, LDO7, LDO8 */
  PmicWrite (PMIC_POWER_EN3, 0x55); /* LDO9, SW1 */

#if defined(DUMP_PMIC_REGS)
  for(int i = 0; i < 0xff; i++) {
      PrintPmicRegister(i);
  }
#endif
}

STATIC
EFI_STATUS
RtcRead (
    IN UINT8    Register,
    OUT UINT8   *Value
    )
{
    return I2cRead (mRtcI2cBusBase, mRtcI2cAddress,
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
    return I2cWrite (mRtcI2cBusBase, mRtcI2cAddress,
                     &Register, sizeof (Register),
                     &Value, sizeof (Value));
}

STATIC
VOID
BoardInitRtc (
  VOID
  )
{
  EFI_STATUS Status;
  UINT8 Seconds;

  DEBUG ((DEBUG_INFO, "BOARD: RTC init\n"));

  /* RTC - i2c5m0_scl */
  GpioPinSetPull (3, GPIO_PIN_PB3, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (3, GPIO_PIN_PB3, GPIO_PIN_INPUT_SCHMITT);
  GpioPinSetFunction (3, GPIO_PIN_PB3, 4);

  /* RTC -  i2c5m0_sda */
  GpioPinSetPull (3, GPIO_PIN_PB4, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (3, GPIO_PIN_PB4, GPIO_PIN_INPUT_SCHMITT);
  GpioPinSetFunction (3, GPIO_PIN_PB4, 4);

  /* Initialize */
  RtcWrite (RTC_CS1, 0x00);
  RtcWrite (RTC_CS2, 0x00);
  RtcWrite (RTC_CLK_OUT, 0x00);

  /* Check voltage low bit, clear if set */
  Status = RtcRead (RTC_SECONDS, &Seconds);
  if (!EFI_ERROR (Status)) {
    if ((Seconds & RTC_SECONDS_DL) != 0) {
        DEBUG ((DEBUG_WARN, "RTC voltage-low detect bit set; clock integrity not guaranteed.\n"));
        RtcWrite (RTC_SECONDS, Seconds & ~RTC_SECONDS_DL);
    }
  }
}

STATIC
VOID
BoardInitNetwork (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BOARD: m.2 init\n"));

  /* m.2 WiFi */
  DEBUG ((DEBUG_INFO, "m.2: Assert reset\n"));
  GpioPinSetPull (2, GPIO_PIN_PD2, GPIO_PIN_PULL_NONE);
  GpioPinSetDirection (2, GPIO_PIN_PD2, GPIO_PIN_OUTPUT);
  GpioPinWrite (2, GPIO_PIN_PD2, FALSE);

  gBS->Stall (100000);
  DEBUG ((DEBUG_INFO, "m.2: Deassert reset\n"));
  GpioPinWrite (2, GPIO_PIN_PD2, TRUE);
}

STATIC
VOID
BoardInitLED (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BOARD: LED init\n"));

  /* POWER - ON */
  GpioPinSetPull (3, GPIO_PIN_PA2, GPIO_PIN_PULL_NONE);
  GpioPinSetDirection (3, GPIO_PIN_PA2, GPIO_PIN_OUTPUT);
  GpioPinWrite (3, GPIO_PIN_PA2, TRUE);

  /* LAN - OFF */
  GpioPinSetPull (3, GPIO_PIN_PA3, GPIO_PIN_PULL_NONE);
  GpioPinSetDirection (3, GPIO_PIN_PA3, GPIO_PIN_OUTPUT);
  GpioPinWrite (3, GPIO_PIN_PA3, FALSE);

  /* WAN - OFF */
  GpioPinSetPull (3, GPIO_PIN_PA4, GPIO_PIN_PULL_NONE);
  GpioPinSetDirection (3, GPIO_PIN_PA4, GPIO_PIN_OUTPUT);
  GpioPinWrite (3, GPIO_PIN_PA4, FALSE);

  /* WLAN - OFF */
  GpioPinSetPull (3, GPIO_PIN_PA5, GPIO_PIN_PULL_NONE);
  GpioPinSetDirection (3, GPIO_PIN_PA5, GPIO_PIN_OUTPUT);
  GpioPinWrite (3, GPIO_PIN_PA5, FALSE);
}

STATIC
UINT32
GetSarAdcValue(
    int channel
    )
{
	UINT32 result = 0;
	int timeout = 0;

  MmioWrite32 (SARADC_CTRL, 0);
  MicroSecondDelay (2);

  MmioWrite32 (SARADC_CTRL, 0x28 | channel);
  MicroSecondDelay (50);

	do {
		if (MmioRead32(SARADC_CTRL) & 0x40) {
			result = MmioRead32(SARADC_DATA) & 0x3FF;
			goto stop_adc;
		}
    MicroSecondDelay (10);
	} while (timeout++ < 100);

stop_adc:
    MmioWrite32 (SARADC_CTRL, 0);

	return result;
}

STATIC
VOID
GetBoardPowerSupplyVoltage (
  OUT int *milli_volt
  ) {
  UINT32 reading = 0;
	*milli_volt = 5000;

  reading = GetSarAdcValue(SARADC_VIN2_VUSBC_CHANNEL);

	if (reading > 200 && reading < 650) {
		*milli_volt = reading * 196 - 2130;
		*milli_volt /= 10;
	}
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
  SocSetDomainVoltage (VCCIO2, VCC_1V8);
  SocSetDomainVoltage (VCCIO3, VCC_3V3);
  SocSetDomainVoltage (VCCIO4, VCC_1V8);
  SocSetDomainVoltage (VCCIO5, VCC_3V3);
  SocSetDomainVoltage (VCCIO6, VCC_1V8);
  SocSetDomainVoltage (VCCIO7, VCC_3V3);

  /* Set FLASH_VOL_SEL - VCCIO2=1.8V */
  //GpioPinSetPull (0, GPIO_PIN_PA7, GPIO_PIN_PULL_NONE);
  //GpioPinSetDirection (0, GPIO_PIN_PA7, GPIO_PIN_OUTPUT);
  //GpioPinWrite (0, GPIO_PIN_PA7, FALSE);

  BoardInitPmic ();
  BoardInitRtc ();
  
  int val = GetSarAdcValue(SARADC_VIN0_KEY_CHANNEL);
  DEBUG((DEBUG_INFO, "SAR-ADC[0] Key: %d\n", val));
  val = GetSarAdcValue(SARADC_VIN1_HW_ID_CHANNEL);
  DEBUG((DEBUG_INFO, "SAR-ADC[1] HW ID: %d\n", val));

  int vcc5v0_sys;
  GetBoardPowerSupplyVoltage(&vcc5v0_sys);
  DEBUG((DEBUG_INFO,"VCC5V0_SYS: %d mV\n", vcc5v0_sys));

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

  /* User Button */
  GpioPinSetPull (0, GPIO_PIN_PB7, GPIO_PIN_PULL_NONE);
  GpioPinSetInput (0, GPIO_PIN_PB7, GPIO_PIN_INPUT_SCHMITT);

  /* enable VBUS (USB_OTG_PWREN_H_GPIO0_A5) */
  /* needs OTG implementation - until then leave VBUS on */
  GpioPinSetPull (0, GPIO_PIN_PA5, GPIO_PIN_PULL_UP);
  GpioPinSetDirection (0, GPIO_PIN_PA5, GPIO_PIN_OUTPUT);
  GpioPinWrite (0, GPIO_PIN_PA5, TRUE);

  /* enable VBUS (USB_HOST_PWREN_H_GPIO0_A6) */
  GpioPinSetPull (0, GPIO_PIN_PA6, GPIO_PIN_PULL_UP);
  GpioPinSetDirection (0, GPIO_PIN_PA6, GPIO_PIN_OUTPUT);
  GpioPinWrite (0, GPIO_PIN_PA6, TRUE);

  BoardInitNetwork ();

  BoardInitLED ();

  return EFI_SUCCESS;
}
