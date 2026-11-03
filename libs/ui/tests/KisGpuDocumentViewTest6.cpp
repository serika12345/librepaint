/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDocumentViewTest.h"

void KisGpuDocumentViewTest::cropPreviewPreservesPixelsHistoryAndCancellation()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig config(false);
    const auto oldStyle = config.newOutlineStyle();
    const auto oldColor = config.getCursorMainColor();
    const auto oldSeparate = config.separateEraserCursor();
    const auto oldFullSize = config.forceAlwaysFullSizedOutline();
    const auto oldMinimum = config.outlineSizeMinimum();
    const auto oldBorder = config.canvasBorderColor();
    const auto oldVastScrolling = config.vastScrolling();
    const auto restoreConfig = qScopeGuard([&] {
        config.setNewOutlineStyle(oldStyle);
        config.setCursorMainColor(oldColor);
        config.setSeparateEraserCursor(oldSeparate);
        config.setForceAlwaysFullSizedOutline(oldFullSize);
        config.setOutlineSizeMinimum(oldMinimum);
        config.setCanvasBorderColor(oldBorder);
        config.setVastScrolling(oldVastScrolling);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    config.setNewOutlineStyle(OUTLINE_NONE);
    config.setCursorMainColor(Qt::white);
    config.setSeparateEraserCursor(false);
    config.setForceAlwaysFullSizedOutline(true);
    config.setOutlineSizeMinimum(1);
    config.setCanvasBorderColor(Qt::white);
    config.setVastScrolling(1);
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
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU crop preview")));
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
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
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
    // This case runs as its own CTest entry. Wait for the new view's layout so
    // centering uses the real canvas size and the native window can present.
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    const auto centerImage = [&] {
        const auto size = converter->imageRectInWidgetPixels().size();
        view->canvasController()->setPreferredCenter(QPointF(size.width() / 2, size.height() / 2));
    };
    centerImage();
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
    // The offscreen platform does not deliver an expose event for the new
    // window, so request the frame that the ordinary exposed window receives.
    native->refresh();
    QTRY_COMPARE(execution.published(owner->handle()), snapshot);
    const auto capture = [&] { return native->screen()->grabWindow(native->winId()).toImage().convertToFormat(QImage::Format_RGBA8888); };

    const auto ratio = native->devicePixelRatio();
    const auto expectedSize = capture();
    QImage expected(expectedSize.size(), QImage::Format_RGBA8888);
    expected.setDevicePixelRatio(expectedSize.devicePixelRatio());
    expected.setColorSpace(expectedSize.colorSpace());
    expected.fill(Qt::white);
    const auto rect = QTransform::fromScale(ratio, ratio).mapRect(converter->imageRectInWidgetPixels()).toAlignedRect().intersected(expected.rect());
    for (int y = rect.top(); y <= rect.bottom(); ++y)
        for (int x = rect.left(); x <= rect.right(); ++x) expected.setPixelColor(x, y, QColor(80, 100, 120));
    QImage baseline;
    QTRY_VERIFY_WITH_TIMEOUT((baseline = capture()) == expected, 10000);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolCrop"));
    auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KisToolCrop"));
    QVERIFY2(tool, "The ordinary crop plugin must be available");
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolCrop"));
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    KConfigGroup cropSettings(KSharedConfig::openConfig(), tool->toolId());
    const auto previousCropSettings = cropSettings.entryMap();
    const auto restoreCropSettings = qScopeGuard([&] {
        for (const auto &key : {QStringLiteral("growCenter"), QStringLiteral("allowGrow"),
                               QStringLiteral("decoration"), QStringLiteral("cropType")}) {
            if (previousCropSettings.contains(key)) cropSettings.writeEntry(key, previousCropSettings[key]);
            else cropSettings.deleteEntry(key);
        }
    });
    QVERIFY(QMetaObject::invokeMethod(tool, "setGrowCenter", Qt::DirectConnection, Q_ARG(bool, false)));
    QVERIFY(QMetaObject::invokeMethod(tool, "setAllowGrow", Qt::DirectConnection, Q_ARG(bool, false)));
    QVERIFY(QMetaObject::invokeMethod(tool, "setCropTypeLegacy", Qt::DirectConnection, Q_ARG(int, 0)));
    auto *snapGuide = view->canvasBase()->snapGuide();
    const bool previousSnapping = snapGuide->isSnapping();
    const auto previousStrategies = snapGuide->enabledSnapStrategies();
    const int previousSnapDistance = snapGuide->snapDistance();
    const auto restoreSnapping = qScopeGuard([&] {
        snapGuide->enableSnapStrategies(previousStrategies);
        snapGuide->setSnapDistance(previousSnapDistance);
        snapGuide->enableSnapping(previousSnapping);
    });
    snapGuide->enableSnapping(false);
    QVERIFY(tool->setProperty("lockWidth", false));
    QVERIFY(tool->setProperty("lockHeight", false));
    QVERIFY(tool->setProperty("lockRatio", false));
    QVERIFY(tool->setProperty("cropX", 32));
    QVERIFY(tool->setProperty("cropY", 32));
    QVERIFY(tool->setProperty("cropWidth", 64));
    QVERIFY(tool->setProperty("cropHeight", 64));
    const auto retainedVersion = owner->operations().preview();
    const QString directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) QVERIFY(QDir().mkpath(directory));
    const auto preserveCropFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed() && !directory.isEmpty())
            capture().save(QDir(directory).filePath(QStringLiteral("crop-failure.png")));
    });
    for (int style = 0; style < 5; ++style) {
        QVERIFY(tool->setProperty("decoration", style));
        auto expectedPreview = baseline;
        expectedPreview.setDevicePixelRatio(ratio);
        QPainter painter(&expectedPreview);
        // The ordinary CPU canvas enables these hints before invoking the tool.
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                               QPainter::SmoothPixmapTransform);
        painter.setTransform(converter->flakeToWidgetTransform());
        tool->paint(painter, *view->canvasBase()->viewConverter());
        painter.end();
        expectedPreview.setDevicePixelRatio(1);
        QImage actual;
        const auto matches = [&] {
            actual = capture();
            if (actual.size() != expectedPreview.size()) return false;
            for (int y = 0; y < actual.height(); ++y) {
                for (int x = 0; x < actual.width(); ++x) {
                    const auto a = actual.pixelColor(x, y);
                    const auto e = expectedPreview.pixelColor(x, y);
                    // Display-only coverage has a two-byte rounding bound.
                    // Document, history and retainedVersion retain exact comparisons.
                    if (qAbs(a.red() - e.red()) > 2 || qAbs(a.green() - e.green()) > 2 ||
                        qAbs(a.blue() - e.blue()) > 2 || qAbs(a.alpha() - e.alpha()) > 2) return false;
                }
            }
            return true;
        };
        const auto preserve = qScopeGuard([&] {
            if (directory.isEmpty()) return;
            actual.save(QDir(directory).filePath(QStringLiteral("crop-%1-actual.png").arg(style)));
            expectedPreview.save(QDir(directory).filePath(QStringLiteral("crop-%1-expected.png").arg(style)));
        });
        native->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 20000);
        QCOMPARE(owner->operations().snapshot(), snapshot);
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(owner->operations().preview() == retainedVersion);
    }
    const auto base = [&] {
        const auto screenshot = capture();
        QImage image(screenshot.size(), QImage::Format_RGBA8888);
        image.setDevicePixelRatio(screenshot.devicePixelRatio());
        image.setColorSpace(screenshot.colorSpace());
        image.fill(Qt::white);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const auto position = converter->widgetToImage(QPointF((x + 0.5) / ratio, (y + 0.5) / ratio));
                if (position.x() >= 0 && position.y() >= 0 && position.x() < 128 && position.y() < 128)
                    image.setPixelColor(x, y, QColor(80, 100, 120));
            }
        }
        return image;
    };
    QImage actualPreview, expectedPreview;
    const auto previewMatches = [&] {
        expectedPreview = base();
        expectedPreview.setDevicePixelRatio(ratio);
        QPainter painter(&expectedPreview);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        painter.setTransform(converter->flakeToWidgetTransform());
        tool->paint(painter, *view->canvasBase()->viewConverter());
        painter.end();
        expectedPreview.setDevicePixelRatio(1);
        actualPreview = capture();
        if (actualPreview.size() != expectedPreview.size()) return false;
        for (int y = 0; y < actualPreview.height(); ++y) {
            for (int x = 0; x < actualPreview.width(); ++x) {
                const auto actual = actualPreview.pixelColor(x, y);
                const auto expected = expectedPreview.pixelColor(x, y);
                if (qAbs(actual.red() - expected.red()) > 2 || qAbs(actual.green() - expected.green()) > 2 ||
                    qAbs(actual.blue() - expected.blue()) > 2 || qAbs(actual.alpha() - expected.alpha()) > 2) return false;
            }
        }
        return true;
    };
    const auto point = [&](QPointF position) { return converter->imageToWidget(position).toPoint(); };
    const auto safeInput = QRectF(QPointF(), native->size()).adjusted(64, 64, -64, -64);
    QVERIFY(safeInput.contains(point({16, 16})));
    QVERIFY(safeInput.contains(point({128, 108})));
    for (int stage = 0; stage < 5; ++stage) {
        if (stage == 0) {
            QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({64, 64}));
            QTest::mouseMove(native.data(), point({70, 68}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({70, 68}));
            QTest::qWait(50);
            QCOMPARE(tool->property("cropX").toInt(), 38);
            QCOMPARE(tool->property("cropY").toInt(), 36);
            QCOMPARE(tool->property("cropWidth").toInt(), 64);
            QCOMPARE(tool->property("cropHeight").toInt(), 64);
        } else if (stage == 1) {
            QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({102, 100}));
            QTest::mouseMove(native.data(), point({110, 108}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({110, 108}));
            QTest::qWait(50);
            QCOMPARE(tool->property("cropWidth").toInt(), 72);
            QCOMPARE(tool->property("cropHeight").toInt(), 72);
        } else if (stage == 2) view->canvasController()->pan(QPoint(7, -5));
        else if (stage == 3) view->canvasController()->setZoom(KoZoomMode::ZOOM_CONSTANT, 0.5);
        else view->canvasController()->rotateCanvas(23);
        const auto preserve = qScopeGuard([&] {
            if (directory.isEmpty()) return;
            actualPreview.save(QDir(directory).filePath(QStringLiteral("crop-operation-%1-actual.png").arg(stage)));
            expectedPreview.save(QDir(directory).filePath(QStringLiteral("crop-operation-%1-expected.png").arg(stage)));
        });
        native->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(previewMatches(), 20000);
        QCOMPARE(owner->operations().snapshot(), snapshot);
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(owner->operations().preview() == retainedVersion);
        if (stage == 0) {
            snapGuide->enableSnapStrategies(KoSnapGuide::DocumentBoundsSnapping);
            snapGuide->setSnapDistance(10);
            snapGuide->enableSnapping(true);
            const QPoint nearRight = point({128, 100}) - QPoint(2, 0);
            QVERIFY(safeInput.contains(nearRight));
            const auto image = view->canvasBase()->image();
            const auto cursorImage = converter->widgetToImage(QPointF(nearRight));
            const auto unsnapped = image->documentToPixel(image->pixelToDocument(cursorImage));
            const auto snapped = image->documentToPixel(snapGuide->snap(image->pixelToDocument(cursorImage), Qt::NoModifier));
            QCOMPARE(snapped.x(), 128.0);
            QCOMPARE(snapped.y(), unsnapped.y());
            QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({102, 100}));
            QTest::mouseMove(native.data(), nearRight, 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, nearRight);
            QTest::qWait(50);
            QCOMPARE(tool->property("cropX").toInt(), 38);
            QCOMPARE(tool->property("cropWidth").toInt(), 90);
            QCOMPARE(tool->property("cropHeight").toInt(), 64);
            const auto preserveSnap = qScopeGuard([&] {
                if (directory.isEmpty()) return;
                actualPreview.save(QDir(directory).filePath(QStringLiteral("crop-snapped-actual.png")));
                expectedPreview.save(QDir(directory).filePath(QStringLiteral("crop-snapped-expected.png")));
            });
            native->refresh();
            QTRY_VERIFY_WITH_TIMEOUT(previewMatches(), 20000);
            QCOMPARE(owner->operations().snapshot(), snapshot);
            QCOMPARE(owner->operations().undoCount(), 0);
            QVERIFY(owner->operations().preview() == retainedVersion);
            snapGuide->enableSnapping(false);
            QVERIFY(tool->setProperty("cropWidth", 64));
        }
    }
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QCOMPARE(owner->operations().undoCount(), 0);
    QVERIFY(QMetaObject::invokeMethod(tool, "requestStrokeCancellation", Qt::DirectConnection));
    native->refresh();
    QTRY_VERIFY(capture() == base());
    const auto after = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels.size());
    QCOMPARE(after.code, Gpu::ExecutionCode::Success);
    QCOMPARE(after.bytes, pixels);
    QVERIFY(!lifetime->isModified());
    view->canvasController()->resetCanvasRotation();
    view->zoomManager()->zoomTo100();
    centerImage();
    QVERIFY(safeInput.contains(point({16, 16})));
    QVERIFY(safeInput.contains(point({112, 96})));
    const auto newCropOrigin = converter->widgetToImage(QPointF(point({16, 16}))).toPoint();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({16, 16}));
    QTest::mouseMove(native.data(), point({112, 96}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({112, 96}));
    QTest::qWait(50);
    QCOMPARE(tool->property("cropX").toInt(), newCropOrigin.x());
    QCOMPARE(tool->property("cropY").toInt(), newCropOrigin.y());
    QCOMPARE(tool->property("cropWidth").toInt(), 96);
    QCOMPARE(tool->property("cropHeight").toInt(), 80);
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QVERIFY(QMetaObject::invokeMethod(tool, "crop", Qt::DirectConnection));
    QTRY_COMPARE(owner->operations().snapshot()->size, QSize(96, 80));
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(lifetime->undoStack()->count(), 1);
    const auto croppedPixels = QByteArray::fromHex("506478ff").repeated(96 * 80);
    const auto readCrop = [&] { return owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 96, 80), croppedPixels.size()).bytes; };
    QCOMPARE(readCrop(), croppedPixels);
    lifetime->undoStack()->undo();
    QTRY_COMPARE(owner->operations().snapshot()->size, QSize(128, 128));
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels.size()).bytes, pixels);
    lifetime->undoStack()->redo();
    QTRY_COMPARE(owner->operations().snapshot()->size, QSize(96, 80));
    QCOMPARE(readCrop(), croppedPixels);
    QTemporaryDir saved;
    QVERIFY(saved.isValid());
    const auto preserveSavedFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            saved.setAutoRemove(false);
            qInfo() << "Crop save failure artifacts at" << saved.path();
        }
    });
    lifetime->setFileBatchMode(true);
    const auto path = saved.filePath(QStringLiteral("crop.kra"));
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QTRY_VERIFY(!lifetime->isModified());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.size, QSize(96, 80));
    Gpu::PaintImage reopened(execution, loaded.document.image);
    QCOMPARE(reopened.readRgbaPixels(reopened.operations().activeLayer(), QRect(0, 0, 96, 80), croppedPixels.size()).bytes, croppedPixels);
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("alpha", true);
    configuration->setProperty("interlaced", false);
    configuration->setProperty("compression", 3);
    configuration->setProperty("saveSRGBProfile", true);
    configuration->setProperty("forceSRGB", false);
    configuration->setProperty("storeAuthor", false);
    configuration->setProperty("storeMetaData", false);
    const auto pngPath = saved.filePath(QStringLiteral("crop.png"));
    QVERIFY2(lifetime->exportDocumentSync(pngPath, QByteArrayLiteral("image/png"), configuration), qPrintable(lifetime->errorMessage()));
    const auto png = QImage(pngPath).convertToFormat(QImage::Format_RGBA8888);
    QCOMPARE(png.size(), QSize(96, 80));
    QCOMPARE(QByteArray::fromRawData(reinterpret_cast<const char*>(png.constBits()), png.sizeInBytes()), croppedPixels);
    if (!directory.isEmpty()) {
        const auto copyArtifact = [&](const QString &source, const QString &name) {
            const QString destination = QDir(directory).filePath(name);
            QFile::remove(destination);
            return QFile::copy(source, destination);
        };
        QVERIFY(copyArtifact(path, QStringLiteral("crop-edited.kra")));
        QVERIFY(copyArtifact(pngPath, QStringLiteral("crop-edited.png")));
    }
}

void KisGpuDocumentViewTest::standardAutoBrushPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("tipCase");
    QTest::addColumn<qreal>("normalDiameter");
    QTest::addColumn<bool>("rotatedCanvas");
    for (const auto *name : {"b)_Basic-5_Size_default.kpp", "b)_Basic-2_Opacity.kpp", "a)_Eraser_Circle.kpp",
                             "b)_Basic-5_Size_Opacity.kpp", "a)_Eraser_Small.kpp", "d)_Ink-3_Gpen.kpp",
                             "b)_Basic-3_Flow.kpp", "b)_Basic-4_Flow_Opacity.kpp", "d)_Ink-2_Fineliner.kpp"})
        for (bool words : {false, true}) {
            const QByteArray row = QByteArray(name) + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(name) << words << 0 << qreal(40) << false;
        }
    for (const auto *name : {"b)_Basic-3_Flow.kpp", "b)_Basic-4_Flow_Opacity.kpp"})
        for (bool words : {false, true}) {
            const QByteArray row = QByteArray("constant-rotation-mirrored-canvas-") + name + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(name) << words << 0 << qreal(40) << true;
        }
    for (bool words : {false, true}) {
        const auto suffix = words ? "-u16" : "-u8";
        QTest::newRow(qPrintable(QStringLiteral("ellipse-fractional") + suffix))
            << QStringLiteral("b)_Basic-5_Size_default.kpp") << words << 1 << qreal(13.75) << false;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-rotated") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 2 << qreal(40) << false;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-rotated-mirrored-canvas") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 2 << qreal(40) << true;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-hard-horizontal-edge") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 3 << qreal(13.75) << false;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-isotropic-spacing") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 4 << qreal(40) << false;
    }
    for (bool words : {false, true})
        for (const auto *mode : {"subtract", "multiply", "darken"})
            for (bool soft : {false, true}) {
                const QByteArray row = QByteArray("texture-") + mode + (soft ? "-soft" : "-hard") + (words ? "-u16" : "-u8");
                QTest::newRow(row.constData()) << QStringLiteral("d)_Ink-7_Brush_Rough.kpp") << words << 0 << qreal(40) << false;
            }

    for (bool words : {false, true}) {
        for (const auto *mode : {"preset", "taper"}) {
            const QByteArray row = QByteArray("texture-") + mode + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QStringLiteral("d)_Ink-7_Brush_Rough.kpp") << words << 0 << qreal(40) << false;
        }
        for (int precision = 1; precision <= 5; ++precision) {
            const QByteArray row = QByteArray("texture-precision-") + QByteArray::number(precision) + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QStringLiteral("d)_Ink-7_Brush_Rough.kpp") << words << 0 << qreal(40) << false;
        }
    }

    for (bool words : {false, true})
        QTest::newRow(words ? "texture-screentone-u16" : "texture-screentone-u8")
            << QStringLiteral("y)_Screentones_Regular.kpp") << words << 0 << qreal(40) << false;
    for (bool words : {false, true})
        for (const auto *mode : {"preset", "taper", "sharpness-soft-aligned", "sharpness-legacy"}) {
            const QByteArray row = QByteArray("dither-") + mode + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QStringLiteral("u)_Pixel_Art_Dithering.kpp") << words << 0 << qreal(12) << false;
        }

    const QList<QPair<QString, qreal>> gaussianPresets{
        {QStringLiteral("u)_Pixel_Art.kpp"), 1}, {QStringLiteral("u)_Pixel_Art_Fill.kpp"), 32},
        {QStringLiteral("h)_Charcoal_pencil_large.kpp"), 32}, {QStringLiteral("h)_Charcoal_Pencil_Medium.kpp"), 12},
        {QStringLiteral("h)_Charcoal_Pencil_Thin.kpp"), 6}};
    for (const auto &preset : gaussianPresets) for (bool words : {false, true}) {
        const QByteArray row = preset.first.toLatin1() + (words ? "-u16" : "-u8");
        QTest::newRow(row.constData()) << preset.first << words << 0 << preset.second << false;
    }

    for (const auto *name : {"c)_Pencil-1_Hard.kpp", "c)_Pencil-2.kpp"}) for (bool words : {false, true}) {
        const QByteArray row = QByteArray(name) + (words ? "-u16" : "-u8");
        QTest::newRow(row.constData()) << QString::fromLatin1(name) << words << 0 << qreal(10) << false;
    }

    for (const auto &preset : QList<QPair<QString, qreal>>{{QStringLiteral("c)_Pencil-4_Soft.kpp"), 16},
                                                         {QStringLiteral("c)_Pencil-6_Quick_Shade.kpp"), 60},
                                                         {QStringLiteral("c)_Pencil-3_Large_4B.kpp"), 10},
                                                         {QStringLiteral("c)_Pencil-5_Tilted.kpp"), 40},
                                                         {QStringLiteral("b)_Basic-6_Details.kpp"), 7.5}})
        for (bool words : {false, true}) {
            const QByteArray row = preset.first.toLatin1() + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << preset.first << words << 0 << preset.second << false;
        }

    for (bool words : {false, true}) for (const auto *mode : {"soft", "ellipse"}) {
        const QByteArray row = QByteArray("gaussian-unaliased-") + mode + (words ? "-u16" : "-u8");
        QTest::newRow(row.constData()) << QStringLiteral("u)_Pixel_Art_Fill.kpp") << words << 0 << qreal(32) << false;
    }

    for (bool words : {false, true})
        QTest::newRow(words ? "default-buildup-u16" : "default-buildup-u8")
            << QStringLiteral("c)_Pencil-3_Large_4B.kpp") << words << 0 << qreal(10) << false;

    for (bool words : {false, true})
        QTest::newRow(words ? "details-tilt-size-u16" : "details-tilt-size-u8")
            << QStringLiteral("b)_Basic-6_Details.kpp") << words << 0 << qreal(7.5) << false;

}

void KisGpuDocumentViewTest::standardAutoBrushPresetsKeepOrdinaryEditing()
{
    QFETCH(QString, presetName);
    const bool imageTip = presetName == QLatin1String("b)_Basic-6_Details.kpp") || presetName == QLatin1String("c)_Pencil-4_Soft.kpp") || presetName == QLatin1String("c)_Pencil-6_Quick_Shade.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    const bool incrementalPencil = presetName == QLatin1String("c)_Pencil-3_Large_4B.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    QFETCH(bool, words);
    QFETCH(int, tipCase);
    QFETCH(qreal, normalDiameter);
    QFETCH(bool, rotatedCanvas);
    const int channelBytes = words ? 2 : 1;
    const int pixelBytes = 4 * channelBytes;
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle");
    const auto bundle = KisResourceStorageSP::create(bundlePath);
    QVERIFY(bundle->valid());
    KisResourceModel brushes(ResourceType::Brushes);
    if (brushes.rowCount() == 0)
        QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
    KisResourceModel patterns(ResourceType::Patterns);
    if (patterns.rowCount() == 0)
        QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
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

    const bool pencil = presetName.startsWith(QLatin1String("c)_Pencil-"));
    const bool directionalTip = pencil || presetName == QLatin1String("b)_Basic-6_Details.kpp");
    const bool fineliner = presetName == QLatin1String("d)_Ink-2_Fineliner.kpp");
    const auto restoreTiming = fixedBrushPointerTiming(fineliner);
    const bool screentone = presetName == QLatin1String("y)_Screentones_Regular.kpp");
    const bool dither = presetName == QLatin1String("u)_Pixel_Art_Dithering.kpp");
    const bool textured = sourcePreset->settings()->getBool("Texture/Pattern/Enabled");
    const QByteArray dataTag(QTest::currentDataTag());
    const bool tiltSize = dataTag.startsWith("details-tilt-size-");
    if (tiltSize) {
        // The existing brush editor combines tilt-driven size with direction
        // rotation and corner fans, including interpolated pen axes at the fan.
        auto settings = sourcePreset->settings();
        settings->setProperty("SizeSensor", QStringLiteral("<params id='declination'><curve>0,.5;1,1;</curve></params>"));
        settings->setProperty("SizeUseSameCurve", false);
    }
    if (dataTag.startsWith("default-buildup-"))
        sourcePreset->settings()->removeProperty("PaintOpAction");
    if (dataTag.startsWith("gaussian-unaliased-")) {
        // Existing brush-editor controls: keep the native Gaussian gradient
        // when sharpness is unchecked, including an elliptical rotated tip.
        sourcePreset->settings()->setProperty("PressureSharpness", false);
        if (dataTag.contains("ellipse")) {
            QDomDocument definition;
            QVERIFY(definition.setContent(sourcePreset->settings()->getString("brush_definition")));
            auto root = definition.documentElement();
            root.setAttribute(QStringLiteral("angle"), .37);
            auto mask = root.firstChildElement(QStringLiteral("MaskGenerator"));
            mask.setAttribute(QStringLiteral("ratio"), .8);
            mask.setAttribute(QStringLiteral("hfade"), .5);
            mask.setAttribute(QStringLiteral("vfade"), .5);
            sourcePreset->settings()->setProperty("brush_definition", definition.toString());
        }
    }
    if (textured) {
        const auto file = QFileInfo(sourcePreset->settings()->getString("Texture/Pattern/PatternFileName")).fileName();
        const auto paper = bundle->resource(QStringLiteral("patterns/") + file).dynamicCast<KoPattern>();
        QVERIFY(paper);
        if (!KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns).bestMatch(paper->md5Sum(), paper->filename(), paper->name()))
            QVERIFY(patterns.addResource(paper));
        auto settings = sourcePreset->settings();
        // Freeze native random outputs as part of the comparison's input.
        settings->setProperty("Texture/Pattern/isRandomOffsetX", false);
        settings->setProperty("Texture/Pattern/isRandomOffsetY", false);
        settings->setProperty("Texture/Pattern/OffsetX", screentone || dither ? 0 : 7);
        settings->setProperty("Texture/Pattern/OffsetY", screentone || dither ? 0 : 11);
        const QByteArray row(QTest::currentDataTag());
        if (row.startsWith("texture-subtract") || row.startsWith("texture-multiply") || row.startsWith("texture-darken")) {
            settings->setProperty("Texture/Pattern/TexturingMode", row.contains("multiply") ? 0 : row.contains("darken") ? 4 : 1);
            settings->setProperty("Texture/Pattern/UseSoftTexturing", row.contains("-soft"));
        }
        // A short fade exercises per-dab sequence and the pressure sum during
        // the ordinary strokes, including reflected and overlapping dabs.
        auto sensors = settings->getString("Texture/Strength/Sensor");
        if (row.startsWith("texture-") && !row.contains("preset")) sensors.replace(QStringLiteral("length=\"1000\""), QStringLiteral("length=\"7\""));
        if (row.contains("precision")) {
            settings->setProperty("KisPrecisionOption/AutoPrecisionEnabled", false);
            settings->setProperty("KisPrecisionOption/precisionLevel", row.split('-').value(2).toInt());
        }
        settings->setProperty("Texture/Strength/Sensor", sensors);
        if (row.contains("sharpness-soft-aligned")) {
            settings->setProperty("SharpnessValue", .65);
            settings->setProperty("SharpnessUseCurve", true);
            settings->setProperty("SharpnessSensor", QStringLiteral("<params id='pressure'><curve>0,0;1,1;</curve></params>"));
            settings->setProperty("SharpnessUseSameCurve", false);
            settings->setProperty("SharpnesscurveMode", 0);
            settings->setProperty("Sharpness/alignoutline", true);
            settings->setProperty("Sharpness/softness", 45);
        } else if (row.contains("sharpness-legacy")) {
            settings->removeProperty("SharpnessValue");
            settings->setProperty("Sharpness/factor", .65);
        }
    }

    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QSize size(128, 128);
    QList<QByteArray> expectedStates;
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto output = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(
        QStringLiteral("ordinary-brush-%1").arg(QString::fromLatin1(QTest::currentDataTag())));
    QVERIFY(QDir().mkpath(output));
    // Random normal strokes retain their own seed. Fixed-seed native comparisons
    // below protect their pixels; this UI path protects editing and persistence.
    for (bool gpu : pencil && !incrementalPencil ? QList<bool>{true} : QList<bool>{false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        auto preset = sourcePreset->clone().dynamicCast<KisPaintOpPreset>();
        QVERIFY(preset);
        if (tipCase) {
            QDomDocument definition;
            QVERIFY(definition.setContent(preset->settings()->getString("brush_definition")));
            auto brush = definition.documentElement();
            auto mask = brush.firstChildElement(QStringLiteral("MaskGenerator"));
            brush.setAttribute(QStringLiteral("angle"), tipCase == 1 ? .37 : .51);
            mask.setAttribute(QStringLiteral("type"), tipCase == 1 ? QStringLiteral("circle") : QStringLiteral("rect"));
            mask.setAttribute(QStringLiteral("ratio"), tipCase == 1 ? .4 : .37);
            mask.setAttribute(QStringLiteral("hfade"), tipCase == 1 ? .6 : tipCase == 3 ? 1. : .8);
            mask.setAttribute(QStringLiteral("vfade"), tipCase == 1 ? .2 : .3);
            if (tipCase == 3) mask.setAttribute(QStringLiteral("antialiasEdges"), 0);
            preset->settings()->setProperty("brush_definition", definition.toString());
            if (tipCase == 4) preset->settings()->setProperty("Spacing/Isotropic", true);
        }
        preset->setFilename((gpu ? QStringLiteral("gpu-") : QStringLiteral("cpu-")) +
                            QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(preset));
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        document->setAutoSaveActive(false);
        document->setFileBatchMode(true);
        QVERIFY(document->newImage(QStringLiteral("Ordinary brush editing"), size.width(), size.height(), cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        QCOMPARE(bool(document->image()->gpuImage()), gpu);
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
        window->resize(800, 600);
        auto *view = window->newView(document.get());
        QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        BrushPointerClock pointerClock;
        QPointer<QWindow> input;
        bool pressed = false;
        const auto closeWindow = qScopeGuard([&] {
            if (pressed && input) {
                if (lifetime) lifetime->image()->requestStrokeCancellation();
                QTest::mouseRelease(input, Qt::LeftButton);
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
        resources->setPaintOpPreset(preset);
        resources->setSize(normalDiameter);
        resources->setOpacity(.7);
        resources->setGlobalAlphaLock(false);
        resources->setMirrorHorizontal(false);
        resources->setMirrorVertical(false);
        resources->setEraserMode(false);
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
        QVERIFY(tool);
        QVERIFY(tool->setProperty("smoothingType", 0));
        QSignalSpy errors(tool, &KoToolBase::statusTextChanged);
        const auto prepareInput = [&] {
            view->canvasBase()->canvasWidget()->setFocus();
            view->canvasController()->resetCanvasRotation();
            view->canvasController()->mirrorCanvas(false);
            view->zoomManager()->zoomTo100();
            const auto extent = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
            view->canvasController()->setPreferredCenter({extent.width() / 2, extent.height() / 2});
            if (rotatedCanvas) {
                view->canvasController()->rotateCanvas(23);
                view->canvasController()->mirrorCanvas(true);
            }
            input = view->canvasBase()->canvasWidget()->window()->windowHandle();
            if (gpu) for (auto *candidate : QGuiApplication::allWindows())
                if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = candidate;
            if (fineliner && input) input->installEventFilter(&pointerClock);
        };
        QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= size.width() &&
                    view->canvasBase()->canvasWidget()->height() >= size.height());
        prepareInput();
        QTRY_VERIFY(input && input->isExposed());
        if (gpu) {
            const auto capture = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                          view->canvasBase()->resourceManager());
            QVERIFY2(capture.diagnostic.isEmpty(), qPrintable(capture.diagnostic));
            QVERIFY(capture.brush.autoBrush);
            QCOMPARE(capture.brush.autoBrush->mask, imageTip ? Gpu::AutoBrush::Mask::Image : incrementalPencil ? Gpu::AutoBrush::Mask::GaussianCircle : (presetName == QLatin1String("b)_Basic-5_Size_Opacity.kpp") ||
                      presetName == QLatin1String("b)_Basic-3_Flow.kpp") || presetName == QLatin1String("b)_Basic-4_Flow_Opacity.kpp") ||
                      presetName.startsWith(QLatin1String("h)_Charcoal_")) || pencil ||
                      presetName == QLatin1String("u)_Pixel_Art.kpp") || presetName == QLatin1String("u)_Pixel_Art_Fill.kpp")) ?
                     Gpu::AutoBrush::Mask::GaussianCircle :
                     (tipCase >= 2 || presetName == QLatin1String("a)_Eraser_Small.kpp")) ?
                     Gpu::AutoBrush::Mask::Rectangle : Gpu::AutoBrush::Mask::Circle);
            if (textured) {
                QVERIFY(capture.brush.texture);
                QCOMPARE(capture.brush.texture->offset, screentone || dither ? QPoint() : QPoint(7, 11));
                const auto frozen = *capture.brush.texture;
                auto currentPreset = resources->currentPreset();
                auto settings = currentPreset->settings();
                settings->setProperty("Texture/Pattern/isRandomOffsetX", true);
                settings->setProperty("Texture/Pattern/isRandomOffsetY", true);
                const auto randomized = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                                 view->canvasBase()->resourceManager());
                QVERIFY2(randomized.diagnostic.isEmpty(), qPrintable(randomized.diagnostic));
                QVERIFY(randomized.brush.texture);
                const auto &paper = *randomized.brush.texture;
                QVERIFY(paper.offset.x() >= 0 && paper.offset.x() < paper.size.width());
                QVERIFY(paper.offset.y() >= 0 && paper.offset.y() < paper.size.height());
                QCOMPARE(paper.alpha, frozen.alpha);
                settings->setProperty("Texture/Pattern/isRandomOffsetX", false);
                settings->setProperty("Texture/Pattern/isRandomOffsetY", false);
                const auto brightness = settings->getDouble("Texture/Pattern/Brightness");
                settings->setProperty("Texture/Pattern/Brightness", .2);
                const auto changed = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                              view->canvasBase()->resourceManager());
                QVERIFY2(changed.diagnostic.isEmpty(), qPrintable(changed.diagnostic));
                QVERIFY(changed.brush.texture && changed.brush.texture->alpha != frozen.alpha);
                QCOMPARE(*capture.brush.texture, frozen);
                settings->setProperty("Texture/Pattern/Brightness", brightness);
            }
            struct PressureProbe { qreal first, second, speed = 0; QPointF tilt; qreal tiltOffset = 0, canvasRotation = 0; bool mirroredH = false, mirroredV = false; };
            QList<PressureProbe> pressureCases{{40, .01}, {40, .05}, {40, .1}, {40, .33}, {40, .75}, {40, 1},
                                              {200, .03}, {200, .05}, {200, .06}};
            if (imageTip) {
                pressureCases.prepend({normalDiameter, 1});
                pressureCases.prepend({normalDiameter, .2});
                QVERIFY(capture.brush.autoBrush->imageTips);
            }
            if (incrementalPencil || tiltSize) {
                if (!imageTip) pressureCases.prepend({normalDiameter, 1, 0, {}});
                for (const QPointF tilt : {QPointF(35, 15), QPointF(-35, 15), QPointF(15, -35), QPointF(60, 60)})
                    pressureCases.append({normalDiameter, .6, 0, tilt, 17});
                pressureCases.append({normalDiameter, .6, 0, {35, 15}, 17, 37, true, false});
                pressureCases.append({normalDiameter, .6, 0, {35, 15}, 17, 37, true, true});
            }
            if (fineliner) for (const auto speed : {qreal(0), qreal(.2), qreal(.9), qreal(1)}) pressureCases.append({5, .6, speed});
            // This bundled size curve starts at x=.00813008. Both low-pressure
            // inputs must retain the native constant endpoint extension.
            if (dither) pressureCases.prepend({40, .004});
            if (dither) pressureCases.prepend({40, 0});
            for (const auto &input : pressureCases) {
                const auto diameter = input.first;
                const auto pressure = input.second;
                const qreal speed = input.speed;
                const QString probeName = QStringLiteral("pressure-d%1-p%2").arg(diameter).arg(pressure) +
                    (fineliner ? QStringLiteral("-s%1").arg(speed) : QString()) +
                    (incrementalPencil || tiltSize ? QStringLiteral("-tilt%1,%2-offset%3-canvas%4-%5%6").arg(input.tilt.x()).arg(input.tilt.y()).arg(input.tiltOffset).arg(input.canvasRotation).arg(input.mirroredH).arg(input.mirroredV) : QString());
                resources->setSize(diameter);
                const auto pressureCapture = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                                      view->canvasBase()->resourceManager());
                QVERIFY2(pressureCapture.diagnostic.isEmpty(), qPrintable(pressureCapture.diagnostic));
                const qreal endPressure = textured && (QByteArray(QTest::currentDataTag()).contains("precision") || QByteArray(QTest::currentDataTag()).contains("taper") || QByteArray(QTest::currentDataTag()).contains("sharpness")) ? std::min(pressure, qreal(.33)) : pressure;
                KisImageSP nativeImage = new KisImage(nullptr, size.width(), size.height(), cs, QStringLiteral("Native preset reference"));
                KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Reference"), 255);
                nativeImage->addNode(nativeLayer, nativeImage->root());
                if (resources->currentPreset()->hasMaskingPreset()) {
                    // The production stroke combines independently spaced primary
                    // and GrayA8 painters; a single paint-op omits its secondary tip.
                    const auto opacity = resources->opacity();
                    resources->setOpacity(1);
                    const auto restoreOpacity = qScopeGuard([&] { resources->setOpacity(opacity); });
                    {
                        KisFigurePaintingStroke stroke(kundo2_i18n("Native brush reference"), nativeImage, nativeLayer,
                            view->canvasBase()->resourceManager(), KisFigurePaintingOptions::StrokeStyleForeground,
                            KisFigurePaintingOptions::FillStyleNone);
                        stroke.paintLine(KisPaintInformation(QPointF(64.25, 64.75), pressure),
                                         KisPaintInformation(QPointF(104.25, 64.75), endPressure));
                    }
                    nativeImage->waitForDone();
                } else {
                    if (!incrementalPencil) nativeLayer->setTemporaryTarget(nativeLayer->paintDevice());
                    KisPainter painter(nativeLayer->paintDevice());
                    painter.setPaintColor(KoColor(QColor(17, 129, 241), cs));
                    painter.setCompositeOpId(incrementalPencil ? COMPOSITE_OVER : COMPOSITE_ALPHA_DARKEN);
                    painter.setPaintOpPreset(resources->currentPreset(), nativeLayer, nativeImage);
                    KisDistanceInformation distance;
                    if (directionalTip) {
                        distance = KisDistanceInformation(QPointF(64.25, 64.75), .37);
                        const KisRandomSourceSP random(new KisRandomSource(17));
                        KisPaintInformation first(QPointF(64.25, 64.75), pressure, input.tilt.x(), input.tilt.y(), 0, 0, 1, 0, speed);
                        KisPaintInformation second(QPointF(104.25, 64.75), endPressure, -input.tilt.x(), input.tilt.y() / 2, 0, 0, 1, 0, speed);
                        KisPaintInformation third(QPointF(66.25, 63.75), pressure / 2, input.tilt.x() / 3, -input.tilt.y(), 0, 0, 1, 0, speed);
                        for (auto *point : {&first, &second, &third}) {
                            point->setRandomSource(random); point->setTiltDirectionOffset(input.tiltOffset);
                            point->setCanvasRotation(input.canvasRotation); point->setCanvasMirroredH(input.mirroredH); point->setCanvasMirroredV(input.mirroredV);
                        }
                        painter.paintLine(first, second, &distance);
                        painter.paintLine(second, third, &distance);
                    } else if (fineliner) painter.paintLine(KisPaintInformation(QPointF(64.25, 64.75), pressure, 0, 0, 0, 0, 1, 0, speed),
                                                    KisPaintInformation(QPointF(104.25, 64.75), endPressure, 0, 0, 0, 0, 1, 1, 1 - speed), &distance);
                    else if (textured) painter.paintLine(KisPaintInformation(QPointF(64.25, 64.75), pressure),
                                                   KisPaintInformation(QPointF(104.25, 64.75), endPressure), &distance);
                    else painter.paintAt(fineliner ? KisPaintInformation(QPointF(64.25, 64.75), pressure, 0, 0, 0, 0, 1, 0, speed) :
                                         KisPaintInformation(QPointF(64.25, 64.75), pressure), &distance);
                    bool pending;
                    do {
                        QVector<KisRunnableStrokeJobData *> jobs;
                        pending = painter.paintOp()->doAsynchronousUpdate(jobs).second;
                        painter.runnableStrokeJobsInterface()->addRunnableJobs(jobs);
                    } while (pending);
                }
                nativeLayer->setTemporaryTarget({});
                nativeImage->waitForDone();
                // BUILDUP's reference is the directly painted layer, including
                // RGB at zero alpha. WASH merges its source with canvas opacity.
                KisPaintDeviceSP destination = nativeLayer->paintDevice();
                if (!incrementalPencil) {
                    destination = new KisPaintDevice(cs);
                    KisPainter merge(destination);
                    merge.setOpacityF(.7);
                    for (const auto &rect : nativeLayer->paintDevice()->region().rects())
                        merge.bitBlt(rect.topLeft(), nativeLayer->paintDevice(), rect);
                }
                QByteArray expected(size.width() * size.height() * pixelBytes, Qt::Uninitialized);
                destination->readBytes(reinterpret_cast<quint8 *>(expected.data()), QRect(QPoint(), size));
                QString diagnostic;
                Gpu::RasterImage raster;
                raster.size = size;
                raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
                raster.colorProfile = cs->profile()->rawData();
                raster.layers.append(Gpu::RasterLayer{QStringLiteral("Pressure")});
                const auto pressureOwner = Gpu::PaintImageSP::create(KisPart::instance()->gpuExecution(diagnostic), raster);
                const auto operation = pressureOwner->beginStroke(pressureOwner->operations().activeLayer(), pressureCapture.brush, 17);
                QVERIFY(!operation.isNull());
                QList<Gpu::InputSample> pressureSamples{{{64.25, 64.75}, pressure}};
                if (textured || fineliner || directionalTip) pressureSamples.append({{104.25, 64.75}, endPressure});
                if (directionalTip) {
                    pressureSamples.append({{66.25, 63.75}, pressure / 2});
                    for (auto &sample : pressureSamples) sample.initialDrawingAngle = .37;
                }
                if (incrementalPencil || tiltSize) {
                    pressureSamples[0].xTilt = input.tilt.x(); pressureSamples[0].yTilt = input.tilt.y();
                    pressureSamples[1].xTilt = -input.tilt.x(); pressureSamples[1].yTilt = input.tilt.y() / 2;
                    pressureSamples[2].xTilt = input.tilt.x() / 3; pressureSamples[2].yTilt = -input.tilt.y();
                    for (auto &sample : pressureSamples) {
                        sample.tiltDirectionOffset = input.tiltOffset; sample.canvasRotation = input.canvasRotation;
                        sample.canvasMirroredH = input.mirroredH; sample.canvasMirroredV = input.mirroredV;
                    }
                }
                for (auto &sample : pressureSamples) sample.speed = speed;
                if (fineliner) { pressureSamples.last().speed = 1 - speed; pressureSamples.last().timestamp = 1; }
                QCOMPARE(pressureOwner->appendSamples(operation, pressureSamples), Gpu::Result::Success);
                QCOMPARE(pressureOwner->commitStroke(operation), Gpu::Result::Success);
                const auto readback = pressureOwner->readRgbaPixels(pressureOwner->operations().activeLayer(), QRect(QPoint(), size), expected.size());
                QVERIFY2(readback.code == Gpu::ExecutionCode::Success, qPrintable(readback.diagnostic));
                auto actual = readback.bytes;
                for (qsizetype i = 0; i < actual.size(); i += pixelBytes)
                    for (int byte = 0; byte < channelBytes; ++byte) std::swap(actual[i + byte], actual[i + 2 * channelBytes + byte]);
                for (const auto &entry : {qMakePair(QStringLiteral("native"), expected), qMakePair(QStringLiteral("gpu"), actual)}) {
                    QFile artifact(QDir(output).filePath(probeName + QLatin1Char('-') + entry.first + QStringLiteral(".bgra")));
                    QVERIFY(artifact.open(QIODevice::WriteOnly));
                    QCOMPARE(artifact.write(entry.second), entry.second.size());
                }
                if (actual != expected) {
                    if (textured) {
                        auto originalBrush = pressureCapture.brush;
                        originalBrush.texture.reset(); originalBrush.opacity = 1;
                        const auto originalOwner = Gpu::PaintImageSP::create(KisPart::instance()->gpuExecution(diagnostic), raster);
                        const auto originalOperation = originalOwner->beginStroke(originalOwner->operations().activeLayer(), originalBrush, 17);
                        QVERIFY(!originalOperation.isNull());
                        QCOMPARE(originalOwner->appendSamples(originalOperation, {{{64.75, 64.75}, pressure}}), Gpu::Result::Success);
                        QCOMPARE(originalOwner->commitStroke(originalOperation), Gpu::Result::Success);
                        auto original = originalOwner->readRgbaPixels(originalOwner->operations().activeLayer(), QRect(QPoint(), size), expected.size()).bytes;
                        for (qsizetype i = 0; i < original.size(); i += pixelBytes)
                            for (int byte = 0; byte < channelBytes; ++byte) std::swap(original[i + byte], original[i + 2 * channelBytes + byte]);
                        QFile artifact(QDir(output).filePath(probeName + QStringLiteral("-original.bgra")));
                        QVERIFY(artifact.open(QIODevice::WriteOnly)); QCOMPARE(artifact.write(original), original.size());
                    }

                    QByteArray nativeTemporary(expected.size(), Qt::Uninitialized);
                    nativeLayer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(nativeTemporary.data()), QRect(QPoint(), size));
                    auto fullOpacityBrush = pressureCapture.brush;
                    fullOpacityBrush.opacity = 1;
                    const auto temporaryOwner = Gpu::PaintImageSP::create(KisPart::instance()->gpuExecution(diagnostic), raster);
                    const auto temporaryOperation = temporaryOwner->beginStroke(temporaryOwner->operations().activeLayer(), fullOpacityBrush, 17);
                    QVERIFY(!temporaryOperation.isNull());
                    QCOMPARE(temporaryOwner->appendSamples(temporaryOperation, pressureSamples), Gpu::Result::Success);
                    QCOMPARE(temporaryOwner->commitStroke(temporaryOperation), Gpu::Result::Success);
                    auto gpuTemporary = temporaryOwner->readRgbaPixels(temporaryOwner->operations().activeLayer(), QRect(QPoint(), size), expected.size()).bytes;
                    for (qsizetype i = 0; i < gpuTemporary.size(); i += pixelBytes)
                        for (int byte = 0; byte < channelBytes; ++byte) std::swap(gpuTemporary[i + byte], gpuTemporary[i + 2 * channelBytes + byte]);
                    for (const auto &entry : {qMakePair(QStringLiteral("native-temporary"), nativeTemporary),
                                              qMakePair(QStringLiteral("gpu-temporary"), gpuTemporary)}) {
                        QFile artifact(QDir(output).filePath(probeName + QLatin1Char('-') + entry.first + QStringLiteral(".bgra")));
                        QVERIFY(artifact.open(QIODevice::WriteOnly));
                        QCOMPARE(artifact.write(entry.second), entry.second.size());
                    }
                }
                QVERIFY2(actual == expected, qPrintable(QStringLiteral("Preset %1 diameter %2 pressure %3 speed %4 must keep native pixels").arg(presetName).arg(diameter).arg(pressure).arg(speed)));
            }
            resources->setSize(normalDiameter);
        }
        const auto stroke = [&](const QList<QPointF> &points) {
            const auto *converter = view->canvasBase()->coordinatesConverter();
            const auto first = converter->imageToWidget(points.first()).toPoint();
            const auto target = gpu ? first : view->canvasBase()->canvasWidget()->mapTo(window.get(), first);
            if (fineliner) pointerClock.advance();
            QTest::mouseMove(input, target);
            QEnterEvent entered(first, first, view->canvasBase()->canvasWidget()->mapToGlobal(first));
            QApplication::sendEvent(gpu ? static_cast<QObject *>(input.data()) : view->canvasBase()->canvasWidget(), &entered);
            pressed = true;
            if (fineliner) pointerClock.advance();
            QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, target);
            for (const auto &point : points.mid(1)) {
                const auto next = converter->imageToWidget(point).toPoint();
                if (fineliner) pointerClock.advance();
                QTest::mouseMove(input, gpu ? next : view->canvasBase()->canvasWidget()->mapTo(window.get(), next), 5);
            }
            const auto last = converter->imageToWidget(points.last()).toPoint();
            if (fineliner) pointerClock.advance();
            QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier,
                                gpu ? last : view->canvasBase()->canvasWidget()->mapTo(window.get(), last));
            pressed = false;
            lifetime->image()->waitForDone();
            QApplication::processEvents();
        };
        const auto pixels = [&] {
            auto image = KisImageSP(lifetime->image());
            image->waitForDone();
            if (const auto owner = image->gpuImage()) {
                const auto value = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(QPoint(), size), size.width() * size.height() * pixelBytes);
                QByteArray bgra = value.bytes;
                for (qsizetype i = 0; i < bgra.size(); i += pixelBytes)
                    for (int byte = 0; byte < channelBytes; ++byte) std::swap(bgra[i + byte], bgra[i + 2 * channelBytes + byte]);
                return bgra;
            }
            QByteArray result(size.width() * size.height() * pixelBytes, Qt::Uninitialized);
            window->viewManager()->nodeManager()->activeNode()->paintDevice()->readBytes(
                reinterpret_cast<quint8 *>(result.data()), QRect(QPoint(), size));
            return result;
        };
        int state = 0;
        const auto compareState = [&](const QString &name) {
            const auto actual = pixels();
            QFile artifact(QDir(output).filePath((gpu ? QStringLiteral("gpu-") : QStringLiteral("cpu-")) + name + QStringLiteral(".bgra")));
            if (!artifact.open(QIODevice::WriteOnly) || artifact.write(actual) != actual.size()) return false;
            if (pencil && !incrementalPencil) return true; // Seeded random comparisons above; ordinary history below.
            if (!gpu) expectedStates.append(actual);
            return !gpu || actual == expectedStates.value(state++);
        };
        stroke({{32, 32}, {80, 48}, {80, 80}});
        QTRY_COMPARE(lifetime->undoStack()->count(), 1);
        const auto painted = pixels();
        QVERIFY(painted != QByteArray(painted.size(), '\0'));
        QVERIFY2(compareState(QStringLiteral("painted")), "Ordinary preset painting must match the native engine");
        resources->setGlobalAlphaLock(true);
        resources->setFGColor(KoColor(QColor(241, 17, 129), cs));
        if (normalDiameter <= 1) stroke({{80, 56}, {80, 72}});
        else stroke({{48, 40}, {64, 48}});
        const auto protectedPixels = pixels();
        QVERIFY(protectedPixels != painted);
        for (qsizetype i = 3 * channelBytes; i < painted.size(); i += pixelBytes)
            QCOMPARE(protectedPixels.mid(i, channelBytes), painted.mid(i, channelBytes));
        QVERIFY2(compareState(QStringLiteral("alpha-locked")), "Alpha protection must preserve the native preset's color result");
        lifetime->undoStack()->undo();
        QCOMPARE(pixels(), painted);
        resources->setGlobalAlphaLock(false);
        resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
        resources->setEraserMode(true);
        // One-pixel tips erase the existing vertical part of the stroke.
        if (normalDiameter <= 1) stroke({{80, 56}, {80, 72}});
        else stroke({{48, 40}, {64, 56}});
        QTRY_COMPARE(lifetime->undoStack()->count(), 2);
        const auto erased = pixels();
        QVERIFY(erased != painted);
        QVERIFY2(compareState(QStringLiteral("erased")), "Ordinary preset erasing must match the native engine");
        lifetime->undoStack()->undo();
        QCOMPARE(pixels(), painted);
        lifetime->undoStack()->redo();
        QCOMPARE(pixels(), erased);
        window->viewManager()->nodeManager()->duplicateActiveNode();
        lifetime->image()->waitForDone();
        QTRY_COMPARE(lifetime->image()->root()->childCount(), 2U);
        QCOMPARE(pixels(), erased);
        lifetime->undoStack()->undo();
        QTRY_COMPARE(lifetime->image()->root()->childCount(), 1U);
        lifetime->undoStack()->redo();
        QTRY_COMPARE(lifetime->image()->root()->childCount(), 2U);
        const auto path = QDir(output).filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("cpu.kra"));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        QVERIFY2(lifetime->openPath(path, KisDocument::DontAddToRecent), qPrintable(lifetime->errorMessage()));
        QCOMPARE(bool(lifetime->image()->gpuImage()), gpu);
        window->viewManager()->nodeManager()->slotUiActivatedNode(lifetime->image()->root()->firstChild());
        QCOMPARE(pixels(), erased);
        prepareInput();
        resources->setPaintOpPreset(preset);
        resources->setSize(normalDiameter);
        resources->setOpacity(.7);
        resources->setGlobalAlphaLock(false);
        resources->setMirrorHorizontal(false);
        resources->setMirrorVertical(false);
        resources->setEraserMode(false);
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        if (normalDiameter <= 1) stroke({{104, 108}, {112, 108}});
        else stroke({{108, 108}});
        QVERIFY(pixels() != erased);
        QVERIFY2(compareState(QStringLiteral("reopened-edited")), "A reopened document must retain the preset's editing result");
        resources->setMirrorHorizontal(true);
        stroke({{92, 24}, {108, 40}});
        QVERIFY2(compareState(QStringLiteral("mirrored")), "Canvas symmetry must reflect the native preset mask");
        stroke({{52, 24}, {76, 40}});
        QVERIFY2(compareState(QStringLiteral("mirror-overlap")), "Overlapping mirrored dabs must preserve the native editing result");
        QVERIFY(window->screen()->grabWindow(window->winId()).save(QDir(output).filePath(gpu ? QStringLiteral("gpu-window.png") : QStringLiteral("cpu-window.png"))));
        for (const auto &error : errors) QVERIFY2(error.first().toString().isEmpty(), qPrintable(error.first().toString()));
    }
}


void KisGpuDocumentViewTest::selectionFillToolKeepsNativeEditing_data()
{
    RegionFillEditingTest::addData();
}

void KisGpuDocumentViewTest::selectionFillToolKeepsNativeEditing()
{
    RegionFillEditingTest::run(m_preset);
}

void KisGpuDocumentViewTest::gradientsKeepNativeEditingHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("shape");
    QTest::addColumn<int>("repeat");
    for (const bool words : {false, true}) for (int shape = 0; shape < 8; ++shape) for (int repeat = 0; repeat < 3; ++repeat) {
        const auto name = QStringLiteral("%1-shape-%2-repeat-%3").arg(words ? "u16" : "u8").arg(shape).arg(repeat).toLatin1();
        QTest::newRow(name.constData()) << words << shape << repeat;
    }
}

void KisGpuDocumentViewTest::gradientsKeepNativeEditingHistoryAndKra()
{
    // Consumer: the ordinary gradient tool. Native and GPU documents must keep
    // identical selected paint/mask pixels, projection and history through
    // resource changes, KRA reopening and another gradient operation.
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words); QFETCH(int, shape); QFETCH(int, repeat);
    const bool maskTarget = (shape + repeat) % 2;
    const bool groupOwner = maskTarget && shape == 3;
    const auto oldEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (oldEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", oldEngine);
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect area(0, 0, 32, 24);
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("native-gradient.kra"));
    QUuid paintUuid, targetUuid, maskUuid;
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), area.width(), area.height(), cs, "Gradient editing");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        layer->paintDevice()->fill(area, KoColor(QColor(77, 113, 157, 176), cs));
        KisNodeSP target = layer;
        if (groupOwner) {
            KisGroupLayerSP group = new KisGroupLayer(image, QStringLiteral("Masked group"), 255);
            QVERIFY(image->addNode(group)); QVERIFY(image->addNode(layer, group)); target = group;
        } else QVERIFY(image->addNode(layer));
        paintUuid = layer->uuid(); targetUuid = target->uuid();
        if (maskTarget) {
            KisSelectionSP coverage = new KisSelection;
            coverage->pixelSelection()->select(area, 177);
            KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
            mask->setSelection(coverage); QVERIFY(image->addNode(mask, target)); maskUuid = mask->uuid();
        }
        KisSelectionSP selected = new KisSelection;
        selected->pixelSelection()->select({3, 4, 26, 17}, 211);
        selected->pixelSelection()->select({12, 10, 5, 5}, 0);
        KisSelectionMaskSP selection = new KisSelectionMask(image);
        selection->setSelection(selected); selection->setActive(true); QVERIFY(image->addNode(selection, image->root()));
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
        QVERIFY(document->saveAs(baseline, KisDocument::nativeFormatMimeType(), false)); document->waitForSavingToComplete();
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            QVERIFY(QDir().mkpath(results));
            QVERIFY(QFile::copy(baseline, QDir(results).filePath(QStringLiteral("gradient-%1-source.kra").arg(QString::fromLatin1(QTest::currentDataTag())))));
        }
    }
    QList<QByteArray> reference;
    for (const bool gpu : {false, true}) {
        // Compare KRA tabs on the application's shared execution, including
        // the reopened document's attachment and retained-resource lifetime.
        if (gpu && !m_gradientExecution) {
            QString diagnostic;
            m_gradientExecution = KisPart::instance()->gpuExecution(diagnostic);
            QVERIFY2(m_gradientExecution, qPrintable(diagnostic));
        }
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        const auto saved = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("native.kra"));
        int stage = 0;
        for (const bool reopened : {false, true}) {
            std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
            document->setFileBatchMode(true); document->setAutoSaveActive(false);
            QVERIFY2(document->openPath(reopened ? saved : baseline, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
            auto image = KisImageSP(document->image()); image->waitForDone();
            QCOMPARE(bool(image->gpuImage()), gpu);
            auto layer = KisLayerUtils::findNodeByUuid(image->root(), paintUuid); QVERIFY(layer);
            auto target = KisLayerUtils::findNodeByUuid(image->root(), targetUuid); QVERIFY(target);
            auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid);
            if (maskTarget) { QVERIFY(mask); QCOMPARE(mask->parent(), target); }
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
            window->viewManager()->setCurrentView(view); window->resize(800, 600); window->show();
            QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
            view->canvasBase()->snapGuide()->enableSnapping(false);
            window->viewManager()->nodeManager()->slotNonUiActivatedNode(maskTarget ? mask : layer);
            QTRY_COMPARE(view->currentNode(), maskTarget ? mask : layer);
            const auto capture = [&]() -> QByteArray {
                image->waitForDone();
                QByteArray pixels(area.width() * area.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0');
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), area);
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), area);
                QByteArray coverage;
                if (maskTarget) {
                    coverage.resize(area.width() * area.height());
                    qobject_cast<KisTransparencyMask *>(mask.data())->selection()->pixelSelection()->readBytes(
                        reinterpret_cast<quint8 *>(coverage.data()), area);
                }
                return pixels + projection + coverage;
            };
            const auto check = [&] {
                const auto state = capture();
                const int currentStage = stage++;
                const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                if (!results.isEmpty()) {
                    QFile output(QDir(results).filePath(QStringLiteral("gradient-%1-%2-%3.bin")
                        .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? "gpu" : "native").arg(currentStage)));
                    QVERIFY(output.open(QIODevice::WriteOnly)); QCOMPARE(output.write(state), state.size());
                }
                if (!gpu) reference.append(state); else QCOMPARE(state, reference.value(currentStage));
            };
            check();
            auto *resources = window->viewManager()->canvasResourceProvider();
            resources->setPaintOpPreset(m_preset);
            resources->setFGColor(KoColor(reopened ? QColor(213, 49, 107) : QColor(17, 129, 241), cs));
            resources->setBGColor(KoColor(QColor(221, 17, 44), cs));
            const auto gradient = QSharedPointer<KoStopGradient>::create();
            gradient->setName(QStringLiteral("Frozen ordinary gradient"));
            gradient->setStops({{0, KoColor(Qt::black, cs), FOREGROUNDSTOP},
                {.43, KoColor(QColor(39, 213, 71, 97), cs)}, {1, KoColor(QColor(221, 17, 44, 0), cs)}});
            gradient->setValid(true); resources->slotGradientActivated(gradient);
            resources->setOpacity(.63); resources->setEraserMode(reopened && repeat == 2);
            resources->setCurrentCompositeOp(repeat == 2 ? QStringLiteral("copy") : QStringLiteral("normal"));
            if (reopened && repeat == 2) resources->setCurrentCompositeOp(QStringLiteral("erase"));
            resources->setMirrorHorizontal(false); resources->setMirrorVertical(false); resources->setGlobalAlphaLock(false);
            if (!maskTarget && shape == 2 && repeat == 2) {
                QBitArray channels(4, false); channels.setBit(0); channels.setBit(2);
                qobject_cast<KisPaintLayer *>(layer.data())->setChannelLockFlags(channels);
            }
            const auto toolId = QStringLiteral("KritaFill/KisToolGradient");
            KoToolManager::instance()->switchToolRequested(toolId);
            auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId)); QVERIFY(tool);
            // Initialize controls before assigning the retained option values.
            tool->optionWidgets();
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetShape", Qt::DirectConnection, Q_ARG(int, shape)));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetRepeat", Qt::DirectConnection, Q_ARG(int, repeat)));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetReverse", Qt::DirectConnection, Q_ARG(bool, repeat == 2 || (reopened && shape >= 4))));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetDither", Qt::DirectConnection, Q_ARG(bool, repeat == 1)));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetAntiAliasThreshold", Qt::DirectConnection, Q_ARG(qreal, .2)));
            const auto before = capture(); const int history = lifetime->undoStack()->index();
            const QPointF start = reopened ? QPointF(20.75, 15.25) : QPointF(9.25, 9.75);
            const QPointF end = reopened ? QPointF(15.5, 8.125) : QPointF(16.875, 13.25);
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            KoPointerEvent first(&press, view->canvasBase()->coordinatesConverter()->imageToDocument(start)); tool->beginPrimaryAction(&first);
            QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            KoPointerEvent update(&move, view->canvasBase()->coordinatesConverter()->imageToDocument(end)); tool->continuePrimaryAction(&update);
            QKeyEvent cancel(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            view->canvasBase()->toolProxy()->keyPressEvent(&cancel); image->waitForDone();
            QVERIFY(cancel.isAccepted());
            QMouseEvent cancelledRelease(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            KoPointerEvent cancelledLast(&cancelledRelease, view->canvasBase()->coordinatesConverter()->imageToDocument(end));
            tool->endPrimaryAction(&cancelledLast);
            QCOMPARE(lifetime->undoStack()->index(), history); QCOMPARE(capture(), before);
            tool->beginPrimaryAction(&first); tool->continuePrimaryAction(&update);
            QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            KoPointerEvent last(&release, view->canvasBase()->coordinatesConverter()->imageToDocument(end)); tool->endPrimaryAction(&last);
            image->waitForDone(); QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
            const auto edited = capture(); QVERIFY(edited != before); check();
            resources->setFGColor(KoColor(Qt::yellow, cs));
            gradient->setStops({{0, KoColor(Qt::red, cs)}, {1, KoColor(Qt::blue, cs)}});
            lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(capture(), before); check();
            lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(capture(), edited); check();
            QVERIFY(lifetime->saveAs(saved, KisDocument::nativeFormatMimeType(), false)); lifetime->waitForSavingToComplete();
        }
        if (gpu) {
            qputenv("LIBREPAINT_GPU_ENGINE", "0");
            QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
            native->setFileBatchMode(true); native->setAutoSaveActive(false);
            QVERIFY2(native->openPath(saved, KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
            auto image = KisImageSP(native->image()); image->waitForDone();
            auto layer = KisLayerUtils::findNodeByUuid(image->root(), paintUuid); QVERIFY(layer);
            QByteArray pixels(area.width() * area.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
            layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), area);
            image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), area);
            if (maskTarget) {
                auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid); QVERIFY(mask);
                coverage.resize(area.width() * area.height());
                qobject_cast<KisTransparencyMask *>(mask.data())->selection()->pixelSelection()->readBytes(
                    reinterpret_cast<quint8 *>(coverage.data()), area);
            }
            QCOMPARE(pixels + projection + coverage, reference.last());
        }
    }
}

void KisGpuDocumentViewTest::multibrushPresetsKeepNativeEditingHistoryAndKra_data()
{
    MultibrushEditingTest::addData();
}

void KisGpuDocumentViewTest::multibrushPresetsKeepNativeEditingHistoryAndKra()
{
    MultibrushEditingTest::run(m_freehandExecution);
}

void KisGpuDocumentViewTest::dynamicBrushPresetsKeepNativeEditingHistoryAndKra_data()
{
    DynamicBrushEditingTest::addData();
}

void KisGpuDocumentViewTest::dynamicBrushPresetsKeepNativeEditingHistoryAndKra()
{
    DynamicBrushEditingTest::run(m_freehandExecution);
}

KISTEST_MAIN(KisGpuDocumentViewTest)

void KisGpuDocumentViewTest::colorFilterActionsKeepNativeEditing_data()
{ ColorFilterEditingTest::addData(); }
void KisGpuDocumentViewTest::colorFilterActionsKeepNativeEditing()
{ ColorFilterEditingTest::run(); }

void KisGpuDocumentViewTest::convolutionFiltersKeepNativeEditing_data()
{ ColorFilterEditingTest::addConvolutionData(); }
void KisGpuDocumentViewTest::convolutionFiltersKeepNativeEditing()
{ ColorFilterEditingTest::run(); }

void KisGpuDocumentViewTest::filterNodesKeepNativeEditing_data()
{ FilterNodeEditingTest::addData(); }
void KisGpuDocumentViewTest::filterNodesKeepNativeEditing()
{ FilterNodeEditingTest::run(); }

void KisGpuDocumentViewTest::convolutionFilterNodesKeepNativeEditing_data()
{ FilterNodeEditingTest::addConvolutionData(); }
void KisGpuDocumentViewTest::convolutionFilterNodesKeepNativeEditing()
{ FilterNodeEditingTest::run(); }

void KisGpuDocumentViewTest::convolutionChainsKeepNativeProjection_data()
{ FilterNodeEditingTest::addConvolutionChainData(); }
void KisGpuDocumentViewTest::convolutionChainsKeepNativeProjection()
{ FilterNodeEditingTest::runConvolutionChain(); }
