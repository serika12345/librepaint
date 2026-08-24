/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_painting_information_builder.h>

#include <KoCanvasResourceProvider.h>
#include <KoPointerEvent.h>

#include <kis_cubic_curve.h>
#include <kis_speed_smoother.h>

const int KisPaintingInformationBuilder::PressureResolution = 1024;

KisPaintingInformationBuilder::KisPaintingInformationBuilder()
    : m_speedSmoother(new KisSpeedSmoother())
{
    setSettings(KisPaintingInformationSettings());
}

KisPaintingInformationBuilder::~KisPaintingInformationBuilder() = default;

void KisPaintingInformationBuilder::setSettings(
    const KisPaintingInformationSettings &settings)
{
    const KisCubicCurve curve(settings.pressureCurve);
    m_pressureSamples = curve.floatTransfer(PressureResolution + 1);
    m_maximumSpeed = settings.maximumSpeed;

    // The setting is stored in [-180, 180] degrees to match the UI.
    m_tiltDirectionOffset = settings.tiltDirectionOffset;
    if (m_tiltDirectionOffset < 0.0) {
        m_tiltDirectionOffset += 360.0;
    }

    m_speedSmoother->setSettings(settings.useEventTimestamps,
                                 settings.speedSmoothingSamples);
}

KisPaintInformation KisPaintingInformationBuilder::startStroke(
    KoPointerEvent *event,
    int timeElapsed,
    const KoCanvasResourceProvider *manager)
{
    if (manager) {
        m_pressureDisabled =
            manager->resource(KoCanvasResource::DisablePressure).toBool();
    }

    m_startPoint = event->point;
    return createPaintingInformation(event, timeElapsed);
}

KisPaintInformation KisPaintingInformationBuilder::continueStroke(
    KoPointerEvent *event,
    int timeElapsed)
{
    return createPaintingInformation(event, timeElapsed);
}

QPointF KisPaintingInformationBuilder::adjustDocumentPoint(
    const QPointF &point,
    const QPointF &)
{
    return point;
}

QPointF KisPaintingInformationBuilder::documentToImage(const QPointF &point)
{
    return point;
}

QPointF KisPaintingInformationBuilder::imageToDocument(const QPointF &point)
{
    return point;
}

QPointF KisPaintingInformationBuilder::imageToView(const QPointF &point)
{
    return point;
}

qreal KisPaintingInformationBuilder::calculatePerspective(const QPointF &)
{
    return 1.0;
}

qreal KisPaintingInformationBuilder::canvasRotation() const
{
    return 0.0;
}

bool KisPaintingInformationBuilder::canvasMirroredX() const
{
    return false;
}

bool KisPaintingInformationBuilder::canvasMirroredY() const
{
    return false;
}

KisPaintInformation KisPaintingInformationBuilder::createPaintingInformation(
    KoPointerEvent *event,
    int timeElapsed)
{
    const QPointF adjusted = adjustDocumentPoint(event->point, m_startPoint);
    const QPointF imagePoint = documentToImage(adjusted);
    const qreal perspective = calculatePerspective(adjusted);
    const qreal speed =
        m_speedSmoother->getNextSpeed(imageToView(imagePoint), event->time());

    KisPaintInformation info(imagePoint,
                             !m_pressureDisabled
                                 ? 1.0
                                 : pressureToCurve(event->pressure()),
                             event->xTilt(),
                             event->yTilt(),
                             event->rotation(),
                             event->tangentialPressure(),
                             perspective,
                             timeElapsed,
                             qMin(1.0, speed / qreal(m_maximumSpeed)));

    info.setCanvasRotation(canvasRotation());
    info.setCanvasMirroredH(canvasMirroredX());
    info.setCanvasMirroredV(canvasMirroredY());
    info.setTiltDirectionOffset(m_tiltDirectionOffset);

    return info;
}

KisPaintInformation KisPaintingInformationBuilder::hover(
    const QPointF &imagePoint,
    const KoPointerEvent *event,
    bool isStrokeStarted)
{
    const qreal perspective = calculatePerspective(imageToDocument(imagePoint));

    const qreal speed = !isStrokeStarted && event
        ? m_speedSmoother->getNextSpeed(imageToView(imagePoint), event->time())
        : m_speedSmoother->lastSpeed();

    if (event) {
        return KisPaintInformation::createHoveringModeInfo(
            imagePoint,
            PRESSURE_DEFAULT,
            event->xTilt(),
            event->yTilt(),
            event->rotation(),
            event->tangentialPressure(),
            perspective,
            qMin(1.0, speed / qreal(m_maximumSpeed)),
            canvasRotation(),
            canvasMirroredX(),
            canvasMirroredY(),
            m_tiltDirectionOffset);
    }

    KisPaintInformation info =
        KisPaintInformation::createHoveringModeInfo(imagePoint);
    info.setCanvasRotation(canvasRotation());
    info.setCanvasMirroredH(canvasMirroredX());
    info.setCanvasMirroredV(canvasMirroredY());
    info.setTiltDirectionOffset(m_tiltDirectionOffset);
    return info;
}

qreal KisPaintingInformationBuilder::pressureToCurve(qreal pressure)
{
    return KisCubicCurve::interpolateLinear(pressure, m_pressureSamples);
}

void KisPaintingInformationBuilder::reset()
{
    m_speedSmoother->clear();
}
