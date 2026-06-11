/** @file
 *  ROC-RK3566-PC GMAC1 for ACPI-only Linux (PRP0001).
 *
 *  Used only from Dsdt-AcpiOnly.asl (FV ROC-RK3566-PC-AcpiOnly.inf). ACPI+DT boots
 *  use the stock Dsdt.asl with Gmac.asl instead (see PlatformAcpiDxe).
 *
 *  PHY: RTL8211F-class RGMII at MDIO address 0 (matches rk3566-roc-pc.dts).
 *  BoardInitDxe resets GPIO0 PB7 and programs GRF; Linux uses phylink + MDIO.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#include <IndustryStandard/Acpi60.h>

// Gigabit Media Access Controller (GMAC1) @ 0xFE010000
Device (MAC1) {
    Name (_HID, "PRP0001")
    Name (_UID, 3)
    Name (_CCA, Zero)

    Name (_STA, FixedPcdGet8 (PcdMac1Status))

    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "compatible", Package () {
                "rockchip,rk3568-gmac",
                "snps,dwmac-4.20a",
                "snps,dwmac"
            } },
            Package () { "phy-mode", "rgmii" },
            Package (2) { "phy-handle", \_SB.MAC1.PHY0 },
            Package () { "clock_in_out", "input" },
            Package () { "tx_delay", 0x4F },
            Package () { "rx_delay", 0x24 },
            //
            // GRF syscon physical base (see rk356x.dtsi grf@fdc60000).
            // Linux maps this when devicetree is absent.
            //
            Package () { "rockchip,grf", 0xFDC60000 },
            //
            // BoardInitDxe already programs clocks, pinmux, PHY reset, and GRF.
            //
            Package () { "rockchip,uefi-initialized", 1 },
            Package () { "interrupt-names", Package () { "macirq", "eth_wake_irq" } },
            Package () { "snps,mixed-burst", 1 },
            Package () { "snps,tso", 1 },
            Package () { "snps,axi-config", "AXIC" },
        }
    })

    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate() {
            Memory32Fixed (ReadWrite, 0xFE010000, 0x10000)
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 64, 61 }
        })
        Return (RBUF)
    }

    Name (AXIC, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
        Package () {
            Package () { "snps,wr_osr_lmt", 4 },
            Package () { "snps,rd_osr_lmt", 8 },
            Package () { "snps,blen", Package () { 0, 0, 0, 0, 16, 8, 4 } },
        }
    })

    //
    // MDIO PHY @ address 0 — same as &mdio1 { ethernet-phy@0 } in rk3566-roc-pc.dts.
    //
    Device (PHY0) {
        Name (_ADR, Zero)
        Name (_DSD, Package () {
            ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
                Package () { "compatible", Package () {
                    "ethernet-phy-ieee802.3-c22"
                } },
            }
        })
    }
}
