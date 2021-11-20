/** @file
 *
 *  Copyright (c) 2021, Andrey Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) 2019, Pete Batard <pete@akeo.ie>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef RK356X_MEM_H__
#define RK356X_MEM_H__

#define RK356X_MEM_UNMAPPED_REGION 0
#define RK356X_MEM_BASIC_REGION    1
#define RK356X_MEM_RUNTIME_REGION  2
#define RK356X_MEM_RESERVED_REGION 3

typedef struct {
  CONST CHAR16*                 Name;
  UINTN                         Type;
} RK356X_MEMORY_REGION_INFO;

VOID
Rk356xPlatformGetVirtualMemoryInfo (
  IN RK356X_MEMORY_REGION_INFO** MemoryInfo
  );

#endif /* RK356X_MEM_H__ */
