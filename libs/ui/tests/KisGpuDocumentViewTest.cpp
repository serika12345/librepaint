/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <QCheckBox>
#include <QMouseEvent>
#include <QDockWidget>
#include <QEnterEvent>
#include <QListView>
#include <QItemSelectionModel>
#include <QIcon>
#include <QPointer>
#include <QSignalSpy>
#include <QStatusBar>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <QSettings>
#include <QFile>
#include <kra/KisKraRasterCodec.h>
#include <KLocalizedString>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <actions/input/KisApplicationInputActions.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <canvas/kis_canvas_controller.h>
#include <canvas/kis_zoom_manager.h>
#include <input/ui/kis_input_profile_manager.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <KoToolManager.h>
#include <KoToolBase.h>
#include <KoColorSpaceRegistry.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_paint_device.h>
#include <nodes/kis_node_manager.h>
#include <kis_canvas2.h>
#include <kundo2stack.h>
#include <brushengine/kis_paintop_registry.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <exception>
#include <memory>

class KisGpuDocumentViewTest : public QObject
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
        const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        auto bundle = KisResourceStorageSP::create(bundlePath);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(bundlePath, bundle));
        auto preset = KisPaintOpRegistry::instance()->defaultPreset(
            KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(preset);
        preset->setName(QStringLiteral("GPU document view fixture"));
        preset->setFilename(QStringLiteral("gpu-document-view-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(preset));
        m_preset = preset;
    }
    void presentsEditsAndReleasesTheClosedTab();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled();
    void replacementUpdatesPresentationInputHistoryAndSaving();
    void layerPanelEditsUseOneDocumentHistory_data();
    void layerPanelEditsUseOneDocumentHistory();
    void ordinaryLayerChangesDuringInput();
    void lineToolPreservesPreviewCancellationAndHistory_data();
    void lineToolPreservesPreviewCancellationAndHistory();
    void layerPanelSelectionSurvivesSaving();
    void layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry_data();
    void layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry();
    void layerPanelAddsAtSelectionWithOneHistoryEntry_data();
    void layerPanelAddsAtSelectionWithOneHistoryEntry();
private:
    KisPaintOpPresetSP m_preset;
};

void KisGpuDocumentViewTest::lineToolPreservesPreviewCancellationAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("reject");
    QTest::addColumn<bool>("closeAfterError");
    QTest::addColumn<bool>("rejectOnRelease");
    QTest::newRow("ordinary") << false << false << false << false;
    QTest::newRow("gpu") << true << false << false << false;
    QTest::newRow("gpu-rejected-preview") << true << true << false << false;
    QTest::newRow("gpu-close-on-error") << true << true << true << false;
    QTest::newRow("gpu-close-on-release") << true << true << true << true;
}

void KisGpuDocumentViewTest::lineToolPreservesPreviewCancellationAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, reject);
    QFETCH(bool, closeAfterError);
    QFETCH(bool, rejectOnRelease);
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreCanvas = qScopeGuard([&] {
        if (hadRenderer) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    Gpu::PaintImageSP owner;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Line input")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Line input"), 64, 64, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<KoToolBase> tool;
    QPointer<QWindow> native;
    QPointer<QWidget> input(view->canvasBase()->canvasWidget());
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (tool) tool->requestStrokeCancellation();
        if (pressed) {
            if (native) QTest::mouseRelease(native.data(), Qt::LeftButton);
            else if (input) QTest::mouseRelease(input.data(), Qt::LeftButton);
        }
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
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolLine"));
    tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolLine"));
    QVERIFY(tool);
    QPointer<QCheckBox> previewOption;
    for (QWidget *panel : tool->optionWidgets()) {
        for (auto *option : panel->findChildren<QCheckBox*>())
            if (option->text() == i18n("Show Preview")) previewOption = option;
    }
    QVERIFY(previewOption);
    const bool previousPreview = previewOption->isChecked();
    const auto restorePreview = qScopeGuard([&] {
        if (previewOption) previewOption->setChecked(previousPreview);
    });
    previewOption->setChecked(!rejectOnRelease);
    QSignalSpy errors(tool, &KoToolBase::statusTextChanged);
    if (closeAfterError) {
        connect(tool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
            if (message.isEmpty()) return;
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
    }
    input->setFocus();
    QTRY_VERIFY(input->width() >= 64 && input->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto size = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({size.width() / 2, size.height() / 2});
    if (gpu) {
        QTRY_COMPARE(execution->published(owner->handle()), owner->operations().snapshot());
        for (auto *candidate : QGuiApplication::allWindows())
            if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = candidate;
        QVERIFY(native);
    }
    const auto point = [&](QPointF position) {
        return view->canvasBase()->coordinatesConverter()->imageToWidget(position).toPoint();
    };
    const auto move = [&](QPointF position) {
        if (native) QTest::mouseMove(native.data(), point(position));
        else {
            const auto p = point(position);
            QMouseEvent event(QEvent::MouseMove, p, input->mapToGlobal(p), Qt::NoButton,
                              pressed ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(input.data(), &event);
        }
        QTest::qWait(10);
    };
    const auto press = [&](QPointF position) {
        move(position);
        const auto p = point(position);
        QEnterEvent entered(p, p, native ? native->mapToGlobal(p) : input->mapToGlobal(p));
        if (native) QApplication::sendEvent(native.data(), &entered);
        else QApplication::sendEvent(input.data(), &entered);
        pressed = true;
        if (native) QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, p);
        else QTest::mousePress(input.data(), Qt::LeftButton, Qt::NoModifier, p);
    };
    const auto release = [&](QPointF position) {
        if (native) QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(position));
        else QTest::mouseRelease(input.data(), Qt::LeftButton, Qt::NoModifier, point(position));
        pressed = false;
    };
    KisImageWSP image = lifetime->image();
    const auto layer = image->root()->firstChild();
    const auto pixel = [&](QPoint position) {
        QByteArray bytes(4, '\0');
        layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), position.x(), position.y(), 1, 1);
        return bytes;
    };
    try {
        if (reject) {
            const auto before = owner->operations().snapshot();
            resources->setCurrentCompositeOp(QStringLiteral("multiply"));
            press({16, 16});
            move({40, 16});
            if (rejectOnRelease) release({40, 16});
            else if (tool) QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
            QVERIFY(!errors.isEmpty());
            if (closeAfterError) {
                if (pressed) QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
                pressed = false;
                QTRY_VERIFY(!lifetime);
                QTRY_VERIFY(!native);
                QCOMPARE(owner->operations().snapshot(), before);
                return;
            }
            release({40, 16});
            QVERIFY(!owner->operations().hasActiveStroke());
            QCOMPARE(owner->operations().snapshot(), before);
            QCOMPARE(lifetime->undoStack()->count(), 0);
            QVERIFY(!lifetime->isModified());
            resources->setCurrentCompositeOp(QStringLiteral("normal"));
        }
        press({16, 16});
        move({40, 16});
        QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
        if (gpu) {
            QVERIFY2(owner->operations().hasActiveStroke(),
                     errors.isEmpty() ? "The line preview did not start a stroke" : qPrintable(errors.last().first().toString()));
            QCOMPARE(owner->operations().undoCount(), 0);
            QTRY_COMPARE(execution->published(owner->handle()), owner->operations().preview());
        }
        move({40, 40});
        QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
        release({40, 40});
        image->waitForDone();
        QTRY_COMPARE(lifetime->undoStack()->count(), 1);
        if (gpu) QCOMPARE(owner->operations().historyAction(1), Gpu::HistoryAction::DrawLine);
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f18111ff"));
        QCOMPARE(pixel({36, 16}), QByteArray(4, '\0'));
        lifetime->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(pixel({28, 28}), QByteArray(4, '\0'));
        lifetime->undoStack()->redo();
        image->waitForDone();
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f18111ff"));
        press({16, 48});
        move({40, 48});
        QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
        tool->requestStrokeCancellation();
        release({40, 48});
        image->waitForDone();
        QCOMPARE(lifetime->undoStack()->count(), 1);
        QCOMPARE(pixel({28, 48}), QByteArray(4, '\0'));
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        lifetime->setFileBatchMode(true);
        const auto path = directory.filePath(QStringLiteral("line.kra"));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        QTRY_VERIFY(!lifetime->isModified());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto reopened = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
        QVERIFY2(reopened.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(reopened.diagnostic));
        QCOMPARE(reopened.document.image.layers.size(), 1);
        const auto &tiles = reopened.document.image.layers.first().tiles;
        QCOMPARE(tiles.size(), 1);
        QCOMPARE(tiles.first().rgba.mid((28 * Gpu::RgbaTile::Size + 28) * 4, 4), QByteArray::fromHex("1181f1ff"));
        resources->setEraserMode(true);
        press({16, 16});
        move({40, 40});
        release({40, 40});
        image->waitForDone();
        QTRY_COMPARE(lifetime->undoStack()->count(), 2);
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f1811100"));
        lifetime->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f18111ff"));
        QTRY_VERIFY(!lifetime->isModified());
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void KisGpuDocumentViewTest::layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<int>("selection");
    for (const bool gpu : {false, true}) {
        const auto prefix = gpu ? QByteArrayLiteral("gpu-") : QByteArrayLiteral("ordinary-");
        QTest::newRow(QByteArray(prefix + "single").constData()) << gpu << 0;
        QTest::newRow(QByteArray(prefix + "multiple").constData()) << gpu << 1;
        QTest::newRow(QByteArray(prefix + "all").constData()) << gpu << 2;
    }
}

void KisGpuDocumentViewTest::layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry()
{
    QFETCH(bool, gpu);
    QFETCH(int, selection);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer removal")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer removal"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    QVERIFY(window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox")));
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
    auto image = KisImageSP(lifetime->image());
    const auto root = image->root();
    KisNodeList originals{root->at(0), root->at(1), root->at(2)};
    const auto pixel = QByteArray::fromHex("332211ff");
    for (const auto &node : originals)
        node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 5, 5, 1, 1);
    auto *manager = window->viewManager()->nodeManager();
    const KisNodeList selected = selection == 0 ? KisNodeList{originals[1]} :
        selection == 1 ? KisNodeList{originals[0], originals[2]} : originals;
    const auto active = selected.last();
    manager->slotUiActivatedNode(active);
    manager->slotSetSelectedNodes(selected);
    const auto before = lifetime->undoStack()->count();
    try {
        manager->removeNode();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), selection == 0 ? 2U : 1U);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QTRY_VERIFY(lifetime->isModified());
    for (const auto &node : selected) QVERIFY(!node->parent());
    QTRY_VERIFY(view->currentNode() && view->currentNode()->parent() == root);
    const auto after = view->currentNode();
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 3U);
    for (int i = 0; i < originals.size(); ++i) QCOMPARE(root->at(i), originals[i]);
    QTRY_COMPARE(view->currentNode(), active);
    QTRY_COMPARE(manager->selectedNodes().size(), selected.size());
    for (const auto &node : selected) QVERIFY(manager->selectedNodes().contains(node));
    QByteArray actual(4, '\0');
    active->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
    QCOMPARE(actual, pixel);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), selection == 0 ? 2U : 1U);
    QTRY_COMPARE(view->currentNode(), after);
}

void KisGpuDocumentViewTest::layerPanelAddsAtSelectionWithOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("pinned");
    QTest::newRow("ordinary") << false << false;
    QTest::newRow("gpu") << true << false;
    QTest::newRow("ordinary-pinned") << false << true;
    QTest::newRow("gpu-pinned") << true << true;
}

void KisGpuDocumentViewTest::layerPanelAddsAtSelectionWithOneHistoryEntry()
{
    QFETCH(bool, gpu);
    QFETCH(bool, pinned);
    const bool previousPin = KisConfig(true).autoPinLayersToTimeline();
    const auto restorePin = qScopeGuard([&] { KisConfig(false).setAutoPinLayersToTimeline(previousPin); });
    KisConfig(false).setAutoPinLayersToTimeline(pinned);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer creation")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer creation"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
    }
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
    auto image = KisImageSP(lifetime->image());
    const auto root = image->root();
    const auto middle = root->at(1);
    const auto top = root->at(2);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(middle);
    QCOMPARE(view->currentNode(), middle);
    const auto before = lifetime->undoStack()->count();
    KisNodeSP added;
    try {
        added = manager->createNode(QStringLiteral("KisPaintLayer"));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QVERIFY(added);
    image->waitForDone();
    QTRY_COMPARE(view->currentNode(), added);
    QCOMPARE(root->childCount(), 4U);
    QCOMPARE(root->at(1), middle);
    QCOMPARE(root->at(2), added);
    QCOMPARE(root->at(3), top);
    QCOMPARE(added->isPinnedToTimeline(), pinned);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QVERIFY(lifetime->isModified());
    const auto pixel = QByteArray::fromHex("332211ff");
    added->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 5, 5, 1, 1);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(root->childCount(), 3U);
    QVERIFY(!added->parent());
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(root->at(2), added);
    QTRY_COMPARE(view->currentNode(), added);
    QByteArray actual(4, '\0');
    added->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
    QCOMPARE(actual, pixel);
}

void KisGpuDocumentViewTest::layerPanelSelectionSurvivesSaving()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    QVERIFY(owner->addLayer(QStringLiteral("Middle")));
    QVERIFY(owner->addLayer(QStringLiteral("Top")));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    const auto undoCount = owner->operations().undoCount();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selected layer")));
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
    QTRY_VERIFY(view->currentNode());
    QCOMPARE(view->currentNode()->uuid(), saved->layers.last().uuid);
    const auto middle = lifetime->image()->root()->at(1);
    QVERIFY(middle);
    window->viewManager()->nodeManager()->slotUiActivatedNode(middle);
    QCOMPARE(view->currentNode(), middle);
    QCOMPARE(lifetime->preActivatedNode(), middle);
    QCOMPARE(owner->operations().activeLayer(), saved->layers[1].id);
    QCOMPARE(owner->operations().undoCount(), undoCount);
    QVERIFY(!owner->operations().isModified());
    QVERIFY(!lifetime->isModified());
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("selected.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.activeLayer, 1);
    QCOMPARE(loaded.document.image.layers[1].uuid, middle->uuid());
    const auto reopened = Gpu::PaintImageSP::create(execution, loaded.document.image);
    lifetime->setCurrentImage(KisImage::fromGpuImage(reopened, QStringLiteral("Reopened selection")));
    QTRY_VERIFY(view->currentNode());
    QTRY_COMPARE(view->currentNode()->uuid(), loaded.document.image.layers[1].uuid);
    QCOMPARE(reopened->operations().activeLayer(), reopened->operations().snapshot()->layers[1].id);
    QCOMPARE(reopened->operations().undoCount(), 0);
    QVERIFY(!lifetime->isModified());
}

void KisGpuDocumentViewTest::layerPanelEditsUseOneDocumentHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("locked");
    QTest::newRow("ordinary") << false << false;
    QTest::newRow("gpu") << true << false;
    QTest::newRow("ordinary-lock") << false << true;
    QTest::newRow("gpu-lock") << true << true;
}

void KisGpuDocumentViewTest::layerPanelEditsUseOneDocumentHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, locked);
    std::shared_ptr<LibrePaint::Image::Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        auto device = LibrePaint::Image::Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<LibrePaint::Image::Gpu::Execution>(device);
        const auto owner = LibrePaint::Image::Gpu::PaintImageSP::create(execution, QSize(32, 16));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer panel editing")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Layer panel editing"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
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
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    const auto originalName = layer->name();
    const auto red = QByteArray::fromHex("0000ffff");
    layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(red.constData()), 5, 5, 1, 1);
    auto *manager = window->viewManager()->nodeManager();
    try {
        manager->setNodeName(layer, QStringLiteral("Adjusting"));
        image->waitForDone();
        manager->setNodeName(layer, QStringLiteral("Renamed"));
        image->waitForDone();
        manager->setNodeOpacity(layer, 192);
        image->waitForDone();
        manager->setNodeOpacity(layer, 128);
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QCOMPARE(layer->name(), QStringLiteral("Renamed"));
    QCOMPARE(layer->opacity(), quint8(128));
    QCOMPARE(lifetime->undoStack()->count(), 2);
    if (gpu) {
        QCOMPARE(lifetime->undoStack()->command(0)->text().toString(), i18n("Rename layer"));
        QCOMPARE(lifetime->undoStack()->command(1)->text().toString(), i18n("Change layer opacity"));
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(layer->opacity(), quint8(255));
    QCOMPARE(layer->name(), QStringLiteral("Renamed"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(layer->name(), originalName);
    lifetime->undoStack()->redo();
    image->waitForDone();
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(layer->name(), QStringLiteral("Renamed"));
    QCOMPARE(layer->opacity(), quint8(128));
    image->refreshGraphAsync();
    image->waitForDone();
    QByteArray pixel(4, '\0');
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 5, 5, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("0000ff80"));
    QCOMPARE(manager->activeNode(), layer);
    const auto toggle = [&] {
        if (locked) manager->toggleLock();
        else manager->toggleVisibility();
    };
    const auto propertySet = [&] { return locked ? layer->userLocked() : !layer->visible(); };
    try {
        toggle();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QVERIFY(propertySet());
    QCOMPARE(lifetime->undoStack()->count(), 3);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 5, 5, 1, 1);
    QCOMPARE(pixel, locked ? QByteArray::fromHex("0000ff80") : QByteArray(4, '\0'));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QVERIFY(!propertySet());
    lifetime->undoStack()->redo();
    image->waitForDone();
    QVERIFY(propertySet());
    lifetime->undoStack()->undo();
    image->waitForDone();
    QVERIFY(!propertySet());
    lifetime->undoStack()->redo();
    image->waitForDone();
    try {
        toggle();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QVERIFY(!propertySet());
    QCOMPARE(lifetime->undoStack()->count(), 2);
    if (gpu) {
        manager->setNodeName(layer, QStringLiteral("Temporary"));
        manager->setNodeName(layer, QStringLiteral("Renamed"));
        QCOMPARE(lifetime->undoStack()->count(), 3);
        QCOMPARE(lifetime->undoStack()->command(2)->text().toString(), i18n("Rename layer"));
    }
}

void KisGpuDocumentViewTest::ordinaryLayerChangesDuringInput()
{
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreCanvas = qScopeGuard([&] {
        if (hadRenderer) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(document->newImage(QStringLiteral("Layer change during input"), 64, 64, cs,
                               KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<QWidget> input = view->canvasBase()->canvasWidget();
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (pressed && input) QTest::mouseRelease(input, Qt::LeftButton);
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    window->viewManager()->setCurrentView(view);
    image->setWorkingThreadsLimit(1);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
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
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QVERIFY(brushTool->setProperty("smoothingType", 0));
    input->setFocus();
    QTRY_VERIFY(input->width() >= 64 && input->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto imageSize = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    const auto first = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(16, 16)).toPoint();
    const auto last = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(40, 40)).toPoint();
    QEnterEvent entered(first, first, input->mapToGlobal(first));
    QApplication::sendEvent(input, &entered);
    QTest::mouseMove(input, first);
    QTest::qWait(10);
    pressed = true;
    QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, first);
    QTest::mouseMove(input, last);
    QTest::qWait(10);
    QTimer release;
    release.setSingleShot(true);
    connect(&release, &QTimer::timeout, this, [&] {
        QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, last);
        pressed = false;
    });
    release.start(0);
    window->viewManager()->nodeManager()->toggleVisibility();
    QVERIFY(!pressed);
    QCOMPARE(lifetime->undoStack()->count(), 2);
    release.stop();
    if (pressed) QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    image->waitForDone();
    QByteArray pixel(4, '\0');
    QVERIFY(!layer->visible());
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 40, 40, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("f18111ff"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QVERIFY(layer->visible());
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 40, 40, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("f18111ff"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), 0);
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 40, 40, 1, 1);
    QCOMPARE(pixel, QByteArray(4, '\0'));
}

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data()
{
    QTest::addColumn<bool>("exhaustInput");
    QTest::addColumn<bool>("closeAfterError");
    QTest::addColumn<bool>("hideDuringStroke");
    QTest::addColumn<bool>("selectDuringStroke");
    QTest::newRow("paint-and-cancel") << false << false << false << false;
    QTest::newRow("input-limit") << true << false << false << false;
    QTest::newRow("close-on-input-limit") << true << true << false << false;
    QTest::newRow("hide-during-stroke") << false << false << true << false;
    QTest::newRow("select-during-stroke") << false << false << false << true;
}

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled()
{
    QFETCH(bool, exhaustInput);
    QFETCH(bool, closeAfterError);
    QFETCH(bool, hideDuringStroke);
    QFETCH(bool, selectDuringStroke);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::Limits limits;
    if (exhaustInput) limits.samplesPerStroke = 4;
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64), limits);
    if (selectDuringStroke) QVERIFY(owner->addLayer(QStringLiteral("Paint target")));
    const auto paintTarget = owner->operations().activeLayer();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU freehand input")));
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
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QVERIFY(brushTool->setProperty("smoothingType", 0));
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
    auto first = converter->imageToWidget(QPointF(16, 16)).toPoint();
    auto last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(first));
    QEnterEvent entered(first, first, native->mapToGlobal(first));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mouseMove(native.data(), first);
    QTest::qWait(10);
    first = converter->imageToWidget(QPointF(16, 16)).toPoint();
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
    if (!exhaustInput) {
        resources->setSize(16);
        resources->setFGColor(KoColor(QColor(221, 17, 44), KoColorSpaceRegistry::instance()->rgb8()));
    }
    QTest::qWait(10);
    last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    if (exhaustInput) {
        for (int step = 1; step <= 8 && owner->operations().hasActiveStroke(); ++step)
            QTest::mouseMove(native.data(), first + (last - first) * step / 8);
    } else {
        QTest::mouseMove(native.data(), last);
    }
    QTest::qWait(10);
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
                 QByteArray::fromHex("1181f1ff"));
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
                 QByteArray::fromHex("1181f1ff"));
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
    QTRY_COMPARE(owner->execution().published(owner->handle()), painted);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(45, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("00000000"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().undoCount(), 0);
    documentLifetime->undoStack()->redo();
    QCOMPARE(owner->operations().snapshot(), painted);

    resources->setSize(8);
    resources->setEraserMode(true);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    QVERIFY(owner->operations().hasActiveStroke());
    resources->setEraserMode(false);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("1181f100"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().snapshot(), painted);

    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    const auto cancelled = converter->imageToWidget(QPointF(48, 16)).toPoint();
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
}

void KisGpuDocumentViewTest::replacementUpdatesPresentationInputHistoryAndSaving()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    const auto previous = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto sourceOwner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto layer = sourceOwner->operations().activeLayer();
    QCOMPARE(sourceOwner->replaceRgbaPixels(layer, QRect(8, 8, 1, 1), QByteArray::fromHex("4488ccff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    source->setCurrentImage(KisImage::fromGpuImage(sourceOwner, QStringLiteral("Replacement source")));
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(previous, QStringLiteral("Replacement target")));
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
    auto *identity = view->image().data();
    documentLifetime->copyFromDocument(*source);
    QCOMPARE(view->image().data(), identity);
    const auto copied = view->image()->gpuImage();
    QVERIFY(copied != previous);
    QVERIFY(copied != sourceOwner);
    QTRY_COMPARE(execution.published(copied->handle()), copied->operations().snapshot());
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
    QTRY_VERIFY(view->currentNode());
    QTRY_COMPARE(view->currentNode()->uuid(), copied->operations().snapshot()->layers.first().uuid);
    const auto point = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(32, 32)).toPoint();
    QEnterEvent entered(point, point, native->mapToGlobal(point));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mouseMove(native.data(), point);
    QTest::qWait(10);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point);
    QVERIFY(copied->operations().hasActiveStroke());
    QVERIFY(!previous->operations().hasActiveStroke());
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

void KisGpuDocumentViewTest::presentsEditsAndReleasesTheClosedTab()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
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

KISTEST_MAIN(KisGpuDocumentViewTest)
#include "KisGpuDocumentViewTest.moc"
