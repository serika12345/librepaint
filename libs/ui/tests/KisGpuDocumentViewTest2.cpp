/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDocumentViewTest.h"

void KisGpuDocumentViewTest::layerSelectionCanCloseTheWindowDuringNotification()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    Gpu::PaintImageSP owner;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        owner = paintImageWithEncoding(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Closing during selection")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Closing during selection"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 2, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    KisImageWSP imageLifetime = lifetime->image();
    auto selected = lifetime->image()->root()->firstChild();
    QPointer<KisNodeManager> manager = window->viewManager()->nodeManager();
    QMetaObject::Connection close;
    const auto closeDuringSelection = [&] {
        QObject::disconnect(close);
        lifetime->setModified(false);
        window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        window.reset();
    };
    close = gpu ? connect(owner.data(), &Gpu::PaintImage::changed, window.get(), closeDuringSelection)
                : connect(manager.data(), &KisNodeManager::sigUiNeedChangeSelectedNodes, window.get(), closeDuringSelection);
    manager->slotSetSelectedNodes({selected});
    QVERIFY(!manager);
    QVERIFY(!window);
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(!lifetime);
    QTRY_VERIFY(!imageLifetime.isValid());
    const auto handle = owner ? owner->handle() : 0;
    selected.clear();
    owner.clear();
    if (execution) QVERIFY(!execution->requested(handle));
}

void KisGpuDocumentViewTest::layerPanelSelectionSurvivesSaving()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
    QPointer<QDockWidget> layerPanel;
    for (auto *dock : window->findChildren<QDockWidget*>())
        if (dock->metaObject()->indexOfSlot("slotNodeCollapsedChanged()") >= 0) layerPanel = dock;
    QVERIFY(layerPanel);
    QSignalSpy panelImageChanged(layerPanel, SIGNAL(imageChanged()));
    QVERIFY(panelImageChanged.isValid());
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
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    lifetime->setCurrentImage(KisImage::fromGpuImage(reopened, QStringLiteral("Reopened selection")));
    QTRY_VERIFY(view->currentNode());
    QTRY_COMPARE(view->currentNode()->uuid(), loaded.document.image.layers[1].uuid);
    QCOMPARE(reopened->operations().activeLayer(), reopened->operations().snapshot()->layers[1].id);
    QCOMPARE(reopened->operations().undoCount(), 0);
    QVERIFY(!lifetime->isModified());
    QVERIFY(QMetaObject::invokeMethod(layerPanel, "slotNodeCollapsedChanged", Qt::DirectConnection));
    QTRY_COMPARE(panelImageChanged.size(), 1);
    QVERIFY(QMetaObject::invokeMethod(layerPanel, "slotNodeCollapsedChanged", Qt::DirectConnection));
    QCOMPARE(panelImageChanged.size(), 1);
    auto *list = layerPanel->findChild<QAbstractItemView*>(QStringLiteral("listLayers"));
    QVERIFY(list);
    auto *model = qobject_cast<KisNodeFilterProxyModel*>(list->model());
    QVERIFY(model);
    const auto newMiddle = lifetime->image()->root()->at(1);
    QVERIFY(newMiddle != middle);
    const auto index = model->indexFromNode(newMiddle);
    QVERIFY(index.isValid());
    QCOMPARE(model->nodeFromIndex(index), newMiddle);
    QVERIFY(!model->indexFromNode(middle).isValid());
    QVERIFY(model->setData(index, QStringLiteral("Reopened middle"), Qt::EditRole));
    QCOMPARE(newMiddle->name(), QStringLiteral("Reopened middle"));
    QCOMPARE(reopened->operations().snapshot()->layers[1].name, QStringLiteral("Reopened middle"));
    QCOMPARE(owner->operations().snapshot()->layers[1].name, QStringLiteral("Middle"));
    QCOMPARE(reopened->operations().undoCount(), 1);

}

void KisGpuDocumentViewTest::layerPanelEditsUseOneDocumentHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("locked");
    QTest::addColumn<bool>("alphaLocked");
    QTest::newRow("ordinary") << false << false << false;
    QTest::newRow("gpu-alpha-lock") << true << false << true;
}

void KisGpuDocumentViewTest::layerPanelEditsUseOneDocumentHistory()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    QFETCH(bool, locked);
    QFETCH(bool, alphaLocked);
    std::shared_ptr<LibrePaint::Image::Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        auto device = LibrePaint::Image::Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<LibrePaint::Image::Gpu::Execution>(device);
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
        if (alphaLocked) manager->toggleAlphaLock();
        else if (locked) manager->toggleLock();
        else manager->toggleVisibility();
    };
    const auto propertySet = [&] {
        return alphaLocked ? qobject_cast<const KisPaintLayer*>(layer.data())->alphaLocked() :
                             locked ? layer->userLocked() : !layer->visible();
    };
    try {
        toggle();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QVERIFY(propertySet());
    QCOMPARE(lifetime->undoStack()->count(), 3);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 5, 5, 1, 1);
    QCOMPARE(pixel, locked || alphaLocked ? QByteArray::fromHex("0000ff80") : QByteArray(4, '\0'));
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

void KisGpuDocumentViewTest::transparencyMaskActionsUseTheActiveGpuPaintLayer()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Transparency mask actions"));
    QVERIFY(image);
    const auto node = image->root()->firstChild();
    QVERIFY(node);
    const auto layer = owner->operations().activeLayer();
    QVERIFY(image->setGpuSelectionRect(QRect(32, 32, 64, 64)));
    QSignalSpy changes(owner.data(), &Gpu::PaintImage::changed);

    QVERIFY(!owner->layerTransparencyMask(layer));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(node));
    QCOMPARE(changes.size(), 1);
    auto version = owner->operations().snapshot();
    QVERIFY(version->layers.first().transparencyMask);
    QCOMPARE(version->layers.first().transparencyMask->bounds(), QRect(32, 32, 64, 64));
    QVERIFY(image->gpuTransparencyMaskVisible(node));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::AddTransparencyMask);

    QVERIFY(image->setGpuTransparencyMaskVisible(node, false));
    QVERIFY(!owner->layerTransparencyMaskVisible(layer));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::ChangeMaskVisibility);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->layerTransparencyMaskVisible(layer));
    QVERIFY(image->gpuTransparencyMaskVisible(node));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(!owner->layerTransparencyMaskVisible(layer));

    QVERIFY(image->removeGpuTransparencyMask(node));
    QVERIFY(!owner->layerTransparencyMask(layer));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::RemoveTransparencyMask);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->layerTransparencyMask(layer));

    // Adding without a selection is rejected without adding history.
    QVERIFY(image->removeGpuTransparencyMask(node));
    QVERIFY(image->clearGpuSelection());
    const auto count = owner->operations().undoCount();
    QVERIFY(!image->addGpuTransparencyMaskFromSelection(node));
    QCOMPARE(owner->operations().undoCount(), count);
    QVERIFY(!owner->operations().snapshot()->layers.first().transparencyMask);
}

void KisGpuDocumentViewTest::invertColorsActionUsesGpuDocumentAndDiagnosesUnsupportedFilters()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!KisFilterRegistry::instance()->value(QStringLiteral("invert")))
        KisFilterRegistry::instance()->add(KisFilterSP(new GpuDocumentInvertFilter));
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->fillLayer(layer, {200, 30, 70}), Gpu::Result::Success);
    QCOMPARE(owner->setSelectionRect(QRect(16, 16, 32, 32)), Gpu::Result::Success);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Invert Colors action")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto *manager = window->viewManager()->nodeManager();
    const auto node = view->image()->root()->firstChild();
    QVERIFY(node);
    manager->slotUiActivatedNode(node);

    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("krita_filter_invert"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto count = lifetime->undoStack()->count();
    QCOMPARE(owner->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::InvertColors);
    const auto inverted = owner->readRgbaPixels(layer, QRect(0, 0, 64, 64), 64 * 64 * 4);
    QCOMPARE(inverted.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(inverted.bytes, 16, 16), QByteArray::fromHex("37e1b9ff"));
    QCOMPARE(pixel(inverted.bytes, 47, 47), QByteArray::fromHex("37e1b9ff"));
    QCOMPARE(pixel(inverted.bytes, 48, 48), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(inverted.bytes, 8, 8), QByteArray::fromHex("c81e46ff"));

    // The action's undo entry restores the unfiltered pixels and redo reapplies.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes == QByteArray::fromHex("c81e46ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("37e1b9ff"));

    // Missing plugins report a diagnostic and preserve the current version.
    QSignalSpy messages(window->viewManager(), &KisViewManager::floatingMessageRequested);
    QVERIFY(messages.isValid());
    KisFilterConfigurationSP unsupported(new KisFilterConfiguration(
        QStringLiteral("missing-filter-plugin"), 1, KisGlobalResourcesInterface::instance()));
    const auto beforeUnsupported = owner->operations().snapshot();
    window->viewManager()->filterManager()->apply(unsupported);
    QTRY_COMPARE(messages.size(), 1);
    QVERIFY(messages.first().at(0).toString().contains(QStringLiteral("could not be found")));
    QCOMPARE(owner->operations().snapshot(), beforeUnsupported);
    QCOMPARE(lifetime->undoStack()->count(), count + 1);
}

void KisGpuDocumentViewTest::desaturateActionAppliesGrayscaleOnGpuDocument()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QVERIFY(KisFilterRegistry::instance()->value(QStringLiteral("desaturate")));
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->fillLayer(layer, {200, 30, 70}), Gpu::Result::Success);
    QCOMPARE(owner->setSelectionRect(QRect(16, 16, 32, 32)), Gpu::Result::Success);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Desaturate action")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto *manager = window->viewManager()->nodeManager();
    const auto node = view->image()->root()->firstChild();
    QVERIFY(node);
    manager->slotUiActivatedNode(node);

    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("krita_filter_desaturate"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto count = lifetime->undoStack()->count();
    QCOMPARE(owner->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::Grayscale);
    const auto filtered = owner->readRgbaPixels(layer, QRect(0, 0, 64, 64), 64 * 64 * 4);
    QCOMPARE(filtered.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    // Rec.709 luminance of (200, 30, 70) is 69 (0x45).
    QCOMPARE(pixel(filtered.bytes, 16, 16), QByteArray::fromHex("454545ff"));
    QCOMPARE(pixel(filtered.bytes, 47, 47), QByteArray::fromHex("454545ff"));
    QCOMPARE(pixel(filtered.bytes, 48, 48), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(filtered.bytes, 8, 8), QByteArray::fromHex("c81e46ff"));

    // The action's undo entry restores the unfiltered pixels and redo reapplies.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes == QByteArray::fromHex("c81e46ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("454545ff"));
}

void KisGpuDocumentViewTest::hsvAdjustmentActionUsesProductSettingsOnGpuDocument()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QVERIFY(KisFilterRegistry::instance()->value(QStringLiteral("hsvadjustment")));
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->fillLayer(layer, {200, 30, 70}), Gpu::Result::Success);
    QCOMPARE(owner->setSelectionRect(QRect(16, 16, 32, 32)), Gpu::Result::Success);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("HSV Adjustment action")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto *manager = window->viewManager()->nodeManager();
    const auto node = view->image()->root()->firstChild();
    QVERIFY(node);
    manager->slotUiActivatedNode(node);

    auto *action = window->viewManager()->actionManager()->actionByName(
        QStringLiteral("krita_filter_hsvadjustment"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto count = lifetime->undoStack()->count();
    QCOMPARE(owner->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }

    // The action opens the product settings dialog and starts no CPU filter
    // stroke against the view's paint-device copy.
    QDialog *dialog = nullptr;
    QTRY_VERIFY((dialog = window->findChild<QDialog *>(QStringLiteral("gpuHsvAdjustmentDialog"))));
    QVERIFY(dialog);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
    auto *hue = dialog->findChild<QSpinBox *>(QStringLiteral("hSpinBox"));
    auto *saturation = dialog->findChild<QSpinBox *>(QStringLiteral("sSpinBox"));
    auto *value = dialog->findChild<QSpinBox *>(QStringLiteral("vSpinBox"));
    auto *type = dialog->findChild<QComboBox *>(QStringLiteral("cmbType"));
    auto *colorize = dialog->findChild<QCheckBox *>(QStringLiteral("chkColorize"));
    auto *compatibility = dialog->findChild<QCheckBox *>(QStringLiteral("chkCompatibilityMode"));
    QVERIFY(hue);
    QVERIFY(saturation);
    QVERIFY(value);
    QVERIFY(type);
    QVERIFY(colorize);
    QVERIFY(compatibility);
    hue->setValue(60);
    saturation->setValue(0);
    value->setValue(0);
    type->setCurrentIndex(0);
    colorize->setChecked(false);
    compatibility->setChecked(false);
    auto *buttons = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(buttons);
    buttons->button(QDialogButtonBox::Ok)->click();

    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()), Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(lifetime->undoStack()->undoText(), kundo2_i18n("HSV Adjustment").toString());
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
    const auto filtered = owner->readRgbaPixels(layer, QRect(0, 0, 64, 64), 64 * 64 * 4);
    QCOMPARE(filtered.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    // Hue +60 on (200,30,70) is (200,160,30); the selection clips the result.
    QCOMPARE(pixel(filtered.bytes, 16, 16), QByteArray::fromHex("c8a01eff"));
    QCOMPARE(pixel(filtered.bytes, 47, 47), QByteArray::fromHex("c8a01eff"));
    QCOMPARE(pixel(filtered.bytes, 48, 48), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(filtered.bytes, 8, 8), QByteArray::fromHex("c81e46ff"));

    // The action's undo entry restores the unfiltered pixels and redo reapplies.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("c8a01eff"));

    // A configuration without a `type` key follows the product default (HSL).
    // The same saturation value differs from the HSV rule asserted above.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    KisFilterConfigurationSP defaultMode(new KisFilterConfiguration(
        QStringLiteral("hsvadjustment"), 1, KisGlobalResourcesInterface::instance()));
    defaultMode->setProperty("h", 0);
    defaultMode->setProperty("s", 50);
    defaultMode->setProperty("v", 0);
    defaultMode->setProperty("compatibilityMode", false);
    window->viewManager()->filterManager()->apply(defaultMode);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()), Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("e60036ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));

    // HSY, YUV and Colorize are applied through the GPU path. The UI freezes
    // the document color space's luma coefficients at apply time; no CPU
    // filter stroke runs against the view's paint-device copy.
    QSignalSpy messages(window->viewManager(), &KisViewManager::floatingMessageRequested);
    QVERIFY(messages.isValid());
    const auto configuration = [](int type, int hue, int saturation, int value,
                                  bool colorize, bool compatibility = false) {
        KisFilterConfigurationSP result(new KisFilterConfiguration(
            QStringLiteral("hsvadjustment"), 1, KisGlobalResourcesInterface::instance()));
        result->setProperty("h", hue);
        result->setProperty("s", saturation);
        result->setProperty("v", value);
        result->setProperty("type", type);
        result->setProperty("colorize", colorize);
        result->setProperty("compatibilityMode", compatibility);
        return result;
    };
    const auto documentLuma = [&] {
        const QVector<qreal> coefficients = view->image()->colorSpace()->lumaCoefficients();
        return std::array<double, 3>{double(coefficients[0]), double(coefficients[1]),
                                     double(coefficients[2])};
    };
    const auto lastFilterOperation = [&] {
        return std::get<Gpu::FilterSnapshot>(
            owner->operations().snapshot()->layers.first().operations.last());
    };

    window->viewManager()->filterManager()->apply(configuration(3, 0, 50, 0, false));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes,
             QByteArray::fromHex("ff0a46ff"));
    {
        const auto operation = lastFilterOperation();
        QCOMPARE(operation->hsvType, Gpu::HsvAdjustmentType::Hsy);
        QVERIFY(!operation->colorize);
        QVERIFY(operation->luma == documentLuma());
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());

    window->viewManager()->filterManager()->apply(configuration(4, 30, 20, -10, false));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes,
             QByteArray::fromHex("f10074ff"));
    {
        const auto operation = lastFilterOperation();
        QCOMPARE(operation->hsvType, Gpu::HsvAdjustmentType::Yuv);
        QVERIFY(!operation->colorize);
        QVERIFY(operation->luma == documentLuma());
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());

    // The product dialog's Colorize option reaches the GPU operation without
    // starting a CPU preview stroke.
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    QTRY_VERIFY((dialog = window->findChild<QDialog *>(QStringLiteral("gpuHsvAdjustmentDialog"))));
    QVERIFY(dialog);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
    hue = dialog->findChild<QSpinBox *>(QStringLiteral("hSpinBox"));
    saturation = dialog->findChild<QSpinBox *>(QStringLiteral("sSpinBox"));
    value = dialog->findChild<QSpinBox *>(QStringLiteral("vSpinBox"));
    type = dialog->findChild<QComboBox *>(QStringLiteral("cmbType"));
    colorize = dialog->findChild<QCheckBox *>(QStringLiteral("chkColorize"));
    buttons = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(hue);
    QVERIFY(saturation);
    QVERIFY(value);
    QVERIFY(type);
    QVERIFY(colorize);
    QVERIFY(buttons);
    type->setCurrentIndex(0);
    colorize->setChecked(true);
    hue->setValue(120);
    saturation->setValue(80);
    value->setValue(0);
    buttons->button(QDialogButtonBox::Ok)->click();
    QTRY_COMPARE(owner->operations().historyAction(owner->operations().undoCount()),
                 Gpu::HistoryAction::HsvAdjustment);
    {
        const auto operation = lastFilterOperation();
        QVERIFY(operation->colorize);
        QCOMPARE(operation->hue, 120);
        QCOMPARE(operation->saturation, 80);
        QCOMPARE(operation->value, 0);
        QVERIFY(operation->luma == documentLuma());
    }
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes,
             QByteArray::fromHex("0e7c0eff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());

    // Unsupported modes and values outside the selected product slider range
    // are diagnosed without changing pixels or history.
    const auto beforeRejected = owner->operations().snapshot();
    const auto beforeRejectedCount = owner->operations().undoCount();
    window->viewManager()->filterManager()->apply(configuration(5, 0, 0, 0, false));
    QTRY_COMPARE(messages.size(), 1);
    QVERIFY(messages.last().at(0).toString().contains(QStringLiteral("not available for GPU documents")));
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), beforeRejectedCount);

    // Colorize with the YUV mode uses the 0..100 Yellow-Blue range.
    window->viewManager()->filterManager()->apply(configuration(4, 120, 80, 0, true));
    QTRY_COMPARE(messages.size(), 2);
    QVERIFY(messages.last().at(0).toString().contains(QStringLiteral("outside the product slider ranges")));
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), beforeRejectedCount);

    // Out-of-range product values are rejected rather than clamped.
    window->viewManager()->filterManager()->apply(configuration(0, 200, 0, 0, false));
    QTRY_COMPARE(messages.size(), 3);
    QVERIFY(messages.last().at(0).toString().contains(QStringLiteral("outside the product slider ranges")));
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), beforeRejectedCount);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
}

void KisGpuDocumentViewTest::layerPanelShowsAndSelectsGpuTransparencyMaskChild()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask child")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    QVERIFY(layer);
    const auto maskNode = layer->firstChild();
    QVERIFY(!maskNode);

    // A nonuniform coverage proves the thumbnail is derived from the mask.
    QByteArray coverage(32 * 32, '\0');
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 16; ++x) coverage[y * 32 + x] = 1;
        // Keep the covered bounds square so the precise thumbnail retains both
        // selected and unselected columns.
        coverage[y * 32 + 31] = 1;
    }
    const auto selectionMask = Gpu::SelectionMask::fromCoverage(QRect(0, 0, 32, 32), coverage,
                                                                QRect(0, 0, 64, 64));
    QVERIFY(selectionMask);
    QVERIFY(image->setGpuSelectionMask(selectionMask));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));

    auto child = layer->firstChild();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));
    QVERIFY(image->isGpuTransparencyMaskView(child));

    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    auto *model = qobject_cast<KisNodeModel*>(proxy->sourceModel());
    QVERIFY(model);
    model->setPreferredThumnalSize(32);
    QTRY_VERIFY(proxy->indexFromNode(child).isValid());
    const auto index = proxy->indexFromNode(child);
    const auto thumbnail = proxy->data(index, int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>();
    QVERIFY(!thumbnail.isNull());
    QCOMPARE(thumbnail.size(), QSize(32, 32));
    QCOMPARE(qGray(thumbnail.pixel(4, 16)), 255);
    QCOMPARE(qGray(thumbnail.pixel(28, 16)), 0);

    auto *manager = window->viewManager()->nodeManager();
    QCOMPARE(view->currentNode(), layer);
    const auto undoCount = lifetime->undoStack()->count();
    manager->slotUiActivatedNode(child);
    QCOMPARE(view->currentNode(), child);
    QCOMPARE(manager->activeNode(), child);
    QVERIFY(view->currentMask());
    QCOMPARE(view->currentMask().data(), child.data());
    QCOMPARE(lifetime->undoStack()->count(), undoCount);

    // A selected mask child routes brush input into the parent layer's retained
    // mask instead of falling back to the paint layer. One stroke adds exactly
    // one history entry and refreshes the panel child and its thumbnail.
    QVERIFY(image->clearGpuSelection());
    const auto layerId = owner->operations().activeLayer();
    const auto maskBefore = owner->layerTransparencyMask(layerId);
    QVERIFY(maskBefore);
    QCOMPARE(maskBefore->coverage(20, 20), quint8(0));
    window->viewManager()->canvasResourceProvider()->setFGColor(KoColor(Qt::white, image->colorSpace()));
    const auto brush = captureGpuBrushResources(image, child, view->canvasBase()->resourceManager());
    QCOMPARE(brush.layer, layerId);
    QVERIFY(brush.diagnostic.isEmpty());
    QVERIFY(brush.transparencyMask);
    const auto beforeThumbnail = proxy->data(index, int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>();
    const auto maskHistory = lifetime->undoStack()->count();
    const auto stroke = owner->beginTransparencyMaskStroke(layerId, brush.brush, 0);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{20, 20}, 1}}), Gpu::Result::Success);
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    QCOMPARE(lifetime->undoStack()->count(), maskHistory + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::PaintStroke);
    const auto paintedCoverage = owner->execution().readSelectionPixels(owner->layerTransparencyMask(layerId), {20, 20, 1, 1}, 1);
    QCOMPARE(paintedCoverage.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(paintedCoverage.bytes[0]) > 0);
    quint8 painted = 0;
    child->paintDevice()->readBytes(&painted, 20, 20, 1, 1);
    QVERIFY(painted > 0);
    QTRY_VERIFY(proxy->data(index, int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>() != beforeThumbnail);

    // The selection fill action targets the retained mask as well and leaves
    // the parent layer's pixels untouched.
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    QVERIFY(image->setGpuSelectionRect(QRect(40, 40, 8, 8)));
    QTRY_COMPARE(owner->selectionRect(), QRect(40, 40, 8, 8));
    auto *fillAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fillAction);
    QVERIFY(fillAction->isEnabled());
    fillAction->trigger();
    QTRY_COMPARE(lifetime->undoStack()->count(), maskHistory + 3);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::FillLayer);
    const auto filledCoverage = owner->execution().readSelectionPixels(owner->layerTransparencyMask(layerId), {44, 44, 1, 1}, 1);
    QCOMPARE(filledCoverage.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(filledCoverage.bytes[0]) > 0);
    quint8 filled = 0;
    child->paintDevice()->readBytes(&filled, 44, 44, 1, 1);
    QVERIFY(filled > 0);
    const auto layerPixels = owner->readRgbaPixels(layerId, QRect(44, 44, 1, 1), 4);
    QCOMPARE(layerPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(layerPixels.bytes.at(3)), quint8(0));

    manager->slotUiActivatedNode(layer);
    QCOMPARE(view->currentNode(), layer);
    QVERIFY(!view->currentMask());
    QCOMPARE(lifetime->undoStack()->count(), maskHistory + 3);
}

void KisGpuDocumentViewTest::maskPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<int>("figureFillStyle");
    QTest::addColumn<bool>("whiteDefault");
    QTest::addColumn<bool>("groupOwner");
    QTest::newRow("normal-u8") << QStringLiteral("b)_Basic-5_Size_default.kpp") << false << qreal(-1) << int(KisFigurePaintingOptions::FillStyleNone) << false << false;
    // #89 A0c: Gaussian charcoal and white-default erasure previously failed.
    QTest::newRow("charcoal-u16") << QStringLiteral("h)_Charcoal_Pencil_Thin.kpp") << true << qreal(-1) << int(KisFigurePaintingOptions::FillStyleNone) << false << false;
    // #89 A1b-2: mask erasure must change white-default coverage.
    QTest::newRow("white-default-details") << QStringLiteral("b)_Basic-6_Details.kpp") << false << qreal(-1) << int(KisFigurePaintingOptions::FillStyleNone) << true << false;
}

void KisGpuDocumentViewTest::groupMaskPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<int>("figureFillStyle");
    QTest::addColumn<bool>("whiteDefault");
    QTest::addColumn<bool>("groupOwner");
    // #89 A1b-2: these three presets previously left group-mask erasure unchanged.
    QTest::newRow("circle-finite") << QStringLiteral("a)_Eraser_Circle.kpp") << false << qreal(-1) << int(KisFigurePaintingOptions::FillStyleNone) << false << true;
    QTest::newRow("soft-white") << QStringLiteral("c)_Pencil-4_Soft.kpp") << true << qreal(-1) << int(KisFigurePaintingOptions::FillStyleNone) << true << true;
    QTest::newRow("quick-finite") << QStringLiteral("c)_Pencil-6_Quick_Shade.kpp") << false << qreal(-1) << int(KisFigurePaintingOptions::FillStyleNone) << false << true;
}

void KisGpuDocumentViewTest::groupMaskPresetsKeepOrdinaryEditing()
{
    maskPresetsKeepOrdinaryEditing();
}

void KisGpuDocumentViewTest::maskPresetsKeepOrdinaryEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(QString, presetName);
    const bool pencil = presetName.startsWith(QLatin1String("c)_Pencil-"));
    const bool imageTip = presetName == QLatin1String("b)_Basic-6_Details.kpp") || presetName == QLatin1String("c)_Pencil-4_Soft.kpp") || presetName == QLatin1String("c)_Pencil-6_Quick_Shade.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    const bool incrementalPencil = presetName == QLatin1String("c)_Pencil-3_Large_4B.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    const bool fineliner = presetName == QLatin1String("d)_Ink-2_Fineliner.kpp");
    const auto restoreTiming = fixedBrushPointerTiming(fineliner);
    QFETCH(bool, words);
    QFETCH(qreal, flow);
    QFETCH(int, figureFillStyle);
    QFETCH(bool, whiteDefault);
    QFETCH(bool, groupOwner);
    const bool filledFigure = figureFillStyle != KisFigurePaintingOptions::FillStyleNone;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    const auto bundle = KisResourceStorageSP::create(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle"));
    QVERIFY(bundle->valid());
    auto sourcePreset = bundle->resource(QStringLiteral("paintoppresets/") + presetName).dynamicCast<KisPaintOpPreset>();
    QVERIFY(sourcePreset);
    if (imageTip) {
        QDomDocument definition;
        QVERIFY(definition.setContent(sourcePreset->settings()->getString("brush_definition")));
        const auto file = QFileInfo(definition.documentElement().attribute("filename")).fileName();
        const auto tip = bundle->resource(QStringLiteral("brushes/") + file);
        QVERIFY(tip);
        auto source = KisGlobalResourcesInterface::instance()->source<KoResource>(ResourceType::Brushes);
        if (!source.bestMatch(tip->md5Sum(), tip->filename(), tip->name())) {
            KisResourceModel brushResources(ResourceType::Brushes);
            QVERIFY(brushResources.addResource(tip));
        }
    }

    KisResourceModel brushes(ResourceType::Brushes);
    if (brushes.rowCount() == 0) QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
    KisResourceModel patterns(ResourceType::Patterns);
    if (patterns.rowCount() == 0) QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    if (sourcePreset->settings()->getBool("Texture/Pattern/Enabled")) {
        const auto file = QFileInfo(sourcePreset->settings()->getString("Texture/Pattern/PatternFileName")).fileName();
        const auto paper = bundle->resource(QStringLiteral("patterns/") + file).dynamicCast<KoPattern>();
        QVERIFY(paper);
        if (!KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns).bestMatch(paper->md5Sum(), paper->filename(), paper->name()))
            QVERIFY(patterns.addResource(paper));
        sourcePreset->settings()->setProperty("Texture/Pattern/isRandomOffsetX", false);
        sourcePreset->settings()->setProperty("Texture/Pattern/isRandomOffsetY", false);
        sourcePreset->settings()->setProperty("Texture/Pattern/OffsetX", presetName.startsWith(QLatin1String("d)")) ? 7 : 0);
        sourcePreset->settings()->setProperty("Texture/Pattern/OffsetY", presetName.startsWith(QLatin1String("d)")) ? 11 : 0);
    }
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect area(0, 0, 64, 64);
    const QByteArray initial(area.width() * area.height(), char(whiteDefault ? 255 : 128));
    QList<QByteArray> expected;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto nativeGroupSource = directory.filePath(QStringLiteral("native-group-source.kra"));
    for (const bool gpu : pencil && !incrementalPencil ? QList<bool>{true} : QList<bool>{false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        auto preset = sourcePreset->clone().dynamicCast<KisPaintOpPreset>();
        if (flow >= 0) {
            preset->settings()->setProperty("FlowValue", flow);
            if (presetName == QLatin1String("u)_Pixel_Art_Dithering.kpp")) {
                preset->settings()->setProperty("SharpnessValue", .65);
                preset->settings()->setProperty("Sharpness/alignoutline", true);
                preset->settings()->setProperty("Sharpness/softness", 45);
            }
        }
        preset->setFilename((gpu ? QStringLiteral("gpu-mask-") : QStringLiteral("cpu-mask-")) + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
        KisResourceModel presets(ResourceType::PaintOpPresets); QVERIFY(presets.addResource(preset));
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        document->setAutoSaveActive(false); document->setFileBatchMode(true);
        const bool nativeGroupInput = groupOwner && gpu && QFileInfo::exists(nativeGroupSource);
        if (nativeGroupInput) {
            QVERIFY2(document->openPath(nativeGroupSource, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
        } else {
            QVERIFY(document->newImage(QStringLiteral("Ordinary mask editing"), area.width(), area.height(), cs,
                                       KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        }
        auto image = KisImageSP(document->image());
        auto layer = image->root()->firstChild(); QVERIFY(layer);
        if (groupOwner && !nativeGroupInput) {
            const KoColor color(QColor(191, 43, 97, 255), cs);
            const QByteArray pixel(reinterpret_cast<const char *>(color.data()), cs->pixelSize());
            layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.repeated(area.width() * area.height()).constData()), area);
            if (gpu) {
                const auto owner = image->gpuImage();
                const auto paint = owner->operations().activeLayer();
                QCOMPARE(owner->groupLayers({paint}, paint, QStringLiteral("Masked group")), Gpu::Result::Success);
                layer = image->root()->firstChild();
            } else {
                KisGroupLayerSP group = new KisGroupLayer(image, QStringLiteral("Masked group"), 255);
                auto paint = layer;
                QVERIFY(image->removeNode(paint)); QVERIFY(image->addNode(group)); QVERIFY(image->addNode(paint, group));
                layer = group;
            }
            QVERIFY(layer && layer->inherits("KisGroupLayer"));
            layer->setDirty(area); image->waitForDone();
        }
        if (nativeGroupInput) {
            QVERIFY(layer->inherits("KisGroupLayer"));
        } else if (gpu && whiteDefault) {
            const auto owner = image->gpuImage();
            const auto version = owner->operations().snapshot();
            const auto target = std::find_if(version->layers.cbegin(), version->layers.cend(),
                [&](const auto &value) { return value.uuid == layer->uuid(); });
            QVERIFY(target != version->layers.cend());
            QCOMPARE(owner->addTransparencyMask(target->id), Gpu::Result::Success);
        } else if (gpu) {
            const auto owner = image->gpuImage(); QVERIFY(owner);
            if (groupOwner) {
                QCOMPARE(owner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, initial, area)), Gpu::Result::Success);
            } else {
                Gpu::RegionFillSettings settings; settings.seed = {16, 16}; settings.threshold = 1;
                QCOMPARE(owner->selectRegion(owner->operations().activeLayer(), settings, Gpu::SelectionCombination::Replace), Gpu::Result::Success);
                QCOMPARE(owner->combineSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, initial, area), Gpu::SelectionCombination::Intersect), Gpu::Result::Success);
                QVERIFY(owner->selectionMask()->isGpuGenerated());
            }
            QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));
            QVERIFY(image->clearGpuSelection());
        } else {
            KisSelectionSP selection = new KisSelection;
            if (whiteDefault) {
                const quint8 value = 255;
                selection->pixelSelection()->setDefaultPixel(KoColor(&value, selection->pixelSelection()->colorSpace()));
            }
            selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), area);
            KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
            mask->setSelection(selection); QVERIFY(image->addNode(mask, layer));
            image->refreshGraphAsync(); image->waitForDone();
        }
        auto child = layer->firstChild();
        while (child && !child->inherits("KisTransparencyMask")) child = child->nextSibling();
        QVERIFY(child);
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow()); window->resize(800, 600);
        auto *view = window->newView(document.get()); QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        BrushPointerClock pointerClock;
        QPointer<QWindow> input; bool pressed = false;
        const auto reportClose = [&](const char *stage) {
            if (!qEnvironmentVariableIsEmpty("LIBREPAINT_GPU_RESULT_DIR"))
                qInfo() << "Mask workflow close" << gpu << stage;
        };
        const auto closeWindow = qScopeGuard([&] {
            reportClose("begin");
            if (pressed && input) { if (lifetime) lifetime->image()->requestStrokeCancellation(); QTest::mouseRelease(input, Qt::LeftButton); }
            if (lifetime) { lifetime->setModified(false); window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger(); }
            reportClose("document closed");
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
            reportClose("window destroyed");
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            reportClose("deferred deletion complete");
        });
        window->viewManager()->setCurrentView(view);
        for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
        window->show(); window->activateWindow();
        auto *profiles = KisInputProfileManager::instance();
        profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                     QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
        profiles->loadProfiles();
        window->viewManager()->nodeManager()->slotUiActivatedNode(child);
        auto *resources = window->viewManager()->canvasResourceProvider();
        QImage patternImage(2, 2, QImage::Format_ARGB32);
        patternImage.setPixelColor(0, 0, QColor(34, 68, 102, 200));
        patternImage.setPixelColor(1, 0, QColor(211, 17, 93, 127));
        patternImage.setPixelColor(0, 1, QColor(71, 181, 241, 255));
        patternImage.setPixelColor(1, 1, QColor(255, 255, 255, 0));
        const KoPatternSP figurePattern(new KoPattern(patternImage, QStringLiteral("Mask pattern"), QString()));
        // Sparse random tips need contrast against the finite mask's 128 coverage
        // so that short strokes survive 8-bit rounding.
        const QColor maskForeground = presetName == QLatin1String("c)_Pencil-6_Quick_Shade.kpp")
            ? QColor(Qt::white) : QColor(17, 129, 241);
        const auto configure = [&] {
            resources->setPaintOpPreset(preset); resources->setSize(13.75); resources->setOpacity(.7);
            resources->setGlobalAlphaLock(false); resources->setMirrorHorizontal(false); resources->setMirrorVertical(false);
            resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
            resources->setFGColor(KoColor(maskForeground, cs));
            resources->setBGColor(KoColor(QColor(71, 241, 129), cs));
            if (figureFillStyle == KisFigurePaintingOptions::FillStylePattern) resources->slotPatternActivated(figurePattern);
        };
        configure();
        if (gpu) {
            const auto capture = captureGpuBrushResources(image, child, view->canvasBase()->resourceManager());
            QVERIFY2(capture.diagnostic.isEmpty(), qPrintable(capture.diagnostic));
            const qreal resourceOpacity = resources->opacity();
            if (incrementalPencil) resources->setOpacity(1);
            const auto restoreProbeOpacity = qScopeGuard([&] { if (incrementalPencil) resources->setOpacity(resourceOpacity); });
            const auto *gray = KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), QString());
            KisImageSP nativeImage = new KisImage(nullptr, 64, 64, cs, QStringLiteral("Native mask dab reference"));
            KisPaintDeviceSP temporary = new KisPaintDevice(gray);
            KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Gray dab"), 255, temporary);
            nativeImage->addNode(nativeLayer, nativeImage->root());
            if (!incrementalPencil) nativeLayer->setTemporaryTarget(temporary);
            {
                KisPainter painter(temporary);
                painter.setPaintColor(KoColor(Qt::white, gray));
                painter.setCompositeOpId(incrementalPencil ? COMPOSITE_OVER : COMPOSITE_ALPHA_DARKEN);
                painter.setPaintOpPreset(resources->currentPreset(), nativeLayer, nativeImage);
                KisDistanceInformation distance;
                KisPaintInformation point(QPointF(32, 32), 1);
                if (pencil || imageTip) {
                    distance = KisDistanceInformation(QPointF(32, 32), 0);
                    point.setRandomSource(KisRandomSourceSP(new KisRandomSource(17)));
                }
                painter.paintAt(point, &distance);
                bool pending;
                do {
                    QVector<KisRunnableStrokeJobData *> jobs;
                    pending = painter.paintOp()->doAsynchronousUpdate(jobs).second;
                    painter.runnableStrokeJobsInterface()->addRunnableJobs(jobs);
                } while (pending);
            }
            nativeLayer->setTemporaryTarget({}); nativeImage->waitForDone();
            QByteArray nativeGray(initial.size() * 2, '\0'), nativeAlpha(initial.size(), '\0');
            temporary->readBytes(reinterpret_cast<quint8 *>(nativeGray.data()), area);
            for (int i = 0; i < nativeAlpha.size(); ++i) nativeAlpha[i] = nativeGray[2 * i + 1];
            Gpu::RasterImage input; input.size = area.size(); input.layers = {{QStringLiteral("Mask dab")}};
            input.layers.first().transparencyMask = Gpu::SelectionMask::empty();
            Gpu::PaintImage probe(image->gpuImage()->execution(), input);
            auto white = capture.brush; white.rgba = {255, 255, 255, 255}; white.opacity = 1;
            // #89 A0c: this direct native probe invokes the primary paintop.
            // The ordinary workflow below compares the complete masking brush.
            white.additiveMaskingBrush.clear();
            const auto operation = probe.beginTransparencyMaskStroke(1, white, 17); QVERIFY(!operation.isNull());
            QCOMPARE(probe.appendSamples(operation, {{{32, 32}, 1}}), Gpu::Result::Success);
            QCOMPARE(probe.commitStroke(operation), Gpu::Result::Success);
            const auto actual = probe.execution().readSelectionPixels(probe.layerTransparencyMask(1), area, nativeAlpha.size());
            QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
            if (actual.bytes != nativeAlpha && imageTip && !qEnvironmentVariableIsEmpty("LIBREPAINT_GPU_RESULT_DIR")) {
                const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR") + QStringLiteral("/image-mask-") +
                    presetName + (words ? QStringLiteral("-u16") : QStringLiteral("-u8"));
                QVERIFY(QDir().mkpath(root));
                const auto save = [&](const QString &name, const QByteArray &bytes) {
                    QFile file(root + '/' + name);
                    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
                };
                QVERIFY(save(QStringLiteral("native.alpha"), nativeAlpha));
                QVERIFY(save(QStringLiteral("gpu.alpha"), actual.bytes));
                const auto &captured = *white.autoBrush->imageTips;
                qDebug() << "frozen image mask" << white.diameter << white.autoBrush->angle << captured.originalSize
                    << captured.tipCount << "drawing angle offset" << white.autoBrush->drawingAngleOffset;
                // Retain each frozen cell's result to distinguish source/selection
                // failures from sampling failures without altering the reference.
                for (int tip = 0; tip < captured.tipCount; ++tip) {
                    auto single = QSharedPointer<Gpu::ImageBrushTips>::create();
                    single->originalSize = captured.originalSize; single->tipCount = 1;
                    for (auto level : captured.levels) {
                        const auto bytes = captured.pixels.mid((level.wordOffset + tip * level.tipWordStride) * 4,
                                                               level.tipWordStride * 4);
                        level.wordOffset = single->pixels.size() / 4;
                        single->levels.append(level); single->pixels.append(bytes);
                    }
                    auto brush = white;
                    brush.autoBrush->imageTips = single;
                    Gpu::PaintImage candidate(image->gpuImage()->execution(), input);
                    const auto stroke = candidate.beginTransparencyMaskStroke(1, brush, 17);
                    QVERIFY(!stroke.isNull());
                    QCOMPARE(candidate.appendSamples(stroke, {{{32, 32}, 1}}), Gpu::Result::Success);
                    QCOMPARE(candidate.commitStroke(stroke), Gpu::Result::Success);
                    const auto pixels = candidate.execution().readSelectionPixels(candidate.layerTransparencyMask(1), area, nativeAlpha.size());
                    QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
                    int differences = 0;
                    for (int p = 0; p < nativeAlpha.size(); ++p) differences += pixels.bytes[p] != nativeAlpha[p];
                    qDebug() << "frozen tip" << tip << "native differing pixels" << differences;
                    QVERIFY(save(QStringLiteral("tip-%1.alpha").arg(tip), pixels.bytes));
                }
            }
            for (int i = 0; i < nativeAlpha.size(); ++i)
                QVERIFY2(actual.bytes[i] == nativeAlpha[i], qPrintable(QStringLiteral("Native GrayA8 dab at %1,%2: GPU %3 native %4")
                    .arg(i % area.width()).arg(i / area.width()).arg(quint8(actual.bytes[i])).arg(quint8(nativeAlpha[i]))));
        }
        const auto toolId = filledFigure ? QStringLiteral("KritaShape/KisToolEllipse") : QStringLiteral("KritaShape/KisToolBrush");
        KoToolManager::instance()->switchToolRequested(toolId);
        auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), toolId);
        QVERIFY(tool);
        if (!filledFigure) QVERIFY(tool->setProperty("smoothingType", 0));
        const auto configureFigure = [&] {
            if (!filledFigure) return;
            QComboBox *outline = nullptr, *fill = nullptr;
            for (const auto &panel : tool->optionWidgets()) {
                if (!panel) continue;
                if (auto *value = panel->findChild<QComboBox*>(QStringLiteral("cmbOutline"))) outline = value;
                if (auto *value = panel->findChild<QComboBox*>(QStringLiteral("cmbFill"))) fill = value;
            }
            QVERIFY(outline && fill);
            outline->setCurrentIndex(KisFigurePaintingOptions::StrokeStyleForeground);
            fill->setCurrentIndex(figureFillStyle);
        };
        configureFigure();
        const auto prepareInput = [&] {
            if (filledFigure) view->canvasBase()->snapGuide()->enableSnapping(false);
            view->canvasBase()->canvasWidget()->setFocus(); view->canvasController()->resetCanvasRotation();
            view->canvasController()->mirrorCanvas(false); view->zoomManager()->zoomTo100();
            const auto extent = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
            view->canvasController()->setPreferredCenter({extent.width() / 2, extent.height() / 2});
            input = window->windowHandle();
            if (gpu) for (auto *candidate : QGuiApplication::allWindows())
                if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = candidate;
            if (fineliner && input) input->installEventFilter(&pointerClock);
        };
        QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= 64); prepareInput(); QTRY_VERIFY(input && input->isExposed());
        const auto pixels = [&] {
            QByteArray bytes(initial.size(), '\0');
            child->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
            return bytes;
        };
        const auto parentPixels = [&] {
            QByteArray bytes(initial.size() * cs->pixelSize(), '\0');
            KisPaintDeviceSP device = layer->paintDevice();
            if (groupOwner) {
                for (auto child = layer->firstChild(); child; child = child->nextSibling())
                    if (child->inherits("KisPaintLayer")) { device = child->paintDevice(); break; }
            }
            if (!device) return QByteArray{};
            device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
            return bytes;
        };
        const auto originalParent = parentPixels(); QVERIFY(!originalParent.isEmpty());
        int state = 0;
        const auto compare = [&] {
            auto actual = pixels();
            if (parentPixels() != originalParent) return false;
            if (groupOwner) {
                QByteArray projection(initial.size() * cs->pixelSize(), '\0');
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), area);
                actual += projection;
            }
            if (pencil && !incrementalPencil) return true; // Fixed-seed random coverage above, state invariants below.
            if (!gpu) expected.append(actual);
            else {
                const auto reference = expected.value(state++);
                const auto alphaDifference = pixelStepDifference(actual.left(initial.size()), reference.left(initial.size()), 1, 1);
                const auto projectionDifference = pixelStepDifference(actual.mid(initial.size()), reference.mid(initial.size()), words ? 2 : 1);
                if (!alphaDifference.isEmpty() || !projectionDifference.isEmpty()) {
                    const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                    if (root.isEmpty()) directory.setAutoRemove(false);
                    const QDir output(QDir(root.isEmpty() ? directory.path() : root).filePath(QString::fromLatin1(QTest::currentDataTag())));
                    if (QDir().mkpath(output.path())) {
                        for (const auto &entry : {qMakePair(QStringLiteral("native"), reference), qMakePair(QStringLiteral("gpu"), actual)}) {
                            QFile file(output.filePath(QStringLiteral("mask-state-%1-%2.alpha").arg(state - 1).arg(entry.first)));
                            if (file.open(QIODevice::WriteOnly)) file.write(entry.second);
                        }
                    }
                    for (int i = 0; i < qMin(actual.size(), reference.size()); ++i) if (actual[i] != reference[i]) {
                        qWarning() << "Mask state" << state - 1 << "at" << QPoint(i % area.width(), i / area.width())
                                   << "GPU" << quint8(actual[i]) << "native" << quint8(reference[i]);
                        break;
                    }
                    return false;
                }
            }
            return true;
        };
        const auto stroke = [&](QList<QPointF> points, bool cancel = false) {
            if (filledFigure && points.size() == 1) points = {points.first() - QPointF(4, 4), points.first() + QPointF(4, 4)};
            const auto *converter = view->canvasBase()->coordinatesConverter();
            const auto point = [&](QPointF value) {
                const auto widget = converter->imageToWidget(value).toPoint();
                return gpu ? widget : view->canvasBase()->canvasWidget()->mapTo(window.get(), widget);
            };
            const auto first = point(points.first());
            // Fix the incoming cursor direction for direction-sensitive tips.
            if (imageTip || pencil) QTest::mouseMove(input, point(points.first() - QPointF(12, 12)));
            if (fineliner) pointerClock.advance();
            QTest::mouseMove(input, first);
            const auto local = converter->imageToWidget(points.first()).toPoint();
            QEnterEvent entered(local, local, view->canvasBase()->canvasWidget()->mapToGlobal(local));
            QApplication::sendEvent(gpu ? static_cast<QObject *>(input.data()) : view->canvasBase()->canvasWidget(), &entered);
            pressed = true;
            if (fineliner) pointerClock.advance();
            QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, first);
            for (const auto &value : points.mid(1)) {
                if (fineliner) pointerClock.advance();
                QTest::mouseMove(input, point(value), 5);
            }
            if (fineliner) pointerClock.advance();
            if (cancel) QTest::keyClick(input, Qt::Key_Escape);
            QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, point(points.last())); pressed = false;
            image->waitForDone(); QApplication::processEvents();
        };
        QCOMPARE(pixels(), initial);
        if (filledFigure) {
            const int historyBefore = lifetime->undoStack()->index();
            QByteArray projectionBefore(area.width() * area.height() * cs->pixelSize(), '\0');
            image->projection()->readBytes(reinterpret_cast<quint8 *>(projectionBefore.data()), area);
            stroke({{16, 16}, {48, 48}}, true);
            QCOMPARE(lifetime->undoStack()->index(), historyBefore);
            QCOMPARE(pixels(), initial);
            QCOMPARE(parentPixels(), originalParent);
            QByteArray projectionAfter(projectionBefore.size(), '\0');
            image->projection()->readBytes(reinterpret_cast<quint8 *>(projectionAfter.data()), area);
            QCOMPARE(projectionAfter, projectionBefore);
        }
        if (groupOwner) QVERIFY2(compare(), "Prepared group projection must apply the full initial mask");
        if (groupOwner && !gpu) {
            QVERIFY2(lifetime->saveAs(nativeGroupSource, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
            lifetime->waitForSavingToComplete();
            const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
            if (!results.isEmpty()) {
                QVERIFY(QDir().mkpath(results));
                QVERIFY(QFile::copy(nativeGroupSource, QDir(results).filePath(QStringLiteral("native-group-%1-source.kra")
                    .arg(QString::fromLatin1(QTest::currentDataTag())))));
            }
        }
        const int initialUndoIndex = lifetime->undoStack()->index();
        stroke({{24, 24}, {40, 36}});
        if (filledFigure) QTRY_COMPARE(lifetime->undoStack()->index(), initialUndoIndex + 1);
        const auto painted = pixels();
        auto maskColor = resources->fgColor();
        maskColor.convertTo(KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), QString()));
        QVERIFY2(painted != initial, qPrintable(QStringLiteral("Unchanged mask: GPU=%1, Gray=%2, history=%3 -> %4")
            .arg(gpu).arg(maskColor.data()[0]).arg(initialUndoIndex).arg(lifetime->undoStack()->index())));
        QVERIFY2(compare(), "Ordinary mask preset painting must match native Alpha8 pixels");
        lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(), initial);
        lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(), painted);
        // #89 A1b-2: synchronizing group children must preserve the selected mask.
        if (gpu) QCOMPARE(window->viewManager()->nodeManager()->activeNode()->uuid(), child->uuid());
        resources->setEraserMode(true); stroke({{32, 32}});
        const auto erased = pixels();
        QVERIFY2(erased != painted, qPrintable(QStringLiteral("Mask erasure is unchanged: GPU=%1, history=%2").arg(gpu).arg(lifetime->undoStack()->index())));
        QVERIFY2(compare(), "Ordinary mask preset erasing must match native Alpha8 pixels");
        lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(), painted);
        lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(), erased);
        resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
        resources->setFGColor(KoColor(QColor(241, 17, 129), cs));
        const QRect selected(4, 4, 12, 12);
        if (gpu) QVERIFY(image->setGpuSelectionRect(selected));
        else { KisPixelSelectionSP selection = new KisPixelSelection; selection->select(selected, 255);
            KisSelectionToolHelper helper(view->canvasBase(), kundo2_i18n("Select Rectangle"));
            helper.selectPixelSelection(selection, SELECTION_REPLACE); image->waitForDone(); }
        QTRY_VERIFY(view->selection());
        QTRY_COMPARE(view->selection()->selectedExactRect(), selected);
        auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color_opacity"));
        QVERIFY(fill && fill->isEnabled()); fill->trigger(); image->waitForDone(); QApplication::processEvents();
        const auto foregroundFilled = pixels(); QVERIFY(foregroundFilled != erased);
        QVERIFY2(compare(), "Selected foreground mask fill must preserve native gray conversion");
        resources->slotPatternActivated(figurePattern);
        resources->setCurrentCompositeOp(QStringLiteral("multiply"));
        auto *patternFill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_pattern_opacity"));
        QVERIFY(patternFill && patternFill->isEnabled()); patternFill->trigger(); image->waitForDone(); QApplication::processEvents();
        const auto patternFilled = pixels(); QVERIFY(patternFilled != foregroundFilled);
        QVERIFY2(compare(), "Selected pattern mask fill must preserve native gray, alpha and blend mode");
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        // Figure painting reuses the same frozen resources and global selection.
        {
            KisFigurePaintingStroke figure(kundo2_i18n("Mask figure"), image, child, view->canvasBase()->resourceManager(),
                KisFigurePaintingOptions::StrokeStyleNone, KisFigurePaintingOptions::FillStyleBackgroundColor);
            figure.paintRect(QRectF(6, 6, 8, 8));
        }
        image->waitForDone(); QApplication::processEvents();
        const auto figureFilled = pixels(); QVERIFY(figureFilled != patternFilled);
        QVERIFY2(compare(), "Selected mask figure must preserve native gray composition");
        {
            KisFigurePaintingStroke figure(kundo2_i18n("Mask outline selection override"), image, child, view->canvasBase()->resourceManager(),
                KisFigurePaintingOptions::StrokeStyleNone, KisFigurePaintingOptions::FillStyleForegroundColor);
            figure.setSelectionOverride({}); figure.paintRect(QRectF(50, 6, 8, 8));
        }
        image->waitForDone(); QApplication::processEvents();
        QVERIFY(pixels() != figureFilled);
        QVERIFY2(compare(), "Selection-outline callers must paint across the frozen selection boundary");
        auto *deselect = window->viewManager()->actionManager()->actionByName(QStringLiteral("deselect"));
        QTRY_VERIFY(deselect && deselect->isEnabled()); deselect->trigger(); image->waitForDone();
        QTRY_VERIFY(!view->selection() || view->selection()->selectedExactRect().isEmpty());
        const auto saved = pixels();
        if (groupOwner) {
            auto *group = qobject_cast<KisGroupLayer *>(layer.data()); QVERIFY(group);
            group->setPassThroughMode(true); image->refreshGraphAsync(); image->waitForDone();
            QCOMPARE(pixels(), saved);
            QVERIFY2(compare(), "Pass-through groups must retain the mask and use native child composition");
        }
        const auto path = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("cpu.kra"));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        QVERIFY2(lifetime->openPath(path, KisDocument::DontAddToRecent), qPrintable(lifetime->errorMessage()));
        image = lifetime->image(); layer = image->root()->firstChild(); child = layer->firstChild();
        while (child && !child->inherits("KisTransparencyMask")) child = child->nextSibling();
        QVERIFY(child);
        window->viewManager()->nodeManager()->slotUiActivatedNode(child);
        QTRY_COMPARE(view->currentNode(), child);
        QTRY_VERIFY(image->isIdle());
        QCOMPARE(pixels(), saved); QVERIFY2(compare(), "KRA reload must preserve the entire edited mask");
        if (groupOwner) {
            const auto *group = qobject_cast<KisGroupLayer *>(layer.data()); QVERIFY(group);
            QVERIFY(group->passThroughMode());
        }
        QTRY_VERIFY(!view->selection() || view->selection()->selectedExactRect().isEmpty());
        configure(); prepareInput();
        window->activateWindow(); input->requestActivate();
        KoToolManager::instance()->switchToolRequested(toolId);
        QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
        tool = KoToolManager::instance()->toolById(view->canvasBase(), toolId);
        QVERIFY(tool); configureFigure();
        resources->setFGColor(KoColor(whiteDefault ? Qt::black : Qt::white, cs));
        stroke({{50, 50}});
        QVERIFY2(pixels() != saved, gpu ? "GPU mask input after normal KRA Open must change coverage" : "Native mask input after normal KRA Open must change coverage");
        QVERIFY2(compare(), "Mask preset editing after KRA reload must preserve native pixels");
        const auto edited = pixels(); lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(), saved);
        lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(), edited);
        if (groupOwner) {
            auto *group = qobject_cast<KisGroupLayer *>(layer.data()); QVERIFY(group);
            group->setPassThroughMode(false); image->refreshGraphAsync(); image->waitForDone();
            QCOMPARE(pixels(), edited);
            QVERIFY2(compare(), "Returning to isolated composition must restore the retained group mask");
        }
        QVERIFY2(lifetime->saveAs(directory.filePath(gpu ? QStringLiteral("gpu-edited.kra") : QStringLiteral("cpu-edited.kra")),
            KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        if (groupOwner && gpu) {
            qputenv("LIBREPAINT_GPU_ENGINE", "0");
            const auto resetEngine = qScopeGuard([&] { qputenv("LIBREPAINT_GPU_ENGINE", "1"); });
            QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
            native->setFileBatchMode(true); native->setAutoSaveActive(false);
            QVERIFY2(native->openPath(directory.filePath(QStringLiteral("gpu-edited.kra")), KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
            auto nativeImage = KisImageSP(native->image()); nativeImage->waitForDone();
            auto nativeGroup = nativeImage->root()->firstChild(); QVERIFY(nativeGroup && nativeGroup->inherits("KisGroupLayer"));
            QCOMPARE(nativeGroup->uuid(), layer->uuid());
            KisNodeSP nativeMask, nativePaint;
            for (auto node = nativeGroup->firstChild(); node; node = node->nextSibling()) {
                if (node->inherits("KisTransparencyMask")) nativeMask = node;
                else if (node->inherits("KisPaintLayer")) nativePaint = node;
            }
            QVERIFY(nativeMask && nativePaint); QCOMPARE(nativeMask->uuid(), child->uuid());
            QByteArray actualMask(initial.size(), '\0'), actualPaint(originalParent.size(), '\0');
            nativeMask->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actualMask.data()), area);
            nativePaint->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actualPaint.data()), area);
            QCOMPARE(actualMask, edited); QCOMPARE(actualPaint, originalParent);
            QByteArray nativeProjection(originalParent.size(), '\0'), gpuProjection(originalParent.size(), '\0');
            nativeImage->projection()->readBytes(reinterpret_cast<quint8 *>(nativeProjection.data()), area);
            image->projection()->readBytes(reinterpret_cast<quint8 *>(gpuProjection.data()), area);
            QCOMPARE(nativeProjection, gpuProjection);
        }
    }
}

void KisGpuDocumentViewTest::maskChildFreehandStrokeEditsOnlyCoverage_data()
{
    QTest::addColumn<int>("creation");
    QTest::addColumn<bool>("groupOwner");
    QTest::newRow("retained-finite-mask") << 0 << false;
    QTest::newRow("ordinary-white-default") << 1 << false;
    QTest::newRow("ordinary-selection-consumed") << 2 << false;
    QTest::newRow("group-retained-finite-mask") << 0 << true;
    QTest::newRow("group-ordinary-white-default") << 1 << true;
    QTest::newRow("group-ordinary-selection-consumed") << 2 << true;
}

void KisGpuDocumentViewTest::maskChildFreehandStrokeEditsOnlyCoverage()
{
    QFETCH(int, creation); QFETCH(bool, groupOwner);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster; raster.size = {64, 64};
    raster.layers = {{QStringLiteral("Mask owner"), true, QUuid::createUuid(), {}}};
    if (groupOwner) {
        raster.layers.first().kind = Gpu::LayerKind::Group;
        Gpu::RasterLayer child{QStringLiteral("Child paint"), true, QUuid::createUuid(), {}};
        child.parentUuid = raster.layers.first().uuid; raster.layers.append(child);
    }
    const auto owner = paintImageWithEncoding(execution, raster);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    const auto layerId = owner->operations().activeLayer();
    if (creation != 1) QCOMPARE(owner->setSelectionRect(QRect(8, 8, 24, 24)), Gpu::Result::Success);
    if (creation == 0) {
        QCOMPARE(owner->addTransparencyMaskFromSelection(layerId), Gpu::Result::Success);
        QCOMPARE(owner->clearSelection(), Gpu::Result::Success);
    }

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask freehand input")));
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
    QTest::qWait(100);
    auto image = KisImageSP(lifetime->image());
    auto layer = image->root()->firstChild();
    QVERIFY(layer);
    auto *manager = window->viewManager()->nodeManager();
    if (creation) {
        const auto before = owner->operations().undoCount();
        const auto child = manager->createNode(QStringLiteral("KisTransparencyMask"));
        QVERIFY(child && image->isGpuTransparencyMaskView(child));
        QCOMPARE(view->currentNode(), child);
        QCOMPARE(owner->operations().undoCount(), before + 1);
        QVERIFY(owner->operations().selectionRect().isEmpty());
        QVERIFY(!view->selection() || view->selection()->selectedExactRect().isEmpty());
    }
    const auto originalMask = owner->layerTransparencyMask(layerId);
    QVERIFY(originalMask);
    QCOMPARE(originalMask->coverage(44, 44), quint8(creation == 1 ? 255 : 0));
    auto child = layer->firstChild();
    while (child && !child->inherits("KisTransparencyMask")) child = child->nextSibling();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));
    QVERIFY(image->isGpuTransparencyMaskView(child));
    if (!creation) manager->slotUiActivatedNode(child);
    QCOMPARE(view->currentNode(), child);
    QCOMPARE(view->currentMask().data(), child.data());

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
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
    const auto pointAt = [&](QPointF imagePoint) {
        const auto point = converter->imageToWidget(imagePoint).toPoint();
        QTest::mouseMove(native.data(), point);
        QTest::qWait(10);
        return point;
    };
    const auto start = pointAt({40, 40});
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(start));
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::qWait(10);
    const int undoBefore = owner->operations().undoCount();
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The pointer press did not start a mask stroke" : qPrintable(errors.first().first().toString()));
    QVERIFY(owner->operations().snapshot()->layers.first().operations.isEmpty());
    const auto end = pointAt({48, 48});
    QTest::qWait(10);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, end);
    pressed = false;
    QTRY_COMPARE(owner->operations().undoCount(), undoBefore + 1);
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::PaintStroke);
    const auto edited = owner->layerTransparencyMask(layerId);
    QVERIFY(edited);
    const auto editedCoverage = owner->execution().readSelectionPixels(edited, {0, 0, 64, 64}, 64 * 64);
    QCOMPARE(editedCoverage.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(editedCoverage.bytes[44 * 64 + 44]) > 0);
    QCOMPARE(quint8(editedCoverage.bytes[12 * 64 + 12]), quint8(255));
    QVERIFY(owner->operations().snapshot()->layers.first().operations.isEmpty());
    const auto layerPixels = owner->readRgbaPixels(layerId, QRect(44, 44, 1, 1), 4);
    QCOMPARE(layerPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(layerPixels.bytes.at(3)), quint8(0));
    quint8 childPixel = 0;
    child->paintDevice()->readBytes(&childPixel, 44, 44, 1, 1);
    QVERIFY(childPixel > 0);

    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    const auto moveId = QStringLiteral("KritaTransform/KisToolMove");
    KoToolManager::instance()->switchToolRequested(moveId);
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), moveId);
    auto *moveTool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), moveId));
    QVERIFY(moveTool);
    for (QWidget *panel : moveTool->optionWidgets())
        if (auto *button = panel->findChild<QRadioButton *>(QStringLiteral("radioSelectedLayer"))) button->setChecked(true);
    const auto beforeMove = owner->operations().snapshot();
    const auto beforeMoveHistory = owner->operations().undoCount();
    window->windowHandle()->requestActivate();
    window->menuBar()->setFocus();
    QTRY_VERIFY(window->menuBar()->hasFocus());
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    pressed = true;
    QTest::mouseMove(native.data(), converter->imageToWidget(QPointF(46, 37)).toPoint(), 20);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, converter->imageToWidget(QPointF(46, 37)).toPoint());
    pressed = false;
    QVERIFY(owner->operations().hasActiveStroke());
    QVERIFY(owner->operations().preview() != beforeMove);
    QTest::keyClick(window->windowHandle(), Qt::Key_Escape);
    QTRY_VERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot()->revision(), beforeMove->revision());
    QCOMPARE(owner->operations().preview()->revision(), beforeMove->revision());
    QCOMPARE(owner->operations().undoCount(), beforeMoveHistory);
}

void KisGpuDocumentViewTest::layerPanelTogglesAndRemovesGpuTransparencyMaskChild()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask child actions")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    QVERIFY(layer);
    QVERIFY(image->setGpuSelectionRect(QRect(8, 8, 32, 32)));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));
    const auto child = layer->firstChild();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));

    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    auto *model = qobject_cast<KisNodeModel*>(proxy->sourceModel());
    QVERIFY(model);
    QTRY_VERIFY(proxy->indexFromNode(child).isValid());
    const auto index = proxy->indexFromNode(child);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(child);
    manager->slotSetSelectedNodes({child});
    QCOMPARE(view->currentNode(), child);

    // The panel's visibility toggle is the mask visibility action.
    auto properties = child->sectionModelProperties();
    bool found = false;
    for (auto &property : properties) {
        if (property.id == KisLayerPropertiesIcons::visible.id()) {
            property.state = false;
            found = true;
        }
    }
    QVERIFY(found);
    const auto visibilityHistory = lifetime->undoStack()->count();
    QVERIFY(proxy->setData(index, QVariant::fromValue(properties), KisNodeModel::PropertiesRole));
    QVERIFY(!image->gpuTransparencyMaskVisible(layer));
    QVERIFY(!child->visible());
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::ChangeMaskVisibility);
    QCOMPARE(lifetime->undoStack()->count(), visibilityHistory + 1);

    lifetime->undoStack()->undo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(image->gpuTransparencyMaskVisible(layer));
    QVERIFY(child->visible());
    lifetime->undoStack()->redo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(!image->gpuTransparencyMaskVisible(layer));
    QVERIFY(!child->visible());

    // Deleting the panel child removes the retained GPU mask.
    manager->slotUiActivatedNode(child);
    manager->slotSetSelectedNodes({child});
    const auto removalHistory = lifetime->undoStack()->count();
    manager->removeNode();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(!owner->operations().snapshot()->layers.first().transparencyMask);
    QVERIFY(!layer->firstChild());
    QCOMPARE(view->currentNode(), layer);
    QVERIFY(!view->currentMask());
    QCOMPARE(lifetime->undoStack()->count(), removalHistory + 1);

    // Undo restores both the child and the visibility it had before removal.
    lifetime->undoStack()->undo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    const auto restored = layer->firstChild();
    QVERIFY(restored);
    QVERIFY(restored->inherits("KisTransparencyMask"));
    QVERIFY(owner->operations().snapshot()->layers.first().transparencyMask);
    QVERIFY(!image->gpuTransparencyMaskVisible(layer));
    QVERIFY(!restored->visible());
    QCOMPARE(view->currentNode(), layer);

    lifetime->undoStack()->redo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(!layer->firstChild());
    QVERIFY(!owner->operations().snapshot()->layers.first().transparencyMask);
}

void KisGpuDocumentViewTest::layerPanelGpuTransparencyMaskChildSurvivesSaving()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask child saving")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    QVERIFY(layer);

    QByteArray coverage(32 * 32, '\0');
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 16; ++x) coverage[y * 32 + x] = 1;
    const auto selectionMask = Gpu::SelectionMask::fromCoverage(QRect(0, 0, 32, 32), coverage,
                                                                QRect(0, 0, 64, 64));
    QVERIFY(selectionMask);
    QVERIFY(image->setGpuSelectionMask(selectionMask));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));
    QVERIFY(image->setGpuTransparencyMaskVisible(layer, false));
    auto child = layer->firstChild();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));
    QVERIFY(!child->visible());

    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    QTRY_VERIFY(proxy->indexFromNode(child).isValid());
    window->viewManager()->nodeManager()->slotUiActivatedNode(child);
    QCOMPARE(view->currentNode(), child);

    const auto identity = child->uuid();
    QCOMPARE(identity, owner->operations().snapshot()->layers.first().transparencyMaskUuid);
    const auto fixed = owner->operations().snapshot();
    auto *manager = window->viewManager()->nodeManager();
    manager->setNodeName(child, QStringLiteral("Retained cutouts"));
    QCOMPARE(child->name(), QStringLiteral("Retained cutouts"));
    QCOMPARE(owner->operations().snapshot()->layers.first().transparencyMaskName, child->name());
    QCOMPARE(layer->name(), fixed->layers.first().name);
    lifetime->undoStack()->undo();
    QCOMPARE(child->name(), fixed->layers.first().transparencyMaskName);
    lifetime->undoStack()->redo();
    QCOMPARE(child->name(), QStringLiteral("Retained cutouts"));
    child->setColorLabelIndex(4);
    QCOMPARE(child->colorLabelIndex(), 4);
    QCOMPARE(owner->operations().snapshot()->layers.first().transparencyMaskColorLabelIndex, 4);
    lifetime->undoStack()->undo();
    QCOMPARE(child->name(), fixed->layers.first().transparencyMaskName);
    QCOMPARE(child->colorLabelIndex(), 4);
    lifetime->undoStack()->redo();
    child->setPinnedToTimeline(true);
    QVERIFY(owner->operations().snapshot()->layers.first().transparencyMaskPinnedToTimeline);
    QCOMPARE(fixed->layers.first().transparencyMaskColorLabelIndex, 0);
    QCOMPARE(fixed->layers.first().transparencyMaskUuid, identity);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("mask-child.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QVERIFY(loaded.document.image.layers.first().transparencyMask);
    QVERIFY(!loaded.document.image.layers.first().transparencyMaskVisible);
    QCOMPARE(loaded.document.image.layers.first().transparencyMaskUuid, identity);
    QCOMPARE(loaded.document.image.layers.first().transparencyMaskName, QStringLiteral("Retained cutouts"));
    QCOMPARE(loaded.document.image.layers.first().transparencyMaskColorLabelIndex, 4);
    QVERIFY(loaded.document.image.layers.first().transparencyMaskPinnedToTimeline);
    QCOMPARE(loaded.document.image.layers.first().transparencyMask->coverage(4, 16), quint8(255));
    QCOMPARE(loaded.document.image.layers.first().transparencyMask->coverage(28, 16), quint8(0));

    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QVERIFY(reopened);
    lifetime->setCurrentImage(KisImage::fromGpuImage(reopened, QStringLiteral("Reopened mask child")));
    QTRY_VERIFY(view->currentNode() && view->currentNode()->image().data() == lifetime->image().data());
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    auto reopenedImage = KisImageSP(lifetime->image());
    QVERIFY(reopenedImage);
    const auto reopenedLayer = reopenedImage->root()->firstChild();
    QVERIFY(reopenedLayer);
    const auto reopenedChild = reopenedLayer->firstChild();
    QVERIFY(reopenedChild);
    QVERIFY(reopenedChild->inherits("KisTransparencyMask"));
    QVERIFY(reopenedImage->isGpuTransparencyMaskView(reopenedChild));
    QVERIFY(!reopenedChild->visible());
    QCOMPARE(reopenedChild->uuid(), identity);
    QCOMPARE(reopenedChild->name(), QStringLiteral("Retained cutouts"));
    QCOMPARE(reopenedChild->colorLabelIndex(), 4);
    QVERIFY(reopenedChild->isPinnedToTimeline());
    window->viewManager()->nodeManager()->setNodeName(reopenedChild, QStringLiteral("Edited after reload"));
    QCOMPARE(reopened->operations().snapshot()->layers.first().transparencyMaskName, QStringLiteral("Edited after reload"));
    lifetime->undoStack()->undo(); QCOMPARE(reopenedChild->name(), QStringLiteral("Retained cutouts"));
    lifetime->undoStack()->redo(); QCOMPARE(reopenedChild->name(), QStringLiteral("Edited after reload"));
    QVERIFY(!reopened->operations().snapshot()->layers.first().transparencyMaskVisible);
    quint8 selected = 0;
    quint8 unselected = 255;
    reopenedChild->paintDevice()->readBytes(&selected, 4, 16, 1, 1);
    reopenedChild->paintDevice()->readBytes(&unselected, 28, 16, 1, 1);
    QCOMPARE(selected, quint8(255));
    QCOMPARE(unselected, quint8(0));

    QTRY_VERIFY(proxy->indexFromNode(reopenedChild).isValid());
}

