/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <testui.h>
#include <QBuffer>
#include <QPointer>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QScreen>
#include <QFile>
#include <QDir>
#include <KLocalizedString>
#include <KoColorSpaceRegistry.h>
#include <KoChannelInfo.h>
#include <KisResourceStorage.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisGlobalResourcesInterface.h>
#include <brushengine/kis_paintop_registry.h>
#include <kis_paintop_preset.h>
#include <kis_config.h>
#include <kis_config_notifier.h>
#include <kis_debug.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/ui/workspace/KisView.h>
#include <actions/input/KisApplicationInputActions.h>
#include <input/ui/kis_input_profile_manager.h>
#include <opengl/kis_opengl.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <memory>
#include "../ChannelDockerWidget.h"
#include "../channelmodel.h"

namespace Gpu = LibrePaint::Image::Gpu;
namespace Files = LibrePaint::Document::Gpu;

class TestChannelDocker : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void displaySelectionPreservesEditingAndSaving_data();
    void displaySelectionPreservesEditingAndSaving();
};

void TestChannelDocker::initTestCase()
{
    Q_INIT_RESOURCE(krita);
    QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
    QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
    KLocalizedString::setApplicationDomain("krita");
    KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
    KisInputProfileManager::instance()->setActions(createApplicationInputActions());
    const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
    auto bundle = KisResourceStorageSP::create(bundlePath);
    QVERIFY(bundle->valid());
    QVERIFY(KisResourceLocator::instance()->addStorage(bundlePath, bundle));
    auto preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
    QVERIFY(preset);
    preset->setName(QStringLiteral("Channel panel window fixture"));
    preset->setFilename(QStringLiteral("channel-panel-window-fixture.kpp"));
    QImage preview(16, 16, QImage::Format_ARGB32);
    preview.fill(Qt::black);
    preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets);
    QVERIFY(presets.addResource(preset));
}

void TestChannelDocker::displaySelectionPreservesEditingAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QSize>("size");
    QTest::newRow("ordinary") << false << QSize(32, 16);
    QTest::newRow("gpu") << true << QSize(32, 16);
    QTest::newRow("gpu-large") << true << QSize(4096, 1024);
}

void TestChannelDocker::displaySelectionPreservesEditingAndSaving()
{
    // The normal panel keeps view selection responsive during painting and
    // releases its rows when the source view closes. Fixed-version saving
    // observes document pixels independently of those display settings.
    QFETCH(bool, gpu);
    QFETCH(QSize, size);
    const bool previousAsColor = KisConfig(true).showSingleChannelAsColor();
    KisConfig(true).setShowSingleChannelAsColor(false);
    KisConfig(true).disableOpenGL();
    const auto restoreConfig = qScopeGuard([&] {
        KisConfig(true).setShowSingleChannelAsColor(previousAsColor);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    KisImageSP image;
    std::unique_ptr<Files::PaintDocument> fileOwner;
    QByteArray encoded;
    const auto encode = [&](QByteArray &bytes) {
        QBuffer output(&bytes);
        if (!output.open(QIODevice::ReadWrite)) return false;
        return fileOwner->exportKra(output).code == Files::FileCode::Success;
    };
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage input;
        input.size = size;
        input.layers = {{QStringLiteral("Channels"), true,
            QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")),
            {{{0, 0}, QByteArray::fromHex("4995e780").repeated(Gpu::RgbaTile::ByteCount / 4)}}}};
        input.layers[0].defaultPixel = {73, 149, 231, 128};
        owner = Gpu::PaintImageSP::create(execution, input);
        image = KisImage::fromGpuImage(owner, QStringLiteral("Display selection"));
        fileOwner = std::make_unique<Files::PaintDocument>(owner);
        QVERIFY(encode(encoded));
    } else {
        image = new KisImage(document->createUndoStore(), size.width(), size.height(), KoColorSpaceRegistry::instance()->rgb8(), "Display selection");
        auto layer = KisPaintLayerSP(new KisPaintLayer(image, QStringLiteral("Channels"), 255));
        image->addNode(layer);
        const auto raw = QByteArray::fromHex("e7954980").repeated(size.width() * size.height());
        layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(raw.constData()), image->bounds());
        image->refreshGraphAsync();
        image->waitForDone();
    }
    document->setCurrentImage(image);
    document->setModified(false);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    ChannelDockerWidget panel;
    const auto cleanup = qScopeGuard([&] {
        panel.setCanvas(nullptr);
        panel.hide();
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.get()));
    panel.setCanvas(view->canvasBase());
    panel.show();
    QVERIFY(panel.isEnabled());
    auto *model = panel.findChild<ChannelModel *>();
    QVERIFY(model);
    QTRY_COMPARE_WITH_TIMEOUT(model->rowCount(), 4, 10000);
    const auto channels = image->colorSpace()->channels();
    const int values[4] = {73, 149, 231, 128};
    int red = -1;
    for (int row = 0; row < 4; ++row) {
        const auto index = model->index(row, 1);
        QTRY_VERIFY_WITH_TIMEOUT(!qvariant_cast<QImage>(model->data(index, Qt::DecorationRole)).isNull(), 10000);
        const auto thumbnail = qvariant_cast<QImage>(model->data(index, Qt::DecorationRole));
        const auto position = channels[row]->displayPosition();
        QCOMPARE(thumbnail.format(), QImage::Format_Grayscale8);
        QCOMPARE(qRed(thumbnail.pixel(thumbnail.width() / 2, thumbnail.height() / 2)), values[position]);
        if (position == 0) red = row;
    }
    QVERIFY(red >= 0);
#ifdef Q_OS_LINUX
    const auto displayed = [&] {
        for (auto *window : QGuiApplication::allWindows()) {
            auto *native = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(window);
            if (!native || !native->isExposed() || native->documentViewport().isEmpty()) continue;
            const auto center = QPointF(native->documentViewport().center()) + QPointF(.5, .5);
            const auto point = native->documentToWindow().map(center).toPoint();
            const auto pixels = native->screen()->grabWindow(native->winId()).toImage();
            return pixels.rect().contains(point) ? pixels.pixelColor(point) : QColor{};
        }
        return QColor{};
    };
    if (owner) QTRY_COMPARE_WITH_TIMEOUT(displayed(), QColor(164, 202, 243), 5000);
#endif
    const auto before = owner ? owner->operations().snapshot() : Gpu::Snapshot{};
    Gpu::OperationId stroke;
    QByteArray recovery;
    if (owner) {
        stroke = owner->beginStroke(owner->operations().activeLayer(), Gpu::Brush{}, 29);
        QVERIFY(!stroke.isNull());
        QCOMPARE(owner->appendSamples(stroke, {{{24, 12}, 1}}), Gpu::Result::Success);
        recovery = owner->operations().recoveryData();
    }
    const auto withoutWaitingForInput = [&](auto change) {
        if (!owner) { change(); return true; }
        QObject callbackLifetime;
        bool enteredWait = false;
        QTimer::singleShot(0, &callbackLifetime, [&] {
            enteredWait = true;
            qWarning().noquote() << "View changes entered an input wait:" << kisBacktrace();
            owner->cancelStroke(stroke);
        });
        change();
        return !enteredWait;
    };
    bool changed = false;
    QVERIFY(withoutWaitingForInput([&] {
        changed = model->setData(model->index(red, 0), Qt::Unchecked, Qt::CheckStateRole);
    }));
    QVERIFY(changed);
    QVERIFY(!image->rootLayer()->channelFlags().testBit(red));
#ifdef Q_OS_LINUX
    if (owner) QTRY_COMPARE_WITH_TIMEOUT(displayed(), QColor(127, 202, 243), 5000);
#endif
    QVERIFY(withoutWaitingForInput([&] { model->rowActivated(model->index(red, 0)); }));
    for (int row = 0; row < 4; ++row) if (channels[row]->channelType() == KoChannelInfo::COLOR)
        QCOMPARE(image->rootLayer()->channelFlags().testBit(row), row == red);
#ifdef Q_OS_LINUX
    if (owner) QTRY_COMPARE_WITH_TIMEOUT(displayed(), QColor(164, 164, 164), 5000);
#endif
    KisConfig(true).setShowSingleChannelAsColor(true);
    QVERIFY(withoutWaitingForInput([&] { KisConfigNotifier::instance()->notifyConfigChanged(); }));
#ifdef Q_OS_LINUX
    if (owner) QTRY_COMPARE_WITH_TIMEOUT(displayed(), QColor(164, 127, 127), 5000);
#endif
    if (owner) {
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(owner->operations().activeOperation(), stroke);
        QCOMPARE(owner->operations().recoveryData(), recovery);
        QByteArray after;
        QVERIFY(encode(after));
        QCOMPARE(after, encoded);
        QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!directory.isEmpty()) {
            QVERIFY(QDir().mkpath(directory));
            const QString name = QStringLiteral("display-channels-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kra");
            QFile output(QDir(directory).filePath(name));
            QVERIFY(output.open(QIODevice::WriteOnly));
            QCOMPARE(output.write(after), after.size());
        }
    }
    panel.setCanvas(nullptr);
    QCOMPARE(model->rowCount(), 0);
    panel.setCanvas(view->canvasBase());
    QVERIFY(panel.isEnabled());
    QTRY_COMPARE_WITH_TIMEOUT(model->rowCount(), 4, 10000);
    if (owner) {
        const QPersistentModelIndex retained(model->index(red, 0));
        QPointer<KisCanvas2> canvas(view->canvasBase());
        lifetime->setModified(false);
        window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        QTRY_VERIFY(canvas.isNull());
        QCOMPARE(model->rowCount(), 0);
        QVERIFY(!retained.isValid());
    }
}

KISTEST_MAIN(TestChannelDocker)
#include "TestChannelDocker.moc"
