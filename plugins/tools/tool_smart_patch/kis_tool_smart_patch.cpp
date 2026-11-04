/*
 *  SPDX-FileCopyrightText: 2017 Eugene Ingerman
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_smart_patch.h"

#include "KoShape.h"
#include "QApplication"
#include "QPainterPath"

#include <klocalizedstring.h>
#include <KoColor.h>
#include <KoPointerEvent.h>
#include <application/ui/workspace/KisViewManager.h>
#include "kis_assert.h"
#include "kis_canvas2.h"
#include "kis_coordinates_converter.h"
#include "kis_cursor.h"
#include "kis_floating_message.h"
#include "kis_image.h"
#include "kis_node.h"
#include "kis_painter.h"
#include "kis_paintop_preset.h"

#include "kis_stroke_job_strategy.h"
#include "kis_tool.h"
#include "kis_tool_paint.h"
#include "kundo2magicstring.h"
#include "kundo2stack.h"
#include "commands_new/kis_transaction_based_command.h"
#include "kis_transaction.h"

#include "kis_processing_applicator.h"
#include "kis_datamanager.h"

#include "KoColorSpaceRegistry.h"
#include <kis_display_color_converter.h>
#include <KisCursorOverrideLock.h>
#include <qimage.h>
#include <qnamespace.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qset.h>
#include <QtGlobal>

#include "kis_tool_smart_patch_options_widget.h"
#include "libs/image/kis_paint_device_debug_utils.h"

#include "kis_algebra_2d.h"
#include "kis_resources_snapshot.h"
#include "kis_selection.h"
#include <KisGpuCpuBrushStroke.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <QKeyEvent>
#include <QPointer>

QRect patchImage(KisPaintDeviceSP imageDev, KisPaintDeviceSP maskDev, int radius, int accuracy, KisSelectionSP selection);

class KisToolSmartPatch::InpaintCommand : public KisTransactionBasedCommand {
public:
    InpaintCommand(KisPaintDeviceSP maskDev, KisNodeSP node, int accuracy, int patchRadius, KisSelectionSP selection) :
        m_maskDev(maskDev), m_node(node), m_accuracy(accuracy), m_patchRadius(patchRadius), m_selection(selection) {}

    KUndo2Command* paint() override {
        KisTransaction transaction(m_node->paintDevice());
        const auto dirty = patchImage(m_node->paintDevice(), m_maskDev, m_patchRadius, m_accuracy, m_selection);
        m_node->setDirty(dirty);
        return transaction.endAndTake();
    }

private:
    KisPaintDeviceSP m_maskDev;
    KisNodeSP m_node;
    int m_accuracy, m_patchRadius;
    KisSelectionSP m_selection;
};

struct KisToolSmartPatch::Private {
    KisPaintDeviceSP maskDev = nullptr;
    KisPainter maskDevPainter;
    float brushRadius = 50.; //initial default. actually read from ui.
    KisToolSmartPatchOptionsWidget *optionsWidget = nullptr;
    QRectF oldOutlineRect;
    QPainterPath brushOutline;
    KisImageWSP targetImage;
    KisNodeSP targetNode;
};


KisToolSmartPatch::KisToolSmartPatch(KoCanvasBase * canvas)
    : KisToolPaint(canvas, KisCursor::blankCursor()),
      m_d(new Private)
{
    setSupportOutline(true);
    setObjectName("tool_SmartPatch");
    m_d->maskDev = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    m_d->maskDevPainter.begin( m_d->maskDev );

    m_d->maskDevPainter.setPaintColor(KoColor(Qt::magenta, m_d->maskDev->colorSpace()));
    m_d->maskDevPainter.setBackgroundColor(KoColor(Qt::white, m_d->maskDev->colorSpace()));
    m_d->maskDevPainter.setFillStyle( KisPainter::FillStyleForegroundColor );
}

KisToolSmartPatch::~KisToolSmartPatch()
{
    requestStrokeCancellation();
    m_d->optionsWidget = nullptr;
    m_d->maskDevPainter.end();
}

void KisToolSmartPatch::activate(const QSet<KoShape*> &shapes)
{
    KisToolPaint::activate(shapes);
}

void KisToolSmartPatch::deactivate()
{
    requestStrokeCancellation();
    KisToolPaint::deactivate();
}

void KisToolSmartPatch::resetCursorStyle()
{
    KisToolPaint::resetCursorStyle();
}

void KisToolSmartPatch::activatePrimaryAction()
{
    setOutlineVisible(true);
    KisToolPaint::activatePrimaryAction();
}

void KisToolSmartPatch::deactivatePrimaryAction()
{
    setOutlineVisible(false);
    KisToolPaint::deactivatePrimaryAction();
}

void KisToolSmartPatch::addMaskPath( KoPointerEvent *event )
{
    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
    KIS_ASSERT(canvas2);
    const KisCoordinatesConverter *converter = canvas2->coordinatesConverter();

    QPointF imagePos = currentImage()->documentToPixel(event->point);
    QPainterPath currentBrushOutline = brushOutline().translated(KisAlgebra2D::alignForZoom(imagePos, converter->effectivePhysicalZoom()));
    m_d->maskDevPainter.fillPainterPath(currentBrushOutline);

    canvas()->updateCanvas(currentImage()->pixelToDocument(m_d->maskDev->exactBounds()));
}

void KisToolSmartPatch::beginPrimaryAction(KoPointerEvent *event)
{
    //we can only apply inpaint operation to paint layer
    if ( currentNode().isNull() || !currentNode()->inherits("KisPaintLayer") || nodePaintAbility()!=NodePaintAbility::PAINT ) {
        KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
        kiscanvas->viewManager()->
                showFloatingMessage(
                    i18n("Select a paint layer to use this tool"),
                    QIcon(), 2000, KisFloatingMessage::Medium, Qt::AlignCenter);
        event->ignore();
        return;
    }
    if (!nodeEditable()) {
        event->ignore();
        return;
    }

    m_d->targetImage = image();
    m_d->targetNode = currentNode();
    addMaskPath(event);
    setMode(KisTool::PAINT_MODE);
    KisToolPaint::beginPrimaryAction(event);
}

void KisToolSmartPatch::continuePrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);
    addMaskPath(event);
    KisToolPaint::continuePrimaryAction(event);
}

void KisToolSmartPatch::endPrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);
    if (event) addMaskPath(event);
    KisToolPaint::endPrimaryAction(event);
    setMode(KisTool::HOVER_MODE);

    auto targetImage = m_d->targetImage.toStrongRef();
    auto targetNode = m_d->targetNode;
    m_d->targetImage = nullptr;
    m_d->targetNode.clear();
    const auto mask = KisPainter::convertToAlphaAsAlpha(m_d->maskDev);
    const auto dirtyOverlay = m_d->maskDev->exactBounds();
    m_d->maskDev->clear();
    if (targetImage) canvas()->updateCanvas(targetImage->pixelToDocument(dirtyOverlay));
    if (!targetImage || image().data() != targetImage.data() || currentNode() != targetNode || !nodeEditable()) return;

    std::unique_ptr<KisGpuCpuBrushStroke> cpuPatch;
    if (targetImage->gpuImage()) {
        namespace Gpu = LibrePaint::Image::Gpu;
        const QPointer<KisToolSmartPatch> tool(this);
        const QPointer<KisViewManager> view(static_cast<KisCanvas2 *>(canvas())->viewManager());
        const auto owner = targetImage->gpuImage();
        if (!view || !view->blockUntilOperationsFinished(targetImage) || !tool || !view ||
            view->image().data() != targetImage.data() || targetImage->gpuImage() != owner || tool->currentNode() != targetNode) return;
        if (Gpu::toolExecutionLocation(Gpu::ToolOperation::SmartPatch) != Gpu::ExecutionLocation::Cpu) return;
        QString diagnostic;
        cpuPatch = KisGpuCpuBrushStroke::create(targetImage, targetNode, diagnostic,
            KisGpuCpuBrushStroke::Projection::None, {}, Gpu::HistoryAction::SmartPatch);
        if (!cpuPatch) {
            Q_EMIT statusTextChanged(i18n("The smart patch could not be applied to the selected paint layer."));
            return;
        }
        targetImage = cpuPatch->image();
        targetNode = cpuPatch->node();
    }

    KisCursorOverrideLock cursorLock(KisCursor::waitCursor());

    int accuracy = 50; //default accuracy - middle value
    int patchRadius = 4; //default radius, which works well for most cases tested

    if (m_d->optionsWidget) {
        accuracy = m_d->optionsWidget->getAccuracy();
        patchRadius = m_d->optionsWidget->getPatchRadius();
    }

    KisResourcesSnapshotSP resources =
        new KisResourcesSnapshot(targetImage, targetNode, this->canvas()->resourceManager()->canvasResourcesInterface());

    KisProcessingApplicator applicator(targetImage, targetNode, KisProcessingApplicator::NONE, KisImageSignalVector(),
                                        kundo2_i18n("Smart Patch"));

    //actual inpaint operation. filling in areas masked by user
    applicator.applyCommand( new InpaintCommand(mask, targetNode,
                                                 accuracy, patchRadius,
                                                 resources->activeSelection()),
                             KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE );

    applicator.end();
    targetImage->waitForDone();

    if (cpuPatch && (!applicator.successfullyCompletedFuture().get() || !cpuPatch->commit())) {
        Q_EMIT statusTextChanged(i18n("The smart patch could not be completed and was cancelled."));
    }
}

void KisToolSmartPatch::requestStrokeEnd()
{
    if (mode() == KisTool::PAINT_MODE) endPrimaryAction(nullptr);
}

void KisToolSmartPatch::requestStrokeCancellation()
{
    const auto dirty = m_d->maskDev->exactBounds();
    m_d->maskDev->clear();
    m_d->targetImage = nullptr;
    m_d->targetNode.clear();
    setMode(KisTool::HOVER_MODE);
    if (!dirty.isEmpty() && currentImage()) canvas()->updateCanvas(currentImage()->pixelToDocument(dirty));
}

void KisToolSmartPatch::keyPressEvent(QKeyEvent *event)
{
    if (mode() == KisTool::PAINT_MODE && event->key() == Qt::Key_Escape) {
        requestStrokeCancellation();
        event->accept();
        return;
    }
    KisToolPaint::keyPressEvent(event);
}

QPainterPath KisToolSmartPatch::brushOutline( void )
{
    const qreal diameter = m_d->brushRadius;
    QPainterPath outline;
    outline.addEllipse(QPointF(0,0), -0.5 * diameter, -0.5 * diameter );
    return outline;
}

QPainterPath KisToolSmartPatch::getBrushOutlinePath(const QPointF &documentPos,
                                          const KoPointerEvent *event)
{
    Q_UNUSED(event);

    QPointF imagePos = currentImage()->documentToPixel(documentPos);
    QPainterPath path = brushOutline();

    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(canvas2, QPainterPath());
    const KisCoordinatesConverter *converter = canvas2->coordinatesConverter();

    return path.translated(KisAlgebra2D::alignForZoom(imagePos, converter->effectivePhysicalZoom()));
}

void KisToolSmartPatch::requestUpdateOutline(const QPointF &outlineDocPoint, const KoPointerEvent *event)
{
    static QPointF lastDocPoint = QPointF(0,0);
    if( event )
        lastDocPoint=outlineDocPoint;

    m_d->brushRadius = currentPaintOpPreset()->settings()->paintOpSize();
    m_d->brushOutline = getBrushOutlinePath(lastDocPoint, event);

    QRectF outlinePixelRect = m_d->brushOutline.boundingRect();
    QRectF outlineDocRect = currentImage()->pixelToDocument(outlinePixelRect);

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

    if (!m_d->oldOutlineRect.isEmpty()) {
        canvas()->updateCanvas(m_d->oldOutlineRect);
    }

    if (!outlineDocRect.isEmpty()) {
        canvas()->updateCanvas(outlineDocRect);
    }

    m_d->oldOutlineRect = outlineDocRect;
}

void KisToolSmartPatch::paint(QPainter &painter, const KoViewConverter &converter)
{
    Q_UNUSED(converter);

    painter.save();
    QPainterPath path = pixelToView(m_d->brushOutline);
    paintToolOutline(&painter, path);
    painter.restore();

    painter.save();
    painter.setBrush(Qt::magenta);
    KisCanvas2 * kiscanvas = dynamic_cast<KisCanvas2*>(canvas());
    QImage img = kiscanvas->displayColorConverter()->convertImageToDisplayColorSpace(m_d->maskDev);
    if( !img.size().isEmpty() ){

        painter.drawImage(pixelToView(m_d->maskDev->exactBounds()), img);
    }
    painter.restore();
}

QWidget * KisToolSmartPatch::createOptionWidget()
{
    KisCanvas2 * kiscanvas = dynamic_cast<KisCanvas2*>(canvas());
    KIS_ASSERT(kiscanvas);

    m_d->optionsWidget = new KisToolSmartPatchOptionsWidget(kiscanvas->viewManager()->canvasResourceProvider(), 0);
    m_d->optionsWidget->setObjectName(toolId() + "option widget");

    return m_d->optionsWidget;
}
