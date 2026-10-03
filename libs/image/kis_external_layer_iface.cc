/*
 *  SPDX-FileCopyrightText: 2020 Sharaf Zaman <sharafzaz121@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_external_layer_iface.h"
#include "KoColorSpace.h"
#include "kis_layer.h"
#include <qpaintdevice.h>
#include <QtGlobal>

// This is a Key function, this will create a strong symbol
void KisExternalLayer::resetCache(const KoColorSpace *colorSpace)
{
    Q_UNUSED(colorSpace);
}

QRect KisExternalLayer::theoreticalBoundingRect() const
{
    return KisLayer::exactBounds();
}
