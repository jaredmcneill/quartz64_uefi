/** @file
 *
 *  Board init for the Orange Pi 3B platform
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *  Copyright (c) 2023, Mario Bălănică <mariobalanica02@gmail.com>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include "EthernetPhy.h"

/* GMAC registers */
#define	GMAC_MAC_MDIO_ADDRESS			    0x0200
#define	 GMAC_MAC_MDIO_ADDRESS_PA_SHIFT		21
#define	 GMAC_MAC_MDIO_ADDRESS_RDA_SHIFT	16
#define	 GMAC_MAC_MDIO_ADDRESS_CR_SHIFT		8
#define	 GMAC_MAC_MDIO_ADDRESS_CR_100_150	(1U << GMAC_MAC_MDIO_ADDRESS_CR_SHIFT)
#define	 GMAC_MAC_MDIO_ADDRESS_GOC_SHIFT	2
#define	 GMAC_MAC_MDIO_ADDRESS_GOC_READ		(3U << GMAC_MAC_MDIO_ADDRESS_GOC_SHIFT)
#define	 GMAC_MAC_MDIO_ADDRESS_GOC_WRITE	(1U << GMAC_MAC_MDIO_ADDRESS_GOC_SHIFT)
#define	 GMAC_MAC_MDIO_ADDRESS_GB		    BIT0
#define	GMAC_MAC_MDIO_DATA			        0x0204

/* MII registers */
#define MII_PHYIDR1                         0x02
#define MII_PHYIDR2                         0x03

/* Motorcomm PHY registers */
#define EXT_REG_ADDR                  0x1E
#define EXT_REG_DATA                  0x1F
#define PHY_CLOCK_GATING_REG          0x0C
#define  RX_CLK_EN                    BIT12
#define  TX_CLK_DELAY_SEL_SHIFT       4
#define  TX_CLK_DELAY_SEL             (0xFU << TX_CLK_DELAY_SEL_SHIFT)
#define  CLK_25M_SEL_SHIFT            1
#define  CLK_25M_SEL_MASK             (0x3U << CLK_25M_SEL_SHIFT)
#define  CLK_25M_SEL_125M             (3U << CLK_25M_SEL_SHIFT)
#define  RX_CLK_DELAY_EN              BIT0
#define PHY_SLEEP_CONTROL1_REG        0x27
#define  SLEEP_SW                     BIT15
#define  PLLON_IN_SLP                 BIT14

#define YTPHY_SYNCE_CFG_REG                 0xA012
#define  YT8531_SCR_SYNCE_ENABLE            BIT6
#define  YT8531_SCR_CLK_FRE_SEL_125M        BIT4
#define  YT8531_SCR_CLK_SRC_SEL_SHIFT       1
#define  YT8531_SCR_CLK_SRC_SEL_PLL_125M    (0 << YT8531_SCR_CLK_SRC_SEL_SHIFT)

#define LED1_CFG_REG            0xA00D
#define LED2_CFG_REG            0xA00E
#define LED_BLINK_CFG_REG       0xA00F

STATIC
VOID
PhyRead (
    IN EFI_PHYSICAL_ADDRESS GmacBase,
    IN UINT8 Phy,
    IN UINT16 Reg,
    OUT UINT16 *Value
    )
{
    UINT32 Addr;
    UINTN Retry;

    Addr = GMAC_MAC_MDIO_ADDRESS_CR_100_150 |
           (Phy << GMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
           (Reg << GMAC_MAC_MDIO_ADDRESS_RDA_SHIFT) |
           GMAC_MAC_MDIO_ADDRESS_GOC_READ |
           GMAC_MAC_MDIO_ADDRESS_GB;
    MmioWrite32 (GmacBase + GMAC_MAC_MDIO_ADDRESS, Addr);

    MicroSecondDelay (10000);

    for (Retry = 1000; Retry > 0; Retry--) {
        Addr = MmioRead32 (GmacBase + GMAC_MAC_MDIO_ADDRESS);
        if ((Addr & GMAC_MAC_MDIO_ADDRESS_GB) == 0) {
            *Value = MmioRead32 (GmacBase + GMAC_MAC_MDIO_DATA) & 0xFFFFu;
            break;
        }
        MicroSecondDelay (10);
    }
    if (Retry == 0) {
        DEBUG ((DEBUG_WARN, "MDIO: PHY read timeout!\n"));
        *Value = 0xFFFFU;
        ASSERT (FALSE);
    }
}

STATIC
VOID
PhyWrite (
    IN EFI_PHYSICAL_ADDRESS GmacBase,
    IN UINT8 Phy,
    IN UINT16 Reg,
    IN UINT16 Value
    )
{
    UINT32 Addr;
    UINTN Retry;

    MmioWrite32 (GmacBase + GMAC_MAC_MDIO_DATA, Value);

    Addr = GMAC_MAC_MDIO_ADDRESS_CR_100_150 |
           (Phy << GMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
           (Reg << GMAC_MAC_MDIO_ADDRESS_RDA_SHIFT) |
           GMAC_MAC_MDIO_ADDRESS_GOC_WRITE |
           GMAC_MAC_MDIO_ADDRESS_GB;
    MmioWrite32 (GmacBase + GMAC_MAC_MDIO_ADDRESS, Addr);

    MicroSecondDelay (10000);

    for (Retry = 1000; Retry > 0; Retry--) {
        Addr = MmioRead32 (GmacBase + GMAC_MAC_MDIO_ADDRESS);
        if ((Addr & GMAC_MAC_MDIO_ADDRESS_GB) == 0) {
            break;
        }
        MicroSecondDelay (10);
    }
    if (Retry == 0) {
        DEBUG ((DEBUG_WARN, "MDIO: PHY write timeout!\n"));
        ASSERT (FALSE);
    }
}

STATIC
VOID
YT8531PhyInit (
    IN EFI_PHYSICAL_ADDRESS GmacBase
    )
{
    UINT16 OldAddr;
    UINT16 Data;

    PhyRead (GmacBase, 0, EXT_REG_ADDR, &OldAddr);

    PhyWrite (GmacBase, 0, EXT_REG_ADDR, YTPHY_SYNCE_CFG_REG);
    PhyWrite (GmacBase, 0, EXT_REG_DATA, YT8531_SCR_SYNCE_ENABLE |
                                        YT8531_SCR_CLK_FRE_SEL_125M |
                                        YT8531_SCR_CLK_SRC_SEL_PLL_125M);

    PhyWrite (GmacBase, 0, EXT_REG_ADDR, PHY_CLOCK_GATING_REG);
    PhyRead (GmacBase, 0, EXT_REG_DATA, &Data);
    Data &= ~RX_CLK_EN;
    Data &= ~CLK_25M_SEL_MASK;
    Data |= CLK_25M_SEL_125M;
    PhyWrite (GmacBase, 0, EXT_REG_DATA, Data);

    PhyWrite (GmacBase, 0, EXT_REG_ADDR, PHY_SLEEP_CONTROL1_REG);
    PhyRead (GmacBase, 0, EXT_REG_DATA, &Data);
    Data &= ~SLEEP_SW;
    PhyWrite (GmacBase, 0, EXT_REG_DATA, Data);

    PhyWrite (GmacBase, 0, EXT_REG_ADDR, LED1_CFG_REG);
    PhyWrite (GmacBase, 0, EXT_REG_DATA, 0x0670);

    PhyWrite (GmacBase, 0, EXT_REG_ADDR, LED2_CFG_REG);
    PhyWrite (GmacBase, 0, EXT_REG_DATA, 0x2070);

    PhyWrite (GmacBase, 0, EXT_REG_ADDR, LED_BLINK_CFG_REG);
    PhyWrite (GmacBase, 0, EXT_REG_DATA, 0x007e);

    PhyWrite (GmacBase, 0, EXT_REG_ADDR, OldAddr);
}

VOID
EthernetPhyInit (
    IN EFI_PHYSICAL_ADDRESS GmacBase
    )
{
    UINT16 PhyId[2];

    PhyRead (GmacBase, 0, MII_PHYIDR1, &PhyId[0]);
    PhyRead (GmacBase, 0, MII_PHYIDR2, &PhyId[1]);

    if (PhyId[0] == 0x4F51 && PhyId[1] == 0xE91B) {
        DEBUG ((DEBUG_INFO, "MDIO: Found Motorcomm YT8531 GbE PHY\n"));
        YT8531PhyInit (GmacBase);
    } else {
        DEBUG ((DEBUG_INFO, "MDIO: Unknown PHY ID %04X %04X\n", PhyId[0], PhyId[1]));
    }
}
