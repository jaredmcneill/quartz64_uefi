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

/* Realtek RTL8211F PHY registers */
#define	PAGSR			                    0x1F
#define LCR                                 0x10
#define LCR_VALUE                           0x6940

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
RTL8211FPhyInit (
    IN EFI_PHYSICAL_ADDRESS GmacBase
    )
{
    PhyWrite (GmacBase, 0, PAGSR, 0xD04);
    MicroSecondDelay (10000);
    PhyWrite (GmacBase, 0, LCR, LCR_VALUE);
    MicroSecondDelay (10000);
    PhyWrite (GmacBase, 0, PAGSR, 0);
}

VOID
EthernetPhyInit (
    IN EFI_PHYSICAL_ADDRESS GmacBase
    )
{
    UINT16 PhyId[2];

    PhyRead (GmacBase, 0, MII_PHYIDR1, &PhyId[0]);
    PhyRead (GmacBase, 0, MII_PHYIDR2, &PhyId[1]);
    
    if (PhyId[0] == 0x001C && PhyId[1] == 0xC916) {
        DEBUG ((DEBUG_INFO, "MDIO: Found Realtek RTL8211F GbE PHY\n"));
        RTL8211FPhyInit (GmacBase);
    } else {
        DEBUG ((DEBUG_INFO, "MDIO: Unknown PHY ID %04X %04X\n", PhyId[0], PhyId[1]));
    }
}