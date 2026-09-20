/*
 *  SPDX-FileCopyrightText: 2003-2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2015 Moritz Molch <kde@moritzmolch.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_paint.h"

#include <QAction>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QWidget>

#include <KoCanvasResourceProvider.h>
#include <KoPointerEvent.h>

#include <application/ui/workspace/KisViewManager.h>
#include <brushengine/kis_paintop_preset.h>
#include <kis_algebra_2d.h>
#include <kis_canvas2.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <application/kis_config.h>
#include <kis_icon.h>
#include <kis_image.h>
#include <resources/kis_popup_palette.h>
#include <kis_tool_canvas_utils.h>
#include <kis_tool_utils.h>


KisToolPaint::KisToolPaint(KoCanvasBase *canvas, const QCursor &cursor)
    : KisToolPaintInteraction(canvas, cursor)
    , m_colorSamplerHelper(dynamic_cast<KisCanvas2 *>(canvas))
{
    KisCanvas2 *kiscanvas = dynamic_cast<KisCanvas2 *>(canvas);
    KIS_ASSERT(kiscanvas);
    connect(this, SIGNAL(sigPaintingFinished()), kiscanvas->viewManager()->canvasResourceProvider(), SLOT(slotPainting()));

    connect(&m_colorSamplerHelper, SIGNAL(sigRequestCursor(QCursor)), this, SLOT(slotColorPickerRequestedCursor(QCursor)));
    connect(&m_colorSamplerHelper, SIGNAL(sigRequestCursorReset()), this, SLOT(slotColorPickerRequestedCursorReset()));
    connect(&m_colorSamplerHelper, SIGNAL(sigRequestUpdateOutline()), this, SLOT(slotColorPickerRequestedOutlineUpdate()));
}


KisToolPaint::~KisToolPaint() = default;


void KisToolPaint::activate(const QSet<KoShape*> &shapes)
{
    KisToolPaintInteraction::activate(shapes);
    if (flags() & KisTool::FLAG_USES_CUSTOM_SIZE) {
        connect(action("increase_brush_size"), SIGNAL(triggered()), this, SLOT(showBrushSize()));
        connect(action("decrease_brush_size"), SIGNAL(triggered()), this, SLOT(showBrushSize()));
    }
}

void KisToolPaint::deactivate()
{
    KisToolPaintInteraction::deactivate();
}

void KisToolPaint::slotColorPickerRequestedCursor(const QCursor &cursor)
{
    useCursor(cursor);
}

void KisToolPaint::slotColorPickerRequestedCursorReset()
{
    resetCursorStyle();
}

void KisToolPaint::slotColorPickerRequestedOutlineUpdate()
{
    requestUpdateOutline(m_outlineDocPoint, 0);
}

KisOptimizedBrushOutline KisToolPaint::tryFixBrushOutline(const KisOptimizedBrushOutline &originalOutline)
{
    KisConfig cfg(true);

    bool useSeparateEraserCursor = cfg.separateEraserCursor() && isEraser();

    const OutlineStyle currentOutlineStyle = !useSeparateEraserCursor ? cfg.newOutlineStyle() : cfg.eraserOutlineStyle();
    if (currentOutlineStyle == OUTLINE_NONE) return originalOutline;

    const qreal minThresholdSize = cfg.outlineSizeMinimum();

    /**
     * If the brush outline is bigger than the canvas itself (which
     * would make it invisible for a user in most of the cases) just
     * add a cross in the center of it
     */

    QSize widgetSize = canvas()->canvasWidget()->size();
    const int maxThresholdSum = widgetSize.width() + widgetSize.height();

    KisOptimizedBrushOutline outline = originalOutline;
    QRectF boundingRect = outline.boundingRect();
    const qreal sum = boundingRect.width() + boundingRect.height();

    QPointF center = boundingRect.center();

    if (sum > maxThresholdSum) {
        const int hairOffset = 7;

        QPainterPath crossIcon;

        crossIcon.moveTo(center.x(), center.y() - hairOffset);
        crossIcon.lineTo(center.x(), center.y() + hairOffset);

        crossIcon.moveTo(center.x() - hairOffset, center.y());
        crossIcon.lineTo(center.x() + hairOffset, center.y());

        outline.addPath(crossIcon);

    } else if (sum < minThresholdSize && !outline.isEmpty()) {
        outline = QPainterPath();
        outline.addEllipse(center, 0.5 * minThresholdSize, 0.5 * minThresholdSize);
    }

    return outline;
}

void KisToolPaint::paint(QPainter &gc, const KoViewConverter &converter)
{
    Q_UNUSED(converter);

    KisOptimizedBrushOutline path = tryFixBrushOutline(pixelToView(m_currentOutline));
    paintToolOutline(&gc, path);

    m_colorSamplerHelper.paint(gc, converter);
}

void KisToolPaint::activateAlternateAction(AlternateAction action)
{
    if (!isSamplingAction(action)) {
        KisTool::activateAlternateAction(action);
        return;
    }

    const bool sampleCurrentLayer = action == SampleFgNode || action == SampleBgNode;
    const bool sampleFgColor = action == SampleFgNode || action == SampleFgImage;
    m_colorSamplerHelper.activate(sampleCurrentLayer, sampleFgColor);
}

void KisToolPaint::deactivateAlternateAction(AlternateAction action)
{
    if (!isSamplingAction(action)) {
        KisTool::deactivateAlternateAction(action);
        return;
    }

    m_colorSamplerHelper.deactivate();
}

bool KisToolPaint::isSamplingAction(AlternateAction action) const
{
    return action == SampleFgNode ||
        action == SampleBgNode ||
        action == SampleFgImage ||
        action == SampleBgImage;
}

void KisToolPaint::beginAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if (isSamplingAction(action)) {
        setMode(SECONDARY_PAINT_MODE);

        KisToolUtils::ColorSamplerConfig config;
        config.load();

        m_colorSamplerHelper.startAction(event->point, config.radius, config.blend);
        requestUpdateOutline(event->point, event);
    } else {
        KisTool::beginAlternateAction(event, action);
    }
}

void KisToolPaint::continueAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if (isSamplingAction(action)) {
        m_colorSamplerHelper.continueAction(event->point);
        requestUpdateOutline(event->point, event);
    } else {
        KisTool::continueAlternateAction(event, action);
    }
}

void KisToolPaint::endAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if (isSamplingAction(action)) {
        m_colorSamplerHelper.endAction();
        requestUpdateOutline(event->point, event);
        setMode(HOVER_MODE);
    } else {
        KisTool::endAlternateAction(event, action);
    }
}

KisPopupWidgetInterface *KisToolPaint::popupWidget()
{
    KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());

    if (!kisCanvas) {
        return nullptr;
    }

    KisPopupWidgetInterface* popupWidget = kisCanvas->popupPalette();
    return popupWidget;
}

QWidget *KisToolPaint::createOptionWidget()
{
    QWidget *optionWidget = new QWidget();
    optionWidget->setObjectName(toolId());

    QVBoxLayout *verticalLayout = new QVBoxLayout(optionWidget);
    verticalLayout->setObjectName("KisToolPaint::OptionWidget::VerticalLayout");
    verticalLayout->setContentsMargins(0,0,0,0);
    verticalLayout->setSpacing(5);

    // See https://bugs.kde.org/show_bug.cgi?id=316896
    QWidget *specialSpacer = new QWidget(optionWidget);
    specialSpacer->setObjectName("SpecialSpacer");
    specialSpacer->setFixedSize(0, 0);
    verticalLayout->addWidget(specialSpacer);
    verticalLayout->addWidget(specialSpacer);

    m_optionsWidgetLayout = new QGridLayout();
    m_optionsWidgetLayout->setColumnStretch(1, 1);
    verticalLayout->addLayout(m_optionsWidgetLayout);
    m_optionsWidgetLayout->setContentsMargins(0,0,0,0);
    m_optionsWidgetLayout->setSpacing(5);

    if (!quickHelp().isEmpty()) {
        QPushButton *push = new QPushButton(KisIconUtils::loadIcon("help-contents"), QString(), optionWidget);
        connect(push, SIGNAL(clicked()), this, SLOT(slotPopupQuickHelp()));
        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->addWidget(push);
        hLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        verticalLayout->addLayout(hLayout);
    }

    return optionWidget;
}

QWidget* findLabelWidget(QGridLayout *layout, QWidget *control)
{
    QWidget *result = 0;

    int index = layout->indexOf(control);

    int row, col, rowSpan, colSpan;
    layout->getItemPosition(index, &row, &col, &rowSpan, &colSpan);

    if (col > 0) {
        QLayoutItem *item = layout->itemAtPosition(row, col - 1);

        if (item) {
            result = item->widget();
        }
    } else {
        QLayoutItem *item = layout->itemAtPosition(row, col + 1);
        if (item) {
            result = item->widget();
        }
    }

    return result;
}

void KisToolPaint::showControl(QWidget *control, bool value)
{
    control->setVisible(value);
    QWidget *label = findLabelWidget(m_optionsWidgetLayout, control);
    if (label) {
        label->setVisible(value);
    }
}

void KisToolPaint::enableControl(QWidget *control, bool value)
{
    control->setEnabled(value);
    QWidget *label = findLabelWidget(m_optionsWidgetLayout, control);
    if (label) {
        label->setEnabled(value);
    }
}

void KisToolPaint::addOptionWidgetLayout(QLayout *layout)
{
    Q_ASSERT(m_optionsWidgetLayout != 0);
    int rowCount = m_optionsWidgetLayout->rowCount();
    m_optionsWidgetLayout->addLayout(layout, rowCount, 0, 1, 2);
}


void KisToolPaint::addOptionWidgetOption(QWidget *control, QWidget *label)
{
    Q_ASSERT(m_optionsWidgetLayout != 0);
    if (label) {
        m_optionsWidgetLayout->addWidget(label, m_optionsWidgetLayout->rowCount(), 0);
        m_optionsWidgetLayout->addWidget(control, m_optionsWidgetLayout->rowCount() - 1, 1);
    }
    else {
        m_optionsWidgetLayout->addWidget(control, m_optionsWidgetLayout->rowCount(), 0, 1, 2);
    }
}


void KisToolPaint::slotPopupQuickHelp()
{
    QWhatsThis::showText(QCursor::pos(), quickHelp());
}

void KisToolPaint::showBrushSize()
{
    KisToolUtils::showBrushSizeFloatingMessage(canvas(), currentPaintOpPreset()->settings()->paintOpSize());
}

void KisToolPaint::requestUpdateOutline(const QPointF &outlineDocPoint, const KoPointerEvent *event)
{
    QRectF outlinePixelRect;
    QRectF outlineDocRect;

    QRectF colorPreviewDocUpdateRect;

    QPointF outlineMoveVector;

    if (supportsOutline()) {
        KisConfig cfg(true);
        KisPaintOpSettings::OutlineMode outlineMode;

        bool useSeparateEraserCursor = cfg.separateEraserCursor() && isEraser();

        const OutlineStyle currentOutlineStyle = !useSeparateEraserCursor ? cfg.newOutlineStyle() : cfg.eraserOutlineStyle();
        const auto outlineStyleIsVisible = [&]() {
            return currentOutlineStyle == OUTLINE_FULL ||
                   currentOutlineStyle == OUTLINE_CIRCLE ||
                   currentOutlineStyle == OUTLINE_TILT;
        };
        const auto shouldShowOutlineWhilePainting = [&]() {
            return !useSeparateEraserCursor ? cfg.showOutlineWhilePainting() : cfg.showEraserOutlineWhilePainting();
        };
        if (isOutlineEnabled() && isOutlineVisible() &&
                (mode() == KisTool::GESTURE_MODE ||
                    (outlineStyleIsVisible() &&
                        (mode() == HOVER_MODE ||
                         (mode() == PAINT_MODE && shouldShowOutlineWhilePainting()))))) { // lisp forever!

            outlineMode.isVisible = true;

            switch (!useSeparateEraserCursor ? cfg.newOutlineStyle() : cfg.eraserOutlineStyle()) {
            case OUTLINE_CIRCLE:
                outlineMode.forceCircle = true;
                break;
            case OUTLINE_TILT:
                outlineMode.forceCircle = true;
                outlineMode.showTiltDecoration = true;
                break;
            default:
                break;
            }
        }

        outlineMode.forceFullSize = !useSeparateEraserCursor ? cfg.forceAlwaysFullSizedOutline() : cfg.forceAlwaysFullSizedEraserOutline();

        outlineMoveVector = outlineDocPoint - m_outlineDocPoint;

        m_outlineDocPoint = outlineDocPoint;
        m_currentOutline = getOutlinePath(m_outlineDocPoint, event, outlineMode);

        outlinePixelRect = tryFixBrushOutline(m_currentOutline).boundingRect();
        outlineDocRect = currentImage()->pixelToDocument(outlinePixelRect);

        // This adjusted call is needed as we paint with a 3 pixel wide brush and the pen is outside the bounds of the path
        // Pen uses view coordinates so we have to zoom the document value to match 2 pixel in view coordinates
        // See BUG 275829
        qreal zoomX;
        qreal zoomY;
        canvas()->viewConverter()->zoom(&zoomX, &zoomY);
        qreal xoffset = 2.0/zoomX;
        qreal yoffset = 2.0/zoomY;

        if (!outlineDocRect.isEmpty()) {
            outlineDocRect.adjust(-xoffset,-yoffset,xoffset,yoffset);
        }

        colorPreviewDocUpdateRect = m_colorSamplerHelper.colorPreviewDocRect(m_outlineDocPoint);

        if (!colorPreviewDocUpdateRect.isEmpty()) {
            colorPreviewDocUpdateRect = colorPreviewDocUpdateRect.adjusted(-xoffset,-yoffset,xoffset,yoffset);
        }

    }

    // DIRTY HACK ALERT: we should fetch the assistant's dirty rect when requesting
    //                   the update, instead of just dumbly update the entire canvas!

    // WARNING: assistants code is also duplicated in KisDelegatedSelectPathWrapper::mouseMoveEvent

    KisCanvas2 *kiscanvas = qobject_cast<KisCanvas2*>(canvas());
    KisPaintingAssistantsDecorationSP decoration = kiscanvas->paintingAssistantsDecoration();
    if (decoration && decoration->visible() && decoration->hasPaintableAssistants()) {
        kiscanvas->updateCanvasDecorations();
    }

    if (!m_oldColorPreviewUpdateRect.isEmpty()) {
        kiscanvas->updateCanvasToolOutlineDoc(m_oldColorPreviewUpdateRect);
    }

    if (!m_oldOutlineRect.isEmpty()) {
        kiscanvas->updateCanvasToolOutlineDoc(m_oldOutlineRect);
    }

    if (!outlineDocRect.isEmpty()) {
        /**
         * A simple "update-ahead" implementation that issues an update a little
         * bigger to accommodate the possible following outline.
         *
         * The point is that canvas rendering comes through two stages of
         * compression and the canvas may request outline update when the
         * outline itself has already been changed. It causes visual tearing
         * on the screen (see https://bugs.kde.org/show_bug.cgi?id=476300).
         *
         * We can solve that in two ways:
         *
         * 1) Pass the actual outline with the update rect itself, which is
         *    a bit complicated and may result in the outline being a bit
         *    delayed visually. We don't implement this method (yet).
         *
         * 2) Just pass the update rect a bit bigger than the actual outline
         *    to accommodate a possible change in the outline. We calculate
         *    this bigger rect by offsetting the rect by the previous cursor
         *    offset.
         */

        /// Don't try to update-ahead if the offset is bigger than 50%
        /// of the brush outline
        const qreal maxUpdateAheadOutlinePortion = 0.5;

        /// 10% of extra move is added to offset
        const qreal offsetFuzzyExtension = 0.1;

        const qreal moveDistance = KisAlgebra2D::norm(outlineMoveVector);

        QRectF offsetRect;

        if (moveDistance < maxUpdateAheadOutlinePortion * KisAlgebra2D::maxDimension(outlineDocRect)) {
            offsetRect = outlineDocRect.translated((1.0 + offsetFuzzyExtension) * outlineMoveVector);
        }

        kiscanvas->updateCanvasToolOutlineDoc(outlineDocRect | offsetRect);
    }

    if (!colorPreviewDocUpdateRect.isEmpty()) {
        kiscanvas->updateCanvasToolOutlineDoc(colorPreviewDocUpdateRect);
    }

    m_oldOutlineRect = outlineDocRect;
    m_oldColorPreviewUpdateRect = colorPreviewDocUpdateRect;
}
