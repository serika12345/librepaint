/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentViewTest.h"
#include <KisGpuPaintDeviceTransfer_p.h>
#include <gpu/KisGpuOperationCapabilities.h>

void KisGpuDocumentViewTest::cpuBrushesKeepNativeEditing_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8-grid") << false;
    QTest::newRow("u16-selected-deform") << true;
}

void KisGpuDocumentViewTest::cpuBrushesKeepNativeEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    const TestUtil::PaintingEngineScope useGpu(true);
    const QString id = words ? QStringLiteral("deformbrush") : QStringLiteral("gridbrush");
    QCOMPARE(Gpu::brushExecutionLocation(id), Gpu::ExecutionLocation::Cpu);
    QCOMPARE(Gpu::brushExecutionLocation(QStringLiteral("paintbrush")), Gpu::ExecutionLocation::Gpu);
    QCOMPARE(Gpu::brushExecutionLocation(QStringLiteral("roundmarker")), Gpu::ExecutionLocation::Gpu);
    KisPaintOpPresetSP preset(new KisPaintOpPreset(QStringLiteral(KRITA_SOURCE_DIR "/plugins/paintops/defaultpresets/") + id + QStringLiteral(".kpp")));
    QVERIFY(preset->load(KisGlobalResourcesInterface::instance()));
    QVERIFY(preset->valid());
    preset->setName(QStringLiteral("CPU brush fixture ") + id);
    preset->setFilename(QStringLiteral("cpu-brush-fixture-") + id + QStringLiteral(".kpp"));
    if (words) {
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
    QByteArray reference;
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
        const auto node = image->root()->firstChild();
        if (words) {
            KisSelectionSP selection = new KisSelection;
            selection->pixelSelection()->setDefaultBounds(new KisDefaultBounds(image));
            selection->pixelSelection()->select(QRect(247, 4, 14, 24), 173);
            KisSelectionMaskSP mask = new KisSelectionMask(image);
            mask->setSelection(selection); mask->setActive(true); QVERIFY(image->addNode(mask, image->root()));
        }
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
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
        window->viewManager()->nodeManager()->slotNonUiActivatedNode(node);
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
            QByteArray bytes(inspected.width() * inspected.height() * cs->pixelSize(), Qt::Uninitialized);
            node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), inspected);
            return bytes;
        };
        const auto original = capture();
        const auto begin = [&] {
            KisPaintInformation first(QPointF(250.5, 12.5)), last(QPointF(260.5, 18.5));
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
        const auto edited = capture(); QVERIFY(edited != original);
        QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
        if (!gpu) reference = edited;
        else QVERIFY2(pixelStepDifference(edited, reference, words ? 2 : 1).isEmpty(),
                      qPrintable(pixelStepDifference(edited, reference, words ? 2 : 1)));
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
                QVERIFY(pixelStepDifference(bytes, reference, words ? 2 : 1).isEmpty());
            }
        }
    }
}
