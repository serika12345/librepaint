/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_painting_information_builder_adapters.h>

#include "kis_painting_information_builder_config_p.h"

#include <kis_canvas2.h>
#include <application/kis_config.h>
#include <kis_config_notifier.h>
#include <kis_coordinates_converter.h>
#include <kis_tool_freehand.h>

namespace {
KisPaintingInformationSettings currentPaintingInformationSettings()
{
    KisConfig config(true);
    KisPaintingInformationSettings settings;
    settings.pressureCurve = config.pressureTabletCurve();
    settings.maximumSpeed = config.readEntry("maxAllowedSpeedValue", 30);
    settings.tiltDirectionOffset =
        config.readEntry("tiltDirectionOffset", 0);
    settings.useEventTimestamps =
        config.readEntry("useTimestampsForBrushSpeed", false);
    settings.speedSmoothingSamples =
        config.readEntry("speedValueSmoothing", 3);
    return settings;
}
}

void initializePaintingInformationBuilderFromConfig(
    KisPaintingInformationBuilder *builder)
{
    builder->setSettings(currentPaintingInformationSettings());
    QObject::connect(
        KisConfigNotifier::instance(),
        &KisConfigNotifier::configChanged,
        builder,
        [builder]() {
            builder->setSettings(currentPaintingInformationSettings());
        });
}

KisConverterPaintingInformationBuilder::
    KisConverterPaintingInformationBuilder(
        const KisCoordinatesConverter *converter)
    : m_converter(converter)
{
    initializePaintingInformationBuilderFromConfig(this);
}

QPointF KisConverterPaintingInformationBuilder::documentToImage(
    const QPointF &point)
{
    return m_converter->documentToImage(point);
}

QPointF KisConverterPaintingInformationBuilder::imageToDocument(
    const QPointF &point)
{
    return m_converter->imageToDocument(point);
}

QPointF KisConverterPaintingInformationBuilder::imageToView(
    const QPointF &point)
{
    return m_converter->imageToWidget(point);
}

qreal KisConverterPaintingInformationBuilder::canvasRotation() const
{
    return m_converter->rotationAngle();
}

bool KisConverterPaintingInformationBuilder::canvasMirroredX() const
{
    return m_converter->xAxisMirrored();
}

bool KisConverterPaintingInformationBuilder::canvasMirroredY() const
{
    return m_converter->yAxisMirrored();
}

KisToolFreehandPaintingInformationBuilder::
    KisToolFreehandPaintingInformationBuilder(KisToolFreehand *tool)
    : m_tool(tool)
{
    initializePaintingInformationBuilderFromConfig(this);
}

QPointF KisToolFreehandPaintingInformationBuilder::documentToImage(
    const QPointF &point)
{
    return m_tool->convertToPixelCoord(point);
}

QPointF KisToolFreehandPaintingInformationBuilder::imageToDocument(
    const QPointF &point)
{
    KisCanvas2 *canvas = dynamic_cast<KisCanvas2 *>(m_tool->canvas());
    KIS_ASSERT_RECOVER_RETURN_VALUE(canvas, point);
    return canvas->coordinatesConverter()->imageToDocument(point);
}

QPointF KisToolFreehandPaintingInformationBuilder::imageToView(
    const QPointF &point)
{
    return m_tool->pixelToView(point);
}

QPointF KisToolFreehandPaintingInformationBuilder::adjustDocumentPoint(
    const QPointF &point,
    const QPointF &startPoint)
{
    return m_tool->adjustPosition(point, startPoint);
}

qreal KisToolFreehandPaintingInformationBuilder::calculatePerspective(
    const QPointF &documentPoint)
{
    return m_tool->calculatePerspective(documentPoint);
}

qreal KisToolFreehandPaintingInformationBuilder::canvasRotation() const
{
    KisCanvas2 *canvas = dynamic_cast<KisCanvas2 *>(m_tool->canvas());
    KIS_ASSERT_RECOVER_RETURN_VALUE(canvas, 0.0);
    return canvas->coordinatesConverter()->rotationAngle();
}

bool KisToolFreehandPaintingInformationBuilder::canvasMirroredX() const
{
    KisCanvas2 *canvas = dynamic_cast<KisCanvas2 *>(m_tool->canvas());
    KIS_ASSERT_RECOVER_RETURN_VALUE(canvas, false);
    return canvas->coordinatesConverter()->xAxisMirrored();
}

bool KisToolFreehandPaintingInformationBuilder::canvasMirroredY() const
{
    KisCanvas2 *canvas = dynamic_cast<KisCanvas2 *>(m_tool->canvas());
    KIS_ASSERT_RECOVER_RETURN_VALUE(canvas, false);
    return canvas->coordinatesConverter()->yAxisMirrored();
}
