/*
 *  SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2004-2008 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2004 Clarence Dang <dang@kde.org>
 *  SPDX-FileCopyrightText: 2004 Adrian Page <adrian@pagenet.plus.com>
 *  SPDX-FileCopyrightText: 2004 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DUPLICATEOP_H_
#define KIS_DUPLICATEOP_H_

#include <QPointF>

#include "kis_brush_based_paintop.h"

#include <kis_types.h>
#include <KisOpacityOption.h>
#include <KisRotationOption.h>
#include <KisStandardOptions.h>
#include <KisDuplicateOptionData.h>


#include "kis_duplicateop_settings.h"

class KisPaintInformation;


class KisPainter;


class KisDuplicateOp : public KisBrushBasedPaintOp
{

public:

    KisDuplicateOp(const KisPaintOpSettingsSP settings, KisPainter *painter, KisNodeSP node, KisImageSP image);
    ~KisDuplicateOp() override;

protected:
    KisSpacingInformation paintAt(const KisPaintInformation& info) override;

    KisSpacingInformation updateSpacingImpl(const KisPaintInformation &info) const override;

private:

    KisImageSP m_image;
    KisNodeSP m_node;

    KisDuplicateOptionData m_duplicateOptionData;
    KisDuplicateOpSettingsSP m_settings;
    KisPaintDeviceSP m_srcdev;
    KisPaintDeviceSP m_target;
    QPointF m_duplicateStart {QPointF(0.0, 0.0)};
    bool m_duplicateStartIsSet {false};
    KisSizeOption m_sizeOption;
    KisOpacityOption m_opacityOption;
    KisRotationOption m_rotationOption;
};

#endif // KIS_DUPLICATEOP_H_
