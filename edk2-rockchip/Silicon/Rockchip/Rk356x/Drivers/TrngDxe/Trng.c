/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
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

#include <Protocol/Rng.h>

#include <IndustryStandard/Rk356x.h>

#define TRNG_RST_CTL            (TRNG_BASE + 0x0004)
#define  SW_RNG_RESET           BIT1
#define TRNG_RNG_CTL            (TRNG_BASE + 0x0400)
#define  RNG_LEN_SHIFT          4
#define  RNG_LEN_256            (3 << RNG_LEN_SHIFT)
#define  RING_SEL_SHIFT         2
#define  RING_SEL_FASTEST       (3 << RING_SEL_SHIFT)
#define  RNG_ENABLE             BIT1
#define  RNG_START              BIT0
#define TRNG_RNG_SAMPLE_COUNT   (TRNG_BASE + 0x0404)
#define TRNG_RNG_DOUT(n)        (TRNG_BASE + 0x0410 + (n) * 0x4)

#define DEFAULT_RNG_SAMPLE_COUNT    100
#define RNG_START_RETRY_COUNT       100000
#define MAX_BYTES_PER_READ          32

STATIC
EFI_STATUS
TrngStart (
    VOID
    )
{
    UINT32 Retry;

    MmioWrite32 (TRNG_RNG_SAMPLE_COUNT, DEFAULT_RNG_SAMPLE_COUNT);
    MmioWrite32 (TRNG_RNG_CTL, (0xFFFFU << 16) | RNG_LEN_256 | RNG_ENABLE | RING_SEL_FASTEST);
    MmioWrite32 (TRNG_RNG_CTL, (RNG_START << 16) | RNG_START);
    for (Retry = 0; Retry < RNG_START_RETRY_COUNT; Retry++) {
        if ((MmioRead32 (TRNG_RNG_CTL) & RNG_START) == 0) {
            break;
        }
    }
    ASSERT (Retry < RNG_START_RETRY_COUNT);
    if (Retry == RNG_START_RETRY_COUNT) {
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

STATIC
VOID
TrngStop (
    VOID
    )
{
    MmioWrite32 (TRNG_RNG_CTL, 0xFFFFU << 16);
}

STATIC
EFI_STATUS
EFIAPI
TrngGetInfo (
    IN EFI_RNG_PROTOCOL *This,
    IN OUT UINTN *RNGAlgorithmListSize,
    OUT EFI_RNG_ALGORITHM *RNGAlgorithmList
    )
{
    if (This == NULL || RNGAlgorithmListSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (*RNGAlgorithmListSize < sizeof (EFI_RNG_ALGORITHM)) {
        *RNGAlgorithmListSize = sizeof (EFI_RNG_ALGORITHM);
        return EFI_BUFFER_TOO_SMALL;
    }

    if (RNGAlgorithmList == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    *RNGAlgorithmListSize = sizeof (EFI_RNG_ALGORITHM);
    CopyGuid (RNGAlgorithmList, &gEfiRngAlgorithmRaw);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
TrngGetRNG (
    IN EFI_RNG_PROTOCOL *This,
    IN EFI_RNG_ALGORITHM *RNGAlgorithm,
    IN UINTN RNGValueLength,
    OUT UINT8 *RNGValue
    )
{
    EFI_STATUS Status;
    UINT8 Count, Index;
    UINT32 Val;

    if (This == NULL || RNGValueLength == 0 || RNGValue == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (RNGAlgorithm != NULL && !CompareGuid (RNGAlgorithm, &gEfiRngAlgorithmRaw)) {
        return EFI_UNSUPPORTED;
    }

    while (RNGValueLength > 0) {
        /* Starting the counter generates 256 random bits */
        Status = TrngStart ();
        if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_WARN, "TRNG: Failed to start: %r\n", Status));
            return Status;
        }

        for (Index = 0; Index < 8; Index++) {
            Val = MmioRead32 (TRNG_RNG_DOUT (Index));
            Count = MIN (RNGValueLength, 4);
            CopyMem (RNGValue, &Val, Count);
            RNGValue += Count;
            RNGValueLength -= Count;
            if (RNGValueLength == 0) {
                break;
            }
        }

        TrngStop ();
    }

    ASSERT (RNGValueLength == 0);

    return EFI_SUCCESS;
}

STATIC EFI_RNG_PROTOCOL mTrngRngProtocol = {
    TrngGetInfo,
    TrngGetRNG
};

STATIC
EFI_STATUS
TrngInit (
    VOID
    )
{
    UINTN Retry;

    MmioWrite32 (TRNG_RST_CTL, (SW_RNG_RESET << 16) | SW_RNG_RESET);
    for (Retry = 0; Retry < 10000; Retry++) {
        if ((MmioRead32 (TRNG_RST_CTL) & SW_RNG_RESET) == 0) {
            break;
        }
        MicroSecondDelay (100);
    }
    ASSERT (Retry < 10000);
    if (Retry == 10000) {
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializeTrng (
    IN EFI_HANDLE            ImageHandle,
    IN EFI_SYSTEM_TABLE      *SystemTable
    )
{
    EFI_HANDLE Handle;
    EFI_STATUS Status;

    Status = TrngInit ();
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "TRNG: Failed to initialize: %r\n", Status));
        return Status;
    }

    Handle = NULL;
    return gBS->InstallMultipleProtocolInterfaces (
        &Handle,
        &gEfiRngProtocolGuid, &mTrngRngProtocol,
        NULL
        );
}