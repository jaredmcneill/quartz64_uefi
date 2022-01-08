/** @file
 *
 *  Copyright (c) 2022, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef I2CLIB_H__
#define I2CLIB_H__

EFI_STATUS
I2cRead (
    IN EFI_PHYSICAL_ADDRESS I2cBase,
    IN UINT8 Address,
    IN UINT8 *Register,
    IN UINT8 RegisterLength,
    OUT UINT8 *Value,
    IN UINT8 ValueLength
    );

EFI_STATUS
I2cWrite (
    IN EFI_PHYSICAL_ADDRESS I2cBase,
    IN UINT8 Address,
    IN UINT8 *Register,
    IN UINT8 RegisterLength,
    IN UINT8 *Value,
    IN UINT8 ValueLength
    );

#endif /* I2CLIB_H__ */