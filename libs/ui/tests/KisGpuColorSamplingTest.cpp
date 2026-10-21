/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <QDockWidget>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QPointer>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QSignalSpy>
#include <KLocalizedString>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColor.h>
#include <KoCanvasResourcesIds.h>
#include <KoPointerEvent.h>
#include <KoToolManager.h>
#include <KisAsyncColorSamplerHelper.h>
#include <actions/input/KisApplicationInputActions.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_preset.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <input/ui/kis_input_profile_manager.h>
#include <kis_canvas2.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_config.h>
#include <kis_tool.h>
#include <kis_tool_utils.h>
#include <nodes/kis_node_manager.h>
#include <kundo2stack.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <exception>
#include <memory>

class KisGpuColorSamplingTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
        KLocalizedString::setApplicationDomain("krita");
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
        KisInputProfileManager::instance()->setActions(createApplicationInputActions());
        QString path = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        const auto prefix = qEnvironmentVariable("KIS_TEST_PREFIX_PATH");
        const auto staged = QDir(prefix).filePath(QStringLiteral("bin/data/test1.bundle"));
        if (!prefix.isEmpty() && QFileInfo(staged).isFile()) path = staged;
        const auto bundle = KisResourceStorageSP::create(path);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(path, bundle));
        m_preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(m_preset);
        m_preset->setName(QStringLiteral("GPU color sampling fixture"));
        m_preset->setFilename(QStringLiteral("gpu-color-sampling-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        m_preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(m_preset));
    }
    void samplesLayerAndProjectionWithoutEditing_data()
    {
        QTest::addColumn<bool>("currentLayer");
        QTest::addColumn<bool>("foreground");
        QTest::addColumn<QString>("transition");
        QTest::addColumn<int>("alpha");
        QTest::addColumn<bool>("gpu");
        QTest::addColumn<int>("blend");
        QTest::addColumn<int>("radius");
        struct Case { const char *name; bool layer, foreground; const char *transition; int alpha; bool gpu; };
        const Case cases[] = {
            {"layer-foreground", true, true, "", 255, true},
            {"layer-background", true, false, "", 255, true},
            {"merged-foreground", false, true, "", 255, true},
            {"merged-background", false, false, "", 255, true},
            {"registered-sampler", false, true, "tool", 255, true},
            {"edit-after-request", false, true, "edit", 255, true},
            {"replace-before-completion", false, true, "replace", 255, true},
            {"destroy-on-raw-color", true, true, "destroy", 255, true},
            {"restart-on-raw-color", true, true, "restart", 255, true},
            {"replace-on-raw-color", true, true, "replace-raw", 255, true},
            {"layer-translucent", true, true, "", 128, true},
            {"layer-transparent", true, true, "transparent", 0, true},
            {"ordinary-layer", true, false, "", 255, false},
            {"ordinary-merged", false, true, "", 255, false},
            {"ordinary-translucent", true, true, "", 128, false},
        };
        QTest::newRow("blend-first-operation") << true << true << QString() << 255 << true << 50 << 1;
        for (const auto &c : cases) QTest::newRow(c.name) << c.layer << c.foreground
            << QString::fromLatin1(c.transition) << c.alpha << c.gpu << 100 << 1;
        for (int blend : {0, 1, 33, 50, 99}) {
            const QByteArray name = QByteArray("blend-foreground-") + QByteArray::number(blend);
            QTest::newRow(name.constData()) << true << true << QString() << 255 << true << blend << 1;
        }
        QTest::newRow("blend-merged-background") << false << false << QString() << 255 << true << 50 << 1;
        QTest::newRow("blend-translucent") << true << true << QString() << 128 << true << 50 << 1;
        QTest::newRow("blend-transparent") << true << true << QString() << 0 << true << 50 << 1;
        QTest::newRow("registered-sampler-blend") << false << true << QStringLiteral("tool") << 255 << true << 50 << 1;
        QTest::newRow("blend-captured-current-color") << true << true << QStringLiteral("palette") << 255 << true << 50 << 1;
        QTest::newRow("blend-preview-without-updating-color") << true << true << QStringLiteral("preview") << 255 << true << 50 << 1;
        QTest::newRow("blend-current-color-rgb16") << true << true << QStringLiteral("rgb16") << 255 << true << 50 << 1;
        QTest::newRow("materialized-rgb") << true << true << QStringLiteral("colorful") << 255 << true << 100 << 1;
        QTest::newRow("blend-materialized-rgb") << true << true << QStringLiteral("colorful") << 255 << true << 50 << 1;
        QTest::newRow("recover-on-raw-color") << true << true << QStringLiteral("recover-raw") << 255 << true << 100 << 1;
        QTest::newRow("close-on-raw-color") << true << true << QStringLiteral("close-raw") << 255 << true << 100 << 1;
        QTest::newRow("recover-before-release") << true << true << QStringLiteral("recover-release") << 255 << true << 100 << 1;
        QTest::newRow("average-layer-foreground") << true << true << QStringLiteral("average") << 255 << true << 100 << 3;
        QTest::newRow("average-layer-background") << true << false << QStringLiteral("average") << 255 << true << 100 << 3;
        QTest::newRow("average-merged-foreground") << false << true << QStringLiteral("average") << 255 << true << 100 << 3;
        QTest::newRow("average-merged-background") << false << false << QStringLiteral("average") << 255 << true << 100 << 3;
        QTest::newRow("average-blend-layer") << true << true << QStringLiteral("average") << 255 << true << 50 << 3;
        QTest::newRow("average-blend-merged-background") << false << false << QStringLiteral("average") << 255 << true << 50 << 3;
        QTest::newRow("registered-sampler-average") << false << true << QStringLiteral("tool-average") << 255 << true << 100 << 3;
        QTest::newRow("registered-sampler-average-blend") << false << true << QStringLiteral("tool-average") << 255 << true << 50 << 3;
        QTest::newRow("average-large-layer") << true << true << QStringLiteral("average") << 255 << true << 100 << 900;
    }
    void samplesLayerAndProjectionWithoutEditing();
private:
    KisPaintOpPresetSP m_preset;
};

void KisGpuColorSamplingTest::samplesLayerAndProjectionWithoutEditing()
{
    QFETCH(bool, currentLayer);
    QFETCH(bool, foreground);
    QFETCH(QString, transition);
    QFETCH(int, alpha);
    QFETCH(bool, gpu);
    QFETCH(int, blend);
    QFETCH(int, radius);
    const bool averaging = transition.contains(QStringLiteral("average"));
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = {64, 48};
    raster.activeLayer = 1;
    raster.layers = {{QStringLiteral("Destination"), true, {}, {}, {129, 129, 129, 255}},
                     {QStringLiteral("Source"), true, {}, {}, {126, 126, 126, 255}}};
    raster.layers[1].opacity = 85;
    raster.layers[1].defaultPixel[3] = quint8(alpha);
    raster.layers[0].uuid = QUuid(QStringLiteral("{d3000001-1122-3344-5566-778899aabbcc}"));
    raster.layers[1].uuid = QUuid(QStringLiteral("{d3000002-1122-3344-5566-778899aabbcc}"));
    if (averaging) {
        raster.layers[1].defaultPixel = {255, 0, 0, 255};
        raster.layers[1].opacity = 255;
    }
    const auto owner = gpu ? Gpu::PaintImageSP::create(execution, raster) : Gpu::PaintImageSP();
    if (averaging) {
        QCOMPARE(owner->replaceRgbaPixels(owner->operations().snapshot()->layers[1].id, {12, 9, 1, 1}, QByteArray::fromHex("0000ffff")), Gpu::Result::Success);
    }
    if (transition == QStringLiteral("colorful")) {
        QCOMPARE(owner->replaceRgbaPixels(owner->operations().snapshot()->layers[1].id, {12, 9, 1, 1}, QByteArray::fromHex("1561c9ff")), Gpu::Result::Success);
    }
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image;
    KisConfig(true).disableOpenGL();
    if (gpu) image = KisImage::fromGpuImage(owner, QStringLiteral("Color sampling"));
    else {
        image = new KisImage(document->createUndoStore(), 64, 48, cs, "Color sampling");
        KisPaintLayerSP bottom = new KisPaintLayer(image, QStringLiteral("Destination"), 255);
        KisPaintLayerSP top = new KisPaintLayer(image, QStringLiteral("Source"), 85);
        bottom->setUuid(raster.layers[0].uuid);
        top->setUuid(raster.layers[1].uuid);
        bottom->paintDevice()->fill({0, 0, 64, 48}, KoColor(QColor(129, 129, 129), cs));
        top->paintDevice()->fill({0, 0, 64, 48}, KoColor(QColor(126, 126, 126, alpha), cs));
        QVERIFY(image->addNode(bottom));
        QVERIFY(image->addNode(top, image->root(), bottom));
        image->refreshGraphAsync();
        image->waitForDone();
    }
    document->setCurrentImage(image);
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto cleanup = qScopeGuard([&] {
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
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    QTRY_VERIFY(!windowReady.isEmpty());
    if (!gpu) window->viewManager()->nodeManager()->slotNonUiActivatedNode(image->root()->lastChild());
    QTRY_COMPARE(view->currentNode()->uuid(), raster.layers[1].uuid);
    const auto previousSpace = transition == QStringLiteral("rgb16")
        ? KoColorSpaceRegistry::instance()->rgb16(cs->profile()) : cs;
    QVERIFY(previousSpace);
    const KoColor oldForeground(QColor(5, 10, 20), previousSpace), oldBackground(QColor(200, 210, 220), cs);
    resources->setFGColor(oldForeground);
    resources->setBGColor(oldBackground);
    lifetime->setModified(false);
    const auto before = owner ? owner->operations().snapshot() : Gpu::Snapshot();
    const auto pixels = [&] {
        QByteArray bytes(64 * 48 * 4, '\0');
        image->projection()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), {0, 0, 64, 48});
        return bytes;
    };
    auto expectedPixels = pixels();
    auto expectedVersion = before;
    auto history = lifetime->undoStack()->count();
    bool modified = false;
    if (transition.startsWith(QStringLiteral("tool"))) {
        KisToolUtils::ColorSamplerConfig previous;
        previous.load();
        KisToolUtils::ColorSamplerConfig config;
        config.radius = radius;
        config.blend = blend;
        config.sampleMerged = true;
        config.toForegroundColor = true;
        config.save();
        const auto restore = qScopeGuard([&] {
            KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
            previous.save();
        });
        view->canvasBase()->resourceManager()->setResource(KoCanvasResource::UsingOtherColor, false);
        const auto toolId = QStringLiteral("KritaSelected/KisToolColorSampler");
        KoToolManager::instance()->switchToolRequested(toolId);
        QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
        auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId));
        QVERIFY(tool);
        const auto point = view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9));
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        KoPointerEvent start(&press, point);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        KoPointerEvent end(&release, point);
        try {
            tool->beginPrimaryAction(&start);
            tool->endPrimaryAction(&end);
        } catch (const std::exception &failure) {
            QFAIL(failure.what());
        }
        const QColor sampled = averaging ? (blend == 50 ? QColor(116, 5, 24) : QColor(227, 0, 28))
            : (blend == 50 ? QColor(66, 69, 74) : QColor(128, 128, 128));
        QTRY_COMPARE(resources->fgColor(), KoColor(sampled, cs));
        QCOMPARE(resources->bgColor(), oldBackground);
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(pixels(), expectedPixels);
        QCOMPARE(lifetime->undoStack()->count(), history);
        QVERIFY(!lifetime->isModified());
        return;
    }
    auto helper = std::make_unique<KisAsyncColorSamplerHelper>(view->canvasBase());
    if (transition == QStringLiteral("preview")) helper->setUpdateGlobalColor(false);
    QSignalSpy raw(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected);
    QSignalSpy selected(helper.get(), &KisAsyncColorSamplerHelper::sigColorSelected);
    QSignalSpy finished(helper.get(), &KisAsyncColorSamplerHelper::sigFinalColorSelected);
    bool restarted = false;
    if (transition == QStringLiteral("destroy")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] { helper.reset(); });
    } else if (transition == QStringLiteral("restart")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] {
            if (restarted) return;
            restarted = true;
            helper->activate(false, foreground);
            helper->startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), radius, blend);
            helper->endAction();
            helper->deactivate();
        });
    } else if (transition == QStringLiteral("close-raw")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] { owner->close(); });
    } else if (transition == QStringLiteral("recover-raw")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] {
            auto recovered = raster;
            recovered.layers[1].defaultPixel = {42, 90, 200, 255};
            recovered.layers[1].opacity = 255;
            Gpu::OperationDocument replacement(recovered);
            QCOMPARE(owner->restoreRecovery(replacement.recoveryData()), Gpu::Result::Success);
            expectedVersion = owner->operations().snapshot();
            expectedPixels = pixels();
            history = lifetime->undoStack()->count();
            modified = lifetime->isModified();
        });
    } else if (transition == QStringLiteral("replace-raw")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] {
            Gpu::RasterImage replacement;
            replacement.size = raster.size;
            replacement.layers = {{QStringLiteral("Replacement"), true, {}, {}, {42, 42, 42, 255}}};
            const auto changed = Gpu::PaintImageSP::create(execution, replacement);
            lifetime->setCurrentImage(KisImage::fromGpuImage(changed, QStringLiteral("Replacement")));
        });
    }
    helper->activate(currentLayer, foreground);
    QString error;
    try {
        helper->startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), radius, blend);
        QCOMPARE(raw.size(), 0);
        if (transition == QStringLiteral("palette")) resources->setFGColor(KoColor(QColor(230, 1, 40), cs));
        if (transition == QStringLiteral("edit")) {
            QCOMPARE(owner->replaceRgbaPixels(before->layers[1].id, {12, 9, 1, 1}, QByteArray::fromHex("fafafaff")), Gpu::Result::Success);
            expectedVersion = owner->operations().snapshot();
            history = lifetime->undoStack()->count();
            modified = lifetime->isModified();
            expectedPixels = pixels();
        }
        if (transition == QStringLiteral("recover-release")) {
            QTRY_COMPARE(selected.size(), 1);
            QCOMPARE(finished.size(), 0);
            auto recovered = raster;
            recovered.layers[1].defaultPixel = {42, 90, 200, 255};
            recovered.layers[1].opacity = 255;
            Gpu::OperationDocument replacement(recovered);
            QCOMPARE(owner->restoreRecovery(replacement.recoveryData()), Gpu::Result::Success);
            expectedVersion = owner->operations().snapshot();
            expectedPixels = pixels();
            history = lifetime->undoStack()->count();
            modified = lifetime->isModified();
        }
        helper->endAction();
    } catch (const std::exception &exception) {
        error = QString::fromUtf8(exception.what());
    }
    helper->deactivate();
    QVERIFY2(error.isEmpty(), qPrintable(error));
    if (transition == QStringLiteral("transparent")) {
        KisAsyncColorSamplerHelper barrier(view->canvasBase());
        barrier.setUpdateGlobalColor(false);
        QSignalSpy ready(&barrier, &KisAsyncColorSamplerHelper::sigFinalColorSelected);
        barrier.activate(false, false);
        barrier.startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), 1, 100);
        barrier.endAction();
        barrier.deactivate();
        QTRY_COMPARE(ready.size(), 1);
        QCOMPARE(qvariant_cast<KoColor>(ready.first().first()).toQColor(), QColor(129, 129, 129));
        QCOMPARE(raw.size(), 0);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(resources->fgColor(), oldForeground);
        QCOMPARE(resources->bgColor(), oldBackground);
        QCOMPARE(pixels(), expectedPixels);
        QCOMPARE(owner->operations().snapshot(), before);
        return;
    }
    if (transition == QStringLiteral("replace")) {
        Gpu::RasterImage replacement;
        replacement.size = raster.size;
        replacement.layers = {{QStringLiteral("Replacement"), true, {}, {}, {42, 42, 42, 255}}};
        const auto changed = Gpu::PaintImageSP::create(execution, replacement);
        lifetime->setCurrentImage(KisImage::fromGpuImage(changed, QStringLiteral("Replacement")));
        KisAsyncColorSamplerHelper latest(view->canvasBase());
        latest.setUpdateGlobalColor(false);
        QSignalSpy ready(&latest, &KisAsyncColorSamplerHelper::sigFinalColorSelected);
        latest.activate(false, false);
        latest.startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), 1, 100);
        latest.endAction();
        latest.deactivate();
        QTRY_COMPARE(ready.size(), 1);
        QCOMPARE(qvariant_cast<KoColor>(ready.first().first()).toQColor(), QColor(42, 42, 42));
        QCOMPARE(raw.size(), 0);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(resources->fgColor(), oldForeground);
        QCOMPARE(resources->bgColor(), oldBackground);
        QCOMPARE(owner->operations().snapshot(), before);
        return;
    }
    if (transition == QStringLiteral("recover-release")) {
        QCOMPARE(raw.size(), 1);
        QCOMPARE(selected.size(), 1);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(resources->fgColor(), KoColor(QColor(126, 126, 126), cs));
        QCOMPARE(owner->operations().snapshot(), expectedVersion);
        QCOMPARE(pixels(), expectedPixels);
        return;
    }
    if (transition == QStringLiteral("close-raw")) {
        QTRY_COMPARE(raw.size(), 1);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(owner->handle(), 0);
        return;
    }
    if (transition == QStringLiteral("destroy") || transition == QStringLiteral("replace-raw") || transition == QStringLiteral("recover-raw")) {
        QTRY_COMPARE(raw.size(), 1);
        if (transition == QStringLiteral("destroy")) QVERIFY(!helper);
        QCOMPARE(raw.size(), 1);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
    } else {
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(raw.size(), restarted ? 2 : 1);
        QCOMPARE(selected.size(), 1);
    }
    QColor expected = currentLayer && !restarted ? QColor(126, 126, 126) : QColor(128, 128, 128);
    if (transition == QStringLiteral("colorful")) expected = QColor(21, 97, 201);
    if (averaging) expected = radius == 900 ? QColor(255, 0, 0) : QColor(227, 0, 28);
    auto rawExpected = expected;
    if (currentLayer && !restarted) rawExpected.setAlpha(alpha);
    if (blend < 100) {
        const auto previous = (foreground ? oldForeground : oldBackground).toQColor();
        const int sampledWeight = blend * 255 / 100;
        const int previousWeight = 255 - sampledWeight;
        const int totalAlpha = previous.alpha() * previousWeight + rawExpected.alpha() * sampledWeight;
        const auto channel = [&](int oldValue, int sampledValue) {
            return (oldValue * previous.alpha() * previousWeight + sampledValue * rawExpected.alpha() * sampledWeight + totalAlpha / 2) / totalAlpha;
        };
        rawExpected = QColor(channel(previous.red(), rawExpected.red()), channel(previous.green(), rawExpected.green()),
                             channel(previous.blue(), rawExpected.blue()), (totalAlpha + 127) / 255);
        expected = rawExpected;
        expected.setAlpha(255);
    }
    QCOMPARE(qvariant_cast<KoColor>(raw.last().first()).toQColor(), rawExpected);
    if (!selected.isEmpty()) QCOMPARE(qvariant_cast<KoColor>(selected.first().first()).toQColor(), expected);
    if (!finished.isEmpty()) QCOMPARE(qvariant_cast<KoColor>(finished.first().first()).toQColor(), rawExpected);
    QCOMPARE(resources->fgColor(), foreground && transition != QStringLiteral("preview") ? KoColor(expected, cs) : oldForeground);
    QCOMPARE(resources->bgColor(), foreground || transition == QStringLiteral("preview") ? oldBackground : KoColor(expected, cs));
    if (owner) QCOMPARE(owner->operations().snapshot(), expectedVersion);
    QCOMPARE(pixels(), expectedPixels);
    if (transition != QStringLiteral("replace-raw")) {
        QCOMPARE(lifetime->undoStack()->count(), history);
        QCOMPARE(lifetime->isModified(), modified);
    }
}

KISTEST_MAIN(KisGpuColorSamplingTest)
#include "KisGpuColorSamplingTest.moc"
