/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentViewTest.h"
#include <QKeyEvent>
#include <kis_tool_rectangle_base.h>
#include <gpu/KisGpuOperationCapabilities.h>

void KisGpuDocumentViewTest::cpuFiguresKeepNativeEditing_data()
{
    QTest::addColumn<bool>("boundary");
    QTest::newRow("u8-grid-ellipse") << false;
    QTest::newRow("u16-selected-mask-rounded-deform") << true;
}

void KisGpuDocumentViewTest::cpuFiguresKeepNativeEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    using namespace KisFigurePaintingOptions;
    QFETCH(bool, boundary);
    const TestUtil::PaintingEngineScope engineScope(false);
    const auto id = boundary ? QStringLiteral("deformbrush") : QStringLiteral("gridbrush");
    QCOMPARE(Gpu::brushExecutionLocation(id), Gpu::ExecutionLocation::Cpu);
    KisPaintOpPresetSP preset(new KisPaintOpPreset(QStringLiteral(KRITA_SOURCE_DIR "/plugins/paintops/defaultpresets/") + id + QStringLiteral(".kpp")));
    QVERIFY(preset->load(KisGlobalResourcesInterface::instance())); QVERIFY(preset->valid());
    preset->setName(QStringLiteral("CPU figure ") + QString::fromLatin1(QTest::currentDataTag()));
    preset->setFilename(QStringLiteral("cpu-figure-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
    preset->settings()->setProperty(AIRBRUSH_ENABLED, false);
    preset->settings()->setProperty("Deform/useMovementPaint", false);
    preset->settings()->setProperty("Grid/randomBorder", false);
    preset->settings()->setProperty("ColorOption/useRandomOpacity", false);
    preset->settings()->setProperty("ColorOption/useRandomHSV", false);
    QImage preview(16, 16, QImage::Format_ARGB32); preview.fill(Qt::black); preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets); QVERIFY(presets.addResource(preset));
    const auto *cs = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect canvas(0, 0, 270, 48), inspected(-8, -8, 286, 64);
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("source.kra"));
    QUuid layerUuid, maskUuid;
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), canvas.width(), canvas.height(), cs, "CPU figures");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        layer->paintDevice()->fill(canvas, KoColor(QColor(77, 113, 157, 176), cs));
        QVERIFY(image->addNode(layer)); layerUuid = layer->uuid();
        if (boundary) {
            KisSelectionSP coverage = new KisSelection;
            const quint8 white = 255;
            coverage->pixelSelection()->setDefaultPixel(KoColor(&white, KoColorSpaceRegistry::instance()->alpha8()));
            for (int y = canvas.top(); y <= canvas.bottom(); ++y)
                for (int x = canvas.left(); x <= canvas.right(); ++x)
                    coverage->pixelSelection()->select(QRect(x, y, 1, 1), (x * 17 + y * 43) % 256);
            KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
            mask->setSelection(coverage); QVERIFY(image->addNode(mask, layer)); maskUuid = mask->uuid();
            KisSelectionSP selection = new KisSelection;
            selection->pixelSelection()->select({246, 5, 17, 35}, 173);
            selection->pixelSelection()->select({253, 20, 4, 4}, 0);
            KisSelectionMaskSP selected = new KisSelectionMask(image);
            selected->setSelection(selection); selected->setActive(true); QVERIFY(image->addNode(selected, image->root()));
        }
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
        QVERIFY(document->saveAs(baseline, KisDocument::nativeFormatMimeType(), false)); document->waitForSavingToComplete();
    }
    const qsizetype colorBytes = 2 * inspected.width() * inspected.height() * cs->pixelSize();
    const auto difference = [&](const QByteArray &actual, const QByteArray &expected) {
        auto result = pixelStepDifference(actual.left(colorBytes), expected.left(colorBytes), boundary ? 2 : 1);
        if (result.isEmpty()) result = pixelStepDifference(actual.mid(colorBytes), expected.mid(colorBytes), 1, 1);
        return result;
    };
    QList<QByteArray> reference;
    for (const bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        if (gpu && !m_freehandExecution) { QString diagnostic; m_freehandExecution = KisPart::instance()->gpuExecution(diagnostic); QVERIFY2(m_freehandExecution, qPrintable(diagnostic)); }
        const auto saved = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("cpu.kra"));
        int stage = 0;
        for (const bool reopened : {false, true}) {
            std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
            document->setFileBatchMode(true); document->setAutoSaveActive(false);
            QVERIFY2(document->openPath(reopened ? saved : baseline, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
            auto image = KisImageSP(document->image()); image->waitForDone(); QCOMPARE(bool(image->gpuImage()), gpu);
            const auto layer = KisLayerUtils::findNodeByUuid(image->root(), layerUuid); QVERIFY(layer);
            const auto target = boundary ? KisLayerUtils::findNodeByUuid(image->root(), maskUuid) : layer; QVERIFY(target);
            QCOMPARE(layer->name(), QStringLiteral("Paint"));
            if (boundary) { QCOMPARE(target->parent(), layer); QVERIFY(image->globalSelection()); }
            std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
            auto *view = window->newView(document.get()); QVERIFY(view);
            QPointer<KisDocument> lifetime(document.release()); QPointer<KisTool> tool;
            const auto cleanup = qScopeGuard([&] {
                if (tool) tool->requestStrokeCancellation();
                if (lifetime) { lifetime->setModified(false); window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger(); }
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            });
            window->viewManager()->setCurrentView(view); window->resize(800, 600); window->show();
            QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
            view->canvasBase()->snapGuide()->enableSnapping(false);
            window->viewManager()->nodeManager()->slotNonUiActivatedNode(target); QTRY_COMPARE(view->currentNode(), target);
            const auto capture = [&]() -> QByteArray {
                image->waitForDone();
                QByteArray pixels(inspected.width() * inspected.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), inspected);
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), inspected);
                if (boundary) {
                    coverage.resize(inspected.width() * inspected.height());
                    target->paintDevice()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), inspected);
                    const auto oldSize = coverage.size(); coverage.resize(oldSize * 2);
                    image->globalSelection()->projection()->readBytes(reinterpret_cast<quint8 *>(coverage.data() + oldSize), inspected);
                }
                return pixels + projection + coverage;
            };
            const auto check = [&] {
                const auto bytes = capture();
                if (!gpu) { reference.append(bytes); ++stage; return QString(); }
                return difference(bytes, reference.value(stage++));
            };
            auto mismatch = check(); QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
            auto *resources = window->viewManager()->canvasResourceProvider();
            resources->setPaintOpPreset(preset); resources->setSize(8.75); resources->setOpacity(.7);
            resources->setFGColor(KoColor(reopened ? QColor(213, 49, 107) : QColor(17, 129, 241), cs));
            resources->setBGColor(KoColor(QColor(221, 199, 57), cs));
            resources->setCurrentCompositeOp(QStringLiteral("normal")); resources->setEraserMode(false);
            resources->setGlobalAlphaLock(false); resources->setMirrorHorizontal(false); resources->setMirrorVertical(false);
            const auto toolId = reopened ? QStringLiteral("KritaShape/KisToolLine") : boundary ?
                QStringLiteral("KritaShape/KisToolRectangle") : QStringLiteral("KritaShape/KisToolEllipse");
            KoToolManager::instance()->switchToolRequested(toolId);
            tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId)); QVERIFY(tool);
            const auto panels = tool->optionWidgets();
            if (!reopened) {
                QComboBox *outline = nullptr, *fill = nullptr;
                for (const auto &panel : panels) if (panel) {
                    if (auto *box = panel->findChild<QComboBox *>(QStringLiteral("cmbOutline"))) outline = box;
                    if (auto *box = panel->findChild<QComboBox *>(QStringLiteral("cmbFill"))) fill = box;
                }
                QVERIFY(outline && fill);
                outline->setCurrentIndex(boundary ? StrokeStyleBackground : StrokeStyleForeground);
                fill->setCurrentIndex(FillStyleForegroundColor);
                if (boundary) qobject_cast<KisToolRectangleBase *>(tool.data())->roundCornersChanged(4, 6);
            }
            const auto gesture = [&](bool cancel) {
                const QPointF start(248.25, 8.75), end(264.5, 36.25);
                QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                KoPointerEvent first(&press, view->canvasBase()->coordinatesConverter()->imageToDocument(start)); tool->beginPrimaryAction(&first);
                QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                KoPointerEvent next(&move, view->canvasBase()->coordinatesConverter()->imageToDocument(end)); tool->continuePrimaryAction(&next);
                if (cancel) tool->requestStrokeCancellation();
                else tool->requestStrokeEnd();
                QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                KoPointerEvent last(&release, view->canvasBase()->coordinatesConverter()->imageToDocument(end)); tool->endPrimaryAction(&last);
                image->waitForDone();
            };
            const auto before = capture(); const int history = lifetime->undoStack()->index();
            gesture(true); QCOMPARE(capture(), before); QCOMPARE(lifetime->undoStack()->index(), history);
            gesture(false); QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
            const auto edited = capture(); QVERIFY(edited != before);
            mismatch = check(); QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
            lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(capture(), before);
            if (gpu) {
                const auto owner = image->gpuImage();
                QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
                const auto savedState = owner->operations().snapshot();
                {
                    KisFigurePaintingStroke cancelled(kundo2_i18n("Cancelled figure"), image, target,
                        view->canvasBase()->resourceManager(), StrokeStyleForeground, FillStyleNone);
                    cancelled.paintRect(QRectF(248, 9, 17, 25));
                    image->requestStrokeCancellation();
                }
                QCOMPARE(owner->operations().snapshot(), savedState); QCOMPARE(capture(), before);
                QCOMPARE(owner->operations().redoCount(), 1); QVERIFY(!owner->operations().isModified());
            }
            lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(capture(), edited);
            // The same native transaction also serves path-tool and public API calls.
            {
                KisFigurePaintingStroke path(kundo2_i18n("CPU path"), image, target,
                    view->canvasBase()->resourceManager(), StrokeStyleForeground, FillStyleNone);
                path.setFGColorOverride(KoColor(Qt::white, cs));
                QPainterPath outline; outline.moveTo(249.25, 14.5);
                outline.cubicTo(251.5, 31.75, 260.5, 5.75, 264.25, 30.25);
                path.paintPainterPath(outline);
            }
            QTRY_COMPARE(lifetime->undoStack()->index(), history + 2);
            mismatch = check(); QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
            QVERIFY2(lifetime->saveAs(saved, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
            lifetime->waitForSavingToComplete();
        }
    }
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
    native->setFileBatchMode(true); native->setAutoSaveActive(false);
    QVERIFY(native->openPath(directory.filePath(QStringLiteral("gpu.kra")), KisDocument::DontAddToRecent));
    auto image = KisImageSP(native->image()); image->waitForDone();
    auto layer = KisLayerUtils::findNodeByUuid(image->root(), layerUuid); QVERIFY(layer);
    QByteArray pixels(inspected.width() * inspected.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), inspected);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), inspected);
    if (boundary) {
        auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid); QVERIFY(mask); QCOMPARE(mask->parent(), layer);
        coverage.resize(2 * inspected.width() * inspected.height());
        mask->paintDevice()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), inspected);
        image->globalSelection()->projection()->readBytes(reinterpret_cast<quint8 *>(coverage.data() + coverage.size() / 2), inspected);
    }
    const auto mismatch = difference(pixels + projection + coverage, reference.last());
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
}
