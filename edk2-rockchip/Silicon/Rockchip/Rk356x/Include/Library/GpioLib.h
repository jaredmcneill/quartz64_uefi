/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef GPIOLIB_H__
#define GPIOLIB_H__

#define GPIO_PIN_PA0    0
#define GPIO_PIN_PA1    1
#define GPIO_PIN_PA2    2
#define GPIO_PIN_PA3    3
#define GPIO_PIN_PA4    4
#define GPIO_PIN_PA5    5
#define GPIO_PIN_PA6    6
#define GPIO_PIN_PA7    7
#define GPIO_PIN_PB0    8
#define GPIO_PIN_PB1    9
#define GPIO_PIN_PB2    10
#define GPIO_PIN_PB3    11
#define GPIO_PIN_PB4    12
#define GPIO_PIN_PB5    13
#define GPIO_PIN_PB6    14
#define GPIO_PIN_PB7    15
#define GPIO_PIN_PC0    16
#define GPIO_PIN_PC1    17
#define GPIO_PIN_PC2    18
#define GPIO_PIN_PC3    19
#define GPIO_PIN_PC4    20
#define GPIO_PIN_PC5    21
#define GPIO_PIN_PC6    22
#define GPIO_PIN_PC7    23
#define GPIO_PIN_PD0    24
#define GPIO_PIN_PD1    25
#define GPIO_PIN_PD2    26
#define GPIO_PIN_PD3    27
#define GPIO_PIN_PD4    28
#define GPIO_PIN_PD5    29
#define GPIO_PIN_PD6    30
#define GPIO_PIN_PD7    31

typedef enum {
  GPIO_PIN_INPUT  = 0,
  GPIO_PIN_OUTPUT = 1
} GPIO_PIN_DIRECTION;

VOID
GpioPinSetDirection (
  IN UINT8 Group,
  IN UINT8 Pin,
  IN GPIO_PIN_DIRECTION Direction
  );

VOID
GpioPinWrite (
  IN UINT8 Group,
  IN UINT8 Pin,
  IN BOOLEAN Value
  );

BOOLEAN
GpioPinRead (
  IN UINT8 Group,
  IN UINT8 Pin
  );

#endif /* GPIOLIB_H__ */
