/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_RANDOM_SUB_ACCESSOR_PAINT_DEVICE_ACCESS_P_H
#define KIS_RANDOM_SUB_ACCESSOR_PAINT_DEVICE_ACCESS_P_H

#include "kis_types.h"

#include <QtGlobal>

class KisPaintDevice;

void kisSharedPtrAddReference(KisPaintDevice *device);
bool kisSharedPtrRelease(KisPaintDevice *device);

KisRandomConstAccessorSP kisRandomSubAccessorCreateRandomConstAccessor(const KisPaintDevice *device);
void kisRandomSubAccessorMixColors(const KisPaintDevice *device,
                                   const quint8 **pixels,
                                   const qint16 *weights,
                                   int numColors,
                                   quint8 *destination,
                                   int weightSum);

#endif // KIS_RANDOM_SUB_ACCESSOR_PAINT_DEVICE_ACCESS_P_H
