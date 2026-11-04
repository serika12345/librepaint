/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentViewTest.h"
#include <KisGpuPaintDeviceTransfer_p.h>
#include <KisGpuFilterConfiguration.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <kis_filter_mask.h>

void KisGpuDocumentViewTest::cpuBrushesKeepNativeEditing_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("engine");
    QTest::addColumn<bool>("alpha");
    QTest::newRow("u8-grid") << false << QStringLiteral("gridbrush") << false;
    QTest::newRow("u16-selected-deform") << true << QStringLiteral("deformbrush") << false;
    QTest::newRow("u8-merged-smudge") << false << QStringLiteral("colorsmudge") << false;
    QTest::newRow("u16-selected-offcanvas-smudge") << true << QStringLiteral("colorsmudge") << false;
    QTest::newRow("u8-merged-clone") << false << QStringLiteral("duplicate") << false;
    QTest::newRow("u16-selected-node-clone") << true << QStringLiteral("duplicate") << false;
    QTest::newRow("u8-white-default-mask-grid") << false << QStringLiteral("gridbrush") << true;
    QTest::newRow("u16-selected-filter-mask-deform") << true << QStringLiteral("deformbrush") << true;
}

void KisGpuDocumentViewTest::cpuBrushesKeepNativeEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words); QFETCH(QString, engine); QFETCH(bool, alpha);
    const TestUtil::PaintingEngineScope useGpu(true);
    const QString id = engine;
    const bool smudge = id == QLatin1String("colorsmudge");
    const bool duplicate = id == QLatin1String("duplicate");
    const bool sampling = smudge || duplicate;
    QCOMPARE(Gpu::brushExecutionLocation(id), Gpu::ExecutionLocation::Cpu);
    QCOMPARE(Gpu::brushExecutionLocation(QStringLiteral("paintbrush")), Gpu::ExecutionLocation::Gpu);
    QCOMPARE(Gpu::brushExecutionLocation(QStringLiteral("roundmarker")), Gpu::ExecutionLocation::Gpu);
    if (sampling) {
        KisResourceModel brushes(ResourceType::Brushes);
        if (brushes.rowCount() == 0)
            QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
        KisResourceModel patterns(ResourceType::Patterns);
        if (patterns.rowCount() == 0)
            QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    }
    KisPaintOpPresetSP preset(new KisPaintOpPreset(QStringLiteral(KRITA_SOURCE_DIR "/plugins/paintops/defaultpresets/") + id + QStringLiteral(".kpp")));
    QVERIFY(preset->load(KisGlobalResourcesInterface::instance()));
    QVERIFY(preset->valid());
    preset->setName(QStringLiteral("CPU brush fixture ") + QString::fromLatin1(QTest::currentDataTag()));
    preset->setFilename(QStringLiteral("cpu-brush-fixture-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
    if (smudge) {
        preset->settings()->setProperty("MergedPaint", true);
        preset->settings()->setProperty("ColorRateValue", 0.0);
    } else if (duplicate) {
        preset->settings()->setProperty("Duplicateop/CloneFromProjection", !words);
        preset->settings()->setProperty("Duplicateop/MoveSourcePoint", true);
        preset->settings()->setProperty("Duplicateop/ResetSourcePoint", false);
        preset->settings()->setProperty("Duplicateop/Healing", false);
    } else if (words) {
        preset->settings()->setProperty(AIRBRUSH_ENABLED, false);
        preset->settings()->setProperty("Deform/useMovementPaint", false);
    } else {
        preset->settings()->setProperty("Grid/randomBorder", false);
        preset->settings()->setProperty("ColorOption/useRandomOpacity", false);
        preset->settings()->setProperty("ColorOption/useRandomHSV", false);
    }
    QImage preview(16, 16, QImage::Format_ARGB32); preview.fill(Qt::black); preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets); QVERIFY(presets.addResource(preset));
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const QRect canvas(0, 0, 270, 32), inspected(-8, -8, 286, 48);
    QByteArray initial(canvas.width() * canvas.height() * cs->pixelSize(), Qt::Uninitialized);
    for (int y = 0; y < canvas.height(); ++y) for (int x = 0; x < canvas.width(); ++x) {
        KoColor color(QColor((x * 11) % 256, (y * 23) % 256, (x * 3 + y * 7) % 256, x % 2 ? 255 : 131), cs);
        std::memcpy(initial.data() + (y * canvas.width() + x) * cs->pixelSize(), color.data(), cs->pixelSize());
    }
    class BrushProbe : public KisToolFreehandHelper {
    public:
        using KisToolFreehandHelper::KisToolFreehandHelper;
        using KisToolFreehandHelper::initPaintImpl;
        using KisToolFreehandHelper::paintAt;
        using KisToolFreehandHelper::paintLine;
    };
    const quint8 defaultAlpha = words ? 0 : 255;
    QByteArray initialAlpha(canvas.width() * canvas.height(), Qt::Uninitialized);
    for (int y = 0; y < canvas.height(); ++y) for (int x = 0; x < canvas.width(); ++x)
        initialAlpha[y * canvas.width() + x] = char((x * 17 + y * 43) % 256);
    QByteArray reference, referenceProjection, sourceProjection, referenceSelection;
    for (const bool gpu : {false, true}) {
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image;
        if (gpu) {
            QString diagnostic;
            const auto execution = KisPart::instance()->gpuExecution(diagnostic);
            QVERIFY2(execution, qPrintable(diagnostic));
            Gpu::RasterImage raster;
            raster.size = canvas.size(); raster.format = format;
            raster.layers = {{QStringLiteral("CPU brush layer"), true, QUuid::createUuid(), {}}};
            const auto owner = paintImageWithEncoding(execution, raster);
            QCOMPARE(owner->writeRgbaPixels(owner->operations().activeLayer(), canvas, kisGpuPixelTransfer(initial, format, true)), Gpu::Result::Success);
            image = KisImage::fromGpuImage(owner, QStringLiteral("CPU brush editing"), cs);
        } else {
            image = new KisImage(document->createUndoStore(), canvas.width(), canvas.height(), cs, "CPU brush editing");
            KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("CPU brush layer"), 255);
            layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), canvas);
            QVERIFY(image->addNode(layer));
        }
        KisNodeSP node = image->root()->firstChild();
        const auto paintNode = node;
        if (alpha) {
            const auto filter = KisFilterRegistry::instance()->value(QStringLiteral("fastcoloroverlay"));
            QVERIFY(filter);
            const auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
            if (gpu) {
                const auto owner = image->gpuImage();
                const auto coverage = Gpu::SelectionMask::fromAlphaTiles([&] {
                    Gpu::SelectionTileMap tiles;
                    for (int tx = 0; tx < 2; ++tx) {
                        QByteArray bytes(Gpu::SelectionMask::AlphaTileBytes, char(defaultAlpha));
                        for (int y = 0; y < canvas.height(); ++y)
                            for (int x = tx * 256; x < qMin((tx + 1) * 256, canvas.width()); ++x)
                                bytes[y * 256 + x % 256] = initialAlpha[y * canvas.width() + x];
                        tiles.insert({tx, 0}, bytes);
                    }
                    return tiles;
                }(), defaultAlpha);
                QVERIFY(coverage);
                if (words) {
                    Gpu::RasterLayer mask{QStringLiteral("CPU brush coverage"), true, QUuid::createUuid(), {}};
                    mask.kind = Gpu::LayerKind::FilterMask; mask.parentUuid = paintNode->uuid();
                    mask.filter = freezeGpuFilterConfiguration(config, cs); mask.filterSelection = coverage;
                    QVERIFY(mask.filter); QVERIFY(owner->addFilterNode(mask, 1));
                    node = KisLayerUtils::findNodeByUuid(image->root(), mask.uuid);
                } else {
                    const auto id = owner->operations().activeLayer();
                    QCOMPARE(owner->addTransparencyMask(id), Gpu::Result::Success);
                    QCOMPARE(owner->replaceTransparencyMask(id, coverage), Gpu::Result::Success);
                    node = paintNode->firstChild();
                }
            } else {
                KisSelectionSP coverage = new KisSelection;
                coverage->pixelSelection()->setDefaultPixel(KoColor(&defaultAlpha, KoColorSpaceRegistry::instance()->alpha8()));
                coverage->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(initialAlpha.constData()), canvas);
                KisMaskSP mask;
                if (words) {
                    auto filterMask = new KisFilterMask(image, QStringLiteral("CPU brush coverage"));
                    filterMask->setFilter(config->cloneWithResourcesSnapshot()); mask = filterMask;
                } else mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
                mask->setSelection(coverage); QVERIFY(image->addNode(mask, paintNode)); node = mask;
            }
            QVERIFY(node); QCOMPARE(node->paintDevice()->pixelSize(), quint32(1));
        }
        const QRect selectionArea = sampling ? QRect(-5, 4, 14, 24) : QRect(247, 4, 14, 24);
        QUuid backdropUuid;
        QByteArray backdropPixels;
        if (sampling) {
            KisPaintDeviceSP pixels = new KisPaintDevice(cs);
            pixels->fill(inspected, KoColor(QColor(17, 201, 33, 209), cs));
            if (duplicate) {
                for (int y = inspected.top(); y <= inspected.bottom(); ++y)
                    for (int x = inspected.left(); x <= inspected.right(); ++x)
                        pixels->fill(QRect(x, y, 1, 1), KoColor(QColor((x + 19) * 7 % 256,
                            (y + 11) * 5 % 256, (x + y + 31) * 3 % 256, 209), cs));
            }
            backdropPixels.resize(inspected.width() * inspected.height() * cs->pixelSize());
            pixels->readBytes(reinterpret_cast<quint8 *>(backdropPixels.data()), inspected);
            if (gpu) {
                const auto owner = image->gpuImage();
                const auto id = owner->addLayer(QStringLiteral("Brush source"), 0, false, {});
                QVERIFY(id);
                QCOMPARE(owner->writeRgbaPixels(id, inspected, kisGpuPixelTransfer(backdropPixels, format, true)), Gpu::Result::Success);
                backdropUuid = owner->operations().snapshot()->layers.first().uuid;
            } else {
                KisPaintLayerSP backdrop = new KisPaintLayer(image, QStringLiteral("Brush source"), 255, pixels);
                backdropUuid = backdrop->uuid();
                QVERIFY(image->addNode(backdrop, image->root(), quint32(0)));
            }
            const auto source = KisLayerUtils::findNodeByUuid(image->root(), backdropUuid);
            QVERIFY(source);
            QByteArray check(backdropPixels.size(), Qt::Uninitialized);
            source->paintDevice()->readBytes(reinterpret_cast<quint8 *>(check.data()), inspected);
            QCOMPARE(check, backdropPixels);
        }
        if (words) {
            if (gpu) {
                const auto coverage = Gpu::SelectionMask::fromAlphaCoverage(selectionArea,
                    QByteArray(selectionArea.width() * selectionArea.height(), char(173)), selectionArea);
                QVERIFY(coverage); QVERIFY(image->setGpuSelectionMask(coverage));
            } else {
                KisSelectionSP selection = new KisSelection;
                selection->pixelSelection()->setDefaultBounds(new KisDefaultBounds(image));
                selection->pixelSelection()->select(selectionArea, 173);
                KisSelectionMaskSP mask = new KisSelectionMask(image);
                mask->setSelection(selection); mask->setActive(true); QVERIFY(image->addNode(mask, image->root()));
            }
        }
        document->setCurrentImage(image);
        image->refreshGraphAsync(image->root(), inspected, inspected); image->waitForDone();
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
        auto *view = window->newView(document.get()); QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        const auto cleanup = qScopeGuard([&] {
            if (lifetime) {
                lifetime->setModified(false);
                window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            }
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        window->viewManager()->setCurrentView(view);
        window->viewManager()->nodeManager()->slotUiActivatedNode(node);
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setPaintOpPreset(preset); resources->setSize(12); resources->setOpacity(.7);
        resources->setCurrentCompositeOp(QStringLiteral("normal")); resources->setEraserMode(false);
        resources->setGlobalAlphaLock(false); resources->setMirrorHorizontal(false); resources->setMirrorVertical(false);
        resources->setFGColor(KoColor(QColor(221, 19, 73), cs));
        view->canvasBase()->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
        KisPaintingInformationBuilder builder;
        BrushProbe helper(&builder, view->canvasBase()->resourceManager(), kundo2_i18n("Paint stroke"));
        helper.smoothingOptions()->setSmoothingType(KisSmoothingOptions::NO_SMOOTHING);
        QSignalSpy errors(&helper, &KisToolFreehandHelper::requestErrorMessage);
        const auto capture = [&] {
            image->waitForDone();
            if (sampling || alpha) {
                image->refreshGraphAsync(image->root(), inspected, inspected);
                image->waitForDone();
            }
            QByteArray bytes(inspected.width() * inspected.height() * node->paintDevice()->pixelSize(), Qt::Uninitialized);
            node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), inspected);
            return bytes;
        };
        const auto difference = [&](const QByteArray &actual, const QByteArray &expected) {
            return pixelStepDifference(actual, expected, alpha ? 1 : words ? 2 : 1, alpha ? 1 : 4);
        };
        const auto original = capture();
        if (alpha) QCOMPARE(quint8(original[0]), defaultAlpha);
        if (sampling) {
            QByteArray pixels(original.size(), Qt::Uninitialized);
            image->projection()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), inspected);
            if (!gpu) sourceProjection = pixels;
            else QVERIFY2(pixelStepDifference(pixels, sourceProjection, words ? 2 : 1).isEmpty(),
                          qPrintable(pixelStepDifference(pixels, sourceProjection, words ? 2 : 1)));
        }
        const auto begin = [&] {
            const qreal x = sampling && words ? -1.5 : 250.5;
            KisPaintInformation first(QPointF(x, 12.5)), last(QPointF(x + 10, 18.5));
            if (words && !image->globalSelection()) return false;
            if (duplicate) {
                const auto source = words ? KisLayerUtils::findNodeByUuid(image->root(), backdropUuid) : node;
                auto settings = resources->currentPreset()->settings();
                if (settings->mousePressEvent(KisPaintInformation(QPointF(20.5, 10.5)), Qt::ControlModifier, source)) return false;
                // Disjoint sampling keeps this fixed input independent of
                // native merged-projection update timing during the stroke.
                if (settings->mousePressEvent(KisPaintInformation(QPointF(words ? 251.5 : 148.5, 12.5)),
                    Qt::ControlModifier | Qt::AltModifier, node)) return false;
                if (settings->sourceNode().data() != source.data()) return false;
                if (!settings->mousePressEvent(first, Qt::NoModifier, node)) return false;
            }
            helper.initPaintImpl(0, first, view->canvasBase()->resourceManager(), image, node, image.data());
            if (!helper.isRunning()) return false;
            helper.paintAt(first); helper.paintLine(first, last);
            return true;
        };
        const int history = lifetime->undoStack()->index();
        QVERIFY2(begin(), errors.isEmpty() ? "CPU brush did not start" : qPrintable(errors.first().first().toString()));
        if (gpu) {
            const auto owner = image->gpuImage();
            const auto before = owner->operations().snapshot();
            QTRY_VERIFY(owner->operations().preview() != before);
            QCOMPARE(owner->operations().snapshot(), before);
            QCOMPARE(owner->operations().undoCount(), history);
        }
        helper.endPaint();
        QVERIFY(errors.isEmpty()); QVERIFY(!helper.isRunning());
        if (duplicate) {
            const auto source = words ? KisLayerUtils::findNodeByUuid(image->root(), backdropUuid) : node;
            QCOMPARE(resources->currentPreset()->settings()->sourceNode().data(), source.data());
            resources->currentPreset()->settings()->mouseReleaseEvent();
        }
        const auto edited = capture(); QVERIFY(edited != original);
        if (!gpu) reference = edited;
        else QVERIFY2(difference(edited, reference).isEmpty(), qPrintable(difference(edited, reference)));
        if (sampling || alpha) {
            QByteArray projection(inspected.width() * inspected.height() * cs->pixelSize(), Qt::Uninitialized);
            image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), inspected);
            if (!gpu) referenceProjection = projection;
            else QVERIFY2(pixelStepDifference(projection, referenceProjection, words ? 2 : 1).isEmpty(),
                          qPrintable(pixelStepDifference(projection, referenceProjection, words ? 2 : 1)));
            if (sampling) {
                QByteArray unchanged(backdropPixels.size(), Qt::Uninitialized);
                KisLayerUtils::findNodeByUuid(image->root(), backdropUuid)->paintDevice()->readBytes(
                    reinterpret_cast<quint8 *>(unchanged.data()), inspected);
                QCOMPARE(unchanged, backdropPixels);
            }
        }
        QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
        lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(capture(), original);
        if (gpu) {
            const auto owner = image->gpuImage();
            QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
            const auto before = owner->operations().snapshot();
            QVERIFY(begin());
            QTRY_VERIFY(owner->operations().preview() != before);
            image->requestStrokeCancellation();
            QVERIFY(!helper.isRunning());
            helper.endPaint();
            QCOMPARE(owner->operations().snapshot(), before);
            QCOMPARE(owner->operations().redoCount(), 1);
            QVERIFY(!lifetime->isModified());
        }
        lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(capture(), edited);
        if (gpu) {
            QCOMPARE(image->gpuImage()->operations().historyAction(history + 1), Gpu::HistoryAction::PaintStroke);
            QTemporaryDir directory; QVERIFY(directory.isValid());
            const auto path = directory.filePath(QStringLiteral("cpu-brush.kra"));
            QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false)); lifetime->waitForSavingToComplete();
            for (const bool cpuReload : {false, true}) {
                std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
                reopened->setFileBatchMode(true);
                QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent |
                    (cpuReload ? KisDocument::UseCpuEngine : KisDocument::OpenFlags{})), qPrintable(reopened->errorMessage()));
                reopened->image()->waitForDone();
                QCOMPARE(bool(reopened->image()->gpuImage()), !cpuReload);
                QCOMPARE(reopened->image()->colorSpace()->profile()->rawData(), cs->profile()->rawData());
                const auto layer = KisLayerUtils::findNodeByUuid(reopened->image()->root(), node->uuid());
                QVERIFY(layer); QCOMPARE(layer->name(), node->name());
                QByteArray bytes(edited.size(), Qt::Uninitialized);
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), inspected);
                QVERIFY2(difference(bytes, reference).isEmpty(), qPrintable(difference(bytes, reference)));
                if (alpha) {
                    QCOMPARE(layer->paintDevice()->defaultPixel(), node->paintDevice()->defaultPixel());
                    QCOMPARE(layer->parent()->uuid(), paintNode->uuid());
                    QCOMPARE(layer->metaObject()->className(), node->metaObject()->className());
                    if (words) QCOMPARE(qobject_cast<const KisFilterMask *>(layer.data())->filter()->toXML(),
                                        qobject_cast<KisFilterMask *>(node.data())->filter()->toXML());
                    if (words) {
                        const auto selection = reopened->image()->globalSelection(); QVERIFY(selection);
                        const auto expected = image->globalSelection(); QVERIFY(expected);
                        QByteArray savedSelection(inspected.width() * inspected.height(), Qt::Uninitialized);
                        QByteArray expectedSelection(savedSelection.size(), Qt::Uninitialized);
                        selection->projection()->readBytes(reinterpret_cast<quint8 *>(savedSelection.data()), inspected);
                        expected->projection()->readBytes(reinterpret_cast<quint8 *>(expectedSelection.data()), inspected);
                        QCOMPARE(savedSelection, expectedSelection);
                    }
                }
                if (sampling) {
                    const auto backdrop = KisLayerUtils::findNodeByUuid(reopened->image()->root(), backdropUuid);
                    QVERIFY(backdrop); QCOMPARE(backdrop->name(), QStringLiteral("Brush source"));
                    backdrop->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), inspected);
                    QCOMPARE(bytes, backdropPixels);
                }
                if (sampling || alpha) {
                    bytes.resize(inspected.width() * inspected.height() * cs->pixelSize());
                    reopened->image()->refreshGraphAsync(reopened->image()->root(), inspected, inspected);
                    reopened->image()->waitForDone();
                    reopened->image()->projection()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), inspected);
                    QVERIFY(pixelStepDifference(bytes, referenceProjection, words ? 2 : 1).isEmpty());
                }
            }
        }
        if (alpha) {
            QByteArray pixels(initial.size(), Qt::Uninitialized);
            paintNode->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), canvas);
            QCOMPARE(pixels, initial);
            if (words) {
                // Editing the global selection must reach pixels outside its
                // current coverage instead of clipping the stroke to itself.
                node = image->rootLayer()->selectionMask(); QVERIFY(node);
                window->viewManager()->nodeManager()->slotUiActivatedNode(node);
                resources->setEraserMode(false); resources->setFGColor(KoColor(Qt::white, cs));
                const auto before = capture();
                QVERIFY(begin()); helper.endPaint(); QVERIFY(errors.isEmpty());
                const auto after = capture(); QVERIFY(after != before);
                if (!gpu) referenceSelection = after;
                else QVERIFY2(difference(after, referenceSelection).isEmpty(), qPrintable(difference(after, referenceSelection)));
                lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(capture(), before);
                lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(capture(), after);
            }
        }
    }
}
