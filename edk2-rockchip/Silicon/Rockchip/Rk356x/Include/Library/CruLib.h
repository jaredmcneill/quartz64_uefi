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
CruAssertSoftReset (
  IN UINT32 Index,
  IN UINT8 Bit
  );

VOID
CruDeassertSoftReset (
  IN UINT32 Index,
  IN UINT8 Bit
  );

VOID
CruEnableClock (
  IN UINT32 Index,
  IN UINT8 Bit
  );

VOID
PmuCruEnableClock (
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

VOID
CruSetEmmcClockRate (
  IN UINTN Rate
  );

VOID
CruSetPciePhySource (
  IN UINT8 Index,
  IN UINT8 Source
  );

VOID
CruSetPciePhyClockRate (
  IN UINT8 Index,
  IN UINTN Rate
  );

UINTN
CruGetPciePhyClockRate (
  IN UINT8 Index
  );

UINTN
CruGetHdmiClockRate (
  VOID
  );

VOID
CruSetHdmiClockRate (
  IN UINTN Rate
  );

VOID
CruSetGpllRate (
  IN UINTN Rate
  );

#endif /* CRULIB_H__ */
