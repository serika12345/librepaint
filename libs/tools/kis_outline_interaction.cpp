/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_outline_interaction.h>

bool KisOutlineInteraction::beginInput(const QPointF &position)
{
    const bool startsInteraction = !m_continuedMode || m_points.isEmpty();
    if (startsInteraction) {
        m_active = true;
    }
    m_inputActive = true;

    if (m_continuedMode) {
        m_points.append(position);
        ++m_continuedPointCount;
    } else {
        m_continuedPointCount = 0;
        m_points.append(position);
    }

    return startsInteraction;
}

void KisOutlineInteraction::addPoint(const QPointF &position)
{
    if (m_active && m_inputActive) {
        m_points.append(position);
    }
}

void KisOutlineInteraction::endInput()
{
    m_inputActive = false;
}

void KisOutlineInteraction::enableContinuedMode()
{
    m_continuedMode = true;
}

void KisOutlineInteraction::disableContinuedMode()
{
    m_continuedMode = false;
}

bool KisOutlineInteraction::isContinuedMode() const
{
    return m_continuedMode;
}

void KisOutlineInteraction::updateCursor(const QPointF &position)
{
    m_cursorPosition = position;
}

bool KisOutlineInteraction::canUndoPoint() const
{
    return m_active
        && !m_inputActive
        && m_continuedMode
        && m_continuedPointCount > 0
        && m_points.size() > 1;
}

void KisOutlineInteraction::undoPoint()
{
    if (!canUndoPoint()) {
        return;
    }

    m_points.pop_back();
    --m_continuedPointCount;
}

QVector<QPointF> KisOutlineInteraction::finish()
{
    if (!m_active) {
        return {};
    }

    QVector<QPointF> result = m_points;
    m_active = false;
    m_inputActive = false;
    m_points.clear();
    return result;
}

void KisOutlineInteraction::cancel()
{
    if (!m_active) {
        return;
    }

    m_active = false;
    m_inputActive = false;
    m_points.clear();
}

bool KisOutlineInteraction::isActive() const
{
    return m_active;
}

bool KisOutlineInteraction::isInputActive() const
{
    return m_inputActive;
}

const QVector<QPointF> &KisOutlineInteraction::points() const
{
    return m_points;
}

QPointF KisOutlineInteraction::cursorPosition() const
{
    return m_cursorPosition;
}
