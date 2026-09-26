/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Sharaf Zaman <sharafzaz121@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QApplication>
#include <QElapsedTimer>
#include <QEasingCurve>
#include <QLineF>
#include <QPointer>
#include <QTouchEvent>
#include <QVariantAnimation>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <klocalizedstring.h>
#include <kis_canvas_controller.h>
#include <kis_canvas2.h>
#include <application/ui/workspace/KisViewManager.h>
#include <kis_algebra_2d.h>
#include <KisQuickPinchRecognizer.h>

#include "kis_zoom_and_rotate_action.h"
#include "KisApplicationInputActions.h"
#include "KisInputActionGroup.h"
#include "KoZoomMode.h"
#include "kis_input_manager.h"
#include <KoViewTransformStillPoint.h>
#include <math.h>
#include <qhash.h>
#include <QDebug>
#include <qlogging.h>
#include <qnumeric.h>
#include <QtGlobal>

namespace
{
constexpr int FitAnimationDurationMilliseconds = 180;

int touchPointCount(const QTouchEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->points().size();
#else
    return event->touchPoints().size();
#endif
}

QPointF touchPosition(const QTouchEvent *event, int index)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->points().at(index).position();
#else
    return event->touchPoints().at(index).pos();
#endif
}

QPointF touchPressPosition(const QTouchEvent *event, int index)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->points().at(index).pressPosition();
#else
    return event->touchPoints().at(index).startPos();
#endif
}

quint64 latestTouchPressTimestamp(const QTouchEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return std::max(event->points().at(0).pressTimestamp(),
                    event->points().at(1).pressTimestamp());
#else
    Q_UNUSED(event);
    return 0;
#endif
}

}

class KisZoomAndRotateAction::Private {
public:
    Private() {}

    qint64 elapsedMilliseconds(const QTouchEvent *event) const
    {
        if (gestureStartTimestamp > 0 && event
            && event->timestamp() >= gestureStartTimestamp) {
            return static_cast<qint64>(event->timestamp() - gestureStartTimestamp);
        }

        return gestureTimer.isValid() ? gestureTimer.elapsed() : -1;
    }

    int shortcutIndex {0};
    QPointF lastPosition {0, 0};
    float lastDistance {0.0};
    qreal previousAngle {0.0};
    qreal initialReferenceAngle {0.0};
    qreal accumRotationAngle {0.0};

    KoViewTransformStillPoint actionStillPoint;
    QElapsedTimer gestureTimer;
    quint64 gestureStartTimestamp {0};
    KisQuickPinchRecognizer quickPinchRecognizer;

    QVariantAnimation fitAnimation;
    QPointer<KisCanvas2> fitCanvas;
    std::optional<KisQuickPinchTransform> fitTransform;
    QPointF fitDocumentCenter;
};

KisZoomAndRotateAction::KisZoomAndRotateAction()
    : KisAbstractInputAction ("Zoom and Rotate Canvas")
    , d(new Private)
{
    setName(i18n("Zoom and Rotate Canvas"));
    QHash<QString, int> shortcuts;
    shortcuts.insert(i18n("Rotate Mode"), ContinuousRotateMode);
    shortcuts.insert(i18n("Discrete Rotate Mode"), DiscreteRotateMode);
    setShortcutIndexes(shortcuts);

    d->fitAnimation.setDuration(FitAnimationDurationMilliseconds);
    d->fitAnimation.setStartValue(0.0);
    d->fitAnimation.setEndValue(1.0);
    d->fitAnimation.setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(&d->fitAnimation, &QVariantAnimation::valueChanged,
                     [this](const QVariant &value) {
        KisCanvas2 *canvas = d->fitCanvas.data();
        if (!canvas) {
            d->fitAnimation.stop();
            return;
        }
        if (!d->fitTransform) {
            d->fitAnimation.stop();
            return;
        }

        KisCanvasController *controller =
            static_cast<KisCanvasController *>(canvas->canvasController());
        const KisQuickPinchTransformFrame frame =
            d->fitTransform->frameAt(value.toReal());
        const KoViewTransformStillPoint stillPoint(d->fitDocumentCenter,
                                                    frame.viewCenter);

        controller->rotateCanvas(frame.rotation - controller->rotation(), stillPoint);
        controller->setZoom(KoZoomMode::ZOOM_CONSTANT, frame.zoom, stillPoint);
    });
    QObject::connect(&d->fitAnimation, &QVariantAnimation::finished,
                     [this]() {
        KisCanvas2 *canvas = d->fitCanvas.data();
        if (!canvas || !d->fitTransform) {
            return;
        }

        KisCanvasController *controller =
            static_cast<KisCanvasController *>(canvas->canvasController());
        const KisQuickPinchTransformFrame frame = d->fitTransform->frameAt(1.0);
        const KoViewTransformStillPoint stillPoint(d->fitDocumentCenter,
                                                    frame.viewCenter);
        controller->rotateCanvas(frame.rotation - controller->rotation(), stillPoint);
        controller->setZoom(KoZoomMode::ZOOM_PAGE, 1.0);
        d->fitCanvas.clear();
        d->fitTransform.reset();
    });
}

KisZoomAndRotateAction::~KisZoomAndRotateAction()
{
}

int KisZoomAndRotateAction::priority() const
{
    return 5;
}

void KisZoomAndRotateAction::activate(int shortcut)
{
    Q_UNUSED(shortcut);
}

void KisZoomAndRotateAction::deactivate(int shortcut)
{
    Q_UNUSED(shortcut);
}

void KisZoomAndRotateAction::begin(int shortcut, QEvent *event)
{
    QTouchEvent *touchEvent = dynamic_cast<QTouchEvent *>(event);

    d->fitAnimation.stop();
    d->fitCanvas.clear();
    d->fitTransform.reset();
    d->gestureTimer.invalidate();
    d->gestureStartTimestamp = 0;
    d->quickPinchRecognizer.cancel();

    if (touchEvent && touchPointCount(touchEvent) == 2) {
        d->shortcutIndex = shortcut;
        d->lastPosition = touchPosition(touchEvent, 0);
        d->lastDistance = 0;
        d->previousAngle = 0;
        d->initialReferenceAngle = 0;
        d->accumRotationAngle = 0;
        d->actionStillPoint = applicationInputCanvas(inputManager())->coordinatesConverter()->makeWidgetStillPoint(d->lastPosition);

        const QPointF firstStartPosition = touchPressPosition(touchEvent, 0);
        const QPointF secondStartPosition = touchPressPosition(touchEvent, 1);
        if (QLineF(firstStartPosition, secondStartPosition).length() >= 40.0) {
            d->quickPinchRecognizer.begin(firstStartPosition, secondStartPosition);
        } else {
            d->quickPinchRecognizer.begin(touchPosition(touchEvent, 0),
                                          touchPosition(touchEvent, 1));
        }
        d->gestureStartTimestamp = latestTouchPressTimestamp(touchEvent);
        d->gestureTimer.start();
    }
}

void KisZoomAndRotateAction::end(QEvent *event)
{
    const QTouchEvent *touchEvent = dynamic_cast<QTouchEvent *>(event);
    const bool endedByTwoFingerRelease = touchEvent
        && touchPointCount(touchEvent) == 2
        && (event->type() == QEvent::TouchEnd
            || (touchEvent->touchPointStates() & Qt::TouchPointReleased));
    const bool fitCanvas = endedByTwoFingerRelease
        && d->quickPinchRecognizer.shouldFitOnRelease(
            d->elapsedMilliseconds(touchEvent));

    d->gestureTimer.invalidate();
    d->gestureStartTimestamp = 0;
    d->quickPinchRecognizer.cancel();

    if (fitCanvas) {
        KisCanvas2 *canvas = applicationInputCanvas(inputManager());
        KisCanvasController *controller = static_cast<KisCanvasController *>(canvas->canvasController());
        const KisCoordinatesConverter *converter = canvas->coordinatesConverter();

        d->fitCanvas = canvas;
        const qreal startRotation = controller->rotation();
        const qreal targetRotation =
            KisQuickPinchRecognizer::snappedCanvasRotation(startRotation);
        const qreal startZoom = converter->zoom();
        const qreal targetZoom = KisQuickPinchTransform::fittedZoom(
            startZoom,
            converter->imageSizeInFlakePixels(),
            converter->getCanvasWidgetSize(),
            converter->zoomMarginSize(),
            targetRotation);
        d->fitDocumentCenter = converter->imageRectInDocumentPixels().center();
        d->fitTransform.emplace(startRotation,
                                targetRotation,
                                startZoom,
                                targetZoom,
                                converter->imageCenterInWidgetPixel(),
                                converter->widgetCenterPoint());
        d->fitAnimation.start();
    }
}

void KisZoomAndRotateAction::cursorMovedAbsolute(const QPointF &, const QPointF &)
{
}

qreal angleForSnapping(qreal angle)
{
    if (angle < 0) {
        return std::fmod(angle - 2, 45) + 2;
    } else {
        return std::fmod(angle + 2, 45) - 2;
    }
}

void KisZoomAndRotateAction::inputEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchUpdate: {
        QTouchEvent *tevent = dynamic_cast<QTouchEvent *>(event);
        if (tevent && touchPointCount(tevent) > 1) {

            const QPointF p0 = touchPosition(tevent, 0);
            const QPointF p1 = touchPosition(tevent, 1);

            if (touchPointCount(tevent) != 2) {
                d->quickPinchRecognizer.cancel();
            } else {
                d->quickPinchRecognizer.update(
                    p0, p1, d->elapsedMilliseconds(tevent));
            }

            const qreal rotationAngle = canvasRotationAngle(p0, p1);
            const float dist = QLineF(p0, p1).length();
            const float scaleDelta = qFuzzyCompare(1.0f, 1.0f + d->lastDistance) ? 1.f : dist / d->lastDistance;

            KisCanvas2 *canvas = applicationInputCanvas(inputManager());
            KisCanvasController *controller = static_cast<KisCanvasController *>(canvas->canvasController());
            const qreal newZoom = canvas->viewConverter()->zoom() * scaleDelta;
            KoViewTransformStillPoint adjustedStillPoint = d->actionStillPoint;
            adjustedStillPoint.second = p0;
            controller->setZoom(KoZoomMode::ZOOM_CONSTANT, newZoom, adjustedStillPoint);
            controller->rotateCanvas(rotationAngle, adjustedStillPoint);

            d->lastPosition = p0;
            d->lastDistance = dist;

            return;
        }
    }
    default:
        break;
    }
    KisAbstractInputAction::inputEvent(event);
}

KisInputActionGroup KisZoomAndRotateAction::inputActionGroup(int shortcut) const
{
    Q_UNUSED(shortcut);
    return ViewTransformActionGroup;
}

qreal KisZoomAndRotateAction::canvasRotationAngle(QPointF p0, QPointF p1)
{
    const QPointF slope = p1 - p0;
    const qreal currentAngle = std::atan2(slope.y(), slope.x());

    switch (d->shortcutIndex) {
    case ContinuousRotateMode: {
        if (!d->previousAngle) {
            d->previousAngle = currentAngle;
            return 0;
        }
        qreal rotationAngle = (180 / M_PI) * (currentAngle - d->previousAngle);
        d->previousAngle = currentAngle;

        KisCanvas2 *canvas = applicationInputCanvas(inputManager());
        KisCanvasController *controller = static_cast<KisCanvasController *>(canvas->canvasController());
        const qreal canvasAnglePostRotation = controller->rotation() + rotationAngle;
        const qreal snapDelta = angleForSnapping(canvasAnglePostRotation);
        // we snap the canvas to an angle that is a multiple of 45
        if (abs(snapDelta) <= 2 && abs(d->accumRotationAngle) <= 2) {
            // accumulate the relative angle of finger from the point when we started snapping
            d->accumRotationAngle += rotationAngle;
            rotationAngle = rotationAngle - snapDelta;
        } else {
            // snap the canvas out using the accumulated angle
            rotationAngle += d->accumRotationAngle;
            d->accumRotationAngle = 0;
        }

        return rotationAngle;
    }
    case DiscreteRotateMode: {
        if (!d->initialReferenceAngle) {
            d->initialReferenceAngle = currentAngle;
            return 0;
        }
        qreal rotationAngle = 0;
        const qreal relativeAngle = (180 / M_PI) * (currentAngle - d->initialReferenceAngle);
        const qreal rotationThreshold = 15;

        // if the canvas is moved in either direction with an angle greater than the threshold, we rotate the canvas in
        // that direction by 15°.
        if (std::abs(relativeAngle) >= rotationThreshold && std::abs(relativeAngle) <= (360 - rotationThreshold)) {
            // set reference as currentAngle to check if we go beyond the threshold next time
            d->initialReferenceAngle = currentAngle;

            if (std::abs(relativeAngle) <= 180) {
                rotationAngle = KisAlgebra2D::copysign(15.0, relativeAngle);
            } else {
                // if we're over 180, it means the canvas has to be rotated in the opposite direction of the current
                // angle. E.g if the relative angle is +341° then we move the canvas by -15° (because the actual effect
                // is 341 - 360 = -19°  on the original theta).
                rotationAngle = KisAlgebra2D::copysign(15.0, -relativeAngle);
            }
        }
        return rotationAngle;
    }
    default:
        qWarning() << "KisZoomAndRotateAction: Unrecognized shortcut" << d->shortcutIndex;
        return 0;
    }
}
