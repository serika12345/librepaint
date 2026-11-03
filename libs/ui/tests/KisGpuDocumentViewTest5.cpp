/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDocumentViewTest.h"

void KisGpuDocumentViewTest::stabilizerPrimitivesRetainIndependentLineStarts()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const QSize size(64, 64);
    auto owner = paintImageWithEncoding(execution, size);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Stabilizer primitives")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoColor color(QColor(17, 129, 241), cs);
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setFGColor(color);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setEraserMode(false);
    resources->setGlobalAlphaLock(false);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    class PrimitiveProbe : public KisToolFreehandHelper {
    public:
        using KisToolFreehandHelper::KisToolFreehandHelper;
        using KisToolFreehandHelper::initPaintImpl;
        using KisToolFreehandHelper::paintAt;
        using KisToolFreehandHelper::paintLine;
        int elapsedStrokeTime() const override { return 0; }
    };
    view->canvasBase()->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
    KisPaintingInformationBuilder builder;
    PrimitiveProbe helper(&builder, view->canvasBase()->resourceManager(), kundo2_i18n("Stabilizer primitives"));
    helper.smoothingOptions()->setSmoothingType(KisSmoothingOptions::STABILIZER);
    helper.smoothingOptions()->setUseDelayDistance(false);
    helper.smoothingOptions()->setFinishStabilizedCurve(false);
    KisConfig cfg(false);
    const bool delayed = cfg.stabilizerDelayedPaint();
    cfg.setStabilizerDelayedPaint(false);
    const auto restore = qScopeGuard([&] { KisConfig(false).setStabilizerDelayedPaint(delayed); });
    const KisPaintInformation first(QPointF(16, 16)), next(QPointF(24, 24)), tap(QPointF(48, 48));
    helper.initPaintImpl(0, first, view->canvasBase()->resourceManager(), lifetime->image(),
                         lifetime->image()->root()->firstChild(), nullptr);
    QVERIFY(helper.isRunning());
    // Releasing before delayed painting starts issues the current tap first,
    // followed by the queued line from its own earlier start position.
    helper.paintAt(tap);
    helper.paintLine(first, next);
    helper.endPaint();
    QVERIFY(!helper.isRunning());
    KisImageSP nativeImage = new KisImage(nullptr, size.width(), size.height(), cs, QStringLiteral("Native primitives"));
    KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Reference"), 255);
    nativeImage->addNode(nativeLayer, nativeImage->root());
    KisPainter painter(nativeLayer->paintDevice());
    painter.setPaintColor(color);
    painter.setPaintOpPreset(resources->currentPreset(), nativeLayer, nativeImage);
    KisDistanceInformation distance;
    painter.paintAt(tap, &distance);
    painter.paintLine(first, next, &distance);
    QByteArray expected(size.width() * size.height() * 4, Qt::Uninitialized);
    nativeLayer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(expected.data()), QRect(QPoint(), size));
    exchangeRgbaBgra(expected, 4);
    const auto actual = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(QPoint(), size), expected.size()).bytes;
    const auto output = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!output.isEmpty()) for (const auto &entry : {qMakePair(QStringLiteral("native"), expected), qMakePair(QStringLiteral("gpu"), actual)}) {
        QFile file(QDir(output).filePath(QStringLiteral("stabilizer-independent-line-%1.rgba").arg(entry.first)));
        QVERIFY(QDir().mkpath(output));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(entry.second), entry.second.size());
    }
    QVERIFY2(actual == expected, "A stabilizer tap must retain the independent start of its queued line");
}

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data()
{
    QTest::addColumn<int>("smoothingType");
    QTest::addColumn<bool>("idlePause");
    QTest::addColumn<bool>("cancelAtIdle");
    QTest::addColumn<bool>("exhaustInput");
    QTest::addColumn<bool>("closeAfterError");
    QTest::addColumn<bool>("hideDuringStroke");
    QTest::addColumn<bool>("selectDuringStroke");
    QTest::addColumn<bool>("alphaLock");
    QTest::addColumn<bool>("layerAlphaLock");
    QTest::addColumn<quint8>("mirrorMode");
    QTest::newRow("paint-and-cancel") << 0 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("input-limit") << 0 << false << false << true << false << false << false << false << false << quint8(0);
    QTest::newRow("close-on-input-limit") << 0 << false << false << true << true << false << false << false << false << quint8(0);
    QTest::newRow("hide-during-stroke") << 0 << false << false << false << false << true << false << false << false << quint8(0);
    QTest::newRow("select-during-stroke") << 0 << false << false << false << false << false << true << false << false << quint8(0);
    QTest::newRow("alpha-lock") << 0 << false << false << false << false << false << false << true << false << quint8(0);
    QTest::newRow("layer-alpha-lock") << 0 << false << false << false << false << false << false << true << true << quint8(0);
    QTest::newRow("horizontal-symmetry") << 0 << false << false << false << false << false << false << false << false << quint8(1);
    QTest::newRow("vertical-symmetry") << 0 << false << false << false << false << false << false << false << false << quint8(2);
    QTest::newRow("both-axes-symmetry") << 0 << false << false << false << false << false << false << false << false << quint8(3);
    QTest::newRow("pixel-perfect") << 4 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("basic-smoothing") << 1 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("basic-smoothing-idle") << 1 << true << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("basic-smoothing-pending-cancel") << 1 << false << true << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing") << 2 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-idle") << 2 << true << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-pending-cancel") << 2 << false << true << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-input-limit") << 2 << false << false << true << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-close-on-limit") << 2 << false << false << true << true << false << false << false << false << quint8(0);
}

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled()
{
    QFETCH(int, smoothingType);
    QFETCH(bool, idlePause);
    QFETCH(bool, cancelAtIdle);
    QFETCH(bool, exhaustInput);
    QFETCH(bool, closeAfterError);
    QFETCH(bool, hideDuringStroke);
    QFETCH(bool, selectDuringStroke);
    QFETCH(bool, alphaLock);
    QFETCH(bool, layerAlphaLock);
    QFETCH(quint8, mirrorMode);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::Limits limits;
    if (exhaustInput) limits.samplesPerStroke = 4;
    auto owner = paintImageWithEncoding(execution, QSize(64, 64), limits);
    if (selectDuringStroke) QVERIFY(owner->addLayer(QStringLiteral("Paint target")));
    const auto paintTarget = owner->operations().activeLayer();
    if (alphaLock) QCOMPARE(owner->writeRgbaPixels(paintTarget, QRect(40, 40, 1, 1),
                                                QByteArray::fromHex("dd112c80")), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU freehand input")));
    if (layerAlphaLock) qobject_cast<KisPaintLayer*>(document->image()->root()->firstChild().data())->setAlphaLocked(true);
    if (mirrorMode) {
        KisMirrorAxisConfig axes;
        axes.setAxisPosition(QPointF(32, 32));
        axes.setMirrorHorizontal(mirrorMode & 1);
        axes.setMirrorVertical(mirrorMode & 2);
        document->setMirrorAxisConfig(axes);
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed && native) {
            if (documentLifetime) documentLifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native.data(), Qt::LeftButton);
        }
        if (documentLifetime) {
            documentLifetime->setModified(false);
            auto *close = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
            if (close) close->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->statusBar()->show();
    window->activateWindow();
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    QVERIFY(profiles->currentProfile());
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setGlobalAlphaLock(alphaLock && !layerAlphaLock);
    resources->setMirrorHorizontal(mirrorMode & 1);
    resources->setMirrorVertical(mirrorMode & 2);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QVERIFY(brushTool->setProperty("smoothingType", smoothingType));
    if (smoothingType == 2) {
        QVERIFY(brushTool->setProperty("smoothnessQualityMin", 12));
        QVERIFY(brushTool->setProperty("smoothnessQualityMax", 24));
        QVERIFY(brushTool->setProperty("smoothnessFactor", 0.25));
        QVERIFY(brushTool->setProperty("smoothPressure", true));
        QVERIFY(brushTool->setProperty("useScalableDistance", false));
    }
    QSignalSpy errors(brushTool, &KoToolBase::statusTextChanged);
    if (closeAfterError) {
        connect(brushTool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
            if (message.isEmpty()) return;
            documentLifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
    }
    view->canvasBase()->canvasWidget()->setFocus();
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= 64 && view->canvasBase()->canvasWidget()->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto imageSize = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().snapshot());
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
    QVERIFY(native);
    const auto *converter = view->canvasBase()->coordinatesConverter();
    const auto enterAt = [&](QPointF imagePoint) {
        QTest::mouseMove(native.data(), converter->imageToWidget(imagePoint).toPoint());
        QTest::qWait(10);
        const auto point = converter->imageToWidget(imagePoint).toPoint();
        QEnterEvent entered(point, point, native->mapToGlobal(point));
        QApplication::sendEvent(native.data(), &entered);
        return point;
    };
    const auto first = enterAt({16, 16});
    auto last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(first));
    const auto beforeStroke = owner->operations().snapshot();
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The pointer press did not start a stroke" : qPrintable(errors.first().first().toString()));
    QTRY_VERIFY(!documentLifetime || owner->execution().published(owner->handle()) == owner->operations().preview());
    if (!documentLifetime) {
        QVERIFY(closeAfterError);
        QVERIFY(!native);
        QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        pressed = false;
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QCOMPARE(errors.size(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        return;
    }
    if (smoothingType == 2) {
        QVERIFY(brushTool->setProperty("smoothnessQualityMin", 80));
        QVERIFY(brushTool->setProperty("smoothnessQualityMax", 160));
        QVERIFY(brushTool->setProperty("smoothnessFactor", 0.75));
        QVERIFY(brushTool->setProperty("smoothPressure", false));
        QVERIFY(brushTool->setProperty("useScalableDistance", true));
    }
    if (smoothingType != 0) QVERIFY(brushTool->setProperty("smoothingType", 3));
    if (!exhaustInput) {
        if (mirrorMode) {
            auto axes = documentLifetime->mirrorAxisConfig();
            axes.setAxisPosition(QPointF(8, 8));
            axes.setMirrorHorizontal(false);
            axes.setMirrorVertical(false);
            documentLifetime->setMirrorAxisConfig(axes);
            resources->setMirrorHorizontal(false);
            resources->setMirrorVertical(false);
        }
        if (alphaLock) resources->setGlobalAlphaLock(false);
        resources->setSize(16);
        resources->setFGColor(KoColor(QColor(221, 17, 44), KoColorSpaceRegistry::instance()->rgb8()));
    }
    QTest::qWait(10);
    last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    if (exhaustInput) {
        for (int step = 1; step <= 8 && owner->operations().hasActiveStroke(); ++step)
            QTest::mouseMove(native.data(), first + (last - first) * step / 8);
    } else {
        if (smoothingType == 2) {
            for (const auto point : {QPointF(20, 20), QPointF(24, 24), QPointF(28, 28), QPointF(32, 32)})
                QTest::mouseMove(native.data(), converter->imageToWidget(point).toPoint());
        }
        QTest::mouseMove(native.data(), last);
    }
    QTest::qWait(10);
    if (cancelAtIdle) {
        view->image()->requestStrokeCancellation();
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
        pressed = false;
        QTest::qWait(150);
        QCOMPARE(owner->operations().snapshot(), beforeStroke);
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QVERIFY(!documentLifetime->isModified());
        QTRY_COMPARE(owner->execution().published(owner->handle()), beforeStroke);
        return;
    }
    if (idlePause) {
        QTRY_VERIFY(!std::get<Gpu::PaintSnapshot>(owner->operations().preview()->layers.first().operations.last())->smoothingFlushes.isEmpty());
        last = converter->imageToWidget(QPointF(48, 16)).toPoint();
        QTest::mouseMove(native.data(), last);
    }
    if (selectDuringStroke) {
        const auto other = documentLifetime->image()->root()->firstChild();
        window->viewManager()->nodeManager()->slotUiActivatedNode(other);
        QCOMPARE(view->currentNode(), other);
        QCOMPARE(owner->operations().activeLayer(), owner->operations().snapshot()->layers.first().id);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
        pressed = false;
        QVERIFY(!owner->operations().hasActiveStroke());
        QCOMPARE(owner->operations().undoCount(), 2);
        QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(40, 40, 1, 1), 4).bytes,
                 alphaLock ? QByteArray::fromHex("1181f180") : QByteArray::fromHex("1181f1ff"));
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray(4, '\0'));
        documentLifetime->undoStack()->undo();
        QCOMPARE(owner->operations().undoCount(), 1);
        QCOMPARE(view->currentNode(), other);
        QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray(4, '\0'));
        return;
    }
    if (hideDuringStroke) {
        auto *manager = window->viewManager()->nodeManager();
        QTimer release;
        release.setSingleShot(true);
        connect(&release, &QTimer::timeout, this, [&] {
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
            pressed = false;
        });
        release.start(0);
        try {
            manager->toggleVisibility();
        } catch (const std::exception &error) {
            QFAIL(error.what());
        }
        QVERIFY(!owner->operations().hasActiveStroke());
        QVERIFY(!pressed);
        QCOMPARE(owner->operations().undoCount(), 2);
        QVERIFY(!manager->activeNode()->visible());
        QVERIFY(documentLifetime->isModified());
        documentLifetime->undoStack()->undo();
        QVERIFY(manager->activeNode()->visible());
        QCOMPARE(owner->operations().undoCount(), 1);
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 alphaLock ? QByteArray::fromHex("1181f180") : QByteArray::fromHex("1181f1ff"));
        documentLifetime->undoStack()->undo();
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!documentLifetime->isModified());
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray(4, '\0'));
        return;
    }
    if (closeAfterError) {
        QVERIFY(!documentLifetime);
        QVERIFY(!native);
        QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        pressed = false;
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QStringList reasons;
        for (const auto &notification : errors)
            if (!notification.first().toString().isEmpty()) reasons.append(notification.first().toString());
        QCOMPARE(reasons.size(), 1);
        return;
    }
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QVERIFY(!owner->operations().hasActiveStroke());
    if (exhaustInput) {
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!documentLifetime->isModified());
        QCOMPARE(errors.size(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        QCOMPARE(window->statusBar()->currentMessage(), errors.first().first().toString());
        QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().snapshot());
        return;
    }
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto painted = owner->operations().snapshot();
    const auto operation = std::get<Gpu::PaintSnapshot>(painted->layers.first().operations.last());
    QCOMPARE(operation->brush.pixelPerfect, smoothingType == 4);
    QCOMPARE(operation->brush.basicSmoothing, smoothingType == 1 || smoothingType == 2);
    QCOMPARE(bool(operation->brush.weightedSmoothing), smoothingType == 2);
    if (smoothingType == 2) {
        QVERIFY(operation->inputSamples().size() >= 6);
        const auto &weighted = *operation->brush.weightedSmoothing;
        QCOMPARE(weighted.distanceMin, 12.0);
        QCOMPARE(weighted.distanceMax, 24.0);
        QCOMPARE(weighted.tailAggressiveness, 0.25);
        QVERIFY(weighted.smoothPressure);
    }
    if (smoothingType == 4) qInfo() << "Pixel-corrected pointer input:" << operation->inputSamples().last().position;
    if (mirrorMode) {
        for (const auto &sample : QList<QPair<QPoint, bool>>{{{48, 16}, bool(mirrorMode & 1)},
             {{16, 48}, bool(mirrorMode & 2)}, {{48, 48}, mirrorMode == 3}}) {
            QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(sample.first, QSize(1, 1)), 4).bytes,
                     sample.second ? QByteArray::fromHex("1181f1ff") : QByteArray(4, '\0'));
        }
    }
    QTRY_COMPARE(owner->execution().published(owner->handle()), painted);
    if (alphaLock) QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("1181f100"));
    // Weighted smoothing moves the endpoint according to the measured input speed.
    // The earlier interior sample is opaque for every accepted input timing.
    const QPoint paintedProbe = smoothingType == 2 ? QPoint(24, 24) : QPoint(40, 40);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(paintedProbe, QSize(1, 1)), 4).bytes,
             alphaLock ? QByteArray::fromHex("1181f180") : QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(mirrorMode ? 56 : smoothingType != 0 ? 48 : 45, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("00000000"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().undoCount(), 0);
    if (alphaLock) QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(40, 40, 1, 1), 4).bytes,
                            QByteArray::fromHex("dd112c80"));
    documentLifetime->undoStack()->redo();
    QCOMPARE(owner->operations().snapshot(), painted);

    QVERIFY(brushTool->setProperty("smoothingType", smoothingType));
    resources->setSize(8);
    resources->setGlobalAlphaLock(alphaLock && !layerAlphaLock);
    resources->setEraserMode(true);
    last = enterAt(paintedProbe);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The erase press did not start a stroke" : qPrintable(errors.last().first().toString()));
    resources->setEraserMode(false);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(paintedProbe, QSize(1, 1)), 4).bytes,
             QByteArray::fromHex("1181f100"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().snapshot(), painted);

    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    const auto cancelled = enterAt({48, 16});
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, cancelled);
    QVERIFY(owner->operations().hasActiveStroke());
    QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().preview());
    view->image()->requestStrokeCancellation();
    QVERIFY(!owner->operations().hasActiveStroke());
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, cancelled);
    pressed = false;
    QCOMPARE(owner->operations().snapshot(), painted);
    QCOMPARE(owner->operations().undoCount(), 1);
    QTRY_COMPARE(owner->execution().published(owner->handle()), painted);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("ordinary-document.kra"));
    documentLifetime->setFileBatchMode(true);
    QVERIFY2(documentLifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(documentLifetime->errorMessage()));
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 8, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    auto *save = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_save"));
    QVERIFY(save);
    QTRY_VERIFY(save->isEnabled());
    QSignalSpy saved(documentLifetime.data(), &KisDocument::sigSavingFinished);
    save->trigger();
    QTRY_COMPARE(saved.size(), 1);
    QVERIFY(!documentLifetime->isModified());
    QCOMPARE(documentLifetime->path(), path);
    QCOMPARE(owner->operations().undoCount(), 2);
    documentLifetime->undoStack()->undo();
    QVERIFY(documentLifetime->isModified());
    documentLifetime->undoStack()->redo();
    QVERIFY(!documentLifetime->isModified());
    if (layerAlphaLock) {
        QFile savedFile(path);
        QVERIFY(savedFile.open(QIODevice::ReadOnly));
        const auto reopened = LibrePaint::ImportExport::Kra::RasterCodec::read(savedFile);
        QVERIFY2(reopened.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(reopened.diagnostic));
        QCOMPARE(reopened.document.image.layers.first().channelWriteMask, quint8(7));
        const auto imported = paintImageWithEncoding(execution, reopened.document.image);
        const auto image = KisImage::fromGpuImage(imported, QStringLiteral("Protected layer reopened"));
        QVERIFY(qobject_cast<KisPaintLayer*>(image->root()->firstChild().data())->alphaLocked());
        Gpu::Brush additional;
        additional.rgba = {10, 20, 30, 255};
        additional.diameter = 8;
        const auto stroke = imported->beginStroke(imported->operations().activeLayer(), additional, 0);
        QCOMPARE(imported->appendSamples(stroke, {{{40, 40}, 1}}), Gpu::Result::Success);
        QCOMPARE(imported->commitStroke(stroke), Gpu::Result::Success);
        QCOMPARE(imported->readRgbaPixels(imported->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray::fromHex("0a141e80"));
    }
}

void KisGpuDocumentViewTest::brushBlendModeUsesEffectiveCompositeOpAndRejectsUnsupported()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    Gpu::RasterLayer paintLayer;
    paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{7187a4a8-3f6c-4d0d-9b71-2f8a2bc3d4e5}"));
    paintLayer.defaultPixel = {17, 34, 51, 255};
    raster.layers = {paintLayer};
    const auto owner = paintImageWithEncoding(execution, raster);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Brush blend mode")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed && native) {
            if (lifetime) lifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native.data(), Qt::LeftButton);
        }
        if (lifetime) {
            lifetime->setModified(false);
            auto *close = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
            if (close) close->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->statusBar()->show();
    window->activateWindow();

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(24);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setFGColor(KoColor(QColor(221, 17, 44), KoColorSpaceRegistry::instance()->rgb8()));
    auto image = KisImageSP(lifetime->image());
    auto layerNode = image->root()->firstChild();
    QVERIFY(layerNode);
    const auto capture = [&](const QString &composite) {
        resources->setCurrentCompositeOp(composite);
        return captureGpuBrushResources(image, layerNode, view->canvasBase()->resourceManager());
    };

    const QList<QPair<QString, Gpu::LayerBlendMode>> mappings = {
        {QStringLiteral("normal"), Gpu::LayerBlendMode::Normal},
        {QStringLiteral("multiply"), Gpu::LayerBlendMode::Multiply},
        {QStringLiteral("soft_light"), Gpu::LayerBlendMode::SoftLightPhotoshop},
        {QStringLiteral("screen"), Gpu::LayerBlendMode::Screen},
        {QStringLiteral("darken"), Gpu::LayerBlendMode::Darken},
        {QStringLiteral("lighten"), Gpu::LayerBlendMode::Lighten},
        {QStringLiteral("add"), Gpu::LayerBlendMode::Addition},
        {QStringLiteral("linear_dodge"), Gpu::LayerBlendMode::LinearDodge},
        {QStringLiteral("erase"), Gpu::LayerBlendMode::Normal}
    };
    for (const auto &mapping : mappings) {
        const auto captured = capture(mapping.first);
        QVERIFY2(captured.diagnostic.isEmpty(), qPrintable(mapping.first + QStringLiteral(": ") + captured.diagnostic));
        QCOMPARE(captured.brush.blendMode, mapping.second);
        QCOMPARE(captured.brush.erase, mapping.first == QLatin1String("erase"));
    }

    // Leave eraser mode before exercising the unsupported painting blend.
    resources->setEraserMode(false);
    QVERIFY(!resources->eraserMode());
    const auto unsupported = capture(QStringLiteral("librepaint-test-invalid-composite"));
    QCOMPARE(resources->resourceManager()->resource(KoCanvasResource::CurrentEffectiveCompositeOp).toString(),
             QStringLiteral("librepaint-test-invalid-composite"));
    QVERIFY(!unsupported.diagnostic.isEmpty());
    QCOMPARE(unsupported.brush.blendMode, Gpu::LayerBlendMode::Normal);

    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QSignalSpy errors(brushTool, &KoToolBase::statusTextChanged);
    view->canvasBase()->canvasWidget()->setFocus();
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= 64 && view->canvasBase()->canvasWidget()->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto imageSize = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().snapshot());
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
    QVERIFY(native);
    const auto *converter = view->canvasBase()->coordinatesConverter();
    const auto enterAt = [&](QPointF imagePoint) {
        QTest::mouseMove(native.data(), converter->imageToWidget(imagePoint).toPoint());
        QTest::qWait(10);
        const auto point = converter->imageToWidget(imagePoint).toPoint();
        QEnterEvent entered(point, point, native->mapToGlobal(point));
        QApplication::sendEvent(native.data(), &entered);
        return point;
    };
    const auto first = enterAt({24, 24});
    auto last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(first));

    // A mapped mode reaches the frozen operation and paints pixels.
    resources->setCurrentCompositeOp(QStringLiteral("multiply"));
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The pointer press did not start a stroke" : qPrintable(errors.first().first().toString()));
    QTest::mouseMove(native.data(), last);
    QTest::qWait(10);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto operation = std::get<Gpu::PaintSnapshot>(
        owner->operations().snapshot()->layers.first().operations.last());
    QCOMPARE(operation->brush.blendMode, Gpu::LayerBlendMode::Multiply);
    const auto layerId = owner->operations().activeLayer();
    const auto painted = owner->readRgbaPixels(layerId, QRect(32, 32, 1, 1), 4);
    QCOMPARE(painted.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(painted.bytes.at(3)) > 0);

    // Invalid resource settings must not add a history entry or draw pixels.
    resources->setCurrentCompositeOp(QStringLiteral("librepaint-test-invalid-composite"));
    const auto beforeRejected = owner->operations().snapshot();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
    QTest::qWait(20);
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), 1);
    QStringList reasons;
    for (const auto &notification : errors)
        if (!notification.first().toString().isEmpty()) reasons.append(notification.first().toString());
    QVERIFY(!reasons.isEmpty());
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, first);

    // KRA stores the resulting blended pixels, not the blend intent.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("brush-blend.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QString reopenDiagnostic;
    const auto reopenExecution = KisPart::instance()->gpuExecution(reopenDiagnostic);
    QVERIFY2(reopenExecution, qPrintable(reopenDiagnostic));
    Gpu::PaintImage reopened(reopenExecution, loaded.document.image);
    const auto reopenedPixel = reopened.readRgbaPixels(reopened.operations().activeLayer(), QRect(32, 32, 1, 1), 4);
    QCOMPARE(reopenedPixel.code, Gpu::ExecutionCode::Success);
    QCOMPARE(reopenedPixel.bytes, painted.bytes);
}

void KisGpuDocumentViewTest::paintingBackendChangesRebindTheCanvas_data()
{
    QTest::addColumn<bool>("initialGpu");
    QTest::addColumn<bool>("replacementGpu");
    QTest::newRow("cpu-to-gpu") << false << true;
    QTest::newRow("gpu-to-cpu") << true << false;
    QTest::newRow("cpu-to-cpu") << false << false;
}

void KisGpuDocumentViewTest::paintingBackendChangesRebindTheCanvas()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, initialGpu);
    QFETCH(bool, replacementGpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                       QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = settings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = settings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreRenderer = qScopeGuard([&] {
        if (hadRenderer) settings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else settings.remove(QStringLiteral("OpenGLRenderer"));
        settings.sync();
    });
    KisConfig(true).disableOpenGL();
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (initialGpu) {
        document->setCurrentImage(KisImage::fromGpuImage(paintImageWithEncoding(execution, QSize(64, 64)), QStringLiteral("Initial image")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Initial image"), 64, 64, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
    const KisImageWSP oldIdentity = document->image();
    connect(document.get(), &KisDocument::sigCurrentImageAboutToChange, this, [oldIdentity] {
        std::thread notification([oldIdentity] {
            auto image = oldIdentity.toStrongRef();
            image->sigSizeChanged({12, 12}, {16, 16});
        });
        notification.join();
    });
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<QWindow> native;
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) lifetime->image()->requestStrokeCancellation();
        if (pressed) QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    auto oldImage = lifetime->image().toStrongRef();
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
    QPointer<QWindow> oldNative;
    if (initialGpu) {
        for (auto *candidate : QGuiApplication::allWindows())
            if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) oldNative = candidate;
        QVERIFY(oldNative);
    }
    window->viewManager()->nodeManager()->setNodeName(oldImage->root()->firstChild(), QStringLiteral("Edited old image"));
    oldImage->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 1);
    KisImageSP replacement;
    if (replacementGpu) {
        replacement = KisImage::fromGpuImage(paintImageWithEncoding(execution, QSize(96, 80)), QStringLiteral("Replacement image"));
    } else {
        std::unique_ptr<KisDocument> fixture(KisPart::instance()->createDocument());
        QVERIFY(fixture->newImage(QStringLiteral("Replacement image"), 96, 80, cs,
                                  KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        replacement = fixture->image()->clone();
    }
    const auto seed = QByteArray::fromHex("cc8844ff");
    replacement->root()->firstChild()->paintDevice()->writeBytes(reinterpret_cast<const quint8*>(seed.constData()), 8, 8, 1, 1);
    QSignalSpy sizes(view->canvasBase(), &KisCanvas2::sigViewImageSizeChanged);
    lifetime->setCurrentImage(replacement);
    replacement.clear();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QCOMPARE(sizes.size(), 0);
    QCOMPARE(lifetime->undoStack()->count(), 0);
    QCOMPARE(view->canvasBase()->coordinatesConverter()->imageRectInImagePixels(), QRect(0, 0, 96, 80));
    QTRY_COMPARE(view->currentNode(), lifetime->image()->root()->firstChild());
    if (initialGpu) QTRY_VERIFY(!oldNative);
    if (replacementGpu) {
        for (auto *candidate : QGuiApplication::allWindows())
            if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = candidate;
        QVERIFY(native);
        const auto owner = lifetime->image()->gpuImage();
        QTRY_COMPARE(execution.published(owner->handle()), owner->operations().snapshot());
    }
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    auto *brush = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brush && brush->setProperty("smoothingType", 0));
    QPointer<QWidget> input(view->canvasBase()->canvasWidget());
    input->setFocus();
    QTRY_VERIFY(!view->canvasBase()->coordinatesConverter()->getCanvasWidgetSize().isEmpty());
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto size = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({size.width() / 2, size.height() / 2});
    const auto first = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(16, 16)).toPoint();
    const auto last = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(40, 40)).toPoint();
    QEnterEvent entered(first, first, native ? native->mapToGlobal(first) : input->mapToGlobal(first));
    QApplication::sendEvent(native ? static_cast<QObject*>(native.data()) : input.data(), &entered);
    const auto before = lifetime->undoStack()->count();
    pressed = true;
    if (native) {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
        QTest::mouseMove(native.data(), last);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    } else {
        QTest::mousePress(input.data(), Qt::LeftButton, Qt::NoModifier, first);
        QMouseEvent moved(QEvent::MouseMove, last, input->mapToGlobal(last), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(input.data(), &moved);
        QTest::mouseRelease(input.data(), Qt::LeftButton, Qt::NoModifier, last);
    }
    pressed = false;
    lifetime->image()->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), before + 1);
    const auto pixel = [](KisImageWSP image, QPoint position) {
        QByteArray result(4, '\0');
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8*>(result.data()), position.x(), position.y(), 1, 1);
        return result;
    };
    QCOMPARE(pixel(lifetime->image(), {28, 28}), QByteArray::fromHex("f18111ff"));
    QCOMPARE(pixel(oldImage, {28, 28}), QByteArray(4, '\0'));
    lifetime->undoStack()->undo();
    lifetime->image()->waitForDone();
    QCOMPARE(pixel(lifetime->image(), {28, 28}), QByteArray(4, '\0'));
    QCOMPARE(pixel(lifetime->image(), {8, 8}), seed);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("replacement.kra"));
    QSignalSpy savingFinished(lifetime, &KisDocument::sigSavingFinished);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QTRY_COMPARE(savingFinished.size(), 1);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto saved = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(saved.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(saved.diagnostic));
    QCOMPARE(saved.document.image.size, QSize(96, 80));
    QCOMPARE(saved.document.image.layers.first().tiles.first().rgba.mid((8 * 256 + 8) * 4, 4), QByteArray::fromHex("4488ccff"));
}

void KisGpuDocumentViewTest::replacementUpdatesPresentationInputHistoryAndSaving_data()
{
    QTest::addColumn<bool>("replaceIdentity");
    QTest::addColumn<bool>("differentExecution");
    QTest::addColumn<bool>("replaceDuringInput");
    QTest::addColumn<int>("closePhase");
    QTest::newRow("copy-keeps-image-identity") << false << false << false << 0;
    QTest::newRow("set-current-image-replaces-identity") << true << false << false << 0;
    QTest::newRow("set-current-image-changes-execution") << true << true << false << 0;
    QTest::newRow("set-current-image-cancels-old-input") << true << false << true << 0;
    QTest::newRow("close-before-adoption") << true << false << false << 1;
    QTest::newRow("close-after-adoption") << true << false << false << 2;
}

void KisGpuDocumentViewTest::replacementUpdatesPresentationInputHistoryAndSaving()
{
    QFETCH(bool, replaceIdentity);
    QFETCH(bool, differentExecution);
    QFETCH(bool, replaceDuringInput);
    QFETCH(int, closePhase);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::Execution otherExecution(device);
    const auto previous = paintImageWithEncoding(execution, QSize(64, 64));
    const auto sourceOwner = paintImageWithEncoding(differentExecution ? otherExecution : execution, replaceIdentity ? QSize(96, 80) : QSize(64, 64));
    const auto layer = sourceOwner->operations().activeLayer();
    QCOMPARE(sourceOwner->replaceRgbaPixels(layer, QRect(8, 8, 1, 1), QByteArray::fromHex("4488ccff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    source->setCurrentImage(KisImage::fromGpuImage(sourceOwner, QStringLiteral("Replacement source")));
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(previous, QStringLiteral("Replacement target")));
    const KisImageWSP previousIdentity = document->image();
    connect(document.get(), &KisDocument::sigCurrentImageAboutToChange, this, [previousIdentity, replaceIdentity, closePhase] {
        if (replaceIdentity && closePhase == 0) {
            auto image = previousIdentity.toStrongRef();
            image->sigRequestNodeReselection(image->root()->firstChild(), {image->root()->firstChild()});
        }
    });
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed) {
            if (documentLifetime) documentLifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native ? native.data() : window->windowHandle(), Qt::LeftButton);
        }
        if (documentLifetime) {
            documentLifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    window->activateWindow();
    QTRY_COMPARE(execution.published(previous->handle()), previous->operations().snapshot());
    if (replaceDuringInput) {
        auto *profiles = KisInputProfileManager::instance();
        profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                     QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
        profiles->loadProfiles();
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setPaintOpPreset(m_preset);
        resources->setSize(8);
        resources->setOpacity(1);
        resources->setEraserMode(false);
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        auto *brush = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
        QVERIFY(brush && brush->setProperty("smoothingType", 0));
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
        QVERIFY(native);
        const auto point = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(32, 32)).toPoint();
        QEnterEvent entered(point, point, native->mapToGlobal(point));
        QApplication::sendEvent(native.data(), &entered);
        QTest::mouseMove(native.data(), point);
        QTest::qWait(10);
        pressed = true;
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(previous->operations().hasActiveStroke());
    }
    auto *identity = view->image().data();
    KisImageSP retainedPrevious = replaceIdentity && closePhase == 0 ? view->image().toStrongRef() : KisImageSP();
    const bool automaticDelivery = KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled();
    const auto restoreDelivery = qScopeGuard([automaticDelivery] {
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(automaticDelivery);
    });
    if (replaceIdentity) {
        auto replacement = KisImage::fromGpuImage(sourceOwner->clone(), QStringLiteral("Replacement image"));
        const auto adopted = replacement->gpuImage();
        if (closePhase) {
            const auto signal = closePhase == 1 ? &KisDocument::sigCurrentImageAboutToChange : &KisDocument::sigCurrentImageChanged;
            connect(documentLifetime, signal, window.get(), [&, view] {
                documentLifetime->setModified(false);
                view->closeView();
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            });
        }
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
        documentLifetime->setCurrentImage(replacement);
        replacement.clear();
        if (closePhase) {
            QTRY_VERIFY(!documentLifetime);
            QCOMPARE(adopted->operations().undoCount(), 0);
            QVERIFY(!adopted->operations().hasActiveStroke());
            QCOMPARE(adopted->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("4488ccff"));
            QCOMPARE(sourceOwner->operations().undoCount(), 1);
            return;
        }
        QVERIFY(view->image().data() != identity);
    } else {
        documentLifetime->copyFromDocument(*source);
        QCOMPARE(view->image().data(), identity);
    }
    const auto copied = view->image()->gpuImage();
    QVERIFY(copied != previous);
    QVERIFY(copied != sourceOwner);
    QTRY_COMPARE(copied->execution().published(copied->handle()), copied->operations().snapshot());
    QCOMPARE(view->canvasBase()->coordinatesConverter()->imageRectInImagePixels(), QRect(QPoint(), sourceOwner->operations().snapshot()->size));
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    auto *brush = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brush);
    QVERIFY(brush->setProperty("smoothingType", 0));
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
    QVERIFY(native);
    if (replaceDuringInput) {
        QVERIFY(!previous->operations().hasActiveStroke());
        QCOMPARE(previous->operations().undoCount(), 0);
        const auto point = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(32, 32)).toPoint();
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point);
        pressed = false;
        QCOMPARE(copied->operations().undoCount(), 0);
    }
    QTRY_VERIFY(view->currentNode());
    QTRY_COMPARE(view->currentNode()->uuid(), copied->operations().snapshot()->layers.first().uuid);
    if (retainedPrevious) {
        const auto active = view->currentNode();
        QSignalSpy sizes(view->canvasBase(), &KisCanvas2::sigViewImageSizeChanged);
        QSignalSpy activation(window->viewManager()->nodeManager(), &KisNodeManager::sigNodeActivated);
        retainedPrevious->sigSizeChanged({}, {});
        retainedPrevious->sigRequestNodeReselection(retainedPrevious->root()->firstChild(), {retainedPrevious->root()->firstChild()});
        QApplication::sendPostedEvents();
        QApplication::processEvents();
        QCOMPARE(sizes.size(), 0);
        QCOMPARE(activation.size(), 0);
        QCOMPARE(view->currentNode(), active);
    }
    const auto point = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(32, 32)).toPoint();
    QEnterEvent entered(point, point, native->mapToGlobal(point));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mouseMove(native.data(), point);
    QTest::qWait(10);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point);
    QVERIFY(copied->operations().hasActiveStroke());
    QVERIFY(!previous->operations().hasActiveStroke());
    if (retainedPrevious) {
        retainedPrevious->requestStrokeCancellation();
        retainedPrevious->requestStrokeEnd();
        QApplication::sendPostedEvents();
        QApplication::processEvents();
        QVERIFY(copied->operations().hasActiveStroke());
    }
    auto *undo = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_undo"));
    QVERIFY(undo);
    QTRY_VERIFY(undo->isEnabled());
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point);
    pressed = false;
    QCOMPARE(copied->operations().undoCount(), 1);
    documentLifetime->undoStack()->undo();
    QCOMPARE(copied->operations().undoCount(), 0);
    QCOMPARE(copied->readRgbaPixels(layer, QRect(32, 32, 1, 1), 4).bytes, QByteArray::fromHex("00000000"));
    QCOMPARE(sourceOwner->operations().undoCount(), 1);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("replacement.kra"));
    documentLifetime->setFileBatchMode(true);
    QVERIFY(documentLifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((8 * 256 + 8) * 4, 4), QByteArray::fromHex("4488ccff"));
}


void KisGpuDocumentViewTest::canvasBackgroundFollowsPreferencesWithoutEditing_data()
{
    QTest::addColumn<bool>("scrollCheckers");
    QTest::addColumn<int>("secondAlpha");
    QTest::addColumn<bool>("imageLimitPan");
    QTest::newRow("fixed-opaque") << false << 255 << false;
    QTest::newRow("scrolling-opaque") << true << 255 << false;
    QTest::newRow("fixed-translucent") << false << 128 << false;
    QTest::newRow("scrolling-translucent") << true << 128 << false;
    QTest::newRow("scrolling-image-limit") << true << 128 << true;
}

void KisGpuDocumentViewTest::canvasBackgroundFollowsPreferencesWithoutEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, scrollCheckers);
    QFETCH(int, secondAlpha);
    QFETCH(bool, imageLimitPan);
    KisConfig config(false);
    const auto oldFirst = config.checkersColor1();
    const auto oldSecond = config.checkersColor2();
    const auto oldBorder = config.canvasBorderColor();
    const auto oldSize = config.checkSize();
    const auto oldScrolling = config.scrollCheckers();
    const auto oldVastScrolling = config.vastScrolling();
    const auto oldHideScrollbars = config.hideScrollbars();
    const auto restoreConfig = qScopeGuard([&] {
        config.setCheckersColor1(oldFirst);
        config.setCheckersColor2(oldSecond);
        config.setCanvasBorderColor(oldBorder);
        config.setCheckSize(oldSize);
        config.setScrollingCheckers(oldScrolling);
        config.setVastScrolling(oldVastScrolling);
        config.setHideScrollbars(oldHideScrollbars);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    // Scrollbar overlays are not part of the canvas background contract.
    config.setHideScrollbars(true);
    QColor first(24, 68, 116, 128);
    QColor second(173, 209, 67, secondAlpha);
    QColor border(9, 33, 57);
    int checkSize = imageLimitPan ? 1 : 8;
    if (imageLimitPan) config.setVastScrolling(1);
    const auto configure = [&] {
        config.setCheckersColor1(first);
        config.setCheckersColor2(second);
        config.setCanvasBorderColor(border);
        config.setCheckSize(checkSize);
        config.setScrollingCheckers(scrollCheckers);
    };
    configure();
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = paintImageWithEncoding(execution, QSize(imageLimitPan ? Gpu::RgbaTile::CoordinateLimit : 128, 128));
    const auto snapshot = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(snapshot), Gpu::Result::Success);
    const auto retainedVersion = owner->operations().preview();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU canvas background")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    if (imageLimitPan) view->canvasBase()->canvasWidget()->setFixedSize(128, 128);
    window->show();
    window->activateWindow();
    KoToolManager::instance()->switchToolRequested(QStringLiteral("PanTool"));
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto *converter = view->canvasBase()->coordinatesConverter();
    view->canvasController()->setPreferredCenter(converter->imageRectInWidgetPixels().center());
    QTRY_COMPARE(execution.published(owner->handle()), snapshot);
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    const auto findNative = [&] {
        native.clear();
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
                if (canvas->isVisible()) native = canvas;
        if (native) native->setCursor(Qt::BlankCursor);
        return bool(native);
    };
    QTRY_VERIFY(findNative());
    const auto reference = [&] {
        const auto ratio = native->devicePixelRatio();
        QImage result(QSizeF(native->size() * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
        result.setDevicePixelRatio(ratio);
        result.fill(border);
        QImage checks(2 * checkSize, 2 * checkSize, QImage::Format_ARGB32_Premultiplied);
        // The configured border supplies a defined base for translucent checks.
        checks.fill(border);
        QPainter pattern(&checks);
        pattern.fillRect(checks.rect(), second);
        pattern.fillRect(0, 0, checkSize, checkSize, first);
        pattern.fillRect(checkSize, checkSize, checkSize, checkSize, first);
        pattern.end();
        QTransform transform;
        QPointF origin;
        QPolygonF unusedPolygon;
        converter->getQPainterCheckersInfo(&transform, &origin, &unusedPolygon, scrollCheckers);
        const auto physicalToLogical = QTransform::fromScale(1.0 / ratio, 1.0 / ratio);
        const auto toDocument = physicalToLogical * converter->imageToWidgetTransform().inverted();
        const auto toPattern = physicalToLogical * transform.inverted();
        const QRgb colors[] = {checks.pixel(0, 0), checks.pixel(checkSize, 0)};
        const auto outside = qPremultiply(border.rgba());
        // Native document and pattern bounds sample physical pixel centers.
        for (int y = 0; y < result.height(); ++y) {
            auto *row = reinterpret_cast<QRgb *>(result.scanLine(y));
            for (int x = 0; x < result.width(); ++x) {
                const QPointF center(x + 0.5, y + 0.5);
                const auto point = toDocument.map(center);
                row[x] = outside;
                if (point.x() >= 0 && point.y() >= 0 && point.x() < snapshot->size.width() && point.y() < snapshot->size.height()) {
                    const auto patternPoint = toPattern.map(center) - origin;
                    const auto column = qint64(std::floor(patternPoint.x() / checkSize));
                    const auto patternRow = qint64(std::floor(patternPoint.y() / checkSize));
                    row[x] = colors[(column ^ patternRow) & 1];
                }
            }
        }
        return result.convertToFormat(QImage::Format_RGBA8888);
    };
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path());
    QString stage = QStringLiteral("initial");
    QImage actual, expected;
    const auto saveFrames = [&] {
        if (!QDir().mkpath(directory)) return false;
        const auto stem = QDir(directory).filePath(QStringLiteral("ordinary-canvas-background-") +
            QString::fromLatin1(QTest::currentDataTag()) + QLatin1Char('-') + stage);
        return actual.save(stem + QStringLiteral("-actual.png")) && expected.save(stem + QStringLiteral("-expected.png"));
    };
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            temporary.setAutoRemove(false);
            saveFrames();
            qInfo() << "Canvas background actual/expected artifacts at" << directory;
        }
    });
    const auto matches = [&] {
        if (!findNative()) return false;
        const auto frame = native->screen()->grabWindow(native->winId()).toImage().convertToFormat(QImage::Format_RGBA8888);
        if (frame.isNull()) return false;
        // The native child's outer two physical pixels can contain the window
        // shadow. Compare the displayed canvas content and its border.
        const QRect interior = frame.rect().adjusted(2, 2, -2, -2);
        actual = frame.copy(interior);
        expected = reference().copy(interior);
        expected.setDevicePixelRatio(actual.devicePixelRatio());
        expected.setColorSpace(actual.colorSpace());
        return actual == expected;
    };
    if (imageLimitPan) {
        const auto center = converter->widgetCenterPoint();
        view->canvasController()->setPreferredCenter(center + QPointF(converter->imageRectInWidgetPixels().width() - 32, 0));
        QTRY_COMPARE(converter->imageRectInWidgetPixels().right(), qreal(32));
        QTRY_COMPARE(native->size(), QSize(128, 128));
    }
    QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
    QVERIFY(saveFrames());
    stage = QStringLiteral("pan");
    view->canvasController()->pan(QPoint(13, 7));
    QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
    QVERIFY(saveFrames());
    if (!imageLimitPan) {
        stage = QStringLiteral("zoom");
        view->canvasController()->setZoom(KoZoomMode::ZOOM_CONSTANT, 0.5);
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
        QVERIFY(saveFrames());
        stage = QStringLiteral("rotation");
        view->canvasController()->rotateCanvas(23);
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
        QVERIFY(saveFrames());
        stage = QStringLiteral("preferences");
        first = QColor(200, 50, 70, 128);
        second = QColor(70, 120, 190, secondAlpha);
        border = QColor(31, 19, 73);
        checkSize = 16;
        configure();
        KisConfigNotifier::instance()->notifyConfigChanged();
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
        QVERIFY(saveFrames());
    }
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QCOMPARE(owner->operations().preview(), retainedVersion);
    QCOMPARE(owner->operations().undoCount(), 0);
    const auto pixels = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), 65536);
    QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixels.bytes, QByteArray(65536, char(0)));
    QVERIFY(!owner->operations().isModified());
    QVERIFY(!lifetime->isModified());
}

void KisGpuDocumentViewTest::brushOutlineFollowsTheOrdinaryPointerWithoutEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig config(false);
    const auto oldStyle = config.newOutlineStyle();
    const auto oldColor = config.getCursorMainColor();
    const auto oldSeparate = config.separateEraserCursor();
    const auto oldFullSize = config.forceAlwaysFullSizedOutline();
    const auto oldMinimum = config.outlineSizeMinimum();
    const auto oldBorder = config.canvasBorderColor();
    const auto restoreConfig = qScopeGuard([&] {
        config.setNewOutlineStyle(oldStyle);
        config.setCursorMainColor(oldColor);
        config.setSeparateEraserCursor(oldSeparate);
        config.setForceAlwaysFullSizedOutline(oldFullSize);
        config.setOutlineSizeMinimum(oldMinimum);
        config.setCanvasBorderColor(oldBorder);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    config.setNewOutlineStyle(OUTLINE_NONE);
    config.setCursorMainColor(Qt::white);
    config.setSeparateEraserCursor(false);
    config.setForceAlwaysFullSizedOutline(true);
    config.setOutlineSizeMinimum(1);
    config.setCanvasBorderColor(Qt::white);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = paintImageWithEncoding(execution, QSize(128, 128));
    QByteArray pixels;
    for (int i = 0; i < 128 * 128; ++i) pixels += QByteArray::fromHex("506478ff");
    QCOMPARE(owner->writeRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels), Gpu::Result::Success);
    const auto snapshot = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(snapshot), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU brush outline")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(24);
    resources->setEraserMode(false);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    view->canvasBase()->canvasWidget()->setFocus();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto *converter = view->canvasBase()->coordinatesConverter();
    view->canvasController()->setPreferredCenter(converter->imageRectInWidgetPixels().center());
    QTRY_COMPARE(execution.published(owner->handle()), snapshot);
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    native->setCursor(Qt::BlankCursor);
    QSignalSpy frames(native, &LibrePaint::Canvas::Gpu::CanvasWindow::frameSubmitted);
    const auto capture = [&] { return native->screen()->grabWindow(native->winId()).toImage().convertToFormat(QImage::Format_RGBA8888); };
    const auto save = [&](const QImage &image, const QString &stage) {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return true;
        return QDir().mkpath(directory) && image.save(QDir(directory).filePath(QStringLiteral("ordinary-brush-outline-") + stage + QStringLiteral(".png")));
    };
    const auto preserveFailureFrame = qScopeGuard([&] {
        if (QTest::currentTestFailed() && native) save(capture(), QStringLiteral("failure"));
    });
    const auto first = converter->imageToWidget(QPointF(40, 64)).toPoint();
    const auto second = converter->imageToWidget(QPointF(88, 64)).toPoint();
    QTest::mouseMove(native.data(), first);
    QEnterEvent entered(first, first, native->mapToGlobal(first));
    QApplication::sendEvent(native.data(), &entered);
    native->refresh();
    QTRY_VERIFY(!frames.isEmpty());
    QImage baseline;
    QTRY_VERIFY(!(baseline = capture()).isNull());
    const auto ratio = native->devicePixelRatio();
    const auto physical = [&](QPoint point) { return (QPointF(point) * ratio).toPoint(); };
    const auto currentBackground = [&] {
        const auto screenshot = capture();
        QImage result(screenshot.size(), QImage::Format_RGBA8888);
        result.setDevicePixelRatio(screenshot.devicePixelRatio());
        result.setColorSpace(screenshot.colorSpace());
        result.fill(Qt::white);
        const auto bounds = QTransform::fromScale(ratio, ratio).mapRect(converter->imageRectInWidgetPixels()).toAlignedRect().intersected(result.rect());
        for (int y = bounds.top(); y <= bounds.bottom(); ++y)
            for (int x = bounds.left(); x <= bounds.right(); ++x)
                result.setPixelColor(x, y, QColor(80, 100, 120));
        return result;
    };
    const auto expectedBaseline = currentBackground();
    // Startup notifications can overlap the native child. Accept its clean frame.
    QTRY_VERIFY_WITH_TIMEOUT((baseline = capture()) == expectedBaseline, 10000);
    QCOMPARE(baseline.pixelColor(physical(first)), QColor(80, 100, 120));
    QCOMPARE(baseline.pixelColor(physical(second)), QColor(80, 100, 120));
    QVERIFY(save(baseline, QStringLiteral("baseline")));
    const auto changedPixels = [&](const QImage &image, QPoint center) {
        const auto region = QRect(physical(center) - QPoint(16 * ratio, 16 * ratio), QSize(32 * ratio, 32 * ratio)).intersected(image.rect());
        int count = 0;
        for (int y = region.top(); y <= region.bottom(); ++y)
            for (int x = region.left(); x <= region.right(); ++x)
                if (image.pixel(x, y) != baseline.pixel(x, y)) ++count;
        return count;
    };
    const auto differenceBounds = [&](const QImage &image) {
        QRect bounds;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                if (image.pixel(x, y) != baseline.pixel(x, y)) bounds |= QRect(x, y, 1, 1);
        return bounds;
    };
    config.setNewOutlineStyle(OUTLINE_CIRCLE);
    KisConfigNotifier::instance()->notifyConfigChanged();
    // Configuration can recreate the native canvas. Follow its current window.
    QCoreApplication::processEvents();
    if (!native) {
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
                if (canvas->isVisible()) native = canvas;
    }
    QTRY_VERIFY(native && native->isVisible());
    native->setCursor(Qt::BlankCursor);
    QEnterEvent reentered(first, first, native->mapToGlobal(first));
    QApplication::sendEvent(native.data(), &reentered);
    QTest::mouseMove(native.data(), first + QPoint(1, 0));
    QTest::mouseMove(native.data(), first);
    QImage visible;
    const auto preserveVisible = qScopeGuard([&] { if (!visible.isNull()) save(visible, QStringLiteral("visible")); });
    QTRY_VERIFY_WITH_TIMEOUT((visible = capture()).size() == baseline.size() && changedPixels(visible, first) >= 40 && changedPixels(visible, second) == 0, 5000);
    QVERIFY(save(visible, QStringLiteral("visible")));
    QCOMPARE(changedPixels(visible, second), 0);
    const auto fullBounds = differenceBounds(visible);
    const auto fullSize = converter->imageToWidget(QRectF(0, 0, 24, 24)).size() * ratio;
    QVERIFY(qAbs(fullBounds.width() - fullSize.width()) <= 2 * ratio);
    QVERIFY(qAbs(fullBounds.height() - fullSize.height()) <= 2 * ratio);
    QVERIFY((fullBounds.center() - physical(first)).manhattanLength() <= 2);

    QTest::mouseMove(native.data(), second);
    QImage moved;
    QTRY_VERIFY((moved = capture()).size() == baseline.size() && changedPixels(moved, second) >= 40 && changedPixels(moved, first) == 0);
    QVERIFY(save(moved, QStringLiteral("moved")));
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(native.data(), &leave);
    QImage cleared;
    QTRY_VERIFY((cleared = capture()) == baseline);
    QVERIFY(save(cleared, QStringLiteral("cleared")));
    view->canvasController()->setZoom(KoZoomMode::ZOOM_CONSTANT, 0.5);
    const auto halfBackground = currentBackground();
    QTRY_VERIFY(capture() == halfBackground);
    baseline = halfBackground;
    const auto halfPoint = converter->imageToWidget(QPointF(64, 64)).toPoint();
    QEnterEvent halfEntered(halfPoint, halfPoint, native->mapToGlobal(halfPoint));
    QApplication::sendEvent(native.data(), &halfEntered);
    QTest::mouseMove(native.data(), halfPoint);
    QImage halfVisible;
    const auto preserveHalfVisible = qScopeGuard([&] {
        if (QTest::currentTestFailed() && !halfVisible.isNull()) save(halfVisible, QStringLiteral("half-zoom"));
    });
    const auto halfOutlineMatches = [&] {
        halfVisible = capture();
        if (halfVisible.size() != baseline.size() || changedPixels(halfVisible, halfPoint) < 20) return false;
        const auto bounds = differenceBounds(halfVisible);
        const auto size = converter->imageToWidget(QRectF(0, 0, 24, 24)).size() * ratio;
        return qAbs(bounds.width() - size.width()) <= 2 * ratio &&
               qAbs(bounds.height() - size.height()) <= 2 * ratio &&
               (bounds.center() - physical(halfPoint)).manhattanLength() <= 2;
    };
    // A delayed zoom notification can cover an otherwise complete native frame.
    QTRY_VERIFY_WITH_TIMEOUT(halfOutlineMatches(), 10000);
    QVERIFY(save(halfVisible, QStringLiteral("half-zoom")));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("PanTool"));
    // Switching tools can resize the canvas viewport through the tool options bar.
    const auto matchesCurrentBackground = [&] {
        const auto background = currentBackground();
        const auto frame = capture();
        if (frame.size() != background.size()) return false;
        baseline = background;
        return frame == background;
    };
    QTRY_VERIFY_WITH_TIMEOUT(matchesCurrentBackground(), 10000);
    const auto currentHalfPoint = converter->imageToWidget(QPointF(64, 64)).toPoint();
    QEnterEvent currentHalfEntered(currentHalfPoint, currentHalfPoint, native->mapToGlobal(currentHalfPoint));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QApplication::sendEvent(native.data(), &currentHalfEntered);
    QTest::mouseMove(native.data(), currentHalfPoint + QPoint(1, 0));
    QTest::mouseMove(native.data(), currentHalfPoint);
    QTRY_VERIFY(changedPixels(capture(), currentHalfPoint) >= 20);
    window->viewManager()->setCurrentView(nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(matchesCurrentBackground(), 10000);
    window->viewManager()->setCurrentView(view);
    QApplication::sendEvent(native.data(), &leave);
    QTRY_VERIFY_WITH_TIMEOUT(matchesCurrentBackground(), 10000);
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QCOMPARE(owner->operations().undoCount(), 0);
    const auto after = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels.size());
    QCOMPARE(after.code, Gpu::ExecutionCode::Success);
    QCOMPARE(after.bytes, pixels);
    QVERIFY(!lifetime->isModified());
}

void KisGpuDocumentViewTest::nonlinearU16CanvasTracksTheActualDisplayConfiguration()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto *registry = KoColorSpaceRegistry::instance();
    const auto raw = QColorSpace(QColorSpace::SRgb).iccProfile();
    const auto *profile = registry->createColorProfileFromData(QStringLiteral("RGBA"), QStringLiteral("U16"), raw);
    QVERIFY(profile);
    const auto *source = registry->colorSpace(QStringLiteral("RGBA"), QStringLiteral("U16"), profile);
    QVERIFY(source);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = Gpu::PixelFormat::Rgba16;
    raster.colorProfile = raw;
    Gpu::RasterLayer layer;
    layer.name = QStringLiteral("Nonlinear RGB");
    layer.defaultPixel = {0x3333, 0x6666, 0x9999, 65535};
    raster.layers.append(layer);
    auto owner = paintImageWithEncoding(execution, raster);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("ICC canvas"), source));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto close = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            if (window) {
                auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
                if (action) action->trigger();
            }
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    LibrePaint::Canvas::Gpu::CanvasWindow *native = nullptr;
    const auto findNative = [&] {
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate))
                if (canvas->isVisible()) native = canvas;
        return bool(native);
    };
    QTRY_VERIFY(findNative());
    native->setCursor(Qt::BlankCursor);
    auto *converter = view->canvasBase()->displayColorConverter();
    const auto pixel = [&] {
        const auto frame = native->screen()->grabWindow(native->winId()).toImage();
        const auto point = view->canvasBase()->coordinatesConverter()->imageToWidgetTransform().map(QPointF(32, 32));
        return frame.pixelColor((point * native->devicePixelRatio()).toPoint());
    };
    const quint16 bgra[] = {0x9999, 0x6666, 0x3333, 65535};
    const auto expected = [&] {
        const auto config = converter->multiSurfaceDisplayConfig().canvasDisplayConfig();
        const auto *target = registry->colorSpace(QStringLiteral("RGBA"), QStringLiteral("U8"), config.profile);
        quint8 result[4]{};
        if (!target || !source->convertPixelsTo(reinterpret_cast<const quint8 *>(bgra), result,
                                                target, 1, config.intent, config.conversionFlags))
            return QColor();
        return QColor(result[2], result[1], result[0], result[3]);
    };
    const auto first = expected();
    QVERIFY(first.isValid());
    native->refresh();
    QTRY_COMPARE(execution.published(owner->handle()), owner->operations().snapshot());
    QTRY_COMPARE(pixel(), first);
    auto configuration = converter->multiSurfaceDisplayConfig();
    configuration.canvasProfile = registry->p709G10Profile();
    QVERIFY(configuration.canvasProfile);
    converter->setMultiSurfaceDisplayConfig(configuration);
    const auto second = expected();
    QVERIFY(second.isValid());
    QVERIFY(second != first);
    QTRY_COMPARE(pixel(), second);
    QCOMPARE(owner->operations().snapshot()->colorProfile, raw);
    QCOMPARE(owner->operations().snapshot()->layers.first().defaultPixel, layer.defaultPixel);
}

void KisGpuDocumentViewTest::presentsEditsAndReleasesTheClosedTab()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto handle = owner->handle();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU document view")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    KisView *view = nullptr;
    try {
        view = window->newView(document.get());
    } catch (const std::exception &exception) {
        qWarning("GPU view initialization failed: %s", exception.what());
        throw;
    }
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (documentLifetime) {
            documentLifetime->setModified(false);
            if (window) {
                auto *close = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
                if (close) close->trigger();
            }
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(view->canvasBase()->canvasWidget());
    QTRY_COMPARE(execution.published(handle), owner->operations().preview());
    auto *overview = window->findChild<QDockWidget*>(QStringLiteral("OverviewDocker"));
    QVERIFY(overview);
    QWidget *preview = nullptr;
    for (auto *child : overview->findChildren<QWidget*>())
        if (QLatin1String(child->metaObject()->className()) == QLatin1String("OverviewWidget")) preview = child;
    QVERIFY(preview);
    QVERIFY(!preview->isEnabled());
    QVERIFY(!preview->toolTip().isEmpty());
    auto *undoAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_undo"));
    auto *redoAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_redo"));
    QVERIFY(undoAction);
    QVERIFY(redoAction);
    auto *history = documentLifetime->undoStack();
    auto *historyDock = window->findChild<QDockWidget*>(QStringLiteral("History"));
    QVERIFY(historyDock);
    auto *historyView = qobject_cast<QListView*>(historyDock->widget());
    QVERIFY(historyView);
    QCOMPARE(historyView->model()->rowCount(), 1);
    QSignalSpy historyChanges(history, &KUndo2Stack::indexChanged);
    QCOMPARE(history->count(), 0);
    const auto initial = owner->operations().snapshot();
    const auto firstStroke = owner->beginStroke(owner->operations().activeLayer(), {}, 17);
    QCOMPARE(owner->appendSamples(firstStroke, {{{24, 24}, 1}}), Gpu::Result::Success);
    QTRY_VERIFY(undoAction->isEnabled());
    undoAction->trigger();
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), initial);
    QCOMPARE(history->count(), 0);
    window->resize(1000, 700);
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1),
                                      QByteArray::fromHex("1181f1ff")), Gpu::Result::Success);
    QTRY_COMPARE(execution.published(handle), owner->operations().snapshot());
    QCOMPARE(history->count(), 1);
    QCOMPARE(history->index(), 1);
    QCOMPARE(historyView->model()->rowCount(), 2);
    QCOMPARE(historyView->selectionModel()->currentIndex().row(), 1);
    QVERIFY(history->canUndo());
    QVERIFY(!history->canRedo());
    QVERIFY(!history->isClean());
    QVERIFY(history->command(0));
    QVERIFY(!history->command(0)->text().toString().isEmpty());
    QTRY_VERIFY(undoAction->isEnabled());
    undoAction->trigger();
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(history->index(), 0);
    QCOMPARE(historyView->selectionModel()->currentIndex().row(), 0);
    QVERIFY(history->isClean());
    QVERIFY(!documentLifetime->isModified());
    QTRY_COMPARE(execution.published(handle), owner->operations().snapshot());
    QTRY_VERIFY(redoAction->isEnabled());
    redoAction->trigger();
    QCOMPARE(owner->operations().undoCount(), 1);
    QVERIFY(!history->isClean());
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1), 4).bytes,
             QByteArray::fromHex("1181f1ff"));
    QCOMPARE(historyChanges.size(), 3);
    historyChanges.clear();
    historyView->selectionModel()->setCurrentIndex(historyView->model()->index(0, 0), QItemSelectionModel::ClearAndSelect);
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(historyChanges.size(), 1);
    history->setIndex(1);
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    QVERIFY(history->isClean());
    QVERIFY(!documentLifetime->isModified());
    const auto stroke = owner->beginStroke(owner->operations().activeLayer(), {}, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{16, 16}, 1}}), Gpu::Result::Success);
    undoAction->trigger();
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), saved);
    QCOMPARE(history->count(), 1);
    QTRY_COMPARE(execution.published(handle), saved);
    QPointer<KisView> retainedView(view);
    QPointer<QWidget> retainedCanvas(view->canvasBase()->canvasWidget());
    documentLifetime->setModified(false);
    auto *closeAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
    QVERIFY(closeAction);
    closeAction->trigger();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(!retainedView);
    QTRY_VERIFY(!retainedCanvas);
    QTRY_VERIFY(!documentLifetime);
    const auto lastPresented = execution.published(handle);
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1),
                                      QByteArray::fromHex("010203ff")), Gpu::Result::Success);
    QApplication::processEvents();
    QCOMPARE(execution.published(handle), lastPresented);
    window->hide();
    window.reset();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

