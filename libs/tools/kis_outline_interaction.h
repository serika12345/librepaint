/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_OUTLINE_INTERACTION_H
#define KIS_OUTLINE_INTERACTION_H

#include <QPointF>
#include <QVector>

#include <kritatools_export.h>

/**
 * Point and input state for a free-form outline interaction.
 *
 * Positions use image pixel coordinates. The UI owner supplies converted
 * pointer positions and controls view feedback for normal and continued input.
 */
class KRITATOOLS_EXPORT KisOutlineInteraction
{
public:
    bool beginInput(const QPointF &position);
    void addPoint(const QPointF &position);
    void endInput();

    void enableContinuedMode();
    void disableContinuedMode();
    bool isContinuedMode() const;

    void updateCursor(const QPointF &position);
    bool canUndoPoint() const;
    void undoPoint();

    QVector<QPointF> finish();
    void cancel();

    bool isActive() const;
    bool isInputActive() const;
    const QVector<QPointF> &points() const;
    QPointF cursorPosition() const;

private:
    QVector<QPointF> m_points;
    QPointF m_cursorPosition;
    int m_continuedPointCount {0};
    bool m_active {false};
    bool m_inputActive {false};
    bool m_continuedMode {false};
};

#endif
