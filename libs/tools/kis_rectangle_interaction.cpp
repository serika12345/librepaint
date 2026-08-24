/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_rectangle_interaction.h>

#include <QTransform>

#include <cmath>

void KisRectangleInteraction::setConstraints(bool forceRatio,
                                             bool forceWidth,
                                             bool forceHeight,
                                             qreal ratio,
                                             qreal width,
                                             qreal height)
{
    m_isWidthForced = forceWidth;
    m_isHeightForced = forceHeight;
    m_isRatioForced = forceRatio;
    m_forcedHeight = height;
    m_forcedWidth = width;
    m_forcedRatio = ratio;

    if (ratio < 0.0001) {
        m_isRatioForced = false;
    }
}

void KisRectangleInteraction::setModifier(Qt::KeyboardModifier modifier,
                                          bool active)
{
    m_currentModifiers.setFlag(modifier, active);
}

void KisRectangleInteraction::begin(const QPointF &position)
{
    m_currentModifiers = Qt::NoModifier;
    m_dragStart = position;
    m_dragCenter = position;
    m_angle = 0.0;
    m_angleBuffer = 0.0;
    m_rotateActive = false;

    QSizeF area(0.0, 0.0);
    applyConstraints(area, false);
    m_dragEnd = m_dragStart + QPointF(area.width(), area.height());
    m_dragCenter = (m_dragStart + m_dragEnd) / 2.0;
}

void KisRectangleInteraction::update(const QPointF &position)
{
    const bool constraintToggle = m_currentModifiers.testFlag(Qt::ShiftModifier);
    const bool translateMode = m_currentModifiers.testFlag(Qt::AltModifier);
    const bool expandFromCenter = m_currentModifiers.testFlag(Qt::ControlModifier);
    const bool rotateMode = expandFromCenter && translateMode;
    const bool fixedSize = isFixedSize() && !constraintToggle;

    if (rotateMode) {
        QPointF angleVector;
        if (!m_rotateActive) {
            m_rotateActive = true;
            angleVector = fixedSize ? m_dragEnd : position;
            angleVector -= m_dragStart;
            m_referenceAngle = std::atan2(angleVector.y(), angleVector.x());
        }
        angleVector = position - m_dragStart;
        const qreal currentAngle = std::atan2(angleVector.y(), angleVector.x());
        m_angleBuffer = currentAngle - m_referenceAngle;
    } else {
        m_rotateActive = false;
        m_angle += m_angleBuffer;
        m_angleBuffer = 0.0;
    }

    if (fixedSize && !rotateMode) {
        m_dragStart = position;
    } else if (translateMode && !rotateMode) {
        const QPointF translation = position - m_dragEnd;
        m_dragStart += translation;
        m_dragEnd += translation;
    }

    QPointF diagonal = position - m_dragStart;
    QTransform fromRotation;
    fromRotation.rotateRadians(-rotationAngle());
    QPointF baseDiagonal = fromRotation.map(diagonal);
    QSizeF area(std::fabs(baseDiagonal.x()), std::fabs(baseDiagonal.y()));

    const bool overrideRatio = constraintToggle
        && !(m_isHeightForced || m_isWidthForced || m_isRatioForced);
    if (!constraintToggle || overrideRatio) {
        applyConstraints(area, overrideRatio);
    }

    baseDiagonal = QPointF(baseDiagonal.x() < 0 ? -area.width() : area.width(),
                           baseDiagonal.y() < 0 ? -area.height() : area.height());

    QTransform toRotation;
    toRotation.rotateRadians(rotationAngle());
    diagonal = toRotation.map(baseDiagonal);

    if (expandFromCenter && !fixedSize && !rotateMode) {
        m_dragStart = m_dragCenter - diagonal / 2.0;
        m_dragEnd = m_dragCenter + diagonal / 2.0;
    } else {
        m_dragEnd = m_dragStart + diagonal;
    }

    m_dragCenter = (m_dragStart + m_dragEnd) / 2.0;
}

QRectF KisRectangleInteraction::rectangle() const
{
    return rectangle(m_dragStart, m_dragEnd);
}

QRectF KisRectangleInteraction::rectangle(const QPointF &start,
                                           const QPointF &end) const
{
    QTransform fromRotation;
    fromRotation.translate(start.x(), start.y());
    fromRotation.rotateRadians(-rotationAngle());
    fromRotation.translate(-start.x(), -start.y());
    const QTransform toRotation = fromRotation.inverted();

    const QPointF rotatedEnd = fromRotation.map(end);
    const QPointF roundedStart(qRound(start.x()), qRound(start.y()));
    const QPointF roundedEnd(qRound(rotatedEnd.x()), qRound(rotatedEnd.y()));
    const QPointF roundedCenter = (roundedStart + roundedEnd) / 2.0;

    QRectF result(roundedStart, roundedEnd);
    result.moveCenter(toRotation.map(roundedCenter));
    return result.normalized();
}

QPointF KisRectangleInteraction::start() const
{
    return m_dragStart;
}

QPointF KisRectangleInteraction::center() const
{
    return m_dragCenter;
}

QPointF KisRectangleInteraction::end() const
{
    return m_dragEnd;
}

qreal KisRectangleInteraction::rotationAngle() const
{
    return m_angle + m_angleBuffer;
}

bool KisRectangleInteraction::isTranslating() const
{
    return m_currentModifiers.testFlag(Qt::AltModifier);
}

bool KisRectangleInteraction::isFixedSize() const
{
    return (m_isWidthForced && m_isHeightForced)
        || (m_isRatioForced && (m_isWidthForced || m_isHeightForced));
}

void KisRectangleInteraction::applyConstraints(QSizeF &area,
                                               bool overrideRatio) const
{
    if (m_isWidthForced) {
        area.setWidth(m_forcedWidth);
    }
    if (m_isHeightForced) {
        area.setHeight(m_forcedHeight);
    }

    if (m_isHeightForced && m_isWidthForced) {
        return;
    }

    if (m_isRatioForced || overrideRatio) {
        const qreal ratio = m_isRatioForced ? m_forcedRatio : 1.0;
        if (m_isWidthForced) {
            area.setHeight(area.width() / ratio);
        } else {
            area.setWidth(area.height() * ratio);
        }
    }
}
