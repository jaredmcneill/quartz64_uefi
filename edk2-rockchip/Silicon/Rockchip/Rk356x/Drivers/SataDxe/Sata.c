/** @file
 *
 *  Copyright (c) 2022-2023, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/NonDiscoverableDevice.h>

#include <IndustryStandard/Rk356x.h>

#define SATA_BASE           FixedPcdGet64 (PcdSataBaseAddr)

#define SATA_CAP            0x0000
#define  SATA_CAP_SSS       BIT27
#define SATA_GHC            0x0004
#define  SATA_GHC_AE        BIT31
#define  SATA_GHC_IE        BIT1
#define  SATA_GHC_HR        BIT0
#define SATA_PI             0x000C
#define SATA_PIS            0x0110
#define SATA_CMD            0x0118
#define  SATA_CMD_FBSCP     BIT22

EFIAPI
EFI_STATUS
InitializeSataController (
    IN  EFI_PHYSICAL_ADDRESS    SataBase
    )
{
    /* Set port implemented flag */
    MmioWrite32 (SataBase + SATA_PI, 0x1);
    /* Supports staggered spin-up */
    MmioOr32 (SataBase + SATA_CAP, SATA_CAP_SSS);
    /* Supports FIS-based switching */
    MmioOr32 (SataBase + SATA_CMD, SATA_CMD_FBSCP);

    /* Reset controller */
    MmioOr32 (SataBase + SATA_GHC, SATA_GHC_HR);
    while ((MmioRead32 (SataBase + SATA_GHC) & SATA_GHC_HR) != 0) {
        DEBUG ((DEBUG_INFO, "SATA: Waiting for reset ...\n"));
        gBS->Stall (10000);
    }
    MmioWrite32 (SataBase + SATA_PIS, 0xFFFFFFFF);

    /* Enable controller */
    MmioOr32 (SataBase + SATA_GHC, SATA_GHC_AE);

    return EFI_SUCCESS;
}

VOID
EFIAPI
SataEndOfDxeCallback (
    IN EFI_EVENT  Event,
    IN VOID       *Context
    )
{
    EFI_STATUS Status;
    EFI_PHYSICAL_ADDRESS SataBase;
    UINTN Index;
    UINT32 NumSataController = PcdGet32 (PcdSataNumController);

    /* Register SATA controllers */
    for (Index = 0, SataBase = SATA_BASE;
         Index < NumSataController;
         Index++, SataBase += PcdGet64 (PcdSataSize)) {

        if ((Index == 0 && FixedPcdGet8(PcdSata0Status) == 0x0) ||
            (Index == 1 && FixedPcdGet8(PcdSata1Status) == 0x0) ||
            (Index == 2 && FixedPcdGet8(PcdSata2Status) == 0x0)) {
            continue;
        }

        DEBUG ((DEBUG_INFO, "SATA: Registering SATA controller at 0x%08X\n", SataBase));

        InitializeSataController (SataBase);

        Status = RegisterNonDiscoverableMmioDevice (
                NonDiscoverableDeviceTypeAhci,
                NonDiscoverableDeviceDmaTypeNonCoherent,
                NULL,
                NULL,
                1,
                SataBase, SIZE_4MB);
        ASSERT_EFI_ERROR (Status);
    }
}

EFI_STATUS
EFIAPI
InitializeSata (
    IN EFI_HANDLE            ImageHandle,
    IN EFI_SYSTEM_TABLE      *SystemTable
    )
{
    EFI_STATUS Status;
    EFI_EVENT EndOfDxeEvent;

    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    SataEndOfDxeCallback,
                    NULL,
                    &gEfiEndOfDxeEventGroupGuid,
                    &EndOfDxeEvent
                    );

    return Status;
}
