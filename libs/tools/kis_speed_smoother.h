/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_SPEED_SMOOTHER_H
#define KIS_SPEED_SMOOTHER_H

#include <QScopedPointer>

#include <kritatools_export.h>

class QPointF;

class KRITATOOLS_EXPORT KisSpeedSmoother
{
public:
    KisSpeedSmoother();
    ~KisSpeedSmoother();

    qreal lastSpeed() const;
    qreal getNextSpeed(const QPointF &point, ulong timestamp);
    void clear();

    void setSettings(bool useEventTimestamps, int smoothingSamples);

private:
    qreal getNextSpeedImpl(const QPointF &point, qreal time);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif
