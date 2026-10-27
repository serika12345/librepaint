/*
 *  SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_shape.h"

#include <QWidget>
#include <QLayout>
#include <QGridLayout>
#include <QPointer>

#include <KoUnit.h>
#include <KoShape.h>
#include <KoCanvasBase.h>
#include <KoCanvasResourceProvider.h>
#include <KoShapeController.h>
#include <KoColorBackground.h>
#include <KoPatternBackground.h>
#include <KoShapeStroke.h>
#include <KoDocumentResourceManager.h>
#include <KoPathShape.h>

#include <klocalizedstring.h>
#include <ksharedconfig.h>

#include <canvas/kis_canvas_resource_provider.h>
#include <kis_image.h>
#include <kis_painting_information_builder.h>
#include <kis_tool_freehand_helper.h>
#include <gpu/KisGpuPaintImage.h>
#include <brushengine/kis_paintop_registry.h>
#include <kis_paint_device.h>
#include <kis_figure_painting_stroke.h>
#include <kis_selection.h>

#include <KoSelectedShapesProxy.h>
#include <KoSelection.h>
#include <commands/KoKeepShapesSelectedCommand.h>
#include "KisAngleGauge.h"
#include "KisFigurePaintingOptions.h"
#include "KoCanvasResourcesIds.h"
#include "KoFlakeTypes.h"
#include "kis_resources_snapshot.h"
#include <qlist.h>
#include <qobjectdefs.h>
#include <qset.h>
#include <qsharedpointer.h>
#include <QtGlobal>
#include "kis_selection_mask.h"
#include "kis_shape_selection.h"
#include "kis_processing_applicator.h"
#include "kis_stroke_job_strategy.h"
#include "kis_tool.h"
#include "kis_tool_paint.h"
#include "kis_types.h"

namespace {
class KisToolGpuFigureHelper final : public KisToolFreehandHelper
{
public:
    using KisToolFreehandHelper::KisToolFreehandHelper;

    void paint(KisImageWSP image, KisNodeSP node, const LibrePaint::Image::Gpu::Shape &shape,
               const QVector<QPointF> &points, const LibrePaint::Image::Gpu::FigureStyle &style)
    {
        namespace Gpu = LibrePaint::Image::Gpu;
        const auto owner = image->gpuImage();
        initPaintImpl(0, KisPaintInformation(shape.first), resourceManager(), image, node, image.data());
        if (!isRunning()) return;
        const auto operation = owner->operations().activeOperation();
        auto result = Gpu::Result::Success;
        if (points.size() > 1) {
            QList<Gpu::InputSample> vertices;
            vertices.reserve(points.size() - 1);
            for (qsizetype i = 1; i < points.size(); ++i) vertices.append({points[i], 1});
            result = owner->appendSamples(operation, vertices);
            if (!isRunning()) return;
        }
        if (result == Gpu::Result::Success) result = owner->updateFigure(operation, shape, style);
        if (result != Gpu::Result::Success) {
            cancelPaint();
            Q_EMIT requestErrorMessage(i18n("The figure could not be drawn on this GPU paint layer."));
            return;
        }
        endPaint();
    }
};
}

void KisToolShape::paintGpuOutline(const LibrePaint::Image::Gpu::Shape &shape, const KUndo2MagicString &name)
{
    paintGpuFigure(shape, {}, name);
}

void KisToolShape::paintGpuPolyline(const QVector<QPointF> &points, bool closed, const KUndo2MagicString &name)
{
    auto vertices = points;
    if (closed && vertices.size() > 1 && vertices.first() == vertices.last()) vertices.removeLast();
    if (vertices.size() < 2) return;
    namespace Gpu = LibrePaint::Image::Gpu;
    paintGpuFigure({closed ? Gpu::ShapeKind::Polygon : Gpu::ShapeKind::Polyline, vertices.first(), vertices.last(), 0},
                   vertices, name);
}

void KisToolShape::paintGpuFigure(const LibrePaint::Image::Gpu::Shape &shape, const QVector<QPointF> &points,
                                const KUndo2MagicString &name)
{
    if (fillStyle() == KisFigurePaintingOptions::FillStylePattern) {
        Q_EMIT statusTextChanged(i18n("Pattern fills are not yet supported for this GPU document."));
        return;
    }
    auto *source = canvas()->resourceManager();
    LibrePaint::Image::Gpu::FigureStyle style;
    style.outline = strokeStyle() != KisFigurePaintingOptions::StrokeStyleNone;
    if (fillStyle() != KisFigurePaintingOptions::FillStyleNone) {
        const auto color = (fillStyle() == KisFigurePaintingOptions::FillStyleForegroundColor ?
                            source->foregroundColor() : source->backgroundColor()).toQColor();
        style.fill = LibrePaint::Image::Gpu::RgbaPixel{quint8(color.red()), quint8(color.green()), quint8(color.blue()), 255};
    }
    KoCanvasResourceProvider resources;
    for (int key : {KoCanvasResource::CurrentPaintOpPreset, KoCanvasResource::CurrentEffectiveCompositeOp,
                    KoCanvasResource::Size, KoCanvasResource::Opacity, KoCanvasResource::MirrorHorizontal,
                    KoCanvasResource::MirrorVertical, KoCanvasResource::GlobalAlphaLock, KoCanvasResource::EraserMode}) {
        resources.setResource(key, source->resource(key));
    }
    resources.setForegroundColor(strokeStyle() == KisFigurePaintingOptions::StrokeStyleBackground ?
                                 source->backgroundColor() : source->foregroundColor());
    if (!style.outline) {
        namespace Gpu = LibrePaint::Image::Gpu;
        const KisImageSP retainedImage = image().toStrongRef();
        const KisNodeSP retainedNode = currentNode();
        const auto captured = captureGpuFillResources(retainedImage, retainedNode, &resources);
        if (!captured.diagnostic.isEmpty()) {
            Q_EMIT statusTextChanged(captured.diagnostic);
            return;
        }
        const auto owner = retainedImage->gpuImage();
        style.fillBlendMode = captured.brush.blendMode;
        QList<Gpu::InputSample> samples{{shape.first, 1}};
        for (qsizetype i = 1; i < points.size(); ++i) samples.append({points[i], 1});
        const QPointer<KisToolShape> tool(this);
        const auto result = captured.transparencyMask ?
            owner->paintTransparencyMaskFigure(captured.layer, captured.brush, samples, shape, style, 0) :
            owner->paintFigure(captured.layer, captured.brush, samples, shape, style, 0);
        if (tool && result != Gpu::Result::Success)
            Q_EMIT tool->statusTextChanged(i18n("The figure could not be drawn on this GPU paint layer."));
        return;
    }
    KisPaintingInformationBuilder builder;
    KisToolGpuFigureHelper helper(&builder, &resources, name);
    connect(&helper, &KisToolFreehandHelper::requestErrorMessage, this, &KoToolBase::statusTextChanged);
    helper.paint(image(), currentNode(), shape, points, style);
}


KisToolShape::KisToolShape(KoCanvasBase * canvas, const QCursor & cursor)
        : KisToolPaint(canvas, cursor)
{
    m_shapeOptionsWidget = 0;
}

KisToolShape::~KisToolShape()
{
    // in case the widget hasn't been shown
    if (m_shapeOptionsWidget && !m_shapeOptionsWidget->parent()) {
        delete m_shapeOptionsWidget;
    }
}

void KisToolShape::activate(const QSet<KoShape*> &shapes)
{
    KisToolPaint::activate(shapes);
    m_configGroup =  KSharedConfig::openConfig()->group(toolId());
}


int KisToolShape::flags() const
{
    return KisTool::FLAG_USES_CUSTOM_COMPOSITEOP|KisTool::FLAG_USES_CUSTOM_PRESET
           |KisTool::FLAG_USES_CUSTOM_SIZE;
}

QWidget * KisToolShape::createOptionWidget()
{
    m_shapeOptionsWidget = new WdgGeometryOptions(0);

    m_shapeOptionsWidget->cmbOutline->setCurrentIndex(KisPainter::StrokeStyleBrush);

    m_shapeOptionsWidget->angleSelectorRotation->setIncreasingDirection(KisAngleGauge::IncreasingDirection_Clockwise);
    m_shapeOptionsWidget->angleSelectorRotation->setFlipOptionsMode(KisAngleSelector::FlipOptionsMode_MenuButton);

    m_shapeOptionsWidget->sldScale->setSuffix("%");
    m_shapeOptionsWidget->sldScale->setRange(0.0, 10000.0, 2);
    m_shapeOptionsWidget->sldScale->setSoftMaximum(500);
    m_shapeOptionsWidget->sldScale->setSingleStep(1.0);

    //connect two combo box event. Inherited classes can call the slots to make appropriate changes
    connect(m_shapeOptionsWidget->cmbOutline, SIGNAL(currentIndexChanged(int)), this, SLOT(outlineSettingChanged(int)));
    connect(m_shapeOptionsWidget->cmbFill, SIGNAL(currentIndexChanged(int)), this, SLOT(fillSettingChanged(int)));
    connect(m_shapeOptionsWidget->angleSelectorRotation, SIGNAL(angleChanged(qreal)), this, SLOT(patternRotationSettingChanged(qreal)));
    connect(m_shapeOptionsWidget->sldScale, SIGNAL(valueChanged(qreal)), this, SLOT(patternScaleSettingChanged(qreal)));

    m_shapeOptionsWidget->cmbOutline->setCurrentIndex(m_configGroup.readEntry("outlineType", 0));
    m_shapeOptionsWidget->cmbFill->setCurrentIndex(m_configGroup.readEntry("fillType", 0));
    m_shapeOptionsWidget->sldScale->setValue(m_configGroup.readEntry("patternTransformScale", 100));
    m_shapeOptionsWidget->angleSelectorRotation->setAngle(m_configGroup.readEntry("patternTransformRotation", 0));

    //if both settings are empty, force the outline to brush so the tool will work when first activated
    if (  m_shapeOptionsWidget->cmbFill->currentIndex() == 0 &&
          m_shapeOptionsWidget->cmbOutline->currentIndex() == 0)
    {
        m_shapeOptionsWidget->cmbOutline->setCurrentIndex(1); // brush
    }

    bool enablePatternTransform = (m_shapeOptionsWidget->cmbFill->currentIndex() == int(KisFigurePaintingOptions::FillStylePattern));
    m_shapeOptionsWidget->gbPatternTransform->setEnabled(enablePatternTransform);

    return m_shapeOptionsWidget;
}

void KisToolShape::outlineSettingChanged(int value)
{
    m_configGroup.writeEntry("outlineType", value);
}

void KisToolShape::fillSettingChanged(int value)
{
    m_configGroup.writeEntry("fillType", value);
    bool enable = (value == int(KisFigurePaintingOptions::FillStylePattern));
    m_shapeOptionsWidget->gbPatternTransform->setEnabled(enable);
}

void KisToolShape::patternRotationSettingChanged(qreal value)
{
    m_configGroup.writeEntry("patternTransformRotation", value);
}

void KisToolShape::patternScaleSettingChanged(qreal value)
{
    m_configGroup.writeEntry("patternTransformScale", value);
}

KisFigurePaintingOptions::FillStyle KisToolShape::fillStyle()
{
    if (m_shapeOptionsWidget) {
        return static_cast<KisFigurePaintingOptions::FillStyle>(m_shapeOptionsWidget->cmbFill->currentIndex());
    } else {
        return KisFigurePaintingOptions::FillStyleNone;
    }
}

KisFigurePaintingOptions::StrokeStyle KisToolShape::strokeStyle()
{
    if (m_shapeOptionsWidget) {
        return static_cast<KisFigurePaintingOptions::StrokeStyle>(m_shapeOptionsWidget->cmbOutline->currentIndex());
    } else {
        return KisFigurePaintingOptions::StrokeStyleNone;
    }
}

QTransform KisToolShape::fillTransform()
{
    QTransform transform;

    if (m_shapeOptionsWidget) {
        transform.rotate(m_shapeOptionsWidget->angleSelectorRotation->angle());
        qreal scale = m_shapeOptionsWidget->sldScale->value()*0.01;
        transform.scale(scale, scale);
    }

    return transform;
}

qreal KisToolShape::currentStrokeWidth() const
{
    const qreal sizeInPx =
        canvas()->resourceManager()->resource(KoCanvasResource::Size).toReal();

    return canvas()->unit().fromUserValue(sizeInPx);
}

KisToolShape::ShapeAddInfo KisToolShape::shouldAddShape(KisNodeSP currentNode) const
{
    ShapeAddInfo info;

    if (currentNode->inherits("KisShapeLayer")) {
        info.shouldAddShape = true;
    } else if (KisSelectionMask *mask = dynamic_cast<KisSelectionMask*>(currentNode.data())) {
        if (mask->selection()->hasShapeSelection()) {
            info.shouldAddShape = true;
            info.shouldAddSelectionShape = true;
        }
    }

    return info;
}

void KisToolShape::ShapeAddInfo::markAsSelectionShapeIfNeeded(KoShape *shape) const
{
    if (this->shouldAddSelectionShape) {
        shape->setUserData(new KisShapeSelectionMarker());
    }
}

void KisToolShape::addShape(KoShape* shape)
{
    using namespace KisFigurePaintingOptions;

    KisResourcesSnapshot resources(image(),
                                   currentNode(),
                                   canvas()->resourceManager()->canvasResourcesInterface());
    switch(fillStyle()) {
        case FillStyleForegroundColor:
            shape->setBackground(QSharedPointer<KoColorBackground>(new KoColorBackground(resources.currentFgColor().toQColor())));
            break;
        case FillStyleBackgroundColor:
            shape->setBackground(QSharedPointer<KoColorBackground>(new KoColorBackground(resources.currentBgColor().toQColor())));
            break;
        case FillStylePattern:
            shape->setBackground(QSharedPointer<KoShapeBackground>(0));
            break;
        case FillStyleNone:
            shape->setBackground(QSharedPointer<KoShapeBackground>(0));
            break;
    }

    switch (strokeStyle()) {
    case KisFigurePaintingOptions::StrokeStyleNone:
        shape->setStroke(KoShapeStrokeModelSP());
        break;
    case KisFigurePaintingOptions::StrokeStyleForeground:
    case KisFigurePaintingOptions::StrokeStyleBackground: {
        KoShapeStrokeSP stroke(new KoShapeStroke());
        stroke->setLineWidth(currentStrokeWidth());
        const QColor color = strokeStyle() == KisFigurePaintingOptions::StrokeStyleForeground ?
                    resources.currentFgColor().toQColor() :
                    resources.currentBgColor().toQColor();
        stroke->setColor(color);
        shape->setStroke(stroke);
        break;
    }
    }

    KUndo2Command *parentCommand = new KUndo2Command();

    KoSelection *selection = canvas()->selectedShapesProxy()->selection();
    const QList<KoShape*> oldSelectedShapes = selection->selectedShapes();

    // reset selection on the newly added shape :)
    // TODO: think about moving this into controller->addShape?
    new KoKeepShapesSelectedCommand(oldSelectedShapes, {shape}, canvas()->selectedShapesProxy(), false, parentCommand);
    KUndo2Command *cmd = canvas()->shapeController()->addShape(shape, 0, parentCommand);
    parentCommand->setText(cmd->text());
    new KoKeepShapesSelectedCommand(oldSelectedShapes, {shape}, canvas()->selectedShapesProxy(), true, parentCommand);

    KisProcessingApplicator::runSingleCommandStroke(image(), cmd, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::EXCLUSIVE);
}

void KisToolShape::addPathShape(KoPathShape* pathShape, const KUndo2MagicString& name)
{
    KisNodeSP node = currentNode();
    if (!node) {
        return;
    }

    // Compute the outline
    KisImageSP image = this->image();
    QTransform matrix;
    matrix.scale(image->xRes(), image->yRes());
    matrix.translate(pathShape->position().x(), pathShape->position().y());
    QPainterPath mappedOutline = matrix.map(pathShape->outline());

    if (node->hasEditablePaintDevice()) {
        KisFigurePaintingStroke stroke(name,
                                       image,
                                       node,
                                       canvas()->resourceManager(),
                                       strokeStyle(),
                                       fillStyle(),
                                       fillTransform());
        stroke.paintPainterPath(mappedOutline);
    } else if (node->inherits("KisShapeLayer")) {
        pathShape->normalize();
        addShape(pathShape);

    }
}
