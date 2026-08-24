/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "canvas/kis_canvas2.h"

#include <KoColor.h>
#include <KoColorModelStandardIds.h>
#include <KoCompositeOpRegistry.h>

#include <kconfiggroup.h>
#include <klocalizedstring.h>

#include <QDomDocument>
#include <QDropEvent>
#include <QMimeData>

#include <KisSwatch.h>
#include <commands_new/KisMergeLabeledLayersCommand.h>
#include <commands_new/kis_processing_command.h>
#include <commands_new/kis_update_command.h>
#include <kis_command_utils.h>
#include <kis_fill_painter.h>
#include <kis_icon_utils.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_resources_snapshot.h>
#include <kis_selection.h>
#include <kis_stroke_strategy_undo_command_based.h>

#include "canvas/kis_canvas_resource_provider.h"
#include "nodes/kis_node_manager.h"
#include "processing/fill_processing_visitor.h"
#include "workspace/KisViewManager.h"

QString KisCanvas2::handleColorDrop(QDropEvent *event,
                                    KisViewManager *viewManager,
                                    const KisNodeSP &currentViewNode,
                                    const QPoint &imagePosition,
                                    QIcon *messageIcon)
{
    Q_ASSERT(image());
    Q_ASSERT(viewManager);
    Q_ASSERT(messageIcon);

    if (!image()) {
        return QString();
    }

    // Cannot fill on non-painting layers (vector layer, clone layer, file layer, group layer)
    if (viewManager->activeNode().isNull() || viewManager->activeNode()->inherits("KisShapeLayer")
        || viewManager->activeNode()->inherits("KisCloneLayer") || !viewManager->activeDevice()) {
        return i18n("You cannot drag and drop colors on the selected layer type.");
    }

    // Cannot fill if the layer is not editable
    if (!viewManager->activeNode()->isEditable()) {
        QString message;
        if (!viewManager->activeNode()->visible() && viewManager->activeNode()->userLocked()) {
            message = i18n("Layer is locked and invisible.");
        } else if (viewManager->activeNode()->userLocked()) {
            message = i18n("Layer is locked.");
        } else if (!viewManager->activeNode()->visible()) {
            message = i18n("Layer is invisible.");
        }
        *messageIcon = KisIconUtils::loadIcon("object-locked");
        return message;
    }

    // The cursor is outside the image
    if (!image()->wrapAroundModePermitted() && !image()->bounds().contains(imagePosition)) {
        return QString();
    }

    KisStrokeStrategyUndoCommandBased *strategy =
        new KisStrokeStrategyUndoCommandBased(kundo2_i18n("Flood Fill Layer"), false, image().data());
    strategy->setSupportsWrapAroundMode(true);
    KisStrokeId fillStrokeId = image()->startStroke(strategy);
    KIS_SAFE_ASSERT_RECOVER(fillStrokeId)
    {
        return QString();
    }

    QSharedPointer<QRect> dirtyRect = QSharedPointer<QRect>(new QRect);

    KisResourcesSnapshotSP resources =
        new KisResourcesSnapshot(image(),
                                 viewManager->activeNode(),
                                 viewManager->canvasResourceProvider()->resourceManager()->canvasResourcesInterface());

    if (event->mimeData()->hasColor()) {
        resources->setFGColorOverride(KoColor(event->mimeData()->colorData().value<QColor>(), image()->colorSpace()));
    } else {
        QByteArray byteData = event->mimeData()->data("krita/x-colorsetentry");
        KisSwatch s = KisSwatch::fromByteArray(byteData);
        resources->setFGColorOverride(s.color());
    }

    // Use same options as the fill tool
    KConfigGroup configGroup = KSharedConfig::openConfig()->group("KritaFill/KisToolFill");
    QString fillMode = configGroup.readEntry<QString>("whatToFill", "");
    if (fillMode.isEmpty()) {
        if (configGroup.readEntry<bool>("fillSelection", false)) {
            fillMode = "fillSelection";
        } else {
            fillMode = "fillContiguousRegion";
        }
    }
    const bool useCustomBlendingOptions = configGroup.readEntry<bool>("useCustomBlendingOptions", false);
    const qreal customOpacity = qBound(0, configGroup.readEntry<int>("customOpacity", 100), 100) / 100.0;
    QString customCompositeOp = configGroup.readEntry<QString>("customCompositeOp", COMPOSITE_OVER);
    if (KoCompositeOpRegistry::instance().getKoID(customCompositeOp).id().isNull()) {
        customCompositeOp = COMPOSITE_OVER;
    }

    if (event->keyboardModifiers() == Qt::ShiftModifier) {
        if (fillMode == "fillSimilarRegions") {
            fillMode = "fillSelection";
        } else {
            fillMode = "fillSimilarRegions";
        }
    } else if (event->keyboardModifiers() == Qt::AltModifier) {
        if (fillMode == "fillContiguousRegion") {
            fillMode = "fillSelection";
        } else {
            fillMode = "fillContiguousRegion";
        }
    }

    if (fillMode == "fillSelection") {
        FillProcessingVisitor *visitor =
            new FillProcessingVisitor(nullptr,
                                      viewManager->nodeManager()->selectionForNode(currentViewNode, image()),
                                      resources);
        visitor->setSeedPoint(imagePosition);
        visitor->setSelectionOnly(true);
        visitor->setUseCustomBlendingOptions(useCustomBlendingOptions);
        if (useCustomBlendingOptions) {
            visitor->setCustomOpacity(customOpacity);
            visitor->setCustomCompositeOp(customCompositeOp);
        }
        visitor->setOutDirtyRect(dirtyRect);

        image()->addJob(fillStrokeId,
                        new KisStrokeStrategyUndoCommandBased::Data(
                            KUndo2CommandSP(new KisProcessingCommand(visitor, viewManager->activeNode())),
                            false,
                            KisStrokeJobData::SEQUENTIAL,
                            KisStrokeJobData::EXCLUSIVE));
    } else {
        const int threshold = configGroup.readEntry("thresholdAmount", 8);
        const int opacitySpread = configGroup.readEntry("opacitySpread", 100);
        const bool antiAlias = configGroup.readEntry("antiAlias", true);
        const int grow = configGroup.readEntry("growSelection", 0);
        const bool stopGrowingAtDarkestPixel = configGroup.readEntry<bool>("stopGrowingAtDarkestPixel", false);
        const int feather = configGroup.readEntry("featherAmount", 0);
        const int closeGap = configGroup.readEntry("closeGapAmount", 0);
        QString sampleLayersMode = configGroup.readEntry("sampleLayersMode", "");
        if (sampleLayersMode.isEmpty()) {
            if (configGroup.readEntry("sampleMerged", false)) {
                sampleLayersMode = "allLayers";
            } else {
                sampleLayersMode = "currentLayer";
            }
        }
        QList<int> colorLabels;
        {
            const QStringList colorLabelsStr =
                configGroup.readEntry<QString>("colorLabels", "").split(',', Qt::SkipEmptyParts);

            for (const QString &colorLabelStr : colorLabelsStr) {
                bool ok;
                const int colorLabel = colorLabelStr.toInt(&ok);
                if (ok) {
                    colorLabels << colorLabel;
                }
            }
        }

        KisPaintDeviceSP referencePaintDevice = nullptr;
        if (sampleLayersMode == "allLayers") {
            referencePaintDevice = image()->projection();
        } else if (sampleLayersMode == "currentLayer") {
            referencePaintDevice = viewManager->activeNode()->paintDevice();
        } else if (sampleLayersMode == "colorLabeledLayers") {
            referencePaintDevice =
                KisMergeLabeledLayersCommand::createRefPaintDevice(image(), "Fill Tool Reference Result Paint Device");
            image()->addJob(fillStrokeId,
                            new KisStrokeStrategyUndoCommandBased::Data(
                                KUndo2CommandSP(new KisMergeLabeledLayersCommand(
                                    image(),
                                    referencePaintDevice,
                                    colorLabels,
                                    KisMergeLabeledLayersCommand::GroupSelectionPolicy_SelectIfColorLabeled)),
                                false,
                                KisStrokeJobData::SEQUENTIAL,
                                KisStrokeJobData::EXCLUSIVE));
        }

        QSharedPointer<KoColor> referenceColor(new KoColor);
        if (sampleLayersMode == "colorLabeledLayers") {
            // We need to obtain the reference color from the reference paint
            // device, but it is produced in a stroke, so we must get the color
            // after the device is ready. So we get it in the stroke
            image()->addJob(fillStrokeId,
                            new KisStrokeStrategyUndoCommandBased::Data(
                                KUndo2CommandSP(new KisCommandUtils::LambdaCommand(
                                    [referenceColor, referencePaintDevice, imagePosition]() -> KUndo2Command * {
                                        *referenceColor = referencePaintDevice->pixel(imagePosition);
                                        return 0;
                                    })),
                                false,
                                KisStrokeJobData::SEQUENTIAL,
                                KisStrokeJobData::EXCLUSIVE));
        } else {
            // Here the reference device is already ready, so we obtain the
            // reference color directly
            *referenceColor = referencePaintDevice->pixel(imagePosition);
        }

        if (fillMode == "fillContiguousRegion") {
            const KisFillPainter::RegionFillingMode regionFillingMode =
                configGroup.readEntry("contiguousFillMode", "") == "boundaryFill"
                ? KisFillPainter::RegionFillingMode_BoundaryFill
                : KisFillPainter::RegionFillingMode_FloodFill;
            KoColor regionFillingBoundaryColor;
            if (regionFillingMode == KisFillPainter::RegionFillingMode_BoundaryFill) {
                const QString xmlColor = configGroup.readEntry("contiguousFillBoundaryColor", QString());
                QDomDocument doc;
                if (doc.setContent(xmlColor)) {
                    QDomElement e = doc.documentElement().firstChild().toElement();
                    QString channelDepthID =
                        doc.documentElement().attribute("channeldepth", Integer16BitsColorDepthID.id());
                    bool ok;
                    if (e.hasAttribute("space") || e.tagName().toLower() == "srgb") {
                        regionFillingBoundaryColor = KoColor::fromXML(e, channelDepthID, &ok);
                    } else if (doc.documentElement().hasAttribute("space")
                               || doc.documentElement().tagName().toLower() == "srgb") {
                        regionFillingBoundaryColor = KoColor::fromXML(doc.documentElement(), channelDepthID, &ok);
                    }
                }
            }
            const bool useSelectionAsBoundary = configGroup.readEntry("useSelectionAsBoundary", false);
            const bool blendingOptionsAreNoOp = useCustomBlendingOptions
                ? (qFuzzyCompare(customOpacity, OPACITY_OPAQUE_F) && customCompositeOp == COMPOSITE_OVER)
                : (qFuzzyCompare(resources->opacity(), OPACITY_OPAQUE_F)
                   && resources->compositeOpId() == COMPOSITE_OVER);
            const bool useFastMode = !resources->activeSelection() && blendingOptionsAreNoOp && opacitySpread == 100
                && useSelectionAsBoundary == false && !antiAlias && grow == 0 && feather == 0 && closeGap == 0
                && sampleLayersMode == "currentLayer";

            FillProcessingVisitor *visitor =
                new FillProcessingVisitor(referencePaintDevice,
                                          viewManager->nodeManager()->selectionForNode(currentViewNode, image()),
                                          resources);
            visitor->setSeedPoint(imagePosition);
            visitor->setUseFastMode(useFastMode);
            visitor->setUseSelectionAsBoundary(useSelectionAsBoundary);
            visitor->setFeather(feather);
            visitor->setSizeMod(grow);
            visitor->setStopGrowingAtDarkestPixel(stopGrowingAtDarkestPixel);
            visitor->setRegionFillingMode(regionFillingMode);
            if (regionFillingMode == KisFillPainter::RegionFillingMode_BoundaryFill) {
                visitor->setRegionFillingBoundaryColor(regionFillingBoundaryColor);
            }
            visitor->setFillThreshold(threshold);
            visitor->setOpacitySpread(opacitySpread);
            visitor->setCloseGap(closeGap);
            visitor->setAntiAlias(antiAlias);
            visitor->setUseCustomBlendingOptions(useCustomBlendingOptions);
            if (useCustomBlendingOptions) {
                visitor->setCustomOpacity(customOpacity);
                visitor->setCustomCompositeOp(customCompositeOp);
            }
            visitor->setOutDirtyRect(dirtyRect);

            image()->addJob(fillStrokeId,
                            new KisStrokeStrategyUndoCommandBased::Data(
                                KUndo2CommandSP(new KisProcessingCommand(visitor, viewManager->activeNode())),
                                false,
                                KisStrokeJobData::SEQUENTIAL,
                                KisStrokeJobData::EXCLUSIVE));
        } else {
            KisSelectionSP fillMask = new KisSelection;
            QSharedPointer<KisProcessingVisitor::ProgressHelper> progressHelper(
                new KisProcessingVisitor::ProgressHelper(currentViewNode));

            {
                KisSelectionSP selection = viewManager->nodeManager()->selectionForNode(currentViewNode, image());
                KisFillPainter painter;
                QRect bounds = image()->bounds();
                if (selection) {
                    bounds = bounds.intersected(selection->projection()->selectedRect());
                }

                painter.setFillThreshold(threshold);
                painter.setOpacitySpread(opacitySpread);
                painter.setAntiAlias(antiAlias);
                painter.setSizemod(grow);
                painter.setStopGrowingAtDarkestPixel(stopGrowingAtDarkestPixel);
                painter.setFeather(feather);

                QVector<KisStrokeJobData *> jobs =
                    painter.createSimilarColorsSelectionJobs(fillMask->pixelSelection(),
                                                             referenceColor,
                                                             referencePaintDevice,
                                                             bounds,
                                                             selection ? selection->projection() : nullptr,
                                                             progressHelper);

                for (KisStrokeJobData *job : jobs) {
                    image()->addJob(fillStrokeId, job);
                }
            }

            {
                FillProcessingVisitor *visitor = new FillProcessingVisitor(nullptr, fillMask, resources);

                visitor->setSeedPoint(imagePosition);
                visitor->setSelectionOnly(true);
                visitor->setProgressHelper(progressHelper);
                visitor->setOutDirtyRect(dirtyRect);

                image()->addJob(fillStrokeId,
                                new KisStrokeStrategyUndoCommandBased::Data(
                                    KUndo2CommandSP(new KisProcessingCommand(visitor, currentViewNode)),
                                    false,
                                    KisStrokeJobData::SEQUENTIAL,
                                    KisStrokeJobData::EXCLUSIVE));
            }
        }
    }

    image()->addJob(fillStrokeId,
                    new KisStrokeStrategyUndoCommandBased::Data(
                        KUndo2CommandSP(new KisUpdateCommand(viewManager->activeNode(), dirtyRect, image().data())),
                        false,
                        KisStrokeJobData::SEQUENTIAL,
                        KisStrokeJobData::EXCLUSIVE));

    image()->endStroke(fillStrokeId);

    return QString();
}
