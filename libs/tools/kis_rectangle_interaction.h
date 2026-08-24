/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_RECTANGLE_INTERACTION_H
#define KIS_RECTANGLE_INTERACTION_H

#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <kritatools_export.h>

/**
 * Geometry and input state for rectangle-shaped tool interactions.
 *
 * Positions use image pixel coordinates. The UI owner supplies converted
 * pointer positions and presents the resulting rectangle and interaction
 * state.
 */
class KRITATOOLS_EXPORT KisRectangleInteraction
{
public:
    void setConstraints(bool forceRatio,
                        bool forceWidth,
                        bool forceHeight,
                        qreal ratio,
                        qreal width,
                        qreal height);
    void setModifier(Qt::KeyboardModifier modifier, bool active);

    void begin(const QPointF &position);
    void update(const QPointF &position);

    QRectF rectangle() const;
    QRectF rectangle(const QPointF &start, const QPointF &end) const;
    QPointF start() const;
    QPointF center() const;
    QPointF end() const;
    qreal rotationAngle() const;
    bool isTranslating() const;

private:
    bool isFixedSize() const;
    void applyConstraints(QSizeF &area, bool overrideRatio) const;

    QPointF m_dragCenter;
    QPointF m_dragStart;
    QPointF m_dragEnd;
    bool m_isRatioForced {false};
    bool m_isWidthForced {false};
    bool m_isHeightForced {false};
    bool m_rotateActive {false};
    qreal m_forcedRatio {1.0};
    qreal m_forcedWidth {0.0};
    qreal m_forcedHeight {0.0};
    qreal m_referenceAngle {0.0};
    qreal m_angle {0.0};
    qreal m_angleBuffer {0.0};
    Qt::KeyboardModifiers m_currentModifiers {Qt::NoModifier};
};

#endif
