/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuMultibrushEditingTest.h"
#include <KoTestConfig.h>
#include <QApplication>
#include <KisGpuPixelTolerance.h>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoPointerEvent.h>
#include <KoSnapGuide.h>
#include <KoToolManager.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <brushengine/kis_paintop_preset.h>
#include <canvas/kis_canvas2.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <input/ui/kis_tool_proxy.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_layer_utils.h>
#include <kis_node.h>
#include <nodes/kis_node_manager.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_selection_mask.h>
#include <kis_slider_spin_box.h>
#include <kis_tool.h>
#include <kis_transparency_mask.h>
#include <cstring>

void MultibrushEditingTest::addData()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("smoothing");
    QTest::addColumn<bool>("maskTarget");
    QTest::newRow("normal-u8") << QStringLiteral("b)_Basic-5_Size_default.kpp") << false << 0 << 0 << false;
    QTest::newRow("group-mask-u16") << QStringLiteral("a)_Eraser_Circle.kpp") << true << 1 << 0 << true;
}

void DynamicBrushEditingTest::addData()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("smoothing");
    QTest::addColumn<bool>("maskTarget");
    QTest::newRow("normal-u8") << QStringLiteral("b)_Basic-5_Size_default.kpp") << false << 0 << 1 << false;
    QTest::newRow("mass-drag-u16") << QStringLiteral("a)_Eraser_Small.kpp") << true << 5 << 1 << false;
}

static void runFreehandEditing(std::shared_ptr<LibrePaint::Image::Gpu::Execution> &sharedExecution, bool dynamic)
{
    // Consumers: ordinary Multibrush and Dynamic Brush settings and input.
    // Each gesture keeps its pixels, target and one transaction across KRA reload.
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(QString, presetName); QFETCH(bool, words); QFETCH(int, mode);
    QFETCH(int, smoothing); QFETCH(bool, maskTarget);
    const auto oldEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (oldEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", oldEngine);
    });
    const QString toolId = dynamic ? QStringLiteral("KritaShape/KisToolDyna") : QStringLiteral("KritaShape/KisToolMultiBrush");
    const QString family = dynamic ? QStringLiteral("dynamic-brush") : QStringLiteral("multibrush");
    KConfigGroup configuration = KSharedConfig::openConfig()->group(toolId);
    const auto entries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = entries.cbegin(); i != entries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    const auto bundle = KisResourceStorageSP::create(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle"));
    KisResourceModel brushes(ResourceType::Brushes);
    if (brushes.rowCount() == 0)
        QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
    KisResourceModel patterns(ResourceType::Patterns);
    if (patterns.rowCount() == 0)
        QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    const auto preset = bundle->resource(QStringLiteral("paintoppresets/") + presetName).dynamicCast<KisPaintOpPreset>();
    QVERIFY(preset);
    preset->setFilename(family + QLatin1Char('-') + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
    KisResourceModel presets(ResourceType::PaintOpPresets); QVERIFY(presets.addResource(preset));
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect canvas(0, 0, 64, 64), inspected(-8, -8, 80, 80);
    const auto stateDifference = [&](const QByteArray &actual, const QByteArray &expected) {
        const qsizetype colorBytes = 2 * inspected.width() * inspected.height() * cs->pixelSize();
        const auto colors = pixelStepDifference(actual.left(colorBytes), expected.left(colorBytes), words ? 2 : 1);
        return colors.isEmpty() ? pixelStepDifference(actual.mid(colorBytes), expected.mid(colorBytes), 1, 1) : colors;
    };
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("multibrush-source.kra"));
    QUuid paintUuid, targetUuid, maskUuid;
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), 64, 64, cs, "Multibrush editing");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        KoColor initial(QColor(77, 113, 157, 176), cs);
        if (words) { const quint16 bgra[]{11153, 3983, 22861, 45172}; std::memcpy(initial.data(), bgra, sizeof(bgra)); }
        layer->paintDevice()->fill(canvas, initial);
        KisNodeSP target = layer;
        if (maskTarget) {
            KisGroupLayerSP group = new KisGroupLayer(image, QStringLiteral("Masked group"), 255);
            QVERIFY(image->addNode(group)); QVERIFY(image->addNode(layer, group)); target = group;
            KisSelectionSP coverage = new KisSelection;
            coverage->pixelSelection()->select(canvas, 177);
            KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
            mask->setSelection(coverage); QVERIFY(image->addNode(mask, group)); maskUuid = mask->uuid();
        } else QVERIFY(image->addNode(layer));
        paintUuid = layer->uuid(); targetUuid = target->uuid();
        KisSelectionSP coverage = new KisSelection;
        coverage->pixelSelection()->select({6, 6, 52, 52}, 211);
        coverage->pixelSelection()->select({29, 29, 6, 6}, 0);
        KisSelectionMaskSP selection = new KisSelectionMask(image);
        selection->setSelection(coverage); selection->setActive(true); QVERIFY(image->addNode(selection, image->root()));
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
        QVERIFY(document->saveAs(baseline, KisDocument::nativeFormatMimeType(), false)); document->waitForSavingToComplete();
    }
    QList<QByteArray> reference;
    for (const bool gpu : {false, true}) {
        if (gpu && !sharedExecution) {
            QString diagnostic; sharedExecution = KisPart::instance()->gpuExecution(diagnostic);
            QVERIFY2(sharedExecution, qPrintable(diagnostic));
        }
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        const auto saved = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("native.kra"));
        int stage = 0;
        for (const bool reopened : {false, true}) {
            std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
            document->setFileBatchMode(true); document->setAutoSaveActive(false);
            QVERIFY2(document->openPath(reopened ? saved : baseline, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
            auto image = KisImageSP(document->image()); image->waitForDone(); QCOMPARE(bool(image->gpuImage()), gpu);
            auto layer = KisLayerUtils::findNodeByUuid(image->root(), paintUuid); QVERIFY(layer);
            auto target = KisLayerUtils::findNodeByUuid(image->root(), targetUuid); QVERIFY(target);
            auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid);
            if (maskTarget) { QVERIFY(mask); QCOMPARE(mask->parent(), target); }
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
            window->viewManager()->nodeManager()->slotNonUiActivatedNode(maskTarget ? mask : layer);
            QTRY_COMPARE(view->currentNode(), maskTarget ? mask : layer);
            const auto capture = [&]() -> QByteArray {
                image->waitForDone();
                QByteArray pixels(inspected.width() * inspected.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), inspected);
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), inspected);
                if (maskTarget) {
                    coverage.resize(inspected.width() * inspected.height());
                    qobject_cast<KisTransparencyMask *>(mask.data())->selection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), inspected);
                }
                return pixels + projection + coverage;
            };
            const auto check = [&]() -> bool {
                const auto state = capture(); const int at = stage++;
                const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                if (!results.isEmpty()) {
                    if (!QDir().mkpath(results)) return false;
                    QFile file(QDir(results).filePath(QStringLiteral("%1-%2-%3-%4.bin").arg(family, QString::fromLatin1(QTest::currentDataTag()), gpu ? "gpu" : "native").arg(at)));
                    if (!file.open(QIODevice::WriteOnly) || file.write(state) != state.size()) return false;
                }
                if (!gpu) { reference.append(state); return true; }
                if (stateDifference(state, reference.value(at)).isEmpty()) return true;
                const auto expected = reference.value(at);
                for (int i = 0; i < qMin(state.size(), expected.size()); ++i) if (state[i] != expected[i]) {
                    qWarning() << family << "state" << at << "byte" << i << "GPU" << quint8(state[i]) << "native" << quint8(expected[i]); break;
                }
                return false;
            };
            QVERIFY2(check(), "Initial/reopened Native and GPU state differs");
            auto *resources = window->viewManager()->canvasResourceProvider();
            resources->setPaintOpPreset(preset); resources->setSize(presetName.contains("Small") ? 13.75 : 4.75);
            resources->setOpacity(.7); resources->setEraserMode(presetName.startsWith("a)"));
            resources->setCurrentCompositeOp(presetName.startsWith("a)") ? QStringLiteral("erase") : QStringLiteral("normal"));
            resources->setFGColor(KoColor(reopened ? QColor(213, 49, 107) : QColor(17, 129, 241), cs));
            resources->setGlobalAlphaLock(false); resources->setMirrorHorizontal(false); resources->setMirrorVertical(false);
            KoToolManager::instance()->switchToolRequested(toolId);
            tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId)); QVERIFY(tool);
            const auto panels = tool->optionWidgets();
            if (!dynamic) {
                QVERIFY(tool->setProperty("smoothingType", smoothing));
                QVERIFY(tool->setProperty("smoothnessQualityMin", 6)); QVERIFY(tool->setProperty("smoothnessQualityMax", 6));
                QVERIFY(tool->setProperty("useDelayDistance", false)); QVERIFY(tool->setProperty("finishStabilizedCurve", true));
            }
            if (dynamic) {
                constexpr qreal massAndDrag[][2]{{.01, .98}, {.45, .25}, {0, 0}, {0, 1}, {1, 0}, {1, 1}};
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetMass", Qt::DirectConnection, Q_ARG(qreal, massAndDrag[mode][0])));
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetDrag", Qt::DirectConnection, Q_ARG(qreal, massAndDrag[mode][1])));
            } else {
                QVERIFY(QMetaObject::invokeMethod(tool, "resetAxes", Qt::DirectConnection));
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetHandsCount", Qt::DirectConnection, Q_ARG(int, 3)));
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetAxesAngle", Qt::DirectConnection, Q_ARG(qreal, 27)));
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetMirrorHorizontally", Qt::DirectConnection, Q_ARG(bool, true)));
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetMirrorVertically", Qt::DirectConnection, Q_ARG(bool, true)));
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetTransformMode", Qt::DirectConnection, Q_ARG(int, mode)));
                if (mode == 5) {
                    KisSliderSpinBox *x = nullptr, *y = nullptr;
                    for (const auto &panel : panels) {
                        if (!panel) continue;
                        if (auto *value = panel->findChild<KisSliderSpinBox *>(QStringLiteral("intervalXSpinBox"))) x = value;
                        if (auto *value = panel->findChild<KisSliderSpinBox *>(QStringLiteral("intervalYSpinBox"))) y = value;
                    }
                    QVERIFY(x && y); x->setValue(16); y->setValue(16);
                    QVERIFY(QMetaObject::invokeMethod(tool, "slotSetIntervals", Qt::DirectConnection));
                }
            }
            const auto dynamicPoint = [](QPointF start, int step) {
                const bool curve = QByteArray(QTest::currentDataTag()).startsWith("curve-");
                const qreal y = curve ? (step <= 4 ? step : step <= 8 ? 4 + 2 * (step - 4) : 12 - 2.5 * (step - 8)) : step;
                return start + QPointF(3 * step, y);
            };
            const auto gesture = [&](bool cancel) {
                const QPointF start = reopened ? QPointF(17.25, 20.75) : QPointF(13.25, 23.75);
                const bool tap = QByteArray(QTest::currentDataTag()).contains("tap-");
                QList<QPointF> points = tap ? QList<QPointF>{} : !dynamic && smoothing ? QList<QPointF>{start + QPointF(3, 1.5), start + QPointF(6, 3), start + QPointF(9, 4.5)} :
                    QList<QPointF>{start + QPointF(4.5, 1), start + QPointF(8, 4.75), start + QPointF(12, 2)};
                if (dynamic && !tap) {
                    points.clear();
                    for (int step = 1; step <= 12; ++step) points.append(dynamicPoint(start, step));
                }
                QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                KoPointerEvent first(&press, view->canvasBase()->coordinatesConverter()->imageToDocument(start)); tool->beginPrimaryAction(&first);
                for (const auto &position : points) {
                    QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                    KoPointerEvent next(&move, view->canvasBase()->coordinatesConverter()->imageToDocument(position)); tool->continuePrimaryAction(&next);
                }
                if (cancel) { QKeyEvent key(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); view->canvasBase()->toolProxy()->keyPressEvent(&key); }
                QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                KoPointerEvent last(&release, view->canvasBase()->coordinatesConverter()->imageToDocument(points.isEmpty() ? start : points.last())); tool->endPrimaryAction(&last);
                image->waitForDone();
            };
            if (!dynamic && mode == 4) {
                QVERIFY(QMetaObject::invokeMethod(tool, "slotAddSubbrushesMode", Qt::DirectConnection, Q_ARG(bool, true)));
                QMouseEvent event(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                KoPointerEvent added(&event, view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(47, 32))); tool->beginPrimaryAction(&added);
                tool->endPrimaryAction(&added);
                QVERIFY(QMetaObject::invokeMethod(tool, "slotAddSubbrushesMode", Qt::DirectConnection, Q_ARG(bool, false)));
            }
            const auto before = capture(); const int history = lifetime->undoStack()->index();
            gesture(true); QCOMPARE(capture(), before); QCOMPARE(lifetime->undoStack()->index(), history);
            gesture(false); QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
            const auto edited = capture(); QVERIFY(edited != before); QVERIFY2(check(), "Native/GPU freehand tool pixels differ");
            if (gpu && !maskTarget) {
                const auto owner = image->gpuImage();
                const auto operations = owner->operations().snapshot()->layers.first().operations;
                const auto operation = std::get<Gpu::PaintSnapshot>(operations.last());
                if (dynamic) {
                    QVERIFY(operation->brush.transforms.isEmpty());
                    if (!QByteArray(QTest::currentDataTag()).startsWith("tap-")) {
                        const auto samples = operation->inputSamples();
                        QCOMPARE(samples.size(), qsizetype(13));
                        const QPointF start = reopened ? QPointF(17.25, 20.75) : QPointF(13.25, 23.75);
                        bool filtered = false;
                        for (int step = 1; step <= 12; ++step)
                            filtered |= samples[step].position != dynamicPoint(start, step);
                        QVERIFY(filtered);
                    }
                } else QCOMPARE(operation->brush.transforms.size(), qsizetype(mode == 0 ? 3 : mode == 1 ? 4 : mode == 3 ? 12 : mode == 4 ? 2 : 16));
            }
            resources->setFGColor(KoColor(Qt::yellow, cs)); resources->setSize(7.25);
            if (dynamic) {
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetMass", Qt::DirectConnection, Q_ARG(qreal, .8)));
                QVERIFY(QMetaObject::invokeMethod(tool, "slotSetDrag", Qt::DirectConnection, Q_ARG(qreal, .1)));
            } else QVERIFY(QMetaObject::invokeMethod(tool, "slotSetHandsCount", Qt::DirectConnection, Q_ARG(int, 1)));
            lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(capture(), before); QVERIFY(check());
            lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(capture(), edited); QVERIFY(check());
            QVERIFY2(lifetime->saveAs(saved, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
            lifetime->waitForSavingToComplete();
        }
    }
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> native(KisPart::instance()->createDocument()); native->setFileBatchMode(true); native->setAutoSaveActive(false);
    QVERIFY(native->openPath(directory.filePath(QStringLiteral("gpu.kra")), KisDocument::DontAddToRecent));
    auto image = KisImageSP(native->image()); image->waitForDone();
    auto layer = KisLayerUtils::findNodeByUuid(image->root(), paintUuid); QVERIFY(layer);
    QByteArray pixels(inspected.width() * inspected.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), inspected);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), inspected);
    if (maskTarget) {
        auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid); QVERIFY(mask);
        coverage.resize(inspected.width() * inspected.height());
        qobject_cast<KisTransparencyMask *>(mask.data())->selection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), inspected);
    }
    const auto difference = stateDifference(pixels + projection + coverage, reference.last());
    QVERIFY2(difference.isEmpty(), qPrintable(difference));
}

void MultibrushEditingTest::run(std::shared_ptr<LibrePaint::Image::Gpu::Execution> &sharedExecution)
{
    runFreehandEditing(sharedExecution, false);
}

void DynamicBrushEditingTest::run(std::shared_ptr<LibrePaint::Image::Gpu::Execution> &sharedExecution)
{
    runFreehandEditing(sharedExecution, true);
}

