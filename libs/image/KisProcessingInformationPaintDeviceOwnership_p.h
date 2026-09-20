/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PROCESSING_INFORMATION_PAINT_DEVICE_OWNERSHIP_P_H
#define KIS_PROCESSING_INFORMATION_PAINT_DEVICE_OWNERSHIP_P_H

class KisPaintDevice;

void kisSharedPtrAddReference(KisPaintDevice *device);
bool kisSharedPtrRelease(KisPaintDevice *device);

#endif // KIS_PROCESSING_INFORMATION_PAINT_DEVICE_OWNERSHIP_P_H
