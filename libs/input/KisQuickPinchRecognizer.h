/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISQUICKPINCHRECOGNIZER_H
#define KISQUICKPINCHRECOGNIZER_H

#include <kritainput_export.h>

#include <QPointF>
#include <QSizeF>
#include <QtGlobal>

struct KRITAINPUT_EXPORT KisQuickPinchTransformFrame
{
    qreal rotation {0.0};
    qreal zoom {1.0};
    QPointF viewCenter;
};

class KRITAINPUT_EXPORT KisQuickPinchTransform
{
public:
    KisQuickPinchTransform(qreal startRotation,
                           qreal targetRotation,
                           qreal startZoom,
                           qreal targetZoom,
                           const QPointF &startViewCenter,
                           const QPointF &targetViewCenter);

    static qreal fittedZoom(qreal currentZoom,
                            QSizeF imageSizeAtCurrentZoom,
                            const QSizeF &viewportSize,
                            qreal margin,
                            qreal targetRotation);

    KisQuickPinchTransformFrame frameAt(qreal progress) const;

private:
    qreal m_startRotation {0.0};
    qreal m_targetRotation {0.0};
    qreal m_startZoom {1.0};
    qreal m_targetZoom {1.0};
    QPointF m_startViewCenter;
    QPointF m_targetViewCenter;
};

/**
 * Recognizes one fast inward two-point pinch from supplied geometry and
 * elapsed time. The caller owns the gesture lifecycle: begin() starts a new
 * sequence, update() records movement, and cancel() invalidates it. The value
 * object owns no external state and rejects incomplete or invalid sequences.
 */
class KRITAINPUT_EXPORT KisQuickPinchRecognizer
{
public:
    static qreal snappedCanvasRotation(qreal currentRotationDegrees);

    void begin(const QPointF &firstPoint, const QPointF &secondPoint);
    void update(const QPointF &firstPoint,
                const QPointF &secondPoint,
                qint64 elapsedMilliseconds);
    void cancel();

    bool shouldFitOnRelease(qint64 elapsedMilliseconds) const;

private:
    static constexpr qint64 MaximumDurationMilliseconds = 350;
    static constexpr qreal MaximumDistanceRatio = 0.65;
    static constexpr qreal MaximumReleaseReversalRatio = 1.08;
    static constexpr qreal MaximumRotationDegrees = 10.0;
    static constexpr qreal MinimumStartingDistance = 40.0;

    qreal m_startingDistance {0.0};
    qreal m_minimumDistance {0.0};
    qreal m_latestDistance {0.0};
    qreal m_startingAngle {0.0};
    qreal m_maximumRotationDegrees {0.0};
    bool m_active {false};
    bool m_hasUpdate {false};
};

#endif // KISQUICKPINCHRECOGNIZER_H
