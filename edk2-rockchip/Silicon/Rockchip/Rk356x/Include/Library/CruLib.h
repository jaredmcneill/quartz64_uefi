/** @file
 *
 *  Copyright (c) 2021, Jared McNeill <jmcneill@invisible.ca>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef CRULIB_H__
#define CRULIB_H__

UINT64
CruGetCoreClockRate (
  VOID
  );

VOID
CruDeassertSoftReset (
  IN UINT32 Index,
  IN UINT8 Bit
  );

UINTN
CruGetSdmmcClockRate (
  IN UINT8 Index
  );

VOID
CruSetSdmmcClockRate (
  IN UINT8 Index,
  IN UINTN Rate
  );

UINTN
CruGetPciePhyClockRate (
  IN UINT8 Index
  );

#endif /* CRULIB_H__ */
