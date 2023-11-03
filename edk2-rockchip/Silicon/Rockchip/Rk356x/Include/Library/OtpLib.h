/** @file
 *
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef OTPLIB_H__
#define OTPLIB_H__

VOID
OtpRead (
    IN UINT16 Offset,
    IN UINT16 Length,
    OUT UINT8 *Data
    );

UINT64
OtpGetSerial (
    VOID
    );

VOID
OtpGetMacAddress (
    OUT UINT32 *MacLo,
    OUT UINT32 *MacHi
    );

#endif /* OTPLIB_H__ */