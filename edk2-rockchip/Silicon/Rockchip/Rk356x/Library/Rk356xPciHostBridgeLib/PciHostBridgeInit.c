/** @file

  Copyright 2017, 2020 NXP
  Copyright 2021-2023, Jared McNeill <jmcneill@invisible.ca>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/CruLib.h>
#include <Library/GpioLib.h>
#include <Library/MultiPhyLib.h>
#include <Library/Pcie30PhyLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Rk356x.h>
#include "PciHostBridgeInit.h"

/* APB Registers */
#define PCIE_CLIENT_GENERAL_CON         0x0000
#define  DEVICE_TYPE_SHIFT              4
#define  DEVICE_TYPE_MASK               (0xFU << DEVICE_TYPE_SHIFT)
#define  DEVICE_TYPE_RC                 (4 << DEVICE_TYPE_SHIFT)
#define  LINK_REQ_RST_GRT               BIT3
#define  LTSSM_ENABLE                   BIT2
#define PCIE_CLIENT_INTR_MASK_LEGACY    0x001C
#define PCIE_CLIENT_GENERAL_DEBUG_INFO  0x0104
#define PCIE_CLIENT_HOT_RESET_CTRL      0x0180
#define  APP_LSSTM_ENABLE_ENHANCE       BIT4
#define PCIE_CLIENT_LTSSM_STATUS        0x0300
#define  RDLH_LINK_UP                   BIT17
#define  SMLH_LINK_UP                   BIT16
#define  SMLH_LTSSM_STATE_MASK          0x3f
#define  SMLH_LTSSM_STATE_LINK_UP       0x11

/* DBI Registers */
#define PCI_COMMAND                     0x0004
#define PCI_DEVICE_CLASS                0x000A
#define PCI_BAR0                        0x0010
#define PCI_BAR1                        0x0014
#define PCIE_LINK_CAPABILITY            0x007C
#define PCIE_LINK_STATUS                0x0080
#define  LINK_STATUS_WIDTH_SHIFT        20
#define  LINK_STATUS_WIDTH_MASK         (0xFU << LINK_STATUS_WIDTH_SHIFT)
#define  LINK_STATUS_SPEED_SHIFT        16
#define  LINK_STATUS_SPEED_MASK         (0xFU << LINK_STATUS_SPEED_SHIFT)
#define PCIE_LINK_CTL_2                 0x00A0
#define PL_PORT_LINK_CTRL_OFF           0x0710
#define  LINK_CAPABLE_SHIFT             16
#define  LINK_CAPABLE_MASK              (0x3FU << LINK_CAPABLE_SHIFT)
#define  FAST_LINK_MODE                 BIT7
#define  DLL_LINK_EN                    BIT5
#define PL_GEN2_CTRL_OFF                0x080C
#define  DIRECT_SPEED_CHANGE            BIT17
#define  NUM_OF_LANES_SHIFT             8
#define  NUM_OF_LANES_MASK              (0x1FU << NUM_OF_LANES_SHIFT)
#define PL_MISC_CONTROL_1_OFF           0x08BC
#define  DBI_RO_WR_EN                   BIT0

/* ATU Registers */
#define ATU_CAP_BASE                    0x300000
#define IATU_REGION_CTRL_OUTBOUND(n)    (ATU_CAP_BASE + ((n) << 9))
#define IATU_REGION_CTRL_INBOUND(n)     (ATU_CAP_BASE + ((n) << 9) + 0x100)
#define IATU_REGION_CTRL_1_OFF          0x000
#define  IATU_TYPE_MEM                  0
#define  IATU_TYPE_IO                   2
#define  IATU_TYPE_CFG0                 4
#define  IATU_TYPE_CFG1                 5
#define IATU_REGION_CTRL_2_OFF          0x004
#define  IATU_ENABLE                    BIT31
#define  IATU_CFG_SHIFT_MODE            BIT28
#define IATU_LWR_BASE_ADDR_OFF          0x008
#define IATU_UPPER_BASE_ADDR_OFF        0x00C
#define IATU_LIMIT_ADDR_OFF             0x010
#define IATU_LWR_TARGET_ADDR_OFF        0x014
#define IATU_UPPER_TARGET_ADDR_OFF      0x018


STATIC CONST GPIO_IOMUX_CONFIG mPcie20IomuxConfig[] = {
  { "pcie20_clkreqnm1", 2, GPIO_PIN_PD0, 4, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "pcie20_wakenm1",   2, GPIO_PIN_PD1, 4, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
  { "pcie20_perstnm1",  3, GPIO_PIN_PC1, 4, GPIO_PIN_PULL_NONE, GPIO_PIN_DRIVE_DEFAULT },
};

STATIC
VOID
PciSetupClocks (
  IN UINTN Segment
  )
{
  UINT32 SoftResetIndex;
  UINT32 ClockIndex;
  UINTN ClockOffset;

  if (Segment == PCIE_SEGMENT_PCIE20) {
      SoftResetIndex = 10;
      ClockIndex = 12;
      ClockOffset = 0;
  } else if (Segment == PCIE_SEGMENT_PCIE30X1) {
      SoftResetIndex = 11;
      ClockIndex = 12;
      ClockOffset = 8;
  } else if (Segment == PCIE_SEGMENT_PCIE30X2) {
      SoftResetIndex = 12;
      ClockIndex = 13;
      ClockOffset = 0;
  } else {
      ASSERT(FALSE);
      return;
  }

  CruDeassertSoftReset (SoftResetIndex, 1); /* resetn_pcie{20,30x1,30x2}_powerup_req */

  CruEnableClock (ClockIndex, 5 + ClockOffset); /* clk_pcie{20,30x1,30x2}_pipe_en */
  CruEnableClock (ClockIndex, 4 + ClockOffset); /* clk_pcie{20,30x1,30x2}_aux_en */
  CruEnableClock (ClockIndex, 3 + ClockOffset); /* pclk_pcie{20,30x1,30x2}_en */
  CruEnableClock (ClockIndex, 2 + ClockOffset); /* aclk_pcie{20,30x1,30x2}_dbi_en */
  CruEnableClock (ClockIndex, 1 + ClockOffset); /* aclk_pcie{20,30x1,30x2}_slv_en */
  CruEnableClock (ClockIndex, 0 + ClockOffset); /* aclk_pcie{20,30x1,30x2}_mst_en */
}

STATIC
VOID
PciSetRcMode (
  IN EFI_PHYSICAL_ADDRESS ApbBase
  )
{
  MmioWrite32 (ApbBase + PCIE_CLIENT_INTR_MASK_LEGACY, 0xFFFF0000);
  MmioWrite32 (ApbBase + PCIE_CLIENT_HOT_RESET_CTRL,
               (APP_LSSTM_ENABLE_ENHANCE << 16) | APP_LSSTM_ENABLE_ENHANCE);
  MmioWrite32 (ApbBase + PCIE_CLIENT_GENERAL_CON,
               (DEVICE_TYPE_MASK << 16) | DEVICE_TYPE_RC);
}

STATIC
VOID
PciSetupBars (
  IN EFI_PHYSICAL_ADDRESS DbiBase
  )
{
  MmioWrite16 (DbiBase + PCI_DEVICE_CLASS, (PCI_CLASS_BRIDGE << 8) | PCI_CLASS_BRIDGE_P2P);
}

STATIC
VOID
PciDirectSpeedChange (
  IN EFI_PHYSICAL_ADDRESS DbiBase
  )
{
  DEBUG ((DEBUG_INFO, "PCIe: SetupBars: Speed change\n"));
  /* Initiate a speed change to Gen2 or Gen3 after the link is initialized as Gen1 speed. */
  MmioOr32 (DbiBase + PL_GEN2_CTRL_OFF, DIRECT_SPEED_CHANGE);
}

STATIC
VOID
PciSetupLinkSpeed (
  IN EFI_PHYSICAL_ADDRESS DbiBase,
  IN UINT32 Speed,
  IN UINT32 NumLanes
  )
{
  /* Select target link speed */
  MmioAndThenOr32 (DbiBase + PCIE_LINK_CTL_2, ~0xFU, Speed);
  MmioAndThenOr32 (DbiBase + PCIE_LINK_CAPABILITY, ~0xFU, Speed);

  /* Disable fast link mode, select number of lanes, and enable link initialization */
  MmioAndThenOr32 (DbiBase + PL_PORT_LINK_CTRL_OFF,
                   ~(LINK_CAPABLE_MASK | FAST_LINK_MODE),
                   DLL_LINK_EN | (((NumLanes * 2) - 1) << LINK_CAPABLE_SHIFT));

  /* Select link width */
  MmioAndThenOr32 (DbiBase + PL_GEN2_CTRL_OFF, ~NUM_OF_LANES_MASK,
                   NumLanes << NUM_OF_LANES_SHIFT);
}

STATIC
VOID
PciGetLinkSpeedWidth (
  IN EFI_PHYSICAL_ADDRESS DbiBase,
  OUT UINT32 *Speed,
  OUT UINT32 *Width
  )
{
  UINT32 Val;

  Val = MmioRead32 (DbiBase + PCIE_LINK_STATUS);
  *Speed = (Val & LINK_STATUS_SPEED_MASK) >> LINK_STATUS_SPEED_SHIFT;
  *Width = (Val & LINK_STATUS_WIDTH_MASK) >> LINK_STATUS_WIDTH_SHIFT;
}

STATIC
VOID
PciPrintLinkSpeedWidth (
  IN UINT32 Speed,
  IN UINT32 Width
  )
{
  char LinkSpeedBuf[6];

  switch (Speed) {
  case 0:
    AsciiStrCpyS (LinkSpeedBuf, sizeof (LinkSpeedBuf) - 1, "1.25");
    break;
  case 1:
    AsciiStrCpyS (LinkSpeedBuf, sizeof (LinkSpeedBuf) - 1, "2.5");
    break;
  case 2:
    AsciiStrCpyS (LinkSpeedBuf, sizeof (LinkSpeedBuf) - 1, "5.0");
    break;
  case 3:
    AsciiStrCpyS (LinkSpeedBuf, sizeof (LinkSpeedBuf) - 1, "8.0");
    break;
  case 4:
    AsciiStrCpyS (LinkSpeedBuf, sizeof (LinkSpeedBuf) - 1, "16.0");
    break;
  default:
    AsciiSPrint (LinkSpeedBuf, sizeof (LinkSpeedBuf), "%u.%u",
                   (Speed * 25) / 10, (Speed * 25) % 10);
    break;
  }
  DEBUG ((DEBUG_INFO, "PCIe: Link up (x%u, %a GT/s)\n", Width, LinkSpeedBuf));
}

STATIC
VOID
PciEnableLtssm (
  IN EFI_PHYSICAL_ADDRESS ApbBase,
  IN BOOLEAN Enable
  )
{
  UINT32 Val;

  Val = (LINK_REQ_RST_GRT | LTSSM_ENABLE) << 16;
  Val |= LINK_REQ_RST_GRT;
  if (Enable) {
    Val |= LTSSM_ENABLE;
  }

  MmioWrite32 (ApbBase + PCIE_CLIENT_GENERAL_CON, Val);
}

STATIC
BOOLEAN
PciIsLinkUp (
  IN EFI_PHYSICAL_ADDRESS ApbBase
  )
{
  STATIC UINT32 LastVal = 0xFFFFFFFF;
  UINT32 Val;

  Val = MmioRead32 (ApbBase + PCIE_CLIENT_LTSSM_STATUS);
  if (Val != LastVal) {
    DEBUG ((DEBUG_INFO, "PCIe: PciIsLinkUp(): LTSSM_STATUS=0x%08X\n", Val));
    LastVal = Val;
  }

  if ((Val & RDLH_LINK_UP) == 0) {
    return FALSE;
  }
  if ((Val & SMLH_LINK_UP) == 0) {
    return FALSE;
  }

  return (Val & SMLH_LTSSM_STATE_MASK) == SMLH_LTSSM_STATE_LINK_UP;
}

STATIC
VOID
PciSetupAtu (
  IN EFI_PHYSICAL_ADDRESS DbiBase,
  IN UINT32 Index,
  IN UINT32 Type,
  IN UINT64 CpuBase,
  IN UINT64 BusBase,
  IN UINT64 Length
  )
{
  UINT32 Ctrl2Off = IATU_ENABLE;

  if (Type == IATU_TYPE_CFG0 || Type == IATU_TYPE_CFG1) {
    Ctrl2Off |= IATU_CFG_SHIFT_MODE;
  }

  MmioWrite32 (DbiBase + IATU_REGION_CTRL_OUTBOUND (Index) + IATU_LWR_BASE_ADDR_OFF,
               (UINT32)CpuBase);
  MmioWrite32 (DbiBase + IATU_REGION_CTRL_OUTBOUND (Index) + IATU_UPPER_BASE_ADDR_OFF,
               (UINT32)(CpuBase >> 32));
  MmioWrite32 (DbiBase + IATU_REGION_CTRL_OUTBOUND (Index) + IATU_LIMIT_ADDR_OFF,
               (UINT32)(CpuBase + Length - 1));
  MmioWrite32 (DbiBase + IATU_REGION_CTRL_OUTBOUND (Index) + IATU_LWR_TARGET_ADDR_OFF,
               (UINT32)BusBase);
  MmioWrite32 (DbiBase + IATU_REGION_CTRL_OUTBOUND (Index) + IATU_UPPER_TARGET_ADDR_OFF,
               (UINT32)(BusBase >> 32));
  MmioWrite32 (DbiBase + IATU_REGION_CTRL_OUTBOUND (Index) + IATU_REGION_CTRL_1_OFF,
               Type);
  MmioWrite32 (DbiBase + IATU_REGION_CTRL_OUTBOUND (Index) + IATU_REGION_CTRL_2_OFF,
               Ctrl2Off);

  gBS->Stall (10000);
}

EFI_STATUS
InitializePciHost (
  EFI_PHYSICAL_ADDRESS     ApbBase,
  EFI_PHYSICAL_ADDRESS     DbiBase,
  EFI_PHYSICAL_ADDRESS     PcieSegment,
  EFI_PHYSICAL_ADDRESS     BaseAddress,
  UINT32                   NumLanes,
  UINT32                   LinkSpeed,
  UINT32                   PowerGpioBank,
  UINT32                   PowerGpioPin,
  UINT32                   ResetGpioBank,
  UINT32                   ResetGpioPin
  )
{
  UINTN                    Retry;
  UINT32                   LinkSpeed_;
  UINT32                   LinkWidth;
  UINT64                   Cfg0Base;
  UINT64                   Cfg0Size;
  UINT64                   Cfg1Base;
  UINT64                   Cfg1Size;
  UINT64                   PciIoBase;
  UINT64                   PciIoSize;

  /* Log settings */
  DEBUG ((DEBUG_INFO, "PCIe: Segment %u\n", PcieSegment));
  DEBUG ((DEBUG_INFO, "PCIe: PciExpressBaseAddress 0x%lx\n", BaseAddress));
  DEBUG ((DEBUG_INFO, "PCIe: ApbBase 0x%lx\n", ApbBase));
  DEBUG ((DEBUG_INFO, "PCIe: DbiBase 0x%lx\n", DbiBase));
  DEBUG ((DEBUG_INFO, "PCIe: NumLanes %u\n", NumLanes));
  DEBUG ((DEBUG_INFO, "PCIe: LinkSpeed %u\n", LinkSpeed));
  DEBUG ((DEBUG_INFO, "PCIe: Reset GPIO %u %u\n", ResetGpioBank, ResetGpioPin));
  DEBUG ((DEBUG_INFO, "PCIe: Power GPIO %u %u\n", PowerGpioBank, PowerGpioPin));

  ASSERT (ResetGpioBank != 0xFFU);
  ASSERT (ResetGpioPin != 0xFFU);

  /* Power PCIe */
  if (PowerGpioBank != 0xFFU) {
    GpioPinSetPull (PowerGpioBank, PowerGpioPin, GPIO_PIN_PULL_NONE);
    GpioPinSetDirection (PowerGpioBank, PowerGpioPin, GPIO_PIN_OUTPUT);
    GpioPinWrite (PowerGpioBank, PowerGpioPin, TRUE);
    gBS->Stall (100000);
  }

  if (PcieSegment == PCIE_SEGMENT_PCIE30X1 || PcieSegment == PCIE_SEGMENT_PCIE30X2) {
    /* Configure PCIe 3.0 PHY */
    Pcie30PhyInit ();
  } else {
    /* Seutp Pin Muxes */
    GpioSetIomuxConfig (mPcie20IomuxConfig, ARRAY_SIZE (mPcie20IomuxConfig));
    /* Configure PCIe 2.0 PHY */
    MultiPhySetMode (2, MULTIPHY_MODE_PCIE);
  }

  DEBUG ((DEBUG_INFO, "PCIe: Setup clocks\n"));
  if (PcieSegment == PCIE_SEGMENT_PCIE20) {
    PciSetupClocks (PCIE_SEGMENT_PCIE20);
  } else {
    PciSetupClocks (PCIE_SEGMENT_PCIE30X1);
    PciSetupClocks (PCIE_SEGMENT_PCIE30X2);
  }

  DEBUG ((DEBUG_INFO, "PCIe: Switching to RC mode\n"));
  PciSetRcMode (ApbBase);

  /* Allow writing RO registers through the DBI */
  DEBUG ((DEBUG_INFO, "PCIe: Enabling DBI access\n"));
  MmioOr32 (DbiBase + PL_MISC_CONTROL_1_OFF, DBI_RO_WR_EN);

  DEBUG ((DEBUG_INFO, "PCIe: Setup BARs\n"));
  PciSetupBars (DbiBase);

  DEBUG ((DEBUG_INFO, "PCIe: Setup iATU\n"));
  Cfg0Base = SIZE_1MB;
  Cfg0Size = SIZE_64KB;
  Cfg1Base = SIZE_2MB;
  Cfg1Size = 0x10000000UL - (SIZE_2MB + SIZE_64KB);
  PciIoBase = 0x2FFF0000UL;
  PciIoSize = SIZE_64KB;

  PciSetupAtu (DbiBase, 0, IATU_TYPE_CFG0, BaseAddress + Cfg0Base, Cfg0Base, Cfg0Size);
  PciSetupAtu (DbiBase, 1, IATU_TYPE_CFG1, BaseAddress + Cfg1Base, Cfg1Base, Cfg1Size);
  PciSetupAtu (DbiBase, 2, IATU_TYPE_IO,   BaseAddress + PciIoBase, 0, PciIoSize);

  DEBUG ((DEBUG_INFO, "PCIe: Set link speed\n"));
  PciSetupLinkSpeed (DbiBase, LinkSpeed, NumLanes);
  PciDirectSpeedChange (DbiBase);

  /* Disallow writing RO registers through the DBI */
  MmioAnd32 (DbiBase + PL_MISC_CONTROL_1_OFF, ~DBI_RO_WR_EN);

  DEBUG ((DEBUG_INFO, "PCIe: Assert reset\n"));
  GpioPinSetPull (ResetGpioBank, ResetGpioPin, GPIO_PIN_PULL_NONE);
  GpioPinSetDirection (ResetGpioBank, ResetGpioPin, GPIO_PIN_OUTPUT);
  GpioPinWrite (ResetGpioBank, ResetGpioPin, FALSE);

  DEBUG ((DEBUG_INFO, "PCIe: Start LTSSM\n"));

  PciEnableLtssm (ApbBase, TRUE);

  gBS->Stall (100000);
  DEBUG ((DEBUG_INFO, "PCIe: Deassert reset\n"));
  GpioPinWrite (ResetGpioBank, ResetGpioPin, TRUE);

  /* Wait for link up */
  DEBUG ((DEBUG_INFO, "PCIe: Waiting for link up...\n"));
  for (Retry = 20; Retry != 0; Retry--) {
    if (PciIsLinkUp (ApbBase)) {
      break;
    }
    gBS->Stall (100000);
  }
  if (Retry == 0) {
    DEBUG ((DEBUG_WARN, "PCIe: Link up timeout!\n"));
    return EFI_TIMEOUT;
  }

  PciGetLinkSpeedWidth (DbiBase, &LinkSpeed_, &LinkWidth);
  PciPrintLinkSpeedWidth (LinkSpeed_, LinkWidth);

  return EFI_SUCCESS;
}
