/*
 *  SPDX-FileCopyrightText: 2008 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2008-2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QPainter>
#include <qtypes.h>


#include "kis_hairy_paintop_settings.h"
#include "KisOptimizedBrushOutline.h"
#include "kis_brush_based_paintop_settings.h"
#include "kis_paint_information.h"
#include "kis_paintop_settings.h"

KisHairyPaintOpSettings::KisHairyPaintOpSettings(KisResourcesInterfaceSP resourcesInterface)
    : KisBrushBasedPaintOpSettings(resourcesInterface)
{
}

KisOptimizedBrushOutline KisHairyPaintOpSettings::brushOutline(const KisPaintInformation &info, const OutlineMode &mode, qreal alignForZoom)
{
    return brushOutlineImpl(info, mode, alignForZoom, getDouble("HairyBristle/scale"));
}

bool KisHairyPaintOpSettings::hasPatternSettings() const
{
    return false;
}
