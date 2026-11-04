/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentViewTest.h"
#include <QKeyEvent>
#include <QSlider>
#include <cstdlib>
#include <gpu/KisGpuOperationCapabilities.h>

void KisGpuDocumentViewTest::cpuSmartPatchKeepsNativeEditing_data()
{
    QTest::addColumn<bool>("boundary");
    QTest::newRow("u8-paint") << false;
    QTest::newRow("u16-selected-paint-with-mask") << true;
}

void KisGpuDocumentViewTest::cpuSmartPatchKeepsNativeEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, boundary);
    const TestUtil::PaintingEngineScope engineScope(false);
    const auto preset = m_preset;
    QCOMPARE(Gpu::toolExecutionLocation(Gpu::ToolOperation::SmartPatch), Gpu::ExecutionLocation::Cpu);
    const auto *cs = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect canvas(0, 0, 270, 48), inspected(-8, -8, 286, 64);
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("source.kra"));
    QUuid layerUuid, maskUuid;
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), canvas.width(), canvas.height(), cs, "CPU smart patch");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 213);
        layer->paintDevice()->fill(canvas, KoColor(QColor(77, 113, 157, 176), cs));
        layer->paintDevice()->fill({253, 13, 4, 4}, KoColor(QColor(219, 31, 53, 255), cs));
        layer->paintDevice()->fill({258, 31, 3, 4}, KoColor(QColor(31, 219, 53, 255), cs));
        QVERIFY(image->addNode(layer)); layerUuid = layer->uuid();
        if (boundary) {
            KisSelectionSP coverage = new KisSelection;
            const quint8 white = 255;
            coverage->pixelSelection()->setDefaultPixel(KoColor(&white, KoColorSpaceRegistry::instance()->alpha8()));
            coverage->pixelSelection()->select({251, 7, 11, 32}, 193);
            KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
            mask->setSelection(coverage); QVERIFY(image->addNode(mask, layer)); maskUuid = mask->uuid();
            KisSelectionSP selection = new KisSelection;
            selection->pixelSelection()->select({250, 5, 13, 38}, 173);
            selection->pixelSelection()->select({254, 14, 1, 1}, 0);
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
            QCOMPARE(layer->name(), QStringLiteral("Paint")); QCOMPARE(layer->opacity(), quint8(213));
            if (boundary) {
                QVERIFY(image->globalSelection());
                const auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid); QVERIFY(mask);
                QCOMPARE(mask->parent(), layer);
            }
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
            window->viewManager()->nodeManager()->slotNonUiActivatedNode(layer); QTRY_COMPARE(view->currentNode(), layer);
            const auto capture = [&]() -> QByteArray {
                image->waitForDone();
                QByteArray pixels(inspected.width() * inspected.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), inspected);
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), inspected);
                if (boundary) {
                    coverage.resize(2 * inspected.width() * inspected.height());
                    KisLayerUtils::findNodeByUuid(image->root(), maskUuid)->paintDevice()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), inspected);
                    image->globalSelection()->projection()->readBytes(reinterpret_cast<quint8 *>(coverage.data() + coverage.size() / 2), inspected);
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
            resources->setPaintOpPreset(preset); resources->setSize(8.75);
            const auto toolId = QStringLiteral("KritaShape/KisToolSmartPatch");
            KoToolManager::instance()->switchToolRequested(toolId);
            tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId)); QVERIFY(tool);
            const auto options = tool->optionWidgets(); QVERIFY(!options.isEmpty());
            auto *accuracy = options.first()->findChild<QSlider *>(QStringLiteral("accuracySlider")); QVERIFY(accuracy); accuracy->setValue(0);
            auto *radius = options.first()->findChild<QSpinBox *>(QStringLiteral("patchRadius")); QVERIFY(radius); radius->setValue(2);
            const auto gesture = [&](bool cancel) {
                const QPointF position = reopened ? QPointF(259, 33) : QPointF(255, 15);
                const auto point = view->canvasBase()->coordinatesConverter()->imageToDocument(position);
                QMouseEvent hover(QEvent::MouseMove, QPointF(100, 100), QPointF(100, 100), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                KoPointerEvent hovering(&hover, point); tool->mouseMoveEvent(&hovering);
                QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                KoPointerEvent first(&press, point); tool->beginPrimaryAction(&first);
                tool->continuePrimaryAction(&first);
                if (cancel) {
                    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); tool->keyPressEvent(&escape);
                } else {
                    std::srand(reopened ? 739 : 197);
                    if (boundary) tool->requestStrokeEnd();
                    else {
                        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                        KoPointerEvent last(&release, point); tool->endPrimaryAction(&last);
                    }
                }
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
                gesture(true);
                QCOMPARE(owner->operations().snapshot(), savedState); QCOMPARE(capture(), before);
                QCOMPARE(owner->operations().redoCount(), 1); QVERIFY(!owner->operations().isModified());
            }
            lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(capture(), edited);
            if (gpu) QCOMPARE(image->gpuImage()->operations().historyAction(history + 1), Gpu::HistoryAction::SmartPatch);
            QCOMPARE(lifetime->undoStack()->undoText(), kundo2_i18n("Smart Patch").toString());
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
    QCOMPARE(layer->name(), QStringLiteral("Paint")); QCOMPARE(layer->opacity(), quint8(213));
    QByteArray pixels(inspected.width() * inspected.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), inspected);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), inspected);
    if (boundary) {
        auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid); QVERIFY(mask); QCOMPARE(mask->parent(), layer);
        QVERIFY(image->globalSelection()); coverage.resize(2 * inspected.width() * inspected.height());
        mask->paintDevice()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), inspected);
        image->globalSelection()->projection()->readBytes(reinterpret_cast<quint8 *>(coverage.data() + coverage.size() / 2), inspected);
    }
    const auto mismatch = difference(pixels + projection + coverage, reference.last());
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
}
