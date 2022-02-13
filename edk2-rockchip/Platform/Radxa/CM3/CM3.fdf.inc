#
# Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

#
# Board specific initialization
#
INF Platform/Radxa/CM3/Drivers/BoardInitDxe/BoardInitDxe.inf

#
# Devicetree support
#
# INF Platform/Rockchip/Rk356x/Drivers/FdtDxe/FdtDxe.inf

#
# ACPI Support
#
INF MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
INF MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf
INF Platform/Rockchip/Rk356x/Drivers/PlatformAcpiDxe/PlatformAcpiDxe.inf
INF RuleOverride = ACPITABLE Platform/Rockchip/Rk356x/AcpiTables/$(PLATFORM_NAME).inf

#
# SMBIOS Support
#
INF Platform/Rockchip/Rk356x/Drivers/PlatformSmbiosDxe/PlatformSmbiosDxe.inf
INF MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

#
# TianoCore logo (splash screen)
#
INF MdeModulePkg/Logo/LogoDxe.inf
