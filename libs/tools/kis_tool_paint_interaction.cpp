/*
 *  SPDX-FileCopyrightText: 2003-2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2015 Moritz Molch <kde@moritzmolch.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool_paint_interaction.h>

#include <QAction>

#include <KoCanvasBase.h>
#include <KoCanvasResourceProvider.h>
#include <KoCompositeOpRegistry.h>
#include <KoPointerEvent.h>

#include <KisToolCanvas.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_utils.h>
#include <kis_algebra_2d.h>
#include <kis_coordinates_converter.h>
#include <kis_distance_information.h>
#include <kis_image_config.h>
#include <kis_paint_information.h>
#include <kis_random_source.h>

struct KisToolPaintInteraction::Private
{
    KisPaintOpUtils::PositionHistory lastCursorPos;
};

KisToolPaintInteraction::KisToolPaintInteraction(KoCanvasBase *canvas,
                                                 const QCursor &cursor)
    : KisTool(canvas, cursor)
    , m_standardBrushSizes(1, KisImageConfig(true).maxBrushSize())
    , m_d(new Private())
{
}

KisToolPaintInteraction::~KisToolPaintInteraction() = default;

int KisToolPaintInteraction::flags() const
{
    return KisTool::FLAG_USES_CUSTOM_COMPOSITEOP;
}

void KisToolPaintInteraction::canvasResourceChanged(int key,
                                                    const QVariant &value)
{
    KisTool::canvasResourceChanged(key, value);

    switch (key) {
    case KoCanvasResource::Opacity:
        break;
    case KoCanvasResource::CurrentPaintOpPreset:
        if (isActive()) {
            requestUpdateOutline(m_outlineDocPoint, nullptr);
        }
        break;
    case KoCanvasResource::CurrentPaintOpPresetName:
        if (isActive()) {
            Q_EMIT statusTextChanged(value.toString().replace("_", " "));
        }
        break;
    default:
        break;
    }
}

void KisToolPaintInteraction::activate(const QSet<KoShape *> &shapes)
{
    if (currentPaintOpPreset()) {
        Q_EMIT statusTextChanged(
            currentPaintOpPreset()->name().replace("_", " "));
    }

    KisTool::activate(shapes);

    if (flags() & KisTool::FLAG_USES_CUSTOM_SIZE) {
        connect(action("increase_brush_size"),
                SIGNAL(triggered()),
                SLOT(increaseBrushSize()),
                Qt::UniqueConnection);
        connect(action("decrease_brush_size"),
                SIGNAL(triggered()),
                SLOT(decreaseBrushSize()),
                Qt::UniqueConnection);
    }

    connect(action("rotate_brush_tip_clockwise"),
            SIGNAL(triggered()),
            SLOT(rotateBrushTipClockwise()),
            Qt::UniqueConnection);
    connect(action("rotate_brush_tip_clockwise_precise"),
            SIGNAL(triggered()),
            SLOT(rotateBrushTipClockwisePrecise()),
            Qt::UniqueConnection);
    connect(action("rotate_brush_tip_counter_clockwise"),
            SIGNAL(triggered()),
            SLOT(rotateBrushTipCounterClockwise()),
            Qt::UniqueConnection);
    connect(action("rotate_brush_tip_counter_clockwise_precise"),
            SIGNAL(triggered()),
            SLOT(rotateBrushTipCounterClockwisePrecise()),
            Qt::UniqueConnection);
}

void KisToolPaintInteraction::deactivate()
{
    if (flags() & KisTool::FLAG_USES_CUSTOM_SIZE) {
        disconnect(action("increase_brush_size"), nullptr, this, nullptr);
        disconnect(action("decrease_brush_size"), nullptr, this, nullptr);
    }

    disconnect(action("rotate_brush_tip_clockwise"), nullptr, this, nullptr);
    disconnect(action("rotate_brush_tip_clockwise_precise"), nullptr, this, nullptr);
    disconnect(action("rotate_brush_tip_counter_clockwise"), nullptr, this, nullptr);
    disconnect(action("rotate_brush_tip_counter_clockwise_precise"),
               nullptr,
               this,
               nullptr);

    Q_EMIT statusTextChanged(QString());
    KisTool::deactivate();
}

void KisToolPaintInteraction::setMode(ToolMode mode)
{
    if (this->mode() == KisTool::PAINT_MODE && mode != KisTool::PAINT_MODE) {
        Q_EMIT sigPaintingFinished();
    }

    KisTool::setMode(mode);
}

void KisToolPaintInteraction::mousePressEvent(KoPointerEvent *event)
{
    KisTool::mousePressEvent(event);
    if (mode() == KisTool::HOVER_MODE) {
        requestUpdateOutline(event->point, event);
    }
}

void KisToolPaintInteraction::mouseMoveEvent(KoPointerEvent *event)
{
    KisTool::mouseMoveEvent(event);
    if (mode() == KisTool::HOVER_MODE) {
        requestUpdateOutline(event->point, event);
    }
}

void KisToolPaintInteraction::mouseReleaseEvent(KoPointerEvent *event)
{
    KisTool::mouseReleaseEvent(event);
    if (mode() == KisTool::HOVER_MODE) {
        requestUpdateOutline(event->point, event);
    }
}

void KisToolPaintInteraction::activatePrimaryAction()
{
    setOutlineVisible(true);
    KisTool::activatePrimaryAction();
}

void KisToolPaintInteraction::deactivatePrimaryAction()
{
    setOutlineVisible(false);
    KisTool::deactivatePrimaryAction();
}

void KisToolPaintInteraction::setSupportOutline(bool supportOutline)
{
    m_supportOutline = supportOutline;
}

bool KisToolPaintInteraction::supportsOutline() const
{
    return m_supportOutline;
}

bool KisToolPaintInteraction::isOutlineEnabled() const
{
    return m_isOutlineEnabled;
}

void KisToolPaintInteraction::setOutlineEnabled(bool enabled)
{
    m_isOutlineEnabled = enabled;
    requestUpdateOutline(m_outlineDocPoint, lastDeliveredPointerEvent());
}

bool KisToolPaintInteraction::isOutlineVisible() const
{
    return m_isOutlineVisible;
}

void KisToolPaintInteraction::setOutlineVisible(bool visible)
{
    m_isOutlineVisible = visible;
    requestUpdateOutline(m_outlineDocPoint, lastDeliveredPointerEvent());
}

void KisToolPaintInteraction::increaseBrushSize()
{
    KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();
    settings->setPaintOpSize(
        m_standardBrushSizes.increaseBrushSize(settings->paintOpSize()));
    requestUpdateOutline(m_outlineDocPoint, nullptr);
}

void KisToolPaintInteraction::decreaseBrushSize()
{
    KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();
    settings->setPaintOpSize(
        m_standardBrushSizes.decreaseBrushSize(settings->paintOpSize()));
    requestUpdateOutline(m_outlineDocPoint, nullptr);
}

void KisToolPaintInteraction::rotateBrushTipClockwise()
{
    KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();
    settings->setPaintOpAngle(settings->paintOpAngle() - 15);
    requestUpdateOutline(m_outlineDocPoint, nullptr);
}

void KisToolPaintInteraction::rotateBrushTipClockwisePrecise()
{
    KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();
    settings->setPaintOpAngle(settings->paintOpAngle() - 1);
    requestUpdateOutline(m_outlineDocPoint, nullptr);
}

void KisToolPaintInteraction::rotateBrushTipCounterClockwise()
{
    KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();
    settings->setPaintOpAngle(settings->paintOpAngle() + 15);
    requestUpdateOutline(m_outlineDocPoint, nullptr);
}

void KisToolPaintInteraction::rotateBrushTipCounterClockwisePrecise()
{
    KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();
    settings->setPaintOpAngle(settings->paintOpAngle() + 1);
    requestUpdateOutline(m_outlineDocPoint, nullptr);
}

bool KisToolPaintInteraction::isEraser() const
{
    return canvas()
               ->resourceManager()
               ->resource(KoCanvasResource::CurrentEffectiveCompositeOp)
               .toString() == COMPOSITE_ERASE;
}

KisOptimizedBrushOutline KisToolPaintInteraction::getOutlinePath(
    const QPointF &documentPos,
    const KoPointerEvent *event,
    KisPaintOpSettings::OutlineMode outlineMode)
{
    KisToolCanvas *toolCanvas = dynamic_cast<KisToolCanvas *>(canvas());
    Q_ASSERT(toolCanvas);
    const KisCoordinatesConverter *converter = toolCanvas->coordinatesConverter();

    const QPointF pixelPos = convertToPixelCoord(documentPos);
    if (event && event->isTouchEvent() && mode() == HOVER_MODE) {
        m_d->lastCursorPos.reset(pixelPos);
    }

    KisPaintInformation info(pixelPos);
    info.setCanvasMirroredH(converter->xAxisMirrored());
    info.setCanvasMirroredV(converter->yAxisMirrored());
    info.setCanvasRotation(converter->rotationAngle());
    info.setRandomSource(new KisRandomSource());
    info.setPerStrokeRandomSource(new KisPerStrokeRandomSource());

    const qreal currentZoom = canvas()->resourceManager()
        ? canvas()
              ->resourceManager()
              ->resource(KoCanvasResource::EffectiveZoom)
              .toReal()
        : 1.0;

    const QPointF previousPoint =
        m_d->lastCursorPos.pushThroughHistory(pixelPos, currentZoom);
    const qreal startAngle =
        KisAlgebra2D::directionBetweenPoints(previousPoint, pixelPos, 0);
    KisDistanceInformation distanceInfo(previousPoint, startAngle);
    KisPaintInformation::DistanceInformationRegistrar registrar =
        info.registerDistanceInformation(&distanceInfo);

    return currentPaintOpPreset()->settings()->brushOutline(
        info,
        outlineMode,
        converter->effectivePhysicalZoom());
}
