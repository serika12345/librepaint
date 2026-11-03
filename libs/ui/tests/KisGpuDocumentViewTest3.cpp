/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDocumentViewTest.h"

void KisGpuDocumentViewTest::rectangularSelectionLimitsPainting()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2 + 0.5, imageSize.height() / 2 + 0.5});
    QTRY_COMPARE(execution->published(owner->handle()), saved);
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);

    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QRect selected(64, 64, 128, 128);
    QTest::mouseMove(native.data(), point({64, 64}));
    const auto start = point({64, 64});
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(native.data(), point({192, 192}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == selected);
    QTRY_COMPARE(owner->selectionRect(), selected);

    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({128, 128}));
    QTest::mouseMove(native.data(), point({32, 128}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({32, 128}));
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(256, 256));
    const auto painted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(painted.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 256 + x) * 4, 4); };
    QCOMPARE(pixel(painted.bytes, 128, 128), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(painted.bytes, 32, 128), QByteArray(4, char(0)));
    QCOMPARE(pixel(painted.bytes, 224, 128), QByteArray(4, char(0)));

    auto *deselect = window->viewManager()->actionManager()->actionByName(QStringLiteral("deselect"));
    QVERIFY(deselect);
    deselect->trigger();
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QVERIFY(!lifetime->image()->globalSelection() ||
            lifetime->image()->globalSelection()->selectedExactRect().isEmpty());
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({32, 32}));
    QTest::mouseMove(native.data(), point({224, 32}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({224, 32}));
    QTRY_COMPARE(owner->operations().undoCount(), 4);
    const auto unrestricted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(unrestricted.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(unrestricted.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(unrestricted.bytes, 128, 128), QByteArray::fromHex("1181f1ff"));
}

void KisGpuDocumentViewTest::selectionFillUsesGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection fill")));
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
    window->show();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    QVERIFY(lifetime->image()->setGpuSelectionRect(QRect(16, 16, 32, 32)));
    QTRY_COMPARE(owner->selectionRect(), QRect(16, 16, 32, 32));
    QCOMPARE(owner->operations().undoCount(), 1);
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 24, 24), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 8, 8), QByteArray(4, char(0)));
    QCOMPARE(pixel(filled.bytes, 56, 24), QByteArray(4, char(0)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 24, 24), QByteArray(4, char(0)));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    const auto redone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(redone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(redone.bytes, 24, 24), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(redone.bytes, 8, 8), QByteArray(4, char(0)));
}

void KisGpuDocumentViewTest::nativeClipboardCoverageKeepsPrecisionAndSavedPixels_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("merged");
    QTest::addColumn<bool>("keepInside");
    QTest::addColumn<bool>("implicit");
    for (const bool words : {false, true}) for (const bool merged : {false, true})
        for (const bool keepInside : {false, true}) for (const bool implicit : {false, true}) {
            const auto name = QStringLiteral("%1-%2-%3-%4").arg(words ? QStringLiteral("u16") : QStringLiteral("u8"),
                merged ? QStringLiteral("merged") : QStringLiteral("layer"),
                keepInside ? QStringLiteral("inside") : QStringLiteral("outside")).arg(implicit ? QStringLiteral("implicit") : QStringLiteral("tiles"));
            QTest::newRow(qPrintable(name)) << words << merged << keepInside << implicit;
        }
}

void KisGpuDocumentViewTest::nativeClipboardCoverageKeepsPrecisionAndSavedPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(bool, merged);
    QFETCH(bool, keepInside);
    QFETCH(bool, implicit);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const Gpu::RgbaPixel color = words ? Gpu::RgbaPixel{4353, 33282, 61955, 65535} : Gpu::RgbaPixel{17, 129, 241, 255};
    const auto pixel = Gpu::rgbaBytes(color, format);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = format;
    raster.layers.append({QStringLiteral("Clipboard source"), true, QUuid::createUuid(),
                          {{{0, 0}, pixel.repeated(Gpu::RgbaTile::byteCount(format) / pixel.size()), format}}});
    if (implicit) {
        raster.layers.first().tiles.clear();
        raster.layers.first().defaultPixel = color;
    } else {
        auto &tile = raster.layers.first().tiles.first().rgba;
        const quint16 alphas[]{0, 1, quint16(words ? 32768 : 128), quint16(words ? 65535 : 255)};
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
            auto value = color;
            value[3] = alphas[(x + y) % 4];
            const auto bytes = Gpu::rgbaBytes(value, format);
            std::memcpy(tile.data() + (y * Gpu::RgbaTile::Size + x) * pixel.size(), bytes.constData(), pixel.size());
        }
    }
    const auto owner = paintImageWithEncoding(execution, raster);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Native clipboard"), cs);
    const auto original = owner->operations().snapshot();
    const QUuid uuid = original->layers.first().uuid;
    GpuClipboardMask mask;
    mask.rect = QRect(16, 16, 32, 32);
    mask.keepInside = keepInside;
    mask.coverage = QImage(mask.rect.size(), QImage::Format_Alpha8);
    for (int y = 0; y < mask.coverage.height(); ++y) for (int x = 0; x < mask.coverage.width(); ++x)
        mask.coverage.scanLine(y)[x] = std::array<quint8, 6>{0, 1, 127, 128, 254, 255}[(x + 2 * y) % 6];
    const QRect captured = keepInside ? mask.rect : QRect(QPoint(), raster.size);
    const auto releaseClipboard = qScopeGuard([] { QGuiApplication::clipboard()->clear(); });
    QVERIFY(merged ? KisClipboard::instance()->setGpuProjection(image, 8 * 1024 * 1024, captured, mask) :
                     KisClipboard::instance()->setGpuLayer(image, uuid, 8 * 1024 * 1024, captured, mask));
    QCOMPARE(owner->operations().snapshot(), original);
    const auto *data = qobject_cast<const KisMimeData *>(KisClipboard::instance()->layersMimeData());
    QVERIFY(data);
    QCOMPARE(data->nodes().size(), 1);
    const auto capturedImage = data->nodes().first()->image();
    QVERIFY(capturedImage);
    QCOMPARE(capturedImage->colorSpace()->colorModelId(), cs->colorModelId());
    QCOMPARE(capturedImage->colorSpace()->colorDepthId(), cs->colorDepthId());
    QCOMPARE(capturedImage->colorSpace()->profile()->rawData(), cs->profile()->rawData());
    const auto copied = capturedImage->gpuImage();
    QVERIFY(copied);
    QCOMPARE(copied->operations().snapshot()->format, format);
    const auto source = execution->readRgbaPixels(owner->handle(), original, captured,
                                                  merged ? 0 : original->activeLayer, 8 * 1024 * 1024);
    QCOMPARE(source.code, Gpu::ExecutionCode::Success);
    QByteArray selectedness(captured.width() * captured.height(), '\0');
    for (int y = 0; y < captured.height(); ++y) for (int x = 0; x < captured.width(); ++x) {
        const QPoint point = captured.topLeft() + QPoint(x, y);
        const QPoint local = point - mask.rect.topLeft();
        const quint8 amount = mask.rect.contains(point) ? mask.coverage.constScanLine(local.y())[local.x()] : 0;
        selectedness[y * captured.width() + x] = char(keepInside ? amount : 255 - amount);
    }
    const QByteArray expected = nativeClipboardMask(cs, source.bytes, selectedness, captured.size());
    const auto actual = copied->readRgbaPixels(copied->operations().activeLayer(), QRect(QPoint(), captured.size()), expected.size());
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        const auto output = QDir(results).filePath(QStringLiteral("native-clipboard-") + QString::fromLatin1(QTest::currentDataTag()));
        QVERIFY(QDir().mkpath(output));
        for (const auto &entry : {qMakePair(QStringLiteral("expected.rgba"), expected), qMakePair(QStringLiteral("actual.rgba"), actual.bytes)}) {
            QFile artifact(QDir(output).filePath(entry.first));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(entry.second), entry.second.size());
        }
    }
    QCOMPARE(actual.bytes, expected);
    QCOMPARE(owner->fillLayerColor(owner->operations().activeLayer(), words ? Gpu::RgbaPixel{1, 257, 65534, 65535} :
                                                                          Gpu::RgbaPixel{1, 2, 254, 255}), Gpu::Result::Success);
    QCOMPARE(copied->readRgbaPixels(copied->operations().activeLayer(), QRect(QPoint(), captured.size()), expected.size()).bytes, expected);
    QByteArray archive = data->data(QStringLiteral("application/zip"));
    QVERIFY(!archive.isEmpty());
    QBuffer buffer(&archive);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(buffer);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.format, format);
    QCOMPARE(loaded.document.image.size, captured.size());
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    const auto layer = reopened->operations().activeLayer();
    QCOMPARE(reopened->readRgbaPixels(layer, QRect(QPoint(), captured.size()), expected.size()).bytes, expected);
    QCOMPARE(reopened->fillLayerColor(layer, color), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(layer, QRect(QPoint(), captured.size()), expected.size()).bytes, pixel.repeated(captured.width() * captured.height()));
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(layer, QRect(QPoint(), captured.size()), expected.size()).bytes, expected);
    QCOMPARE(reopened->redo(), Gpu::Result::Success);
    if (!results.isEmpty()) {
        QFile artifact(QDir(results).filePath(QStringLiteral("native-clipboard-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kra")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(archive), archive.size());
    }
}

void KisGpuDocumentViewTest::selectionMergedCopyUsesGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QVERIFY(owner->fillLayer(owner->operations().activeLayer(), {17, 129, 241}, 255.0 / 255, false, false) == Gpu::Result::Success);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection copy")));
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
    window->show();
    QVERIFY(lifetime->image()->setGpuSelectionRect(QRect(16, 16, 32, 32)));
    QTRY_COMPARE(owner->selectionRect(), QRect(16, 16, 32, 32));
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *data = QGuiApplication::clipboard()->mimeData();
    QVERIFY(data);
    const QImage copied = KisClipboard::instance()->getImageWithFallback(data);
    QVERIFY(!copied.isNull());
    QCOMPARE(copied.size(), QSize(32, 32));
    QCOMPARE(copied.pixelColor(16, 16), QColor(17, 129, 241));
}

void KisGpuDocumentViewTest::selectionCopyAndCutUseGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QVERIFY(owner->fillLayer(owner->operations().activeLayer(), {17, 129, 241}, 255.0 / 255, false, false) == Gpu::Result::Success);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection copy cut")));
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
    window->show();
    QVERIFY(lifetime->image()->setGpuSelectionRect(QRect(16, 16, 32, 32)));
    QTRY_COMPARE(owner->selectionRect(), QRect(16, 16, 32, 32));
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *data = QGuiApplication::clipboard()->mimeData();
    QVERIFY(data);
    const QImage copied = KisClipboard::instance()->getImageWithFallback(data);
    QVERIFY(!copied.isNull());
    QCOMPARE(copied.size(), QSize(32, 32));
    QCOMPARE(copied.pixelColor(16, 16), QColor(17, 129, 241));
    QCOMPARE(owner->operations().undoCount(), 2);

    manager->cut();
    QTRY_COMPARE(owner->operations().undoCount(), 3);
    const QRect region(QPoint(), QSize(64, 64));
    const auto cut = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cut.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(quint8(pixel(cut.bytes, 24, 24).at(3)), 0);
    QCOMPARE(pixel(cut.bytes, 8, 8), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 24, 24), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    const auto redone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(redone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(redone.bytes, 24, 24).at(3)), 0);
    QCOMPARE(pixel(redone.bytes, 8, 8), QByteArray::fromHex("1181f1ff"));
}

void KisGpuDocumentViewTest::selectionMovePreviewsAndCommitsOneHistory()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KConfigGroup configuration = KSharedConfig::openConfig()->group(QStringLiteral("KisToolSelectRectangular"));
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("roundCornersX", 0);
    configuration.writeEntry("roundCornersY", 0);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection move")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == QLatin1String("KisToolSelectRectangularoption widget"))
            if (auto *candidate = qobject_cast<KisSelectionOptions *>(widget)) options = candidate;
    QVERIFY(options);
    const auto oldAction = options->action();
    const auto oldMode = options->mode();
    const bool oldAA = options->antiAliasSelection();
    const int oldGrow = options->growSelection(), oldFeather = options->featherSelection();
    const auto restoreOptions = qScopeGuard([&] {
        if (!options) return;
        options->setAction(oldAction); options->setMode(oldMode); options->setAntiAliasSelection(oldAA);
        options->setGrowSelection(oldGrow); options->setFeatherSelection(oldFeather);
    });
    options->setMode(PIXEL_SELECTION); options->setAction(SELECTION_REPLACE);
    options->setAntiAliasSelection(false); options->setGrowSelection(0); options->setFeatherSelection(0);
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QRect selected(64, 64, 128, 128);
    QTest::mouseMove(native.data(), point(selected.topLeft()));
    const auto start = point(selected.topLeft());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(native.data(), point({192, 192}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == selected);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Rectangle);

    QVERIFY(lifetime->image()->setGpuSelectionRect(selected, true));
    QTRY_VERIFY(owner->selectionInverted());
    const QPoint delta(16, 16);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({32, 32}));
    QTest::mouseMove(native.data(), point({48, 48}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({48, 48}));
    QTRY_COMPARE(owner->selectionRect(), selected.translated(delta));
    QVERIFY(owner->selectionInverted());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(!owner->selectionInverted());
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->operations().undoCount(), 1);

    const QRect moved = selected.translated(delta);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(selected.center()));
    QTest::mouseMove(native.data(), point(selected.center() + QPointF(delta)), 30);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved);
    QCOMPARE(owner->selectionRect(), selected);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(selected.center() + QPointF(delta)));
    QTRY_COMPARE(owner->selectionRect(), moved);
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == selected);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), moved);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved);

    const int history = owner->operations().undoCount();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(moved.center()));
    QTest::mouseMove(native.data(), point(moved.center() + QPointF(delta)), 30);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved.translated(delta));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Escape);
    QTRY_COMPARE(owner->selectionRect(), moved);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved);
    QCOMPARE(owner->operations().undoCount(), history);

}

void KisGpuDocumentViewTest::selectionGrowAndShrinkUseGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection shrink")));
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
    window->show();
    const QRect selected(64, 64, 128, 128);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    const auto runFilter = [&](const QString &id, int x, int y) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", false);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("shrinkselection"), 8, 4);
    const QRect shrunk(72, 68, 112, 120);
    QTRY_COMPARE(owner->selectionRect(), shrunk);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == shrunk);
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), shrunk);
    runFilter(QStringLiteral("growselection"), 8, 4);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->operations().undoCount(), 3);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), shrunk);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    runFilter(QStringLiteral("shrinkselection"), 100, 100);
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QTRY_VERIFY(!lifetime->image()->globalSelection() ||
                lifetime->image()->globalSelection()->selectedExactRect().isEmpty());
    QCOMPARE(owner->operations().undoCount(), 4);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
}

void KisGpuDocumentViewTest::nativeInvertedSelectionsKeepEditingAndSaving_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("soft");
    for (bool words : {false, true}) for (bool soft : {false, true}) {
        const QByteArray tag = QByteArray(words ? "u16" : "u8") + (soft ? "-soft-offcanvas" : "-rectangle");
        QTest::newRow(tag.constData()) << words << soft;
    }
}

void KisGpuDocumentViewTest::nativeInvertedSelectionsKeepEditingAndSaving()
{
    // Normal fill/clear actions consume native KRA's inverse selection; history
    // and reopened edits must preserve coverage and document-precision pixels.
    QFETCH(bool, words); QFETCH(bool, soft);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("native-selection.kra"));
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), 64, 64, cs, "Inverse selection");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        layer->paintDevice()->fill(image->bounds(), KoColor(QColor(77, 113, 157, 255), cs));
        QVERIFY(image->addNode(layer));
        KisSelectionSP selection = new KisSelection;
        const quint8 selected = 255;
        selection->pixelSelection()->setDefaultPixel(KoColor(&selected, KoColorSpaceRegistry::instance()->alpha8()));
        selection->pixelSelection()->select({16, 16, 32, 32}, 0);
        if (soft) {
            const quint8 coverage[]{0, 1, 128, 254, 255};
            selection->pixelSelection()->writeBytes(coverage, {-3, 9, 5, 1});
            selection->pixelSelection()->writeBytes(coverage, {14, 20, 5, 1});
        }
        KisSelectionMaskSP mask = new KisSelectionMask(image);
        mask->setSelection(selection); mask->setActive(true);
        QVERIFY(image->addNode(mask, image->root()));
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
        QVERIFY(document->saveAs(baseline, KisDocument::nativeFormatMimeType(), false));
        document->waitForSavingToComplete();
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            QVERIFY(QDir().mkpath(results));
            QVERIFY(QFile::copy(baseline, QDir(results).filePath(QStringLiteral("native-inverse-%1-source.kra")
                .arg(QString::fromLatin1(QTest::currentDataTag())))));
        }
    }
    QList<QByteArray> reference;
    for (bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        const auto saved = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("native.kra"));
        int stage = 0;
        for (bool reopened : {false, true}) {
            std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
            document->setFileBatchMode(true); document->setAutoSaveActive(false);
            QVERIFY2(document->openPath(reopened ? saved : baseline, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
            auto image = KisImageSP(document->image()); image->waitForDone();
            QCOMPARE(bool(image->gpuImage()), gpu);
            auto layer = image->root()->firstChild();
            while (layer && !layer->inherits("KisPaintLayer")) layer = layer->nextSibling();
            QVERIFY(layer);
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
            window->viewManager()->setCurrentView(view); window->show();
            window->viewManager()->nodeManager()->slotNonUiActivatedNode(layer);
            QTRY_COMPARE(view->currentNode(), layer);
            const auto capture = [&]() -> QByteArray {
                image->waitForDone();
                QByteArray pixels(64 * 64 * cs->pixelSize(), '\0'), projection(pixels.size(), '\0');
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), image->bounds());
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), image->bounds());
                const auto selection = image->globalSelection();
                if (!selection) return {};
                QByteArray coverage(70 * 66, '\0');
                selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), {-3, -1, 70, 66});
                return pixels + projection + coverage + char(selection->pixelSelection()->defaultPixel().opacityU8());
            };
            const auto check = [&] {
                const auto state = capture(); QVERIFY(!state.isEmpty());
                const int currentStage = stage++;
                const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                if (!results.isEmpty()) {
                    QFile output(QDir(results).filePath(QStringLiteral("native-inverse-%1-%2-%3.bin")
                        .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? QStringLiteral("gpu") : QStringLiteral("native")).arg(currentStage)));
                    QVERIFY(output.open(QIODevice::WriteOnly)); QCOMPARE(output.write(state), state.size());
                }
                if (!gpu) reference.append(state);
                else QCOMPARE(state, reference.value(currentStage));
            };
            check();
            auto *resources = window->viewManager()->canvasResourceProvider();
            resources->setPaintOpPreset(m_preset);
            resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
            resources->setOpacity(1); resources->setEraserMode(false);
            resources->setCurrentCompositeOp(QStringLiteral("normal"));
            auto *action = window->viewManager()->actionManager()->actionByName(reopened
                ? QStringLiteral("clear") : QStringLiteral("fill_selection_foreground_color"));
            QVERIFY(action);
            const auto before = capture();
            action->trigger(); image->waitForDone();
            QTRY_COMPARE(lifetime->undoStack()->index(), 1);
            const auto edited = capture(); QVERIFY(edited != before); check();
            lifetime->undoStack()->undo(); image->waitForDone();
            QTRY_COMPARE(lifetime->undoStack()->index(), 0);
            QCOMPARE(capture(), before); check();
            lifetime->undoStack()->redo(); image->waitForDone();
            QTRY_COMPARE(lifetime->undoStack()->index(), 1);
            QCOMPARE(capture(), edited); check();
            QVERIFY(lifetime->saveAs(saved, KisDocument::nativeFormatMimeType(), false));
            lifetime->waitForSavingToComplete();
        }
        if (gpu) {
            // The native reader must recover the re-saved global default and
            // exact off-canvas coverage for subsequent selection editing.
            qputenv("LIBREPAINT_GPU_ENGINE", "0");
            QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
            native->setFileBatchMode(true); native->setAutoSaveActive(false);
            QVERIFY2(native->openPath(saved, KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
            auto image = KisImageSP(native->image()); image->waitForDone();
            const auto selection = image->globalSelection(); QVERIFY(selection);
            QCOMPARE(selection->pixelSelection()->defaultPixel().opacityU8(), quint8(255));
            QByteArray coverage(70 * 66, '\0');
            selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), {-3, -1, 70, 66});
            QCOMPARE(coverage, reference.last().mid(2 * 64 * 64 * cs->pixelSize(), coverage.size()));
        }
    }
}

void KisGpuDocumentViewTest::invertedSelectionLimitsPaintingAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Inverted selection")));
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
    window->show();
    const QRect hole(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionRect(hole));
    QTRY_COMPARE(owner->selectionRect(), hole);
    KisOperationConfigurationSP invert = new KisOperationConfiguration("invertselection");
    window->viewManager()->actionManager()->runOperationFromConfiguration(invert);
    QTRY_VERIFY(owner->selectionInverted());
    QTRY_COMPARE(owner->selectionRect(), hole);
    QCOMPARE(owner->operations().undoCount(), 2);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == QRect(QPoint(), QSize(64, 64)));
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 3);
    const QRect region(QPoint(), QSize(64, 64));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 4, 4), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 24, 24), QByteArray(4, char(0)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 4, 4), QByteArray(4, char(0)));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    {
        KisOperationConfigurationSP growInverted = new KisOperationConfiguration("growselection");
        growInverted->setProperty("x-radius", 2);
        growInverted->setProperty("y-radius", 2);
        growInverted->setProperty("edgeLock", true);
        window->viewManager()->actionManager()->runOperationFromConfiguration(growInverted);
        lifetime->image()->waitForDone();
        QTRY_COMPARE(owner->selectionRect(), hole.adjusted(2, 2, -2, -2));
        QVERIFY(owner->selectionInverted());
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->selectionRect(), hole);
        QVERIFY(owner->selectionInverted());
    }
    auto *copyMerged = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(copyMerged);
    copyMerged->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *clipboardData = QGuiApplication::clipboard()->mimeData();
    QVERIFY(clipboardData);
    const QImage copiedMerged = KisClipboard::instance()->getImageWithFallback(clipboardData);
    QVERIFY(!copiedMerged.isNull());
    QCOMPARE(copiedMerged.size(), QSize(64, 64));
    QCOMPARE(copiedMerged.pixelColor(4, 4), QColor(17, 129, 241));
    QCOMPARE(copiedMerged.pixelColor(24, 24).alpha(), 0);
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    const auto *copyData = QGuiApplication::clipboard()->mimeData();
    QVERIFY(copyData);
    const QImage copiedLayer = KisClipboard::instance()->getImageWithFallback(copyData);
    QVERIFY(!copiedLayer.isNull());
    QCOMPARE(copiedLayer.size(), QSize(64, 64));
    QCOMPARE(copiedLayer.pixelColor(4, 4), QColor(17, 129, 241));
    QCOMPARE(copiedLayer.pixelColor(24, 24).alpha(), 0);
    manager->cut();
    QTRY_COMPARE(owner->operations().undoCount(), 4);
    const auto cut = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cut.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(cut.bytes, 4, 4).at(3)), 0);
    QCOMPARE(quint8(pixel(cut.bytes, 24, 24).at(3)), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto restored = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(restored.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(restored.bytes, 4, 4), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("inverted-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.selection, hole);
    QVERIFY(loaded.document.image.selectionInverted);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QVERIFY(reopened->selectionInverted());
    QCOMPARE(reopened->selectionRect(), hole);

    const auto runFilter = [&](const QString &id, int x, int y, bool edgeLock = false) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", edgeLock);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("growselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), QRect(20, 20, 24, 24));
    QVERIFY(owner->selectionInverted());
    runFilter(QStringLiteral("shrinkselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), hole);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::ellipticalSelectionLimitsPaintingAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Elliptical selection")));
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
    window->show();
    const QRect ellipseRect(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipseRect));
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QTRY_VERIFY(lifetime->image()->globalSelection());
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 16, 16), QByteArray(4, char(0)));
    QCOMPARE(pixel(filled.bytes, 47, 47), QByteArray(4, char(0)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 32, 32), QByteArray(4, char(0)));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("elliptical-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.selection, ellipseRect);
    QCOMPARE(loaded.document.image.selectionKind, Gpu::SelectionKind::Ellipse);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QCOMPARE(reopened->selectionKind(), Gpu::SelectionKind::Ellipse);
    QCOMPARE(reopened->selectionRect(), ellipseRect);
    const auto reopenedPixels = reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(reopenedPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(reopenedPixels.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(reopenedPixels.bytes, 16, 16), QByteArray(4, char(0)));

    auto *copyMerged = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(copyMerged);
    copyMerged->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *ellipseClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(ellipseClipboard);
    const QImage copiedEllipse = KisClipboard::instance()->getImageWithFallback(ellipseClipboard);
    QVERIFY(!copiedEllipse.isNull());
    QCOMPARE(copiedEllipse.size(), QSize(32, 32));
    QCOMPARE(copiedEllipse.pixelColor(16, 16), QColor(17, 129, 241));
    QCOMPARE(copiedEllipse.pixelColor(0, 0).alpha(), 0);
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    const auto *layerClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(layerClipboard);
    const QImage copiedEllipseLayer = KisClipboard::instance()->getImageWithFallback(layerClipboard);
    QVERIFY(!copiedEllipseLayer.isNull());
    QCOMPARE(copiedEllipseLayer.size(), QSize(32, 32));
    QCOMPARE(copiedEllipseLayer.pixelColor(0, 0).alpha(), 0);
    manager->cut();
    const auto cutEllipse = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cutEllipse.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(cutEllipse.bytes, 32, 32).at(3)), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto runFilter = [&](const QString &id, int x, int y, bool edgeLock = false) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", edgeLock);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("growselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), QRect(12, 12, 40, 40));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    runFilter(QStringLiteral("shrinkselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    runFilter(QStringLiteral("invertselection"), 0, 0);
    QTRY_VERIFY(owner->selectionInverted());
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    const QRect invertedHole = owner->selectionRect();
    runFilter(QStringLiteral("growselection"), 2, 2, true);
    QTRY_COMPARE(owner->selectionRect(), invertedHole.adjusted(2, 2, -2, -2));
    QVERIFY(owner->selectionInverted());
    runFilter(QStringLiteral("shrinkselection"), 2, 2, true);
    QTRY_COMPARE(owner->selectionRect(), invertedHole);
    QVERIFY(owner->selectionInverted());
    const auto invertedPath = directory.filePath(QStringLiteral("inverted-ellipse.kra"));
    QVERIFY2(lifetime->saveAs(invertedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile invertedFile(invertedPath);
    QVERIFY(invertedFile.open(QIODevice::ReadOnly));
    const auto invertedLoaded = LibrePaint::ImportExport::Kra::RasterCodec::read(invertedFile);
    QVERIFY2(invertedLoaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(invertedLoaded.diagnostic));
    QCOMPARE(invertedLoaded.document.image.selection, ellipseRect);
    QCOMPARE(invertedLoaded.document.image.selectionKind, Gpu::SelectionKind::Ellipse);
    QVERIFY(invertedLoaded.document.image.selectionInverted);

    fill->trigger();
    QTRY_VERIFY(owner->selectionInverted());
    copyMerged->trigger();
    const auto *invertedClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(invertedClipboard);
    const QImage copiedInverted = KisClipboard::instance()->getImageWithFallback(invertedClipboard);
    QVERIFY(!copiedInverted.isNull());
    QCOMPARE(copiedInverted.size(), QSize(64, 64));
    QCOMPARE(copiedInverted.pixelColor(4, 4), QColor(17, 129, 241));
    QCOMPARE(copiedInverted.pixelColor(32, 32).alpha(), 0);
    manager->cut();
    const auto cutInverted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cutInverted.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(cutInverted.bytes, 4, 4).at(3)), 0);
    QCOMPARE(pixel(cutInverted.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto restoredInverted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(restoredInverted.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(restoredInverted.bytes, 4, 4), QByteArray::fromHex("1181f1ff"));

    runFilter(QStringLiteral("growselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), QRect(20, 20, 24, 24));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
    runFilter(QStringLiteral("shrinkselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::ellipticalToolCreatesDocumentSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KConfigGroup configuration = KSharedConfig::openConfig()->group(QStringLiteral("KisToolSelectElliptical"));
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("antiAliasSelection", false);
    configuration.writeEntry("growSelection", 0);
    configuration.writeEntry("featherSelection", 0);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Ellipse tool")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2 + 0.5, imageSize.height() / 2 + 0.5});
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectElliptical"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectElliptical"));
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == QLatin1String("KisToolSelectEllipticaloption widget"))
            if (auto *candidate = qobject_cast<KisSelectionOptions *>(widget)) options = candidate;
    QVERIFY(options);
    const auto oldAction = options->action();
    const auto oldMode = options->mode();
    const bool oldAA = options->antiAliasSelection();
    const int oldGrow = options->growSelection(), oldFeather = options->featherSelection();
    const auto restoreOptions = qScopeGuard([&] {
        if (!options) return;
        options->setAction(oldAction); options->setMode(oldMode); options->setAntiAliasSelection(oldAA);
        options->setGrowSelection(oldGrow); options->setFeatherSelection(oldFeather);
    });
    options->setMode(PIXEL_SELECTION); options->setAction(SELECTION_REPLACE);
    options->setAntiAliasSelection(false); options->setGrowSelection(0); options->setFeatherSelection(0);
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QRect ellipseRect(64, 64, 128, 128);
    QTest::mouseMove(native.data(), point(ellipseRect.topLeft()));
    const auto start = point(ellipseRect.topLeft());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(native.data(), point({192, 192}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == ellipseRect);

    QVERIFY(lifetime->image()->clearGpuSelection());
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    const QRect secondRect(32, 48, 128, 152);
    QTest::mouseMove(native.data(), point(secondRect.topLeft()));
    const auto secondStart = point(secondRect.topLeft());
    QEnterEvent secondEntered(secondStart, secondStart, native->mapToGlobal(secondStart));
    QApplication::sendEvent(native.data(), &secondEntered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, secondStart);
    QTest::mouseMove(native.data(), point({160, 200}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({160, 200}));
    QTRY_COMPARE(owner->selectionRect(), secondRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);

    const QPoint delta(16, 16);
    const QRect movedSecond = secondRect.translated(delta);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(secondRect.center()));
    QTest::mouseMove(native.data(), point(secondRect.center() + QPointF(delta)), 30);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == movedSecond);
    QCOMPARE(owner->selectionRect(), secondRect);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(secondRect.center() + QPointF(delta)));
    QTRY_COMPARE(owner->selectionRect(), movedSecond);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), secondRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), movedSecond);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);

    QVERIFY(lifetime->image()->setGpuSelectionEllipse(movedSecond, true));
    QTRY_VERIFY(owner->selectionInverted());
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({200, 200}));
    QTest::mouseMove(native.data(), point({216, 216}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({216, 216}));
    const QRect invertedEllipse = movedSecond.translated(delta);
    QTRY_COMPARE(owner->selectionRect(), invertedEllipse);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), movedSecond);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::freehandToolCreatesArbitraryMaskSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Freehand tool")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectOutline"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectOutline"));
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == QLatin1String("KisToolSelectOutlineoption widget"))
            if (auto *candidate = qobject_cast<KisSelectionOptions *>(widget)) options = candidate;
    QVERIFY(options);
    const auto oldAction = options->action(); const auto oldMode = options->mode();
    const auto oldAA = options->antiAliasSelection();
    const int oldGrow = options->growSelection(), oldFeather = options->featherSelection();
    const auto restoreOptions = qScopeGuard([&] {
        if (!options) return;
        options->setAction(oldAction); options->setMode(oldMode); options->setAntiAliasSelection(oldAA);
        options->setGrowSelection(oldGrow); options->setFeatherSelection(oldFeather);
    });
    options->setMode(PIXEL_SELECTION); options->setAction(SELECTION_REPLACE);
    options->setAntiAliasSelection(false); options->setGrowSelection(0); options->setFeatherSelection(0);

    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QList<QPointF> polygon{{64, 64}, {192, 64}, {192, 128}, {128, 192}, {64, 192}};
    QTest::mouseMove(native.data(), point(polygon.first()));
    const auto start = point(polygon.first());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    for (qsizetype i = 1; i < polygon.size(); ++i)
        QTest::mouseMove(native.data(), point(polygon[i]), 20);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(polygon.last()));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    const auto mask = owner->selectionMask();
    QVERIFY(mask);
    QVERIFY((readSelectionCoverage(owner, QPoint(80, 80)) > 0));
    QVERIFY((readSelectionCoverage(owner, QPoint(176, 176)) == 0));
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QTRY_VERIFY(lifetime->image()->globalSelection());
    QTRY_COMPARE(lifetime->image()->globalSelection()->selectedExactRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(256, 256));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 256 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 80, 80), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 176, 176), QByteArray(4, char(0)));

    auto *copyMerged = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(copyMerged);
    QTRY_VERIFY(copyMerged->isEnabled());
    copyMerged->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *mergedClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(mergedClipboard);
    const QImage merged = KisClipboard::instance()->getImageWithFallback(mergedClipboard);
    QVERIFY(!merged.isNull());
    QCOMPARE(merged.size(), mask->bounds().size());
    QCOMPARE(merged.pixelColor(80 - mask->bounds().x(), 80 - mask->bounds().y()), QColor(17, 129, 241));
    QCOMPARE(merged.pixelColor(176 - mask->bounds().x(), 176 - mask->bounds().y()).alpha(), 0);

    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    const auto *layerClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(layerClipboard);
    const QImage layerCopy = KisClipboard::instance()->getImageWithFallback(layerClipboard);
    QVERIFY(!layerCopy.isNull());
    QCOMPARE(layerCopy.size(), mask->bounds().size());
    QCOMPARE(layerCopy.pixelColor(80 - mask->bounds().x(), 80 - mask->bounds().y()), QColor(17, 129, 241));
    QCOMPARE(layerCopy.pixelColor(176 - mask->bounds().x(), 176 - mask->bounds().y()).alpha(), 0);

    auto *copyLayer = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_selection_to_new_layer"));
    QVERIFY(copyLayer);
    QTRY_VERIFY(copyLayer->isEnabled());
    copyLayer->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    QVERIFY(owner->operations().activeLayer() != owner->operations().snapshot()->layers.first().id);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    const QRect beforeMove = owner->selectionRect();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({80, 80}));
    QTest::mouseMove(native.data(), point({96, 96}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({96, 96}));
    QTRY_COMPARE(owner->selectionRect(), beforeMove.translated(16, 16));
    QVERIFY(owner->selectionMask());
    QVERIFY((readSelectionCoverage(owner, QPoint(96, 96)) > 0));
    QVERIFY((readSelectionCoverage(owner, QPoint(beforeMove.left(), beforeMove.top())) == 0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), beforeMove);
    QVERIFY((readSelectionCoverage(owner, QPoint(80, 80)) > 0));

    const auto runFilter = [&](const QString &id, int x, int y) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", false);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("growselection"), 2, 2);
    QTRY_VERIFY((readSelectionCoverage(owner, QPoint(62, 80)) > 0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), beforeMove);
    runFilter(QStringLiteral("shrinkselection"), 2, 2);
    QTRY_VERIFY((readSelectionCoverage(owner, QPoint(64, 80)) == 0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), beforeMove);
    runFilter(QStringLiteral("invertselection"), 0, 0);
    const auto selectedness = [&](QPoint point) {
        const auto selection = lifetime->image()->globalSelection();
        return selection ? int(selection->pixelSelection()->pixel(point).opacityU8()) : -1;
    };
    QTRY_COMPARE(selectedness({80, 80}), 0);
    QCOMPARE(selectedness({176, 176}), 255);
    runFilter(QStringLiteral("growselection"), 2, 2);
    QTRY_COMPARE(selectedness({64, 80}), 255);
    QCOMPARE(selectedness({80, 80}), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    runFilter(QStringLiteral("shrinkselection"), 2, 2);
    QTRY_COMPARE(selectedness({62, 80}), 0);
    QCOMPARE(selectedness({176, 176}), 255);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    runFilter(QStringLiteral("borderselection"), 2, 2);
    QTRY_COMPARE(selectedness({62, 80}), 255);
    QCOMPARE(selectedness({80, 80}), 0);
    QCOMPARE(selectedness({200, 80}), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY((readSelectionCoverage(owner, QPoint(80, 80)) > 0));
    runFilter(QStringLiteral("borderselection"), 2, 2);
    QTRY_VERIFY(owner->selectionMask() && (readSelectionCoverage(owner, QPoint(80, 80)) == 0));
    QVERIFY((readSelectionCoverage(owner, QPoint(64, 80)) > 0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY((readSelectionCoverage(owner, QPoint(80, 80)) > 0));

    runFilter(QStringLiteral("shrinkselection"), 200, 200);
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionMask(), mask);

    QVERIFY(lifetime->image()->clearGpuSelection());
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QVERIFY(lifetime->image()->reselectGpuSelection());
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QVERIFY(owner->selectionMask());
    QVERIFY((readSelectionCoverage(owner, QPoint(80, 80)) > 0));
    QVERIFY((readSelectionCoverage(owner, QPoint(176, 176)) == 0));
    QCOMPARE(owner->selectionRect(), mask->bounds());
}

void KisGpuDocumentViewTest::polygonToolVerticesCreateFrozenCoverage()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Polygon tool coverage"));
    QVERIFY(image);

    // Document-pixel vertices as collected by the polygonal tool. The L-shape
    // separates a closed polygon from the line or triangle a truncated input
    // sequence would produce.
    const QList<QPointF> vertices{{16, 16}, {96, 16}, {96, 64}, {48, 64}, {48, 96}, {16, 96}};
    const auto expected = [](int x, int y) {
        return (x >= 16 && x < 96 && y >= 16 && y < 64) ||
            (x >= 16 && x < 48 && y >= 64 && y < 96);
    };
    const auto mask = KisSelectionToolHelper::gpuSelectionMaskFromPolygon(vertices, QSize(128, 128));
    QVERIFY(mask);
    QVERIFY(!mask->isAlpha());
    QCOMPARE(mask->bounds(), QRect(16, 16, 80, 80));
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            if (mask->contains(x, y) != expected(x, y)) {
                QFAIL(qPrintable(QStringLiteral("coverage mismatch at %1,%2").arg(x).arg(y)));
            }
        }
    }

    // The tool commits that frozen coverage as one replacement history entry.
    QVERIFY(image->setGpuSelectionMask(mask));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QVERIFY(owner->selectionMask() == mask);
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().historyAction(1), Gpu::HistoryAction::ChangeSelection);
    QTRY_VERIFY(image->globalSelection());
    QTRY_COMPARE(image->globalSelection()->selectedExactRect(), mask->bounds());

    // It uses the same coverage path as freehand for the shared filters.
    QVERIFY(image->growGpuSelectionMask(2, 2));
    QTRY_VERIFY((readSelectionCoverage(owner, QPoint(14, 16)) > 0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(owner->selectionMask() == mask);
    QVERIFY(image->invertGpuSelectionMask());
    QTRY_VERIFY(owner->selectionMask() && (readSelectionCoverage(owner, QPoint(16, 16)) == 0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(owner->selectionMask() == mask);
}

void KisGpuDocumentViewTest::pathToolClosedPathCreatesPolygonCoverage()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Path tool coverage"));
    QVERIFY(image);

    QPainterPath path;
    path.moveTo(16, 16);
    path.lineTo(80, 16);
    path.lineTo(80, 64);
    path.lineTo(16, 64);
    path.closeSubpath();

    const QList<QPointF> vertices = KisSelectionToolHelper::gpuSelectionPolygonVertices(path);
    QVERIFY(vertices.size() >= 3);
    const auto mask = KisSelectionToolHelper::gpuSelectionMaskFromPolygon(vertices, QSize(128, 128));
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(16, 16, 64, 48));
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            const bool expected = x >= 16 && x < 80 && y >= 16 && y < 64;
            if (mask->contains(x, y) != expected) {
                QFAIL(qPrintable(QStringLiteral("coverage mismatch at %1,%2").arg(x).arg(y)));
            }
        }
    }
    QVERIFY(image->setGpuSelectionMask(mask));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QCOMPARE(owner->operations().undoCount(), 1);

    // A curved closed path flattens to a single polygon instead of being
    // dropped or reduced to its endpoints.
    QPainterPath curved;
    curved.moveTo(32, 96);
    curved.cubicTo(32, 32, 96, 32, 96, 96);
    curved.closeSubpath();
    const QList<QPointF> curvedVertices = KisSelectionToolHelper::gpuSelectionPolygonVertices(curved);
    QVERIFY(curvedVertices.size() > 4);
    QVERIFY(KisSelectionToolHelper::gpuSelectionMaskFromPolygon(curvedVertices, QSize(128, 128)));

    // Several contours cannot become one polygon, so the tool reports a
    // conversion failure instead of selecting only part of the path.
    QPainterPath contours;
    contours.addRect(QRectF(8, 8, 16, 16));
    contours.addRect(QRectF(48, 48, 16, 16));
    const QList<QPointF> contourVertices = KisSelectionToolHelper::gpuSelectionPolygonVertices(contours);
    QVERIFY(contourVertices.isEmpty());
    QVERIFY(!KisSelectionToolHelper::gpuSelectionMaskFromPolygon(contourVertices, QSize(128, 128)));
}

void KisGpuDocumentViewTest::polygonalToolCreatesArbitraryMaskSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Polygon tool")));
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
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectPolygonal"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectPolygonal"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QList<QPointF> vertices{{64, 64}, {192, 64}, {192, 192}, {64, 192}};

    QTest::mouseMove(native.data(), point(vertices.first()));
    const auto start = point(vertices.first());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    for (const QPointF &vertex : vertices) {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
    }
    // The double-click ends the polyline without adding a new corner.
    QTest::mouseDClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    const auto mask = owner->selectionMask();
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(64, 64, 128, 128));
    QVERIFY((readSelectionCoverage(owner, QPoint(80, 80)) > 0));
    QVERIFY((readSelectionCoverage(owner, QPoint(176, 176)) > 0));
    QVERIFY((readSelectionCoverage(owner, QPoint(48, 48)) == 0));
    QVERIFY((readSelectionCoverage(owner, QPoint(208, 208)) == 0));
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);
    QTRY_VERIFY(lifetime->image()->globalSelection());
    QTRY_COMPARE(lifetime->image()->globalSelection()->selectedExactRect(), mask->bounds());
}

void KisGpuDocumentViewTest::selectionOutlineKeepsOnlyEffectiveCoverageBoundaries()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const QRect canvas(0, 0, 64, 64);
    const auto owner = paintImageWithEncoding(execution, canvas.size());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Selection boundary"));
    const auto ellipse = Gpu::SelectionMask::fromEllipse({8, 8, 48, 48}, canvas);
    const auto hole = Gpu::SelectionMask::fromRectangle({24, 24, 12, 16}, canvas);
    const auto hard = ellipse->combined(*hole, Gpu::SelectionCombination::Subtract, canvas);
    QVERIFY(hard);
    for (const bool alpha : {false, true}) for (const bool inverted : {false, true}) {
        QByteArray raw(64 * 64, '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
            raw[y * 64 + x] = char(hard->contains(x, y) ? (alpha ? (x < 20 ? 96 : 200) : 255) : 0);
        const auto coverage = alpha ? Gpu::SelectionMask::fromAlphaCoverage(canvas, raw, canvas) : hard;
        QByteArray effective = raw;
        if (inverted) for (char &value : effective) value = char(255 - quint8(value));
        QVERIFY(image->setGpuSelectionMask(coverage, inverted));
        const auto synchronized = [&] {
            if (!image->globalSelection()) return false;
            QByteArray actual(raw.size(), '\0');
            image->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), canvas);
            return actual == effective;
        };
        QTRY_VERIFY(synchronized());
        const auto outline = image->globalSelection()->outlineCache();
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
            QCOMPARE(outline.contains(QPointF(x + 0.5, y + 0.5)), quint8(effective[y * 64 + x]) >= 128);
        QPainterPathStroker stroker;
        stroker.setWidth(1.5);
        const auto border = stroker.createStroke(outline);
        QVERIFY(border.contains(QPointF(24, 32)));
        for (int y = 2; y < 62; ++y) for (int x = 2; x < 62; ++x) {
            bool interior = true;
            for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx)
                interior &= quint8(effective[(y + dy) * 64 + x + dx]) >= 128;
            const QByteArray context = QStringLiteral("alpha=%1 inverted=%2 interior=(%3,%4)")
                .arg(alpha).arg(inverted).arg(x).arg(y).toLatin1();
            if (interior) QVERIFY2(!border.contains(QPointF(x + 0.5, y)), context.constData());
        }
    }
}

void KisGpuDocumentViewTest::generatedSelectionViewRetainsAlphaAfterClose()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    const QRect canvas(0, 0, 64, 64), area(16, 16, 16, 16), softArea(8, 8, 32, 32);
    const auto owner = paintImageWithEncoding(execution, canvas.size());
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->setSelectionRect(area), Gpu::Result::Success);
    QCOMPARE(owner->fillLayerColor(layer, {197, 43, 17, 255}), Gpu::Result::Success);
    QCOMPARE(owner->clearSelection(), Gpu::Result::Success);
    Gpu::RegionFillSettings input;
    input.seed = {20, 20}; input.threshold = 1;
    QCOMPARE(owner->selectRegion(layer, input, Gpu::SelectionCombination::Replace), Gpu::Result::Success);
    const quint8 levels[]{0, 1, 127, 128, 254, 255};
    QByteArray soft(softArea.width() * softArea.height(), '\0');
    for (int y = 0; y < softArea.height(); ++y) for (int x = 0; x < softArea.width(); ++x)
        soft[y * softArea.width() + x] = char(levels[(x + y) % 6]);
    const auto literal = Gpu::SelectionMask::fromAlphaCoverage(softArea, soft, canvas);
    QVERIFY(literal);
    QCOMPARE(owner->combineSelectionMask(literal, Gpu::SelectionCombination::Intersect), Gpu::Result::Success);
    const auto mask = owner->selectionMask();
    QVERIFY(mask && mask->isGpuGenerated());
    KisPixelSelectionSP native = new KisPixelSelection();
    QByteArray expected(canvas.width() * canvas.height(), '\0');
    for (int y = area.top(); y <= area.bottom(); ++y) for (int x = area.left(); x <= area.right(); ++x)
        expected[y * canvas.width() + x] = char(literal->coverage(x, y));
    native->writeBytes(reinterpret_cast<const quint8 *>(expected.constData()), canvas);
    const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Exact Alpha8 selection view"));
    QTRY_VERIFY(image->globalSelection());
    KisPixelSelectionSP pixels = image->globalSelection()->pixelSelection();
    const auto verify = [&](const KisPixelSelectionSP &value, const QByteArray &reference) {
        QByteArray bytes(reference.size(), '\0'); value->readBytes(reinterpret_cast<quint8 *>(bytes.data()), canvas);
        return bytes == reference;
    };
    QVERIFY(verify(pixels, expected));
    QCOMPARE(pixels->selectedExactRect(), native->selectedExactRect());
    QCOMPARE(pixels->pixel({18, 17}).opacityU8(), quint8(1));
    QCOMPARE(pixels->pixel({17, 17}).opacityU8(), quint8(0));
    QVERIFY(!pixels->outlineCache().contains(QPointF(18.5, 17.5)));
    QVERIFY(pixels->outlineCache().contains(QPointF(16.5, 16.5)));
    KisPixelSelectionSP copied = new KisPixelSelection(*pixels);
    QVERIFY(verify(copied, expected));
    pixels->recalculateThumbnailImage(Qt::red); native->recalculateThumbnailImage(Qt::red);
    QCOMPARE(pixels->thumbnailImage(), native->thumbnailImage());
    const auto history = owner->operations().undoCount();
    QCOMPARE(owner->setSelectionMask(mask, true), Gpu::Result::Success);
    QTRY_VERIFY(image->globalSelection());
    // The view object can be reused by synchronization; the independent copy
    // keeps the earlier immutable mask while current selectedness is inverted.
    QByteArray inverse = expected;
    for (char &value : inverse) value = char(255 - quint8(value));
    QTRY_VERIFY(verify(image->globalSelection()->pixelSelection(), inverse));
    const auto invertedPixels = image->globalSelection()->pixelSelection();
    QVERIFY(verify(invertedPixels, inverse));
    QCOMPARE(invertedPixels->selectedExactRect(), canvas);
    QCOMPARE(owner->operations().undoCount(), history + 1);
    QVERIFY(verify(copied, expected));
    owner->close();
    QVERIFY(verify(copied, expected));
    QVERIFY(verify(invertedPixels, inverse));
}

void KisGpuDocumentViewTest::colourSelectionToolsKeepPixelsHistoryAndKra_data()
{
    QTest::addColumn<QString>("toolId");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("action");
    QTest::addColumn<QString>("scenario");
    QTest::newRow("contiguous-byte-lab") << QStringLiteral("KisToolSelectContiguous") << false << int(SELECTION_REPLACE) << QStringLiteral("lab");
    // #89 A0c: a masked similar-color selection exercises the delayed GPU read.
    QTest::newRow("similar-word-masked") << QStringLiteral("KisToolSelectSimilar") << true << int(SELECTION_INTERSECT) << QStringLiteral("masked");
}

