/** @file
 *
 *  Defines for constructing ACPI tables
 *
 *  Copyright (c) 2020, Pete Batard <pete@akeo.ie>
 *  Copyright (c) 2019, ARM Ltd. All rights reserved.
 *  Copyright (c) 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __ACPIHEADER_H__
#define __ACPIHEADER_H__

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_OEM_ID                       {'R','K','C','P',' ',' '}
#define EFI_ACPI_OEM_TABLE_ID                 SIGNATURE_64 ('R','K','3','5','6','X',' ',' ')
#define EFI_ACPI_OEM_REVISION                 0x00000001
#define EFI_ACPI_CREATOR_ID                   SIGNATURE_32 ('E','D','K','2')
#define EFI_ACPI_CREATOR_REVISION             0x00000001

#define ACPI_HEADER(Signature, Type, Revision) {                  \
    Signature,                      /* UINT32  Signature */       \
    sizeof (Type),                  /* UINT32  Length */          \
    Revision,                       /* UINT8   Revision */        \
    0,                              /* UINT8   Checksum */        \
    EFI_ACPI_OEM_ID,                /* UINT8   OemId[6] */        \
    EFI_ACPI_OEM_TABLE_ID,          /* UINT64  OemTableId */      \
    EFI_ACPI_OEM_REVISION,          /* UINT32  OemRevision */     \
    EFI_ACPI_CREATOR_ID,            /* UINT32  CreatorId */       \
    EFI_ACPI_CREATOR_REVISION       /* UINT32  CreatorRevision */ \
  }

#endif