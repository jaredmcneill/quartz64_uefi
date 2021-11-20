/** @file
 *
 *  RK3566/RK3568 GPIO Library.
 * 
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/GpioLib.h>
#include <IndustryStandard/Rk356x.h>

#define GPIO_SWPORT_DR(Pin)             ((Pin) < 16 ? 0x0000 : 0x0004)
#define GPIO_SWPORT_DDR(Pin)            ((Pin) < 16 ? 0x0008 : 0x000C)

#define GPIO_WRITE_MASK(Pin)            (1U << (((Pin) % 16) + 16))
#define GPIO_VALUE_MASK(Pin, Value)     ((UINT32)Value << ((Pin) % 16))

VOID
GpioPinSetDirection (
  IN UINT8 Group,
  IN UINT8 Pin,
  IN GPIO_PIN_DIRECTION Direction
  )
{
    MmioWrite32 (GPIO_BASE (Group) + GPIO_SWPORT_DDR (Pin),
                 GPIO_WRITE_MASK (Pin) | GPIO_VALUE_MASK (Pin, Direction));
}

VOID
GpioPinWrite (
  IN UINT8 Group,
  IN UINT8 Pin,
  IN BOOLEAN Value
  )
{
    MmioWrite32 (GPIO_BASE (Group) + GPIO_SWPORT_DR (Pin),
                 GPIO_WRITE_MASK (Pin) | GPIO_VALUE_MASK (Pin, Value));
}

BOOLEAN
GpioPinRead (
  IN UINT8 Group,
  IN UINT8 Pin
  )
{
    CONST UINT32 Value = MmioRead32 (GPIO_BASE (Group) + GPIO_SWPORT_DR (Pin));
    return (Value & GPIO_VALUE_MASK (Pin, 1)) != 0;
}