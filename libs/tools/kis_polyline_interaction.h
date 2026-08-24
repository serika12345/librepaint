/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_POLYLINE_INTERACTION_H
#define KIS_POLYLINE_INTERACTION_H

#include <QPointF>
#include <QVector>

#include <kritatools_export.h>

/**
 * Point and cursor state for a multi-click polyline interaction.
 *
 * Positions use image pixel coordinates. The UI owner supplies converted
 * pointer positions and decides close snapping from view-space distance.
 */
class KRITATOOLS_EXPORT KisPolylineInteraction
{
public:
    void begin();
    bool isActive() const;

    void addPoint(const QPointF &position);
    void updateCursor(const QPointF &position, bool closeSnappingActive);

    bool canUndoPoint() const;
    void undoPoint();

    QVector<QPointF> finish(bool closePolyline);
    void cancel();

    const QVector<QPointF> &points() const;
    QPointF dragStart() const;
    QPointF dragEnd() const;
    bool closeSnappingActive() const;

private:
    QPointF m_dragStart;
    QPointF m_dragEnd;
    QVector<QPointF> m_points;
    bool m_active {false};
    bool m_closeSnappingActive {false};
};

#endif
