/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_polyline_interaction.h>

void KisPolylineInteraction::begin()
{
    if (!m_active) {
        m_points.clear();
        m_closeSnappingActive = false;
    }
    m_active = true;
}

bool KisPolylineInteraction::isActive() const
{
    return m_active;
}

void KisPolylineInteraction::addPoint(const QPointF &position)
{
    if (!m_active) {
        return;
    }

    m_dragStart = position;
    m_dragEnd = position;
    m_points.append(position);
}

void KisPolylineInteraction::updateCursor(const QPointF &position,
                                          bool closeSnappingActive)
{
    if (!m_active || m_points.isEmpty()) {
        return;
    }

    m_dragEnd = position;
    m_closeSnappingActive = closeSnappingActive;
}

bool KisPolylineInteraction::canUndoPoint() const
{
    return m_active && m_points.size() > 1;
}

void KisPolylineInteraction::undoPoint()
{
    if (!canUndoPoint()) {
        return;
    }

    m_points.pop_back();
    m_dragStart = m_points.last();
}

QVector<QPointF> KisPolylineInteraction::finish(bool closePolyline)
{
    if (!m_active) {
        return {};
    }

    if (closePolyline && !m_points.isEmpty()) {
        m_points.append(m_points.first());
    }

    QVector<QPointF> result = m_points;
    m_active = false;
    m_points.clear();
    m_closeSnappingActive = false;
    return result;
}

void KisPolylineInteraction::cancel()
{
    if (!m_active) {
        return;
    }

    m_active = false;
    m_points.clear();
    m_closeSnappingActive = false;
}

const QVector<QPointF> &KisPolylineInteraction::points() const
{
    return m_points;
}

QPointF KisPolylineInteraction::dragStart() const
{
    return m_dragStart;
}

QPointF KisPolylineInteraction::dragEnd() const
{
    return m_dragEnd;
}

bool KisPolylineInteraction::closeSnappingActive() const
{
    return m_closeSnappingActive;
}
