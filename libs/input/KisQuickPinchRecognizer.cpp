/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisQuickPinchRecognizer.h"

#include <QLineF>

#include <algorithm>
#include <cmath>

namespace
{
qreal lineAngle(const QPointF &firstPoint, const QPointF &secondPoint)
{
    const QPointF delta = secondPoint - firstPoint;
    return std::atan2(delta.y(), delta.x());
}

qreal rotationFrom(qreal startingAngle, qreal currentAngle)
{
    constexpr qreal Pi = 3.14159265358979323846;
    constexpr qreal RadiansToDegrees = 180.0 / Pi;
    return std::abs(std::remainder((currentAngle - startingAngle) * RadiansToDegrees,
                                   360.0));
}
}

KisQuickPinchTransform::KisQuickPinchTransform(
    qreal startRotation,
    qreal targetRotation,
    qreal startZoom,
    qreal targetZoom,
    const QPointF &startViewCenter,
    const QPointF &targetViewCenter)
    : m_startRotation(startRotation)
    , m_targetRotation(targetRotation)
    , m_startZoom(startZoom)
    , m_targetZoom(targetZoom)
    , m_startViewCenter(startViewCenter)
    , m_targetViewCenter(targetViewCenter)
{
}

qreal KisQuickPinchTransform::fittedZoom(qreal currentZoom,
                                         QSizeF imageSizeAtCurrentZoom,
                                         const QSizeF &viewportSize,
                                         qreal margin,
                                         qreal targetRotation)
{
    return fittedTarget(currentZoom,
                        imageSizeAtCurrentZoom,
                        QRectF(QPointF(), viewportSize),
                        margin,
                        targetRotation)
        .zoom;
}

KisQuickPinchFitTarget KisQuickPinchTransform::fittedTarget(
    qreal currentZoom,
    QSizeF imageSizeAtCurrentZoom,
    const QRectF &availableViewport,
    qreal margin,
    qreal targetRotation)
{
    KisQuickPinchFitTarget target {currentZoom, availableViewport.center()};

    const int quarterTurns = qRound(targetRotation / 90.0);
    if (std::abs(quarterTurns) % 2 == 1) {
        imageSizeAtCurrentZoom.transpose();
    }

    if (imageSizeAtCurrentZoom.isEmpty() || availableViewport.isEmpty()) {
        return target;
    }

    const qreal availableWidth = std::max<qreal>(1.0,
        availableViewport.width() - 2.0 * margin);
    const qreal availableHeight = std::max<qreal>(1.0,
        availableViewport.height() - 2.0 * margin);
    const qreal scale = std::min(
        availableWidth / imageSizeAtCurrentZoom.width(),
        availableHeight / imageSizeAtCurrentZoom.height());

    target.zoom = currentZoom * scale;
    return target;
}

const char *KisQuickPinchTransform::availableViewportPropertyName()
{
    return "kritaQuickPinchAvailableViewport";
}

KisQuickPinchTransformFrame KisQuickPinchTransform::frameAt(qreal progress) const
{
    progress = std::clamp(progress, 0.0, 1.0);

    KisQuickPinchTransformFrame frame;
    frame.rotation = m_startRotation
        + (m_targetRotation - m_startRotation) * progress;
    frame.zoom = m_startZoom > 0.0 && m_targetZoom > 0.0
        ? std::exp(std::log(m_startZoom)
                   + (std::log(m_targetZoom) - std::log(m_startZoom)) * progress)
        : m_startZoom + (m_targetZoom - m_startZoom) * progress;
    frame.viewCenter = m_startViewCenter
        + (m_targetViewCenter - m_startViewCenter) * progress;
    return frame;
}

qreal KisQuickPinchRecognizer::snappedCanvasRotation(qreal currentRotationDegrees)
{
    constexpr qreal QuarterTurnDegrees = 90.0;
    return std::round(currentRotationDegrees / QuarterTurnDegrees)
        * QuarterTurnDegrees;
}

void KisQuickPinchRecognizer::begin(const QPointF &firstPoint,
                                    const QPointF &secondPoint)
{
    m_startingDistance = QLineF(firstPoint, secondPoint).length();
    m_minimumDistance = m_startingDistance;
    m_latestDistance = m_startingDistance;
    m_startingAngle = lineAngle(firstPoint, secondPoint);
    m_maximumRotationDegrees = 0.0;
    m_active = m_startingDistance >= MinimumStartingDistance;
    m_hasUpdate = false;
}

void KisQuickPinchRecognizer::update(const QPointF &firstPoint,
                                     const QPointF &secondPoint,
                                     qint64 elapsedMilliseconds)
{
    if (!m_active) {
        return;
    }

    if (elapsedMilliseconds < 0 || elapsedMilliseconds > MaximumDurationMilliseconds) {
        cancel();
        return;
    }

    const qreal distance = QLineF(firstPoint, secondPoint).length();
    if (qFuzzyIsNull(distance)) {
        cancel();
        return;
    }

    m_latestDistance = distance;
    m_minimumDistance = std::min(m_minimumDistance, distance);
    m_maximumRotationDegrees = std::max(
        m_maximumRotationDegrees,
        rotationFrom(m_startingAngle, lineAngle(firstPoint, secondPoint)));
    m_hasUpdate = true;
}

void KisQuickPinchRecognizer::cancel()
{
    m_active = false;
}

bool KisQuickPinchRecognizer::shouldFitOnRelease(qint64 elapsedMilliseconds) const
{
    if (!m_active || !m_hasUpdate || elapsedMilliseconds < 0
        || elapsedMilliseconds > MaximumDurationMilliseconds) {
        return false;
    }

    const qreal distanceRatio = m_minimumDistance / m_startingDistance;
    const qreal releaseReversalRatio = m_latestDistance / m_minimumDistance;

    return distanceRatio <= MaximumDistanceRatio
        && releaseReversalRatio <= MaximumReleaseReversalRatio
        && m_maximumRotationDegrees <= MaximumRotationDegrees;
}
