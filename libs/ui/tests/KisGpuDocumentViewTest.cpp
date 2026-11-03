/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDocumentViewTest.h"

void KisGpuDocumentViewTest::layerPanelThumbnailsReadOnlyReducedGpuPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device, 8 * Gpu::TileRenderer::TileBytes, 1, 4096);
    QByteArray pixels(Gpu::RgbaTile::ByteCount, '\0');
    for (qsizetype i = 0; i < pixels.size(); i += 4) pixels.replace(i, 4, QByteArray::fromHex("316ac2c8"));
    Gpu::RasterImage raster;
    raster.size = {256, 256};
    raster.layers = {{QStringLiteral("Thumbnail"), true, {}, {{{0, 0}, pixels}}}};
    const auto owner = paintImageWithEncoding(execution, raster);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer thumbnails")));
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
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
    window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    QTRY_VERIFY(!windowReady.isEmpty());
    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    auto *model = qobject_cast<KisNodeModel*>(proxy->sourceModel());
    QVERIFY(model);
    model->setPreferredThumnalSize(32);
    const auto node = lifetime->image()->root()->firstChild();
    QTRY_VERIFY(proxy->indexFromNode(node).isValid());
    const auto thumbnail = [&] { return proxy->data(proxy->indexFromNode(node), int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>(); };
    QSignalSpy changes(owner.data(), &Gpu::PaintImage::changed);
    const auto before = thumbnail();
    QCOMPARE(before.size(), QSize(32, 32));
    QCOMPARE(before.pixelColor(16, 16), QColor(49, 106, 194, 200));
    QCOMPARE(changes.size(), 0);
    QCOMPARE(owner->operations().snapshot(), saved);
    QCOMPARE(lifetime->undoStack()->count(), 0);
    QCOMPARE(execution->beginReadbackFrame().code, Gpu::ExecutionCode::Success);
    QCOMPARE(thumbnail(), before);
    QCOMPARE(execution->cancelFrame().code, Gpu::ExecutionCode::Success);
    Gpu::Brush brush;
    brush.diameter = 9;
    brush.rgba = {211, 39, 17, 255};
    const auto stroke = owner->beginStroke(owner->operations().activeLayer(), brush, 17);
    QCOMPARE(owner->appendSamples(stroke, {{{128.5, 128.5}, 1}}), Gpu::Result::Success);
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    QTRY_COMPARE(thumbnail().pixelColor(16, 16), QColor(211, 39, 17, 255));
    QCOMPARE(thumbnail().pixelColor(31, 31), QColor(49, 106, 194, 200));
    QCOMPARE(lifetime->undoStack()->count(), 1);
    auto *historyDock = window->findChild<QDockWidget*>(QStringLiteral("History"));
    QVERIFY(historyDock);
    auto *historyView = qobject_cast<QListView*>(historyDock->widget());
    QVERIFY(historyView);
    QImage historyImage;
    QTRY_VERIFY(!(historyImage = historyView->model()->data(historyView->model()->index(1, 0), Qt::DecorationRole).value<QImage>()).isNull());
    QCOMPARE(historyImage.pixelColor(historyImage.width() / 2, historyImage.height() / 2), QColor(211, 39, 17, 255));
    const auto after = thumbnail();
    lifetime->undoStack()->undo();
    QTRY_COMPARE(thumbnail(), before);
    QCOMPARE(lifetime->undoStack()->count(), 1);
    QCOMPARE(historyView->model()->data(historyView->model()->index(1, 0), Qt::DecorationRole).value<QImage>(), historyImage);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const QList<QPair<QString, QImage>> images{{QStringLiteral("layer-thumbnail-before"), before},
            {QStringLiteral("layer-thumbnail-after"), after}, {QStringLiteral("history-thumbnail-fixed"), historyImage}};
        for (const auto &entry : images) {
            const auto rgba = entry.second.convertToFormat(QImage::Format_RGBA8888);
            QFile file(QDir(results).filePath(entry.first + QStringLiteral(".rgba")));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(reinterpret_cast<const char*>(rgba.constBits()), rgba.sizeInBytes()), rgba.sizeInBytes());
            QVERIFY(entry.second.save(QDir(results).filePath(entry.first + QStringLiteral(".png"))));
        }
    }
}

void KisGpuDocumentViewTest::colorFillActionsPreservePixelsHistoryAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<QByteArray>("blank");
    QTest::addColumn<QByteArray>("covered");
    struct Case { const char *name; const char *blank; const char *covered; };
    const Case cases[] = {
        {"foreground", "f18111ff", "f18111ff"},
        {"background", "2c11ddff", "2c11ddff"},
        {"foreground-opacity", "f1811180", "815222ff"},
        {"background-opacity", "2c11dd80", "1f1988ff"},
        {"half-alpha-opacity", "f1811180", "815222ff"},
        {"alpha-lock", "f1811100", "f18111ff"},
        {"layer-alpha-lock", "f1811100", "f18111ff"},
        {"channel-lock", "f18100ff", "f18133ff"},
        {"erase", "00000000", "11223300"},
        {"zero-opacity", "00000000", "112233ff"}
    };
    for (const bool gpu : {false, true}) for (const auto &c : cases) {
        const QByteArray name = QByteArray(gpu ? "gpu-" : "ordinary-") + c.name;
        QTest::newRow(name.constData()) << gpu << QString::fromLatin1(c.name)
            << QByteArray::fromHex(c.blank) << QByteArray::fromHex(c.covered);
    }
    QTest::newRow("gpu-pattern") << true << QStringLiteral("pattern")
        << QByteArray::fromHex("664422c8") << QByteArray::fromHex("543d26ff");
    QTest::newRow("gpu-pattern-opacity") << true << QStringLiteral("pattern-opacity")
        << QByteArray::fromHex("66442264") << QByteArray::fromHex("322f2cff");
    QTest::newRow("gpu-pattern-erase") << true << QStringLiteral("pattern-erase")
        << QByteArray::fromHex("00000000") << QByteArray::fromHex("11223337");
    QTest::newRow("gpu-multiply") << true << QStringLiteral("multiply")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("101103ff");
    QTest::newRow("gpu-screen") << true << QStringLiteral("screen")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("f29241ff");
    QTest::newRow("gpu-darken") << true << QStringLiteral("darken")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("112211ff");
    QTest::newRow("gpu-lighten") << true << QStringLiteral("lighten")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("f18133ff");
    QTest::newRow("gpu-add") << true << QStringLiteral("add")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("ffa344ff");
    QTest::newRow("gpu-linear-dodge") << true << QStringLiteral("linear_dodge")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("ffa344ff");
    QTest::newRow("gpu-pattern-multiply") << true << QStringLiteral("pattern-multiply")
        << QByteArray::fromHex("664422c8") << QByteArray::fromHex("090e10ff");
    for (const auto &scenario : {"reject-pattern-blend", "reject-blend", "close-during-fill"}) {
        const QByteArray name = QByteArray("gpu-") + scenario;
        QTest::newRow(name.constData()) << true << QString::fromLatin1(scenario)
            << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("f18111ff");
    }
}

void KisGpuDocumentViewTest::colorFillActionsPreservePixelsHistoryAndSaving()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    QFETCH(QString, scenario);
    QFETCH(QByteArray, blank);
    QFETCH(QByteArray, covered);
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto oldRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreCanvas = qScopeGuard([&] {
        if (hadRenderer) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), oldRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    namespace Gpu = LibrePaint::Image::Gpu;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        owner = paintImageWithEncoding(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Other")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Color fill")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Color fill"), 32, 16, cs,
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
    auto image = KisImageSP(lifetime->image());
    auto target = image->root()->at(1);
    auto other = image->root()->at(0);
    other->setOpacity(0);
    const auto seed = QByteArray::fromHex("112233ff");
    const auto read = [](KisNodeSP node, QPoint point) {
        QByteArray bytes(4, '\0');
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), point.x(), point.y(), 1, 1);
        return bytes;
    };
    for (auto node : {target, other}) for (const auto &point : {QPoint(3, 5), QPoint(-5, 4), QPoint(40, 20)})
        node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(seed.constData()), point.x(), point.y(), 1, 1);
    if (scenario == QLatin1String("layer-alpha-lock")) qobject_cast<KisPaintLayer *>(target.data())->setAlphaLocked(true);
    if (scenario == QLatin1String("channel-lock")) {
        QBitArray channels(4, true);
        channels.setBit(2, false); // Public BGRA8 paint-device channels protect red.
        qobject_cast<KisPaintLayer *>(target.data())->setChannelLockFlags(channels);
    }
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(target);
    manager->slotSetSelectedNodes({other});
    auto *resources = view->resourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setFGColor(KoColor(QColor(17, 129, 241, scenario == QLatin1String("half-alpha-opacity") ? 128 : 255), cs));
    resources->setBGColor(KoColor(QColor(221, 17, 44), cs));
    const bool withOpacity = scenario.endsWith(QLatin1String("opacity"));
    resources->setOpacity(scenario == QLatin1String("zero-opacity") ? 0 : qreal(withOpacity ? 128 : 64) / 255);
    const bool patternScenario = scenario.startsWith(QLatin1String("pattern")) ||
        scenario.startsWith(QLatin1String("reject-pattern"));
    if (patternScenario) {
        QImage patternImage(2, 2, QImage::Format_ARGB32);
        patternImage.fill(QColor(34, 68, 102, 200));
        resources->slotPatternActivated(KoPatternSP(new KoPattern(patternImage,
            QStringLiteral("GPU test pattern"), QString())));
    }
    const bool eraseScenario = scenario == QLatin1String("erase") || scenario == QLatin1String("pattern-erase");
    resources->setEraserMode(eraseScenario);
    QString compositeOp = QStringLiteral("normal");
    if (scenario == QLatin1String("pattern-multiply"))
        compositeOp = QStringLiteral("multiply");
    else if (scenario == QLatin1String("multiply") || scenario == QLatin1String("screen") ||
             scenario == QLatin1String("darken") || scenario == QLatin1String("lighten") ||
             scenario == QLatin1String("add") || scenario == QLatin1String("linear_dodge"))
        compositeOp = scenario;
    else if (scenario.startsWith(QLatin1String("reject-")))
        compositeOp = QStringLiteral("librepaint-test-invalid-composite");
    resources->setCurrentCompositeOp(compositeOp);
    resources->resourceManager()->setResource(KoCanvasResource::GlobalAlphaLock,
        scenario == QLatin1String("alpha-lock") || eraseScenario);
    const auto count = lifetime->undoStack()->count();
    const auto index = lifetime->undoStack()->index();
    if (owner) QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    else lifetime->undoStack()->setClean();
    const auto actionName = (patternScenario ? QStringLiteral("fill_selection_pattern") :
        QStringLiteral("fill_selection_") +
            (scenario.startsWith(QLatin1String("background")) ? QStringLiteral("background_color") : QStringLiteral("foreground_color"))) +
        (withOpacity ? QStringLiteral("_opacity") : QString());
    auto *action = window->viewManager()->actionManager()->actionByName(actionName);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto before = owner ? owner->operations().snapshot() : Gpu::Snapshot{};
    QMetaObject::Connection close;
    if (scenario == QLatin1String("close-during-fill")) {
        close = connect(owner.data(), &Gpu::PaintImage::changed, window.get(), [&] {
            QObject::disconnect(close);
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            window.reset();
        });
    }
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    if (scenario == QLatin1String("close-during-fill")) {
        QVERIFY(!window);
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!lifetime);
        QCOMPARE(owner->operations().undoCount(), count + 1);
        QCOMPARE(read(target, {8, 8}), blank);
        return;
    }
    if (scenario.startsWith(QLatin1String("reject-"))) {
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(lifetime->undoStack()->count(), count);
        QVERIFY(lifetime->undoStack()->isClean());
        QCOMPARE(read(target, {8, 8}), QByteArray(4, '\0'));
        QCOMPARE(read(target, {3, 5}), seed);
        return;
    }
    resources->setFGColor(KoColor(Qt::green, cs));
    resources->setBGColor(KoColor(Qt::yellow, cs));
    resources->setOpacity(1);
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(lifetime->undoStack()->undoText(), kundo2_i18n("Flood Fill Layer").toString());
    QCOMPARE(read(target, {8, 8}), blank);
    QCOMPARE(read(target, {3, 5}), covered);
    QCOMPARE(read(target, {-5, 4}), seed);
    QCOMPARE(read(target, {40, 20}), seed);
    QCOMPARE(read(other, {3, 5}), seed);
    QCOMPARE(manager->activeNode(), target);
    QCOMPARE(manager->selectedNodes(), KisNodeList{other});
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), index);
    QCOMPARE(read(target, {8, 8}), QByteArray(4, '\0'));
    QCOMPARE(read(target, {3, 5}), seed);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(read(target, {8, 8}), blank);
    QCOMPARE(read(target, {3, 5}), covered);
    const QRect region(-8, -8, 64, 40);
    QHash<QUuid, QByteArray> expectedLayers;
    for (const auto &node : {target, other}) {
        QByteArray bytes(region.width() * region.height() * 4, '\0');
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region.x(), region.y(), region.width(), region.height());
        for (qsizetype i = 0; i < bytes.size(); i += 4) std::swap(bytes[i], bytes[i + 2]);
        expectedLayers.insert(node->uuid(), bytes);
    }
    const auto projected = image->projection()->convertToQImage(nullptr, 0, 0, 32, 16).convertToFormat(QImage::Format_RGBA8888);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("filled.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QVERIFY(lifetime->undoStack()->isClean());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::PaintImage reopened(execution, loaded.document.image);
    QCOMPARE(reopened.operations().snapshot()->layers.size(), 2);
    QByteArray canonical;
    for (const auto &layer : reopened.operations().snapshot()->layers) {
        QVERIFY(expectedLayers.contains(layer.uuid));
        const auto pixels = reopened.readRgbaPixels(layer.id, region, region.width() * region.height() * 4);
        QVERIFY2(pixels.code == Gpu::ExecutionCode::Success, qPrintable(pixels.diagnostic));
        QCOMPARE(pixels.bytes, expectedLayers.value(layer.uuid));
        canonical.append(pixels.bytes);
    }
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("alpha", true);
    configuration->setProperty("interlaced", false);
    configuration->setProperty("compression", 3);
    configuration->setProperty("saveSRGBProfile", true);
    configuration->setProperty("forceSRGB", false);
    configuration->setProperty("storeAuthor", false);
    configuration->setProperty("storeMetaData", false);
    const auto pngPath = directory.filePath(QStringLiteral("filled.png"));
    QVERIFY2(lifetime->exportDocumentSync(pngPath, QByteArrayLiteral("image/png"), configuration), qPrintable(lifetime->errorMessage()));
    const auto png = QImage(pngPath).convertToFormat(QImage::Format_RGBA8888);
    QCOMPARE(png.size(), projected.size());
    QCOMPARE(QByteArray::fromRawData(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()),
             QByteArray::fromRawData(reinterpret_cast<const char *>(projected.constBits()), projected.sizeInBytes()));
    QByteArray expectedProjection(32 * 16 * 4, '\0');
    const auto projectionPixel = [](QByteArray pixel) {
        if (pixel[3] == 0) return QByteArray(4, '\0');
        std::swap(pixel[0], pixel[2]);
        return pixel;
    };
    const auto backgroundPixel = projectionPixel(blank);
    for (qsizetype i = 0; i < expectedProjection.size(); i += 4)
        std::copy(backgroundPixel.cbegin(), backgroundPixel.cend(), expectedProjection.begin() + i);
    const auto coveredPixel = projectionPixel(covered);
    std::copy(coveredPixel.cbegin(), coveredPixel.cend(), expectedProjection.begin() + (5 * 32 + 3) * 4);
    QCOMPARE(QByteArray::fromRawData(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), expectedProjection);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        const auto stem = QDir(resultDirectory).filePath(QStringLiteral("fill-") + QString::fromLatin1(QTest::currentDataTag()));
        QFile pixels(stem + QStringLiteral(".rgba"));
        QVERIFY(pixels.open(QIODevice::WriteOnly));
        QCOMPARE(pixels.write(canonical), canonical.size());
        QFile exported(stem + QStringLiteral(".png.rgba"));
        QVERIFY(exported.open(QIODevice::WriteOnly));
        QCOMPARE(exported.write(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), png.sizeInBytes());
        QVERIFY(QFile::copy(path, stem + QStringLiteral(".kra")));
        QVERIFY(QFile::copy(pngPath, stem + QStringLiteral(".png")));
    }
}

void KisGpuDocumentViewTest::clearActionPreservesSelectedLayersAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("multiple");
    QTest::addColumn<bool>("locked");
    QTest::addColumn<bool>("closeDuringClear");
    for (const bool gpu : {false, true}) {
        const auto prefix = gpu ? QByteArray("gpu-") : QByteArray("ordinary-");
        QTest::newRow(QByteArray(prefix + "single").constData()) << gpu << false << false << false;
        QTest::newRow(QByteArray(prefix + "multiple").constData()) << gpu << true << false << false;
        QTest::newRow(QByteArray(prefix + "locked").constData()) << gpu << true << true << false;
    }
    QTest::newRow("gpu-close-during-clear") << true << true << false << true;
}

void KisGpuDocumentViewTest::clearActionPreservesSelectedLayersAndHistory()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    QFETCH(bool, multiple);
    QFETCH(bool, locked);
    QFETCH(bool, closeDuringClear);
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
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        owner = paintImageWithEncoding(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Clear layers")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Clear layers"), 32, 16, cs,
                                  KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
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
    auto image = KisImageSP(lifetime->image());
    KisNodeList layers{image->root()->at(0), image->root()->at(1), image->root()->at(2)};
    const QByteArray pixel = QByteArray::fromHex("112233ff");
    const QList<QPoint> points{{3, 5}, {-4, 8}, {40, 20}};
    for (auto layer : layers) {
        for (const auto &point : points)
            layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), point.x(), point.y(), 1, 1);
    }
    if (locked) layers[1]->setUserLocked(true);
    layers[0]->setOpacity(0);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(layers[0]);
    const KisNodeList selected = multiple ? KisNodeList{layers[1], layers[2]} : KisNodeList{layers[2]};
    manager->slotSetSelectedNodes(selected);
    view->resourceProvider()->setPaintOpPreset(m_preset);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QApplication::processEvents();
    const auto count = lifetime->undoStack()->count();
    const auto index = lifetime->undoStack()->index();
    if (owner) QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    else lifetime->undoStack()->setClean();
    auto *clear = window->viewManager()->actionManager()->actionByName(QStringLiteral("clear"));
    QVERIFY(clear);
    QVERIFY(clear->isEnabled());
    if (owner) {
        const auto before = owner->operations().snapshot();
        QVERIFY(!KisToolUtils::clearImage(image, selected, KisSelectionSP(new KisSelection)));
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(lifetime->undoStack()->count(), count);
    }
    QMetaObject::Connection close;
    if (closeDuringClear) {
        close = connect(owner.data(), &Gpu::PaintImage::changed, window.get(), [&] {
            QObject::disconnect(close);
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            window.reset();
        });
    }
    try {
        clear->trigger();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    if (closeDuringClear) {
        QVERIFY(!window);
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!lifetime);
        QCOMPARE(owner->operations().undoCount(), count + 1);
        for (auto layer : selected) {
            QByteArray actual(4, '\0');
            layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 3, 5, 1, 1);
            QCOMPARE(actual, QByteArray(4, '\0'));
        }
        return;
    }
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(lifetime->undoStack()->index(), index + 1);
    const auto checkPixels = [&](bool cleared) {
        for (const auto &layer : layers) {
            QCOMPARE(image->root()->at(layers.indexOf(layer)), layer);
            for (const auto &point : points) {
                QByteArray actual(4, '\0');
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), point.x(), point.y(), 1, 1);
                QCOMPARE(actual, cleared && selected.contains(layer) && !layer->userLocked() ? QByteArray(4, '\0') : pixel);
            }
        }
        QCOMPARE(manager->activeNode(), layers[0]);
        QCOMPARE(manager->selectedNodes().size(), selected.size());
        for (const auto &layer : selected) QVERIFY(manager->selectedNodes().contains(layer));
    };
    checkPixels(true);
    QTRY_VERIFY(lifetime->isModified());
    lifetime->undoStack()->undo();
    image->waitForDone();
    checkPixels(false);
    QVERIFY(lifetime->undoStack()->isClean());
    lifetime->undoStack()->redo();
    image->waitForDone();
    checkPixels(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("cleared.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.size(), 3);
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("alpha", true);
    configuration->setProperty("interlaced", false);
    configuration->setProperty("compression", 3);
    configuration->setProperty("saveSRGBProfile", true);
    configuration->setProperty("forceSRGB", false);
    configuration->setProperty("storeAuthor", false);
    configuration->setProperty("storeMetaData", false);
    const auto pngPath = directory.filePath(QStringLiteral("cleared.png"));
    QVERIFY2(lifetime->exportDocumentSync(pngPath, QByteArrayLiteral("image/png"), configuration),
             qPrintable(lifetime->errorMessage()));
    const QImage png(pngPath);
    QCOMPARE(png.size(), QSize(32, 16));
    QCOMPARE(png.pixelColor(3, 5), multiple && !locked ? QColor(0, 0, 0, 0) : QColor(0x33, 0x22, 0x11));
    QVERIFY(lifetime->undoStack()->isClean());
    if (owner) {
        Gpu::PaintImage reopened(owner->execution(), loaded.document.image);
        QByteArray canonical;
        for (const auto &layer : reopened.operations().snapshot()->layers) {
            const auto found = std::find_if(layers.cbegin(), layers.cend(), [&](const auto &node) { return node->uuid() == layer.uuid; });
            QVERIFY(found != layers.cend());
            QCOMPARE(layer.opacity, (*found)->opacity());
            QCOMPARE(layer.userLocked, (*found)->userLocked());
            for (const auto &point : points) {
                const auto pixels = reopened.readRgbaPixels(layer.id, QRect(point, QSize(1, 1)), 4);
                QVERIFY2(pixels.code == Gpu::ExecutionCode::Success, qPrintable(pixels.diagnostic));
                QCOMPARE(pixels.bytes, selected.contains(*found) && !(*found)->userLocked() ? QByteArray(4, '\0') : QByteArray::fromHex("332211ff"));
            }
            const auto pixels = reopened.readRgbaPixels(layer.id, QRect(-8, -8, 64, 40), 64 * 40 * 4);
            QVERIFY2(pixels.code == Gpu::ExecutionCode::Success, qPrintable(pixels.diagnostic));
            canonical.append(pixels.bytes);
        }
        const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!resultDirectory.isEmpty()) {
            QVERIFY(QDir().mkpath(resultDirectory));
            const auto stem = QDir(resultDirectory).filePath(QStringLiteral("clear-") + QString::fromLatin1(QTest::currentDataTag()));
            QFile pixels(stem + QStringLiteral(".rgba"));
            QVERIFY(pixels.open(QIODevice::WriteOnly));
            QCOMPARE(pixels.write(canonical), canonical.size());
            QVERIFY(QFile::copy(path, stem + QStringLiteral(".kra")));
            QVERIFY(QFile::copy(pngPath, stem + QStringLiteral(".png")));
            const auto rgba = png.convertToFormat(QImage::Format_RGBA8888);
            QFile exported(stem + QStringLiteral(".png.rgba"));
            QVERIFY(exported.open(QIODevice::WriteOnly));
            QCOMPARE(exported.write(reinterpret_cast<const char *>(rgba.constBits()), rgba.sizeInBytes()), rgba.sizeInBytes());
        }
        const auto clearText = lifetime->undoStack()->undoText();
        const auto direct = QByteArray::fromHex("445566ff");
        layers[2]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(direct.constData()), 9, 9, 1, 1);

        QTRY_COMPARE(lifetime->undoStack()->undoText(), clearText);
        QCOMPARE(lifetime->undoStack()->count(), count + 1);
    }
}

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
    const TestUtil::PaintingEngineScope cpuReference(false);
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
        owner = paintImageWithEncoding(execution, QSize(64, 64));
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
            resources->setCurrentCompositeOp(QStringLiteral("librepaint-test-invalid-composite"));
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
        if (native) QTest::keyClick(native.data(), Qt::Key_Escape);
        else QTest::keyClick(input.data(), Qt::Key_Escape);
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
        QTRY_VERIFY(resources->eraserMode());
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

void KisGpuDocumentViewTest::layerPanelReordersSelectionWithOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("raise");
    QTest::addColumn<int>("selection");
    QTest::newRow("ordinary-lower-single") << false << false << 0;
    QTest::newRow("gpu-raise-disjoint") << true << true << 2;
}

void KisGpuDocumentViewTest::layerPanelReordersSelectionWithOneHistoryEntry()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    QFETCH(bool, raise);
    QFETCH(int, selection);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        auto owner = paintImageWithEncoding(execution, QSize(32, 16));
        for (const auto &name : {QStringLiteral("Second"), QStringLiteral("Third"), QStringLiteral("Top")})
            QVERIFY(owner->addLayer(name));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer ordering")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer ordering"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 4, {}, 1));
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
    const KisNodeList original{root->at(0), root->at(1), root->at(2), root->at(3)};
    const QList<QByteArray> pixels{QByteArray::fromHex("332211ff"), QByteArray::fromHex("665544ff"),
                                  QByteArray::fromHex("998877ff"), QByteArray::fromHex("ccbbaaff")};
    for (qsizetype i = 0; i < original.size(); ++i)
        original[i]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels[i].constData()), 5, 5, 1, 1);
    const KisNodeList selected = selection == 0 ? KisNodeList{original[1]} : selection == 1 ?
        KisNodeList{original[1], original[2]} : raise ?
        KisNodeList{original[0], original[2]} : KisNodeList{original[1], original[3]};
    auto *manager = window->viewManager()->nodeManager();
    const auto active = selected.last();
    manager->slotUiActivatedNode(active);
    manager->slotSetSelectedNodes(selected);
    const auto before = lifetime->undoStack()->count();
    try {
        if (raise) manager->raiseNode();
        else manager->lowerNode();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    const KisNodeList expected = selection == 2 ? KisNodeList{original[1], original[3], original[0], original[2]} : raise ?
        (selection == 1 ? KisNodeList{original[0], original[3], original[1], original[2]} :
                    KisNodeList{original[0], original[2], original[1], original[3]}) :
        (selection == 1 ? KisNodeList{original[1], original[2], original[0], original[3]} :
                    KisNodeList{original[1], original[0], original[2], original[3]});
    const auto order = [&] {
        KisNodeList result;
        for (quint32 i = 0; i < root->childCount(); ++i) result.append(root->at(i));
        return result;
    };
    QTRY_COMPARE(order(), expected);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QByteArray projection(4, '\0');
    image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), 5, 5, 1, 1);
    QCOMPARE(projection, pixels[original.indexOf(expected.last())]);
    QCOMPARE(view->currentNode(), active);
    const auto actualSelection = [&] {
        auto nodes = manager->selectedNodes();
        std::sort(nodes.begin(), nodes.end());
        return nodes;
    };
    auto sortedSelection = selected;
    std::sort(sortedSelection.begin(), sortedSelection.end());
    QTRY_COMPARE(actualSelection(), sortedSelection);
    if (gpu) {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath(QStringLiteral("ordered.kra"));
        lifetime->setFileBatchMode(true);
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        QTRY_VERIFY(!lifetime->isModified());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto reopened = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
        QVERIFY2(reopened.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(reopened.diagnostic));
        QCOMPARE(reopened.document.image.layers.size(), expected.size());
        for (qsizetype i = 0; i < expected.size(); ++i)
            QCOMPARE(reopened.document.image.layers[i].uuid, expected[i]->uuid());
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(order(), original);
    QCOMPARE(view->currentNode(), active);
    QTRY_COMPARE(actualSelection(), sortedSelection);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(order(), expected);
    QCOMPARE(view->currentNode(), active);
    QTRY_COMPARE(actualSelection(), sortedSelection);
    for (qsizetype i = 0; i < original.size(); ++i) {
        QByteArray actual(4, '\0');
        original[i]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
        QCOMPARE(actual, pixels[i]);
    }
}

void KisGpuDocumentViewTest::quickGroupsKeepPixelsHistoryAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("clipping");
    QTest::addColumn<bool>("outsideActive");
    QTest::newRow("native-u8-group") << false << false << false << false;
    QTest::newRow("gpu-u16-clipping-outside") << true << true << true << true;
}

void KisGpuDocumentViewTest::quickGroupsKeepPixelsHistoryAndSaving()
{
    QFETCH(bool, gpu);
    QFETCH(bool, words);
    QFETCH(bool, clipping);
    QFETCH(bool, outsideActive);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect area(0, 0, 32, 16);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setAutoSaveActive(false);
    document->setFileBatchMode(true);
    QVERIFY(document->newImage(QStringLiteral("Layer groups"), area.width(), area.height(), cs,
                               KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, outsideActive ? 3 : 2, {}, 1));
    auto image = KisImageSP(document->image());
    KisNodeList originals{image->root()->at(0), image->root()->at(1)};
    originals[0]->setName(QStringLiteral("One")); originals[1]->setName(QStringLiteral("Two"));
    for (auto node : originals) {
        node->setVisible(true); node->setUserLocked(false); node->setOpacity(255);
        node->setCompositeOpId(QStringLiteral("normal"));
    }
    const QList<KoColor> colors{KoColor(Qt::red, cs), KoColor(QColor(0, 0, 255, 128), cs)};
    for (int i = 0; i < originals.size(); ++i) {
        const auto pixels = QByteArray(reinterpret_cast<const char *>(colors[i].data()), cs->pixelSize()).repeated(area.width() * area.height());
        originals[i]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), area);
    }
    if (!gpu) image->refreshGraphAsync();
    image->waitForDone();
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get()); QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto close = qScopeGuard([&] {
        if (lifetime) { lifetime->setModified(false); window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger(); }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show(); window->activateWindow();
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= area.width());
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->height() >= area.height());
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(outsideActive ? image->root()->at(2) : originals.last());
    manager->slotSetSelectedNodes(originals);
    const auto allNodes = [&] {
        KisNodeList result;
        for (auto child = image->root()->firstChild(); child; child = child->nextSibling()) result.append(child);
        for (qsizetype i = 0; i < result.size(); ++i)
            for (auto child = result[i]->firstChild(); child; child = child->nextSibling()) result.append(child);
        return result;
    };
    const auto find = [&](const QUuid &uuid) {
        for (const auto &node : allNodes()) if (node->uuid() == uuid) return node;
        return KisNodeSP();
    };
    const auto selected = [&] {
        QSet<QUuid> result;
        for (const auto &node : manager->selectedNodes()) result.insert(node->uuid());
        return result;
    };
    const auto pixels = [&](const KisNodeSP &node) {
        QByteArray bytes(area.width() * area.height() * cs->pixelSize(), '\0');
        const auto device = node == image->root() ? image->projection() : node->paintDevice();
        device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        return bytes;
    };
    const auto state = [&] {
        QByteArray bytes;
        QDataStream output(&bytes, QIODevice::WriteOnly);
        for (const auto &node : allNodes()) {
            output << node->uuid() << (node->parent() == image->root() ? QUuid() : node->parent()->uuid())
                   << node->name() << QByteArray(node->metaObject()->className()) << node->x() << node->y()
                   << node->opacity() << node->visible() << node->userLocked() << node->colorLabelIndex() << node->isPinnedToTimeline();
            if (const auto layer = qobject_cast<const KisLayer *>(node.data())) {
                auto flags = layer->channelFlags();
                if (flags.isEmpty()) flags.fill(true, layer->colorSpace()->channelCount());
                output << flags;
            }
            if (const auto group = qobject_cast<const KisGroupLayer *>(node.data())) output << group->passThroughMode();
            if (qobject_cast<const KisPaintLayer *>(node.data())) {
                const auto value = node->paintDevice()->defaultPixel();
                output << QByteArray(reinterpret_cast<const char *>(value.data()), value.colorSpace()->pixelSize()) << pixels(node);
            }
        }
        return bytes;
    };
    const auto before = state();
    QTRY_COMPARE(selected(), (QSet<QUuid>{originals[0]->uuid(), originals[1]->uuid()}));
    const auto beforeProjection = pixels(image->root());
    const auto history = lifetime->undoStack()->count();
    auto *action = window->viewManager()->actionManager()->actionByName(clipping ?
        QStringLiteral("create_quick_clipping_group") : QStringLiteral("create_quick_group"));
    QVERIFY(action && action->isEnabled()); action->trigger(); image->waitForDone();
    QTRY_COMPARE(image->root()->childCount(), quint32(outsideActive ? 2 : 1));
    auto group = image->root()->lastChild();
    QVERIFY(qobject_cast<KisGroupLayer *>(group.data()));
    QTRY_COMPARE(group->childCount(), quint32(clipping ? 3 : 2));
    QCOMPARE(lifetime->undoStack()->count(), history + 1);
    QCOMPARE(group->at(0)->uuid(), originals[0]->uuid());
    QCOMPARE(group->at(1)->uuid(), originals[1]->uuid());
    QCOMPARE(pixels(image->root()), beforeProjection);
    const auto groupUuid = group->uuid();
    const auto targetUuid = clipping ? group->lastChild()->uuid() : originals[outsideActive ? 0 : 1]->uuid();
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == targetUuid);
    const QSet<QUuid> selectedAfter = clipping ? QSet<QUuid>{targetUuid} : QSet<QUuid>{originals[0]->uuid(), originals[1]->uuid()};
    QTRY_COMPARE(selected(), selectedAfter);
    if (clipping) QVERIFY(qobject_cast<KisLayer *>(group->lastChild().data())->alphaChannelDisabled());
    const auto grouped = state();
    lifetime->undoStack()->undo(); image->waitForDone(); QTRY_COMPARE(state(), before);
    QTRY_COMPARE(selected(), (QSet<QUuid>{outsideActive ? image->root()->at(2)->uuid() : originals.last()->uuid()}));
    lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), grouped);
    QTemporaryDir directory; QVERIFY(directory.isValid());
    int saves = 0;
    const auto saveAndOpen = [&] {
        const auto path = directory.filePath(QStringLiteral("groups-%1.kra").arg(++saves));
        if (!lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false)) return false;
        lifetime->waitForSavingToComplete();
        if (!lifetime->openPath(path, KisDocument::DontAddToRecent)) return false;
        image = lifetime->image(); image->waitForDone(); QApplication::processEvents();
        return true;
    };
    QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage()));
    QTRY_COMPARE(state(), grouped);
    auto target = find(targetUuid); QVERIFY(target);
    manager->slotUiActivatedNode(target); manager->slotSetSelectedNodes({target});
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == targetUuid);
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    const KoColor foreground(QColor(241, 17, 129), cs);
    resources->setFGColor(foreground); resources->setOpacity(1); resources->setCurrentCompositeOp(QStringLiteral("normal"));
    const auto beforeFill = pixels(target);
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color_opacity"));
    QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger(); image->waitForDone(); QApplication::processEvents();
    const auto expected = QByteArray(reinterpret_cast<const char *>(foreground.data()), cs->pixelSize()).repeated(area.width() * area.height());
    QTRY_COMPARE(pixels(target), expected);
    lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(target), beforeFill);
    lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(target), expected);
    const auto filledProjection = pixels(image->root());
    if (!outsideActive) QCOMPARE(filledProjection, expected);
    const auto filled = state();
    QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), filled);
    group = find(groupUuid); QVERIFY(group);
    manager->slotUiActivatedNode(group); manager->slotSetSelectedNodes({group});
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == groupUuid);
    const auto ungroupHistory = lifetime->undoStack()->count();
    auto *ungroup = window->viewManager()->actionManager()->actionByName(QStringLiteral("quick_ungroup"));
    QVERIFY(ungroup); QTRY_VERIFY(ungroup->isEnabled()); ungroup->trigger(); image->waitForDone();
    QTRY_VERIFY(!find(groupUuid));
    QCOMPARE(image->root()->childCount(), quint32((clipping ? 3 : 2) + (outsideActive ? 1 : 0)));
    QCOMPARE(lifetime->undoStack()->count(), ungroupHistory + 1);
    QCOMPARE(pixels(find(targetUuid)), expected);
    const auto ungrouped = state();
    lifetime->undoStack()->undo(); image->waitForDone(); QTRY_COMPARE(state(), filled);
    lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), ungrouped);
    QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), ungrouped);
    QCOMPARE(pixels(image->root()), filledProjection);
}

void KisGpuDocumentViewTest::layerMergesKeepNativeEditing_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("varied");
    QTest::addColumn<bool>("sparse");
    QTest::addColumn<bool>("groups");
    QTest::addColumn<int>("protection");
    QTest::newRow("down-u8-normal") << 0 << false << false << false << false << 0;
    QTest::newRow("selected-u16-mask-default") << 1 << true << false << false << false << 4;
}

void KisGpuDocumentViewTest::layerMergesKeepNativeEditing()
{
    QFETCH(int, mode);
    QFETCH(bool, words);
    QFETCH(bool, varied);
    QFETCH(bool, sparse);
    QFETCH(bool, groups);
    QFETCH(int, protection);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect canvas(0, 0, 32, 16), inspection(-16, -16, sparse ? 288 : 64, 48);
    struct Run {
        bool ok = false;
        QList<QByteArray> states, projections, mergedPixels;
        QList<bool> rootReplaced;
    } native, gpu;
    const auto run = [&](bool useGpu, Run &result) {
        qInfo() << "Merge editing backend" << (useGpu ? "gpu" : "native") << "mode" << mode << "groups" << groups;
        qputenv("LIBREPAINT_GPU_ENGINE", useGpu ? "1" : "0");
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        document->setAutoSaveActive(false); document->setFileBatchMode(true);
        QVERIFY(document->newImage(QStringLiteral("Layer merging"), canvas.width(), canvas.height(), cs,
            KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 2, {}, 1));
        auto image = KisImageSP(document->image());
        image->setDefaultProjectionColor(KoColor(QColor(12, 34, 56, 64), cs));
        KisNodeList originals{image->root()->at(0), image->root()->at(1)};
        originals[0]->setName(QStringLiteral("Lower")); originals[1]->setName(QStringLiteral("Upper"));
        const QList<KoColor> colors{KoColor(QColor(17, 91, 203, 127), cs), KoColor(QColor(231, 19, 73, 159), cs)};
        for (int i = 0; i < originals.size(); ++i) {
            auto node = originals[i];
            node->setVisible(true); node->setUserLocked(false);
            node->setOpacity(varied ? (i ? 205 : 137) : 179);
            node->setCompositeOpId(varied ? (i ? QStringLiteral("screen") : QStringLiteral("multiply")) : QStringLiteral("normal"));
            node->setColorLabelIndex(i ? 3 : 2); node->setPinnedToTimeline(i == 0);
            if (varied) { node->setX(i ? 3 : -7); node->setY(i ? -5 : 4); }
            const QRect input = sparse ? QRect(5, 3, 1, 1) : varied ? (i ? QRect(3, -3, 24, 18) : QRect(-5, 2, 24, 18)) : canvas;
            const auto pixels = QByteArray(reinterpret_cast<const char *>(colors[i].data()), cs->pixelSize()).repeated(input.width() * input.height());
            node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), input);
            if (protection == 4 && i == 1)
                node->paintDevice()->setDefaultPixel(KoColor(QColor(11, 63, 129, 127), cs));
            node->setDirty(input);
            if (sparse && i == 0) {
                node->paintDevice()->writeBytes(colors[i].data(), QRect(197, 3, 1, 1));
                node->setDirty(QRect(197, 3, 1, 1));
            }
        }
        if (protection == 1 || protection == 2)
            qobject_cast<KisLayer *>(originals[protection - 1].data())->disableAlphaChannel(true);
        else if (protection == 3) for (auto node : originals) {
            auto flags = cs->channelFlags(true, true); flags.clearBit(0);
            qobject_cast<KisLayer *>(node.data())->setChannelFlags(flags);
        }
        if (!useGpu) image->refreshGraphAsync();
        image->waitForDone();
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
        window->resize(800, 600);
        auto *view = window->newView(document.get()); QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        const auto close = qScopeGuard([&] {
            if (lifetime) { lifetime->setModified(false); window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger(); }
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        window->viewManager()->setCurrentView(view);
        window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
        window->show(); window->activateWindow();
        QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= canvas.width());
        auto *manager = window->viewManager()->nodeManager();
        auto *panel = window->findChild<QDockWidget *>(QStringLiteral("KisLayerBox")); QVERIFY(panel);
        auto *tree = panel->findChild<QTreeView *>(QStringLiteral("listLayers")); QVERIFY(tree);
        auto *proxy = qobject_cast<KisNodeFilterProxyModel *>(tree->model()); QVERIFY(proxy);
        QTRY_VERIFY(proxy->indexFromNode(originals[1]).isValid());
        manager->slotUiActivatedNode(originals[1]); manager->slotSetSelectedNodes({originals[1]});
        auto *addMask = window->viewManager()->actionManager()->actionByName(QStringLiteral("add_new_transparency_mask"));
        QVERIFY(addMask); QTRY_VERIFY(addMask->isEnabled()); addMask->trigger(); image->waitForDone();
        QTRY_COMPARE(originals[1]->childCount(), 1U);
        auto *mask = qobject_cast<KisTransparencyMask *>(originals[1]->firstChild().data()); QVERIFY(mask);
        const quint8 coverage = 211;
        KisSelectionSP selection = new KisSelection();
        selection->setDefaultBounds(KisDefaultBoundsBaseSP(new KisDefaultBounds(image)));
        selection->pixelSelection()->setDefaultPixel(KoColor(&coverage, KoColorSpaceRegistry::instance()->alpha8()));
        const QByteArray hole(16, '\0');
        selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(hole.constData()), QRect(8, 4, 4, 4));
        selection->updateProjection();
        if (useGpu) QVERIFY(image->writeGpuTransparencyMaskSelection(mask, selection));
        else { mask->setSelection(selection); mask->setDirty(); image->refreshGraphAsync(); }
        image->waitForDone();
        KisNodeList heads = originals;
        if (groups) {
            auto *group = window->viewManager()->actionManager()->actionByName(QStringLiteral("create_quick_group"));
            QVERIFY(group);
            if (mode == 2) {
                manager->slotUiActivatedNode(originals[1]); manager->slotSetSelectedNodes(originals);
                QTRY_VERIFY(group->isEnabled()); group->trigger(); image->waitForDone();
                QTRY_VERIFY(originals[1]->parent() && originals[1]->parent() != image->root());
                heads = {originals[1]->parent()};
                QTRY_VERIFY(proxy->indexFromNode(heads.first()).isValid());
            } else for (int i = 0; i < originals.size(); ++i) {
                manager->slotUiActivatedNode(originals[i]); manager->slotSetSelectedNodes({originals[i]});
                QTRY_VERIFY(group->isEnabled()); group->trigger(); image->waitForDone();
                QTRY_VERIFY(originals[i]->parent() && originals[i]->parent() != image->root());
                heads[i] = originals[i]->parent();
                QTRY_VERIFY(proxy->indexFromNode(heads[i]).isValid());
                heads[i]->setName(i ? QStringLiteral("Upper group") : QStringLiteral("Lower group"));
            }
        }
        manager->slotUiActivatedNode(heads.last());
        manager->slotSetSelectedNodes(mode == 1 ? heads : KisNodeList{heads.last()});
        QTRY_COMPARE(view->currentNode(), heads.last());
        const auto allNodes = [&] {
            KisNodeList nodes;
            for (auto child = image->root()->firstChild(); child; child = child->nextSibling()) nodes.append(child);
            for (qsizetype i = 0; i < nodes.size(); ++i)
                for (auto child = nodes[i]->firstChild(); child; child = child->nextSibling()) nodes.append(child);
            return nodes;
        };
        const auto pixels = [&](KisPaintDeviceSP device, const QRect &area) {
            QByteArray bytes(area.width() * area.height() * device->pixelSize(), '\0');
            device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area); return bytes;
        };
        const auto state = [&] {
            QByteArray bytes; QDataStream output(&bytes, QIODevice::WriteOnly);
            const auto background = image->defaultProjectionColor();
            output << QByteArray(reinterpret_cast<const char *>(background.data()), cs->pixelSize());
            const auto nodes = allNodes();
            for (auto node : nodes) {
                output << nodes.indexOf(node->parent()) << node->name() << QByteArray(node->metaObject()->className())
                       << node->x() << node->y() << node->opacity() << node->visible() << node->userLocked()
                       << node->colorLabelIndex() << node->isPinnedToTimeline();
                if (const auto layer = qobject_cast<KisLayer *>(node.data())) {
                    auto flags = layer->channelFlags();
                    if (flags.isEmpty()) flags.fill(true, layer->colorSpace()->channelCount());
                    output << layer->compositeOpId() << flags;
                }
                if (const auto group = qobject_cast<KisGroupLayer *>(node.data()))
                    output << group->passThroughMode() << group->projection()->extent();
                if (qobject_cast<KisPaintLayer *>(node.data()) || qobject_cast<KisTransparencyMask *>(node.data())) {
                    const auto value = node->paintDevice()->defaultPixel();
                    output << QByteArray(reinterpret_cast<const char *>(value.data()), value.colorSpace()->pixelSize())
                           << pixels(node->paintDevice(), inspection);
                }
            }
            return bytes;
        };
        auto oldRoot = image->root();
        const auto before = state(), beforeProjection = pixels(image->projection(), canvas);
        const auto history = lifetime->undoStack()->count();
        QByteArray observedRoot;
        QMetaObject::Connection rootObservation;
        const bool observeRoot = mode == 3 && !words && !varied && !groups;
        if (observeRoot) rootObservation = connect(image.data(), &KisImage::sigLayersChangedAsync, window.get(), [&] {
            if (image->root() == oldRoot) return;
            disconnect(rootObservation); observedRoot = state();
        });
        const auto disconnectRoot = qScopeGuard([&] { disconnect(rootObservation); });
        const QStringList actions{QStringLiteral("merge_layer"), QStringLiteral("merge_layer"), QStringLiteral("flatten_layer"),
                                  QStringLiteral("flatten_image"), QStringLiteral("new_from_visible")};
        auto *action = window->viewManager()->actionManager()->actionByName(actions[mode]);
        QVERIFY(action); QTRY_VERIFY(action->isEnabled());
        if (mode == 0 && groups)
            image->mergeDown(qobject_cast<KisLayer *>(heads.last().data()), KisMetaData::MergeStrategyRegistry::instance()->get("Drop"));
        else action->trigger();
        image->waitForDone();
        QTRY_COMPARE(lifetime->undoStack()->count(), history + 1);
        QTRY_VERIFY(view->currentNode() && view->currentNode() != heads.last());
        auto merged = view->currentNode();
        if (mode == 0 && groups) QVERIFY(qobject_cast<KisGroupLayer *>(merged.data()));
        else QVERIFY(qobject_cast<KisPaintLayer *>(merged.data()));
        QVERIFY(merged->uuid() != originals[0]->uuid() && merged->uuid() != originals[1]->uuid());
        const auto mergedUuid = merged->uuid();
        const auto mergedDevice = [&] { return merged->paintDevice() ? merged->paintDevice() : merged->projection(); };
        const auto capture = [&] {
            result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
            result.mergedPixels.append(pixels(mergedDevice(), inspection));
            result.rootReplaced.append(image->root() != oldRoot);
        };
        capture();
        if (mode == 3) QCOMPARE(pixels(oldRoot->projection(), canvas), beforeProjection);
        if (sparse) {
            merged->paintDevice()->setDefaultPixel(KoColor(QColor(9, 37, 71, 83), cs));
            merged->setDirty(inspection);
            if (!useGpu) image->refreshGraphAsync();
            image->waitForDone(); capture();
        }
        const auto after = state(); const auto mergedRaw = pixels(mergedDevice(), inspection);
        if (observeRoot) QTRY_COMPARE(observedRoot, after);
        lifetime->undoStack()->undo(); image->waitForDone();
        QTRY_COMPARE(state(), before); QTRY_COMPARE(pixels(image->projection(), canvas), beforeProjection);
        QCOMPARE(image->root(), oldRoot);
        QCOMPARE(pixels(mergedDevice(), inspection), mergedRaw);
        lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), after);
        QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == mergedUuid);
        capture();
        QTemporaryDir directory; QVERIFY(directory.isValid());
        int saves = 0;
        const auto saveAndOpen = [&] {
            const auto path = directory.filePath(QStringLiteral("merge-%1.kra").arg(++saves));
            if (!lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false)) return false;
            lifetime->waitForSavingToComplete();
            if (!lifetime->openPath(path, KisDocument::DontAddToRecent)) return false;
            image = lifetime->image(); image->waitForDone(); QApplication::processEvents();
            merged.clear();
            for (auto node : allNodes()) if (node->uuid() == mergedUuid) merged = node;
            return bool(merged);
        };
        QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), after);
        auto target = merged;
        if (mode == 0 && groups) {
            QVERIFY(merged->firstChild() && merged->firstChild()->paintDevice());
            target = merged->firstChild();
        }
        manager->slotUiActivatedNode(target); manager->slotSetSelectedNodes({target});
        QTRY_COMPARE(view->currentNode(), target);
        // Root identity is compared around merge/history, while KRA keeps the
        // persistent identity of the editable output layer.
        result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
        result.mergedPixels.append(pixels(mergedDevice(), inspection));
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setPaintOpPreset(m_preset); resources->setFGColor(KoColor(QColor(29, 211, 137), cs));
        resources->setOpacity(0.5); resources->setCurrentCompositeOp(QStringLiteral("normal"));
        auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color_opacity"));
        QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger(); image->waitForDone();
        const auto filled = state(); QVERIFY(filled != after);
        lifetime->undoStack()->undo(); image->waitForDone(); QTRY_COMPARE(state(), after);
        lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), filled);
        result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
        result.mergedPixels.append(pixels(mergedDevice(), inspection));
        QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), filled);
        result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
        result.mergedPixels.append(pixels(mergedDevice(), inspection));
        result.ok = true;
    };
    run(false, native); QVERIFY(native.ok);
    run(true, gpu); QVERIFY(gpu.ok);
    if (!qEnvironmentVariableIsEmpty("LIBREPAINT_GPU_RESULT_DIR")) {
        const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR") + QStringLiteral("/merge-") + QLatin1String(QTest::currentDataTag());
        QVERIFY(QDir().mkpath(root));
        for (const bool useGpu : {false, true}) {
            const auto &result = useGpu ? gpu : native;
            const auto prefix = useGpu ? QStringLiteral("gpu-") : QStringLiteral("native-");
            for (int i = 0; i < result.states.size(); ++i) {
                const QList<QPair<QString, QByteArray>> files{{QStringLiteral("state"), result.states[i]},
                    {QStringLiteral("projection"), result.projections[i]}, {QStringLiteral("layer"), result.mergedPixels[i]}};
                for (const auto &entry : files) {
                    QFile file(root + '/' + prefix + QString::number(i) + '.' + entry.first);
                    QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(entry.second), entry.second.size());
                }
            }
        }
    }
    QCOMPARE(gpu.states, native.states);
    QCOMPARE(gpu.projections, native.projections);
    QCOMPARE(gpu.mergedPixels, native.mergedPixels);
    QCOMPARE(gpu.rootReplaced, native.rootReplaced);
}

void KisGpuDocumentViewTest::layerPanelDuplicationPreservesPixelsSelectionAndOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("rename");
    QTest::addColumn<bool>("closeAfterChange");
    QTest::newRow("ordinary") << false << true << false;
    QTest::newRow("gpu-close-on-change") << true << false << true;
}

void KisGpuDocumentViewTest::layerPanelDuplicationPreservesPixelsSelectionAndOneHistoryEntry()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    QFETCH(bool, rename);
    QFETCH(bool, closeAfterChange);
    const bool previousRename = KisImageConfig(true).renameDuplicatedLayers();
    KisImageConfig(false).setRenameDuplicatedLayers(rename);
    const auto restoreNames = qScopeGuard([&] { KisImageConfig(false).setRenameDuplicatedLayers(previousRename); });
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer duplication")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer duplication"), 32, 16, cs,
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
    auto root = image->root();
    KisNodeList originals{root->at(0), root->at(1), root->at(2)};
    const auto pixel = QByteArray::fromHex("332211ff");
    for (int index = 0; index < originals.size(); ++index) {
        originals[index]->setName(QStringLiteral("Source %1").arg(index));
        originals[index]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 5, 5, 1, 1);
    }
    originals[2]->setUserLocked(true);
    originals[2]->setVisible(false);
    originals[0]->setOpacity(128);
    originals[0]->setPinnedToTimeline(true);
    qobject_cast<KisPaintLayer*>(originals[0].data())->setAlphaLocked(true);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(originals[0]);
    manager->slotSetSelectedNodes({originals[2], originals[0]});
    const auto before = lifetime->undoStack()->count();
    const KisImageWSP imageLifetime = image;
    const auto retainedOwner = image->gpuImage();
    QMetaObject::Connection closeOnChange;
    if (closeAfterChange) {
        closeOnChange = connect(retainedOwner.data(), &Gpu::PaintImage::changed, window.get(), [&] {
            if (!lifetime || retainedOwner->operations().snapshot()->layers.size() != 5) return;
            disconnect(closeOnChange);
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        });
        image.clear();
    }
    const auto disconnectClose = qScopeGuard([&] { disconnect(closeOnChange); });
    try {
        manager->duplicateActiveNode();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    if (closeAfterChange) {
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!lifetime);
        QTRY_VERIFY(!imageLifetime.isValid());
        QCOMPARE(root->childCount(), 5U);
        return;
    }
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 5U);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QCOMPARE(lifetime->undoStack()->undoText(), kundo2_i18n("Duplicate Nodes").toString());
    QTRY_VERIFY(lifetime->isModified());
    for (int index = 0; index < originals.size(); ++index) QCOMPARE(root->at(index), originals[index]);
    KisNodeList copies{root->at(3), root->at(4)};
    for (int index = 0; index < copies.size(); ++index) {
        auto original = originals[index * 2];
        QVERIFY(copies[index]->uuid() != original->uuid());
        QVERIFY(copies[index]->name().contains(original->name()));
        if (rename) QVERIFY(copies[index]->name() != original->name());
        else QCOMPARE(copies[index]->name(), original->name());
        QCOMPARE(copies[index]->userLocked(), original->userLocked());
        QCOMPARE(copies[index]->visible(), original->visible());
        QCOMPARE(copies[index]->opacity(), original->opacity());
        QCOMPARE(copies[index]->isPinnedToTimeline(), original->isPinnedToTimeline());
        QCOMPARE(qobject_cast<KisPaintLayer*>(copies[index].data())->alphaLocked(),
                 qobject_cast<KisPaintLayer*>(original.data())->alphaLocked());
        QByteArray actual(4, '\0');
        copies[index]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
        QCOMPARE(actual, pixel);
    }
    QTRY_COMPARE(view->currentNode(), copies[0]);
    QTRY_COMPARE(manager->selectedNodes().size(), 2);
    const auto changed = QByteArray::fromHex("665544aa");
    copies[0]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(changed.constData()), 6, 6, 1, 1);
    QByteArray actual(4, '\0');
    originals[0]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
    QCOMPARE(actual, QByteArray(4, '\0'));
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 3U);
    QTRY_COMPARE(view->currentNode(), originals[0]);
    QTRY_COMPARE(manager->selectedNodes().size(), 2);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 5U);
    QCOMPARE(root->at(3), copies[0]);
    QCOMPARE(root->at(4), copies[1]);
    copies[0]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
    QCOMPARE(actual, changed);
}

void KisGpuDocumentViewTest::layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<int>("selection");
    QTest::newRow("ordinary-single") << false << 0;
    QTest::newRow("gpu-all") << true << 2;
}

void KisGpuDocumentViewTest::layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
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
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
    const auto detachedPixel = QByteArray::fromHex("665544aa");
    try {
        for (const auto &node : selected) {
            QByteArray retained(4, '\0');
            node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(retained.data()), 5, 5, 1, 1);
            QCOMPARE(retained, pixel);
            node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(detachedPixel.constData()), 6, 6, 1, 1);
            node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(retained.data()), 6, 6, 1, 1);
            QCOMPARE(retained, detachedPixel);
            QVERIFY(node->paintDevice()->exactBounds().contains(QPoint(6, 6)));
            QVERIFY(!node->paintDevice()->createThumbnail(8, 8).isNull());
        }
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QByteArray remaining(4, '\0');
    root->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(remaining.data()), 6, 6, 1, 1);
    QCOMPARE(remaining, QByteArray(4, '\0'));
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
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
    for (const auto &node : selected) {
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
        QCOMPARE(actual, gpu ? QByteArray(4, '\0') : detachedPixel);
    }
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), selection == 0 ? 2U : 1U);
    QTRY_COMPARE(view->currentNode(), after);
    for (const auto &node : selected) {
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
        QCOMPARE(actual, gpu ? QByteArray(4, '\0') : detachedPixel);
    }
}

void KisGpuDocumentViewTest::layerPanelAddsAtSelectionWithOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("pinned");
    QTest::newRow("ordinary") << false << false;
    QTest::newRow("gpu-pinned") << true << true;
}

void KisGpuDocumentViewTest::layerPanelAddsAtSelectionWithOneHistoryEntry()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
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
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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

void KisGpuDocumentViewTest::layerPanelSelectionPreservesAnUnselectedActiveLayer_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("multiple");
    QTest::newRow("ordinary-single") << false << false;
    QTest::newRow("ordinary-multiple") << false << true;
    QTest::newRow("gpu-single") << true << false;
    QTest::newRow("gpu-multiple") << true << true;
}

void KisGpuDocumentViewTest::layerPanelSelectionPreservesAnUnselectedActiveLayer()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    QFETCH(bool, multiple);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer selection")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer selection"), 32, 16, cs,
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
    const auto active = image->root()->at(0);
    const auto middle = image->root()->at(1);
    const auto top = image->root()->at(2);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(active);
    const KisNodeList selected = multiple ? KisNodeList{middle, top} : KisNodeList{top};
    const auto before = lifetime->undoStack()->count();
    manager->slotSetSelectedNodes(selected);
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QCOMPARE(view->currentNode(), active);
    QCOMPARE(manager->selectedNodes().size(), selected.size());
    for (const auto &node : selected) QVERIFY(manager->selectedNodes().contains(node));
    QCOMPARE(lifetime->undoStack()->count(), before);
    manager->duplicateActiveNode();
    image->waitForDone();
    QTRY_COMPARE(image->root()->childCount(), 3U + quint32(selected.size()));
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QCOMPARE(image->root()->childCount(), 3U);
    QTRY_COMPARE(view->currentNode(), active);
    QCOMPARE(manager->selectedNodes().size(), selected.size());
    for (const auto &node : selected) QVERIFY(manager->selectedNodes().contains(node));
    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    tree->selectionModel()->clearSelection();
    QTRY_COMPARE(manager->selectedNodes(), KisNodeList{active});
    QCOMPARE(view->currentNode(), active);
}

void KisGpuDocumentViewTest::layerSelectionCanCloseTheWindowDuringNotification_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("ordinary-selection-notification") << false;
    QTest::newRow("gpu-image-notification") << true;
}

