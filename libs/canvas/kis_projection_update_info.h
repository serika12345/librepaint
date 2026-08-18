/*
 * SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_PROJECTION_UPDATE_INFO_H
#define KIS_PROJECTION_UPDATE_INFO_H

#include <QPainter>

#include <kritacanvas_export.h>

#include "kis_update_info.h"

class KisProjectionUpdateInfo;
using KisProjectionUpdateInfoSP = KisSharedPtr<KisProjectionUpdateInfo>;

class KRITACANVAS_EXPORT KisProjectionUpdateInfo : public KisUpdateInfo
{
public:
    enum TransferType {
        Direct,
        Patch
    };

    explicit KisProjectionUpdateInfo(const QRect &dirtyImageRect = QRect());

    QRect dirtyViewportRect() override;
    QRect dirtyImageRect() const override;
    int levelOfDetail() const override;

    QRect imageRect;
    QRectF viewportRect;
    qreal scaleX {1.0};
    qreal scaleY {1.0};
    TransferType transfer {Direct};
    QPainter::RenderHints renderHints;
    qint32 borderWidth {0};

private:
    QRect m_dirtyImageRect;
};

#endif
