/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDocumentViewTest.h"

void KisGpuDocumentViewTest::colourSelectionToolsKeepPixelsHistoryAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(QString, toolId);
    QFETCH(bool, words);
    QFETCH(int, action);
    QFETCH(QString, scenario);
    const bool contiguous = toolId == QLatin1String("KisToolSelectContiguous");
    const auto selectionAction = SelectionAction(action);
    const bool aa = scenario != QLatin1String("hard");
    const int threshold = aa ? 32 : 1, spread = aa ? 45 : 100;
    const int grow = scenario == QLatin1String("shrink") ? -2 : aa ? 3 : 0;
    KConfigGroup configuration = KSharedConfig::openConfig()->group(toolId);
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("threshold", threshold); configuration.writeEntry("opacitySpread", spread);
    configuration.writeEntry("closeGapAmount", 0); configuration.writeEntry("useSelectionAsBoundary", false);
    configuration.writeEntry("contiguousSelectionMode", "floodFill");
    configuration.writeEntry("stopGrowingAtDarkestPixel", false); configuration.writeEntry("featherSelection", 0);
    const QRect region(0, 0, 256, 256);
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int stride = Gpu::pixelByteCount(format);
    const QPoint seed(64, 64);
    KisImageSP nativeImage = new KisImage(new KisSurrogateUndoStore(), 256, 256, cs, QStringLiteral("Native colour selection"));
    KisPaintDeviceSP source = new KisPaintDevice(cs);
    const auto maximum = Gpu::maximumComponent(format);
    QByteArray sourceBytes(region.width() * region.height() * stride, '\0');
    for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x) {
        const bool inside = ((x - 72) * (x - 72) + (y - 76) * (y - 76) < 38 * 38) ||
                            QRect(160, 40, 58, 49).contains(x, y);
        const auto pixel = Gpu::rgbaBytes(inside ? Gpu::RgbaPixel{quint16(maximum * (scenario == QLatin1String("spread-gradient") ? 96 + x % 129 : 140 + x % 21) / 255u),
            quint16(maximum * (scenario == QLatin1String("spread-gradient") ? 30 + y % 97 : 43 + y % 17) / 255u), quint16(maximum * 29u / 255u), maximum} : Gpu::RgbaPixel{0, 0, 0, maximum}, format);
        sourceBytes.replace((y * region.width() + x) * stride, stride, pixel);
    }
    auto bgra = sourceBytes; exchangeRgbaBgra(bgra, stride);
    source->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), region);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = region.size(); raster.format = format;
    raster.layers = {{QStringLiteral("Colour source"), true, QUuid::createUuid(), {}},
                     {QStringLiteral("Selected drawing"), true, QUuid::createUuid(), {}}};
    raster.layers[0].colorLabelIndex = 1;
    const auto owner = paintImageWithEncoding(execution, raster);
    QCOMPARE(owner->writeRgbaPixels(1, region, sourceBytes), Gpu::Result::Success);
    if (scenario == QLatin1String("masked")) {
        const QRect maskRect(32, 32, 57, 72);
        QCOMPARE(owner->setSelectionRect(maskRect), Gpu::Result::Success);
        QCOMPARE(owner->addTransparencyMaskFromSelection(1), Gpu::Result::Success);
        KisSelectionSP mask = new KisSelection(); mask->pixelSelection()->select(maskRect);
        KisPaintDeviceSP projected = new KisPaintDevice(cs);
        KisPainter(projected, mask).bitBlt(region.topLeft(), source, region);
        source = projected;
    }
    QByteArray initial(region.width() * region.height(), '\0');
    const bool inverted = selectionAction == SELECTION_INTERSECT;
    if (selectionAction == SELECTION_REPLACE) {
        for (int y = 8; y < 16; ++y) for (int x = 8; x < 16; ++x) initial[y * 256 + x] = char(255);
    } else {
        for (int y = 24; y < 160; ++y) for (int x = 24; x < 192; ++x)
            initial[y * 256 + x] = char(32 + ((x + 3 * y) % 7) * 31);
    }
    const auto previousMask = Gpu::SelectionMask::fromAlphaCoverage(region, initial, region);
    QVERIFY(previousMask);
    QCOMPARE(owner->setSelectionMask(previousMask, inverted), Gpu::Result::Success);
    if (inverted) for (char &value : initial) value = char(255 - quint8(value));
    const auto before = owner->operations().snapshot();
    const int history = owner->operations().undoCount();
    KisSelectionSP expectedSelection = new KisSelection();
    expectedSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), region);
    KisPixelSelectionSP added = new KisPixelSelection(new KisDefaultBounds(nativeImage));
    KisFillPainter classifier;
    classifier.setWidth(256); classifier.setHeight(256);
    classifier.setFillThreshold(threshold); classifier.setOpacitySpread(spread);
    if (contiguous) {
        classifier.setAntiAlias(aa); classifier.setSizemod(grow); classifier.setUseCompositing(true);
        classifier.createFloodSelection(added, seed.x(), seed.y(), source, KisPixelSelectionSP());
    } else {
        classifier.createSimilarColorsSelection(added, source->pixel(seed), source, region, KisPixelSelectionSP());
        if (grow > 0) {
            KisGrowSelectionFilter filter(grow, grow);
            filter.process(added, added->selectedRect().adjusted(-grow, -grow, grow, grow));
        } else if (grow < 0) {
            KisShrinkSelectionFilter filter(-grow, -grow, false); filter.process(added, added->selectedRect());
        }
        if (aa) { KisAntiAliasSelectionFilter filter; filter.process(added, added->selectedRect()); }
    }
    expectedSelection->pixelSelection()->applySelection(added, selectionAction);
    auto checkedCoverage = region.united(expectedSelection->pixelSelection()->selectedExactRect());
    QByteArray expectedMask(checkedCoverage.width() * checkedCoverage.height(), '\0');
    expectedSelection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(expectedMask.data()), checkedCoverage);
    const bool empty = expectedSelection->pixelSelection()->selectedExactRect().isEmpty();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Colour selection workflow"), cs));
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
    window->resize(800, 600); window->show(); window->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
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
    KoToolManager::instance()->switchToolRequested(toolId);
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == toolId + QLatin1String("option widget"))
            if (auto *candidate = qobject_cast<KisSelectionOptions *>(widget)) options = candidate;
    QVERIFY(options);
    options->setMode(PIXEL_SELECTION); options->setAction(selectionAction);
    options->setAntiAliasSelection(aa); options->setGrowSelection(grow); options->setFeatherSelection(0);
    options->setStopGrowingAtDarkestPixel(false);
    options->setReferenceLayers(scenario == QLatin1String("all") ? KisSelectionOptions::AllLayers :
        scenario == QLatin1String("label") ? KisSelectionOptions::ColorLabeledLayers : KisSelectionOptions::CurrentLayer);
    options->setSelectedColorLabels({1});
    auto *thresholdSlider = options->findWidgetAs<KisSliderSpinBox *>(QStringLiteral("sectionSelectionExtent/sliderThreshold"));
    auto *spreadSlider = options->findWidgetAs<KisSliderSpinBox *>(QStringLiteral("sectionSelectionExtent/sliderSpread"));
    QVERIFY(thresholdSlider && spreadSlider);
    thresholdSlider->setValue(threshold); spreadSlider->setValue(spread);
    native->requestActivate(); view->canvasBase()->canvasWidget()->setFocus();
    const auto point = converter->imageToWidget(seed).toPoint();
    QTest::mouseMove(native.data(), point);
    QEnterEvent entered(point, point, native->mapToGlobal(point)); QApplication::sendEvent(native.data(), &entered);
    QTest::mouseClick(native.data(), Qt::LeftButton, Qt::NoModifier, point);
    int selectionHistory = history + 1;
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
    auto image = KisImageSP(lifetime->image());
    QString mismatch;
    const auto checkSelection = [&](const KisImageSP &candidate) {
        if (empty) return !candidate->globalSelection();
        const auto selection = candidate->globalSelection();
        if (!selection) return false;
        QByteArray values(expectedMask.size(), '\0');
        selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(values.data()), checkedCoverage);
        if (values != expectedMask) {
            qsizetype index = 0; while (index < values.size() && values[index] == expectedMask[index]) ++index;
            mismatch = QStringLiteral("colour selectedness %1,%2: actual%3 native%4")
                .arg(checkedCoverage.x() + index % checkedCoverage.width()).arg(checkedCoverage.y() + index / checkedCoverage.width())
                .arg(quint8(values[index])).arg(quint8(expectedMask[index]));
        }
        return values == expectedMask;
    };
    QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
    QVERIFY(empty || owner->selectionMask()->isGpuGenerated());
    if (!empty) {
        auto *pixels = image->globalSelection()->pixelSelection().data();
        QCOMPARE(pixels->colorSpace(), KoColorSpaceRegistry::instance()->alpha8());
        QCOMPARE(pixels->selectedExactRect(), expectedSelection->pixelSelection()->selectedExactRect());
        KisPixelSelectionSP retained = new KisPixelSelection(*pixels);
        QByteArray copied(expectedMask.size(), '\0'); retained->readBytes(reinterpret_cast<quint8 *>(copied.data()), checkedCoverage);
        QCOMPARE(copied, expectedMask);
        pixels->recalculateThumbnailImage(QColor(12, 94, 177, 128));
        QVERIFY(pixels->thumbnailImageValid());
        expectedSelection->pixelSelection()->recalculateThumbnailImage(QColor(12, 94, 177, 128));
        QCOMPARE(pixels->thumbnailImage(), expectedSelection->pixelSelection()->thumbnailImage());
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success); QCOMPARE(owner->operations().snapshot()->revision(), before->revision());
    QCOMPARE(owner->redo(), Gpu::Result::Success); QTRY_VERIFY(checkSelection(image));
    if (scenario.startsWith(QLatin1String("move"))) {
        const bool inverseMove = scenario == QLatin1String("move-inverted");
        const auto adoptExpected = [&](const QByteArray &values) {
            expectedSelection = new KisSelection();
            expectedSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), region);
            checkedCoverage = region;
            expectedMask = values;
        };
        if (inverseMove) {
            QVERIFY(image->invertGpuSelectionMask());
            ++selectionHistory;
            auto values = expectedMask;
            for (char &value : values) value = char(255 - quint8(value));
            adoptExpected(values);
            QTRY_VERIFY(checkSelection(image));
        }
        const auto expectedMoved = [&](QPoint offset) {
            QByteArray values(region.width() * region.height(), inverseMove ? char(255) : char(0));
            for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x) {
                const QPoint source(x - offset.x(), y - offset.y());
                if (checkedCoverage.contains(source))
                    values[y * region.width() + x] = expectedMask[(source.y() - checkedCoverage.y()) * checkedCoverage.width() + source.x() - checkedCoverage.x()];
            }
            return values;
        };
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
        QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
        native->requestActivate(); view->canvasBase()->canvasWidget()->setFocus();
        const QPoint start = inverseMove ? QPoint(20, 20) : seed;
        const QPoint offset = scenario == QLatin1String("move-clipped") ? QPoint(-50, -39) : QPoint(13, 9);
        const auto sourcePoint = converter->imageToWidget(start).toPoint();
        const auto targetPoint = converter->imageToWidget(start + offset).toPoint();
        const auto moveStart = owner->operations().snapshot();
        const auto originalExpected = expectedMask;
        const auto movedExpected = expectedMoved(offset);
        QTest::mouseMove(native.data(), sourcePoint);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, sourcePoint);
        QTest::mouseMove(native.data(), targetPoint, 30);
        adoptExpected(movedExpected);
        QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QCOMPARE(owner->operations().undoCount(), selectionHistory);
        QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Escape);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, targetPoint);
        adoptExpected(originalExpected);
        QTRY_VERIFY(checkSelection(image));
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QTest::mouseMove(native.data(), sourcePoint);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, sourcePoint);
        QTest::mouseMove(native.data(), targetPoint, 30);
        adoptExpected(movedExpected);
        QTRY_VERIFY(checkSelection(image));
        auto *selectionTool = dynamic_cast<KisTool *>(KoToolManager::instance()->toolById(
            view->canvasBase(), QStringLiteral("KisToolSelectRectangular")));
        QVERIFY(selectionTool);
        selectionTool->deactivatePrimaryAction();
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, targetPoint);
        adoptExpected(originalExpected);
        QTRY_VERIFY(checkSelection(image));
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QCOMPARE(owner->operations().undoCount(), selectionHistory);
        selectionTool->activatePrimaryAction();
        QTest::mouseMove(native.data(), sourcePoint);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, sourcePoint);
        QTest::mouseMove(native.data(), targetPoint, 30);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, targetPoint);
        ++selectionHistory;
        QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
        adoptExpected(movedExpected);
        QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QTRY_VERIFY(checkSelection(image));
        for (const auto &step : {QPair<QPoint, Qt::KeyboardModifiers>{{1, 0}, Qt::NoModifier},
                                QPair<QPoint, Qt::KeyboardModifiers>{{0, 10}, Qt::ShiftModifier}}) {
            const auto nudgedExpected = expectedMoved(step.first);
            QTest::keyClick(view->canvasBase()->canvasWidget(), step.first.x() ? Qt::Key_Right : Qt::Key_Down, step.second);
            ++selectionHistory;
            QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
            adoptExpected(nudgedExpected);
            QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
        }
        QCOMPARE(owner->selectionInverted(), inverseMove);
    }
    QCOMPARE(owner->selectLayer(2), Gpu::Result::Success);
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == raster.layers[1].uuid);
    const auto combined = owner->operations().snapshot();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setOpacity(1); resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory + 1);
    KisPaintDeviceSP expectedPixels = new KisPaintDevice(cs);
    const auto nativeFill = [&](const KoColor &color) {
        KisPaintDeviceSP source = new KisPaintDevice(cs);
        KisFillPainter(source).fillRect(region, color);
        KisPainter painter(expectedPixels, empty ? KisSelectionSP() : expectedSelection);
        painter.bitBlt(region.topLeft(), source, region);
    };
    const auto nativeBytes = [&] {
        QByteArray bytes(region.width() * region.height() * stride, '\0');
        expectedPixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    nativeFill(resources->fgColor());
    const auto actual = owner->readRgbaPixels(owner->operations().activeLayer(), region, region.width() * region.height() * stride);
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    QCOMPARE(actual.bytes, nativeBytes());
    QCOMPARE(combined->selectionMask, owner->operations().snapshot()->selectionMask);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), combined);
    QCOMPARE(owner->redo(), Gpu::Result::Success);

    QTemporaryDir directory;
    QVERIFY(directory.isValid()); lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("combined-selection.kra"));
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    auto reopenedImage = KisImage::fromGpuImage(reopened, QStringLiteral("Reopened combined selection"), cs);
    const bool automaticDelivery = KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled();
    const auto restoreDelivery = qScopeGuard([automaticDelivery] {
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(automaticDelivery);
    });
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
    lifetime->setCurrentImage(reopenedImage);
    QTRY_COMPARE(view->image().data(), reopenedImage.data());
    QTRY_COMPARE(window->viewManager()->image().data(), reopenedImage.data());
    QTRY_VERIFY(view->currentNode() && view->currentNode()->image().data() == reopenedImage.data());
    QTRY_VERIFY(checkSelection(reopenedImage));
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    resources->setFGColor(KoColor(QColor(221, 53, 19), cs));
    QTRY_VERIFY(fill->isEnabled());
    fill->trigger();
    QTRY_COMPARE(reopened->operations().undoCount(), 1);
    nativeFill(resources->fgColor());
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    QCOMPARE(reopened->redo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    const auto editedPath = directory.filePath(QStringLiteral("reedited-selection.kra"));
    QVERIFY2(lifetime->saveAs(editedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QFile editedFile(editedPath); QVERIFY(editedFile.open(QIODevice::ReadOnly));
    const auto edited = LibrePaint::ImportExport::Kra::RasterCodec::read(editedFile);
    QVERIFY2(edited.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(edited.diagnostic));
    const auto editedOwner = paintImageWithEncoding(execution, edited.document.image);
    QCOMPARE(editedOwner->readRgbaPixels(editedOwner->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
}


void KisGpuDocumentViewTest::shapeSelectionCombinationsKeepEditingHistoryAndKra_data()
{
    QTest::addColumn<QString>("toolId");
    QTest::addColumn<int>("action");
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("initialState");
    QTest::addColumn<QString>("shapeOptions");
    QTest::newRow("rect-add") << QStringLiteral("KisToolSelectRectangular") << int(SELECTION_ADD) << false << QStringLiteral("soft") << QString();
    // #89 A0c: retain the selection-combination path that previously crashed.
    QTest::newRow("ellipse-intersect-feather") << QStringLiteral("KisToolSelectElliptical") << int(SELECTION_INTERSECT) << true << QStringLiteral("soft") << QStringLiteral("feather");
}

void KisGpuDocumentViewTest::shapeSelectionCombinationsKeepEditingHistoryAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(QString, toolId);
    QFETCH(int, action);
    QFETCH(bool, words);
    QFETCH(QString, initialState);
    QFETCH(QString, shapeOptions);
    KConfigGroup configuration = KSharedConfig::openConfig()->group(toolId);
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("roundCornersX", 0);
    configuration.writeEntry("roundCornersY", 0);
    const auto selectionAction = SelectionAction(action);
    const QRect region(0, 0, 256, 256);
    const QRect shape(64, 64, 128, 128);
    const QList<QPointF> vertices = shapeOptions.isEmpty() ? QList<QPointF>{{64, 64}, {192, 64}, {192, 192}, {64, 192}}
        : QList<QPointF>{{64, 64}, {192, 71}, {175, 192}, {69, 178}};
    const bool aa = !shapeOptions.isEmpty() && shapeOptions != QLatin1String("feather-no-aa");
    const int grow = shapeOptions.startsWith(QLatin1String("grow")) ? 7 :
        shapeOptions == QLatin1String("shrink-empty") ? -100 : shapeOptions.startsWith(QLatin1String("shrink")) ? -5 : 0;
    const int feather = shapeOptions.contains(QLatin1String("feather")) ? 5 : 0;
    const bool rounded = shapeOptions.startsWith(QLatin1String("rounded"));
    const bool rotated = shapeOptions.startsWith(QLatin1String("rotate"));
    const auto incoming = toolId == QLatin1String("KisToolSelectElliptical")
        ? Gpu::SelectionMask::fromEllipse(shape, region) : Gpu::SelectionMask::fromRectangle(shape, region);
    QVERIFY(incoming);
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const auto stride = Gpu::pixelByteCount(format);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = region.size(); raster.format = format;
    raster.layers.append({QStringLiteral("Selection edit"), true, QUuid::createUuid(), {}});
    const auto owner = paintImageWithEncoding(execution, raster);
    QByteArray initial(region.width() * region.height(), '\0');
    if (initialState != QLatin1String("none")) {
        for (int y = 32; y < 176; ++y) for (int x = 32; x < 160; ++x)
            initial[y * region.width() + x] = char(32 + ((x + 3 * y) % 8) * 31);
        if (initialState == QLatin1String("disjoint")) {
            initial.fill('\0');
            for (int y = 8; y < 24; ++y) for (int x = 8; x < 24; ++x) initial[y * region.width() + x] = char(255);
        }
        const auto mask = Gpu::SelectionMask::fromAlphaCoverage(region, initial, region);
        QVERIFY(mask);
        QCOMPARE(owner->setSelectionMask(mask, initialState == QLatin1String("inverted")), Gpu::Result::Success);
        if (initialState == QLatin1String("inverted"))
            for (char &value : initial) value = char(255 - quint8(value));
    }
    const auto before = owner->operations().snapshot();
    const int history = owner->operations().undoCount();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    KisSelectionSP expectedSelection = new KisSelection();
    expectedSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), region);
    if (initialState == QLatin1String("none") && selectionAction == SELECTION_SUBTRACT)
        expectedSelection->pixelSelection()->select(region);
    KisImageSP nativeImage = new KisImage(new KisSurrogateUndoStore(), region.width(), region.height(), cs,
                                        QStringLiteral("Native geometric selection"));
    KisPixelSelectionSP added = new KisPixelSelection(new KisDefaultBounds(nativeImage));
    if (shapeOptions.isEmpty()) {
        QByteArray addedBytes(initial.size(), '\0');
        for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x)
            addedBytes[y * region.width() + x] = char(incoming->coverage(x, y));
        added->writeBytes(reinterpret_cast<const quint8 *>(addedBytes.constData()), region);
    } else {
        QRectF inputRect(shape);
        if (rotated) {
            // Ctrl+Alt rotates the fixed-size rectangle about its drag start.
            inputRect.moveCenter(QPointF(64 + std::sqrt(2.) * 64, 64));
        }
        QPainterPath path;
        if (rounded) path.addRoundedRect(inputRect.toRect(), 18, 18);
        else if (toolId == QLatin1String("KisToolSelectRectangular")) path.addRect(inputRect.toRect());
        else if (toolId == QLatin1String("KisToolSelectElliptical")) path.addEllipse(inputRect);
        else { path.addPolygon(QPolygonF(vertices)); path.closeSubpath(); }
        if (rotated) {
            const QPointF center = toolId == QLatin1String("KisToolSelectRectangular") ? QPointF(inputRect.toRect().center()) : inputRect.center();
            QTransform transform;
            transform.translate(center.x(), center.y());
            transform.rotateRadians(-std::atan2(128., 128.));
            transform.translate(-center.x(), -center.y());
            path = transform.map(path);
        }
        KisPainter painter(added);
        painter.setPaintColor(KoColor(Qt::black, added->colorSpace()));
        painter.setAntiAliasPolygonFill(aa && feather == 0);
        painter.setFillStyle(KisPainter::FillStyleForegroundColor);
        painter.setStrokeStyle(KisPainter::StrokeStyleNone);
        painter.fillPainterPath(path);
        painter.end();
        if (grow > 0) {
            KisGrowSelectionFilter filter(grow, grow);
            filter.process(added, added->selectedRect().adjusted(-grow, -grow, grow, grow));
        } else if (grow < 0) {
            KisShrinkSelectionFilter filter(-grow, -grow, false);
            filter.process(added, added->selectedRect());
        }
        if (feather) {
            KisFeatherSelectionFilter filter(feather);
            filter.process(added, added->selectedRect().adjusted(-feather, -feather, feather, feather));
        }
    }
    expectedSelection->pixelSelection()->applySelection(added, selectionAction);
    const QRect checkedCoverage = region.united(expectedSelection->pixelSelection()->selectedExactRect());
    QByteArray expectedMask(checkedCoverage.width() * checkedCoverage.height(), '\0');
    expectedSelection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(expectedMask.data()), checkedCoverage);
    const bool empty = expectedSelection->pixelSelection()->selectedExactRect().isEmpty();

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Shape selection combinations"), cs));
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
    window->resize(800, 600); window->show(); window->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
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
    KoToolManager::instance()->switchToolRequested(toolId);
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == toolId + QLatin1String("option widget"))
            if (auto *selectionOptions = qobject_cast<KisSelectionOptions *>(widget)) options = selectionOptions;
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
    options->setMode(PIXEL_SELECTION); options->setAction(selectionAction);
    options->setAntiAliasSelection(aa); options->setGrowSelection(grow); options->setFeatherSelection(feather);
    QList<QPair<QPointer<QSpinBox>, int>> oldDimensions;
    QList<QPair<QPointer<QPushButton>, bool>> oldLocks;
    const auto restoreDimensions = qScopeGuard([&] {
        for (const auto &entry : oldDimensions) if (entry.first) entry.first->setValue(entry.second);
        for (const auto &entry : oldLocks) if (entry.first) entry.first->setChecked(entry.second);
    });
    if (rounded || rotated) {
        for (const auto &name : {QStringLiteral("lockWidthButton"), QStringLiteral("lockHeightButton"), QStringLiteral("lockRatioButton")}) {
            auto *button = options->findChild<QPushButton *>(name);
            QVERIFY(button);
            oldLocks.append({button, button->isChecked()});
            button->setChecked(false);
        }
        const QStringList names = rounded ? QStringList{QStringLiteral("intRoundCornersX"), QStringLiteral("intRoundCornersY")}
                                         : QStringList{QStringLiteral("intWidth"), QStringLiteral("intHeight")};
        for (const auto &name : names) {
            auto *spin = options->findChild<QSpinBox *>(name);
            QVERIFY(spin);
            oldDimensions.append({spin, spin->value()});
        }
        for (const auto &entry : oldDimensions) entry.first->setValue(rounded ? 18 : 128);
        if (rotated) {
            options->findChild<QPushButton *>(QStringLiteral("lockWidthButton"))->setChecked(true);
            options->findChild<QPushButton *>(QStringLiteral("lockHeightButton"))->setChecked(true);
        }
    }
    if (!words && toolId == QLatin1String("KisToolSelectRectangular") && shapeOptions == QLatin1String("aa")) {
        KisSelectionToolHelper helper(view->canvasBase(), kundo2_i18n("Select Rectangle"));
        QPainterPath oversized;
        oversized.addRect(QRectF(0, 0, 100000, 100000));
        QVERIFY(!helper.selectGpuSelectionPath(oversized, true, 0, 0, SELECTION_REPLACE));
        QPainterPath wide;
        wide.addRect(QRectF(0, 0, 200000, 1));
        QVERIFY(!helper.selectGpuSelectionPath(wide, true, -400, 0, SELECTION_REPLACE));
        QPainterPath valid;
        valid.addRect(shape);
        QVERIFY(!helper.selectGpuSelectionPath(valid, true, 401, 0, SELECTION_REPLACE));
        QVERIFY(!helper.selectGpuSelectionPath(valid, true, 0, 401, SELECTION_REPLACE));
        QCOMPARE(owner->operations().snapshot()->revision(), before->revision());
        QCOMPARE(owner->operations().undoCount(), history);
    }
    native->requestActivate(); view->canvasBase()->canvasWidget()->setFocus();
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const auto start = point(vertices.first());
    QTest::mouseMove(native.data(), start);
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    if (toolId == QLatin1String("KisToolSelectRectangular") || toolId == QLatin1String("KisToolSelectElliptical") ||
        toolId == QLatin1String("KisToolSelectPolygonal")) {
        // The held-pointer Escape must preserve the existing selection and
        // leave the next gesture independent, including its saved pixels.
        auto selectionImage = KisImageSP(lifetime->image());
        QByteArray displayedBefore(region.width() * region.height(), '\0');
        if (selectionImage->globalSelection()) selectionImage->globalSelection()->pixelSelection()->readBytes(
            reinterpret_cast<quint8 *>(displayedBefore.data()), region);
        const auto cancelledStart = point({16, 208});
        const auto cancelledEnd = point({48, 240});
        QTest::mouseMove(native.data(), cancelledStart);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, cancelledStart);
        QTest::mouseMove(native.data(), cancelledEnd, 20);
        QTest::keyClick(native.data(), Qt::Key_Escape);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, cancelledEnd);
        selectionImage->waitForDone();
        QCOMPARE(owner->operations().snapshot()->revision(), before->revision());
        QCOMPARE(owner->operations().undoCount(), history);
        QByteArray displayedAfter(displayedBefore.size(), '\0');
        if (selectionImage->globalSelection()) selectionImage->globalSelection()->pixelSelection()->readBytes(
            reinterpret_cast<quint8 *>(displayedAfter.data()), region);
        QCOMPARE(displayedAfter, displayedBefore);
        QTest::mouseMove(native.data(), start);
    }
    if (toolId == QLatin1String("KisToolSelectPolygonal") || toolId == QLatin1String("KisToolSelectPath")) {
        for (const auto &vertex : vertices) QTest::mouseClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
        QTest::mouseDClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
    } else {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
        if (toolId == QLatin1String("KisToolSelectOutline")) {
            for (qsizetype i = 1; i < vertices.size(); ++i) QTest::mouseMove(native.data(), point(vertices[i]), 20);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
        } else if (rotated) {
            QTest::keyPress(native.data(), Qt::Key_Control);
            QTest::keyPress(native.data(), Qt::Key_Alt);
            QTest::mouseMove(native.data(), point({192, 64}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::ControlModifier | Qt::AltModifier, point({192, 64}));
            QTest::keyRelease(native.data(), Qt::Key_Alt);
            QTest::keyRelease(native.data(), Qt::Key_Control);
        } else {
            QTest::mouseMove(native.data(), point({192, 192}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
        }
    }
    const int selectionHistory = history + 1;
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
    auto image = KisImageSP(lifetime->image());
    QString selectionMismatch;
    const auto checkSelection = [&](const KisImageSP &candidate) {
        if (empty) return !candidate->globalSelection();
        if (!candidate->globalSelection()) return false;
        QByteArray actual(expectedMask.size(), '\0');
        candidate->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), checkedCoverage);
        if (actual != expectedMask) {
            qsizetype index = 0;
            while (index < actual.size() && actual[index] == expectedMask[index]) ++index;
            selectionMismatch = QStringLiteral("coverage at %1,%2: actual%3 expected%4; bounds%5,%6 %7x%8")
                .arg(checkedCoverage.x() + index % checkedCoverage.width()).arg(checkedCoverage.y() + index / checkedCoverage.width())
                .arg(quint8(actual[index])).arg(quint8(expectedMask[index]))
                .arg(owner->selectionRect().x()).arg(owner->selectionRect().y()).arg(owner->selectionRect().width()).arg(owner->selectionRect().height());
        }
        return actual == expectedMask;
    };
    QTRY_VERIFY2(checkSelection(image), qPrintable(selectionMismatch));
    const auto combined = owner->operations().snapshot();
    if (selectionHistory > history) {
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot()->revision(), before->revision());
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QTRY_VERIFY(checkSelection(image));
    }

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setOpacity(1); resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory + 1);
    KisPaintDeviceSP expectedPixels = new KisPaintDevice(cs);
    const auto nativeFill = [&](const KoColor &color) {
        KisPaintDeviceSP source = new KisPaintDevice(cs);
        KisFillPainter(source).fillRect(region, color);
        KisPainter painter(expectedPixels, empty ? KisSelectionSP() : expectedSelection);
        painter.bitBlt(region.topLeft(), source, region);
    };
    const auto nativeBytes = [&] {
        QByteArray bytes(region.width() * region.height() * stride, '\0');
        expectedPixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    nativeFill(resources->fgColor());
    const auto actual = owner->readRgbaPixels(owner->operations().activeLayer(), region, region.width() * region.height() * stride);
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    QCOMPARE(actual.bytes, nativeBytes());
    QCOMPARE(combined->selectionMask, owner->operations().snapshot()->selectionMask);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), combined);
    QCOMPARE(owner->redo(), Gpu::Result::Success);

    QTemporaryDir directory;
    QVERIFY(directory.isValid()); lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("combined-selection.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    auto reopenedImage = KisImage::fromGpuImage(reopened, QStringLiteral("Reopened combined selection"), cs);
    const bool automaticDelivery = KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled();
    const auto restoreDelivery = qScopeGuard([automaticDelivery] {
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(automaticDelivery);
    });
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
    lifetime->setCurrentImage(reopenedImage);
    QTRY_COMPARE(view->image().data(), reopenedImage.data());
    QTRY_COMPARE(window->viewManager()->image().data(), reopenedImage.data());
    QTRY_VERIFY(view->currentNode() && view->currentNode()->image().data() == reopenedImage.data());
    QTRY_VERIFY(checkSelection(reopenedImage));
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    resources->setFGColor(KoColor(QColor(221, 53, 19), cs));
    QTRY_VERIFY(fill->isEnabled());
    fill->trigger();
    QTRY_COMPARE(reopened->operations().undoCount(), 1);
    nativeFill(resources->fgColor());
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    QCOMPARE(reopened->redo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    const auto editedPath = directory.filePath(QStringLiteral("reedited-selection.kra"));
    QVERIFY(lifetime->saveAs(editedPath, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile editedFile(editedPath); QVERIFY(editedFile.open(QIODevice::ReadOnly));
    const auto edited = LibrePaint::ImportExport::Kra::RasterCodec::read(editedFile);
    QVERIFY2(edited.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(edited.diagnostic));
    const auto editedOwner = paintImageWithEncoding(execution, edited.document.image);
    QCOMPARE(editedOwner->readRgbaPixels(editedOwner->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
}

void KisGpuDocumentViewTest::pathToolCreatesArbitraryMaskSelection()
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
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Path tool")));
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
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectPath"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectPath"));
    QPointer<KisSelectionOptions> options;
    const auto selectionTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KisToolSelectPath"));
    QVERIFY(selectionTool);
    for (QWidget *widget : selectionTool->optionWidgets())
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
    const QList<QPointF> vertices{{64, 64}, {192, 64}, {192, 192}, {64, 192}};

    QTest::mouseMove(native.data(), point(vertices.first()));
    const auto start = point(vertices.first());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    for (const QPointF &vertex : vertices) {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
    }
    QTest::mouseDClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    const auto mask = owner->selectionMask();
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(64, 64, 128, 128));
    QVERIFY(mask->contains(80, 80));
    QVERIFY(mask->contains(176, 176));
    QVERIFY(!mask->contains(48, 48));
    QVERIFY(!mask->contains(208, 208));
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);
}

void KisGpuDocumentViewTest::featherActionSoftensFreehandSelection()
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
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Feather selection")));
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

    // A freehand polygon fixes hard arbitrary coverage as the feather source.
    const QList<QPointF> polygon{{64, 64}, {192, 64}, {192, 128}, {128, 192}, {64, 192}};
    QVERIFY(lifetime->image()->setGpuSelectionFreehand(polygon));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QVERIFY(owner->selectionMask());
    QVERIFY(!owner->selectionMask()->isAlpha());
    const QRect hardBounds = owner->selectionRect();
    QCOMPARE(hardBounds, owner->selectionMask()->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);

    KisOperationConfigurationSP config = new KisOperationConfiguration(QStringLiteral("featherselection"));
    config->setProperty("radius", 6);
    window->viewManager()->actionManager()->runOperationFromConfiguration(config);
    lifetime->image()->waitForDone();
    QTRY_VERIFY(owner->selectionMask() && owner->selectionMask()->isAlpha());
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QCOMPARE(owner->operations().undoCount(), 2);
    const auto soft = owner->selectionMask();
    QCOMPARE(soft->coverage(128, 128), 255);
    const quint8 edge = soft->coverage(hardBounds.left(), 128);
    QVERIFY(edge > 0 && edge < 255);
    QVERIFY(soft->coverage(hardBounds.left() - 4, 128) < edge);

    // The CPU display mirror carries the fractional coverage and an outline.
    QTRY_VERIFY(lifetime->image()->globalSelection());
    const auto mirror = [&] {
        QByteArray bytes(256 * 256, '\0');
        lifetime->image()->globalSelection()->pixelSelection()->readBytes(
            reinterpret_cast<quint8 *>(bytes.data()), QRect(0, 0, 256, 256));
        return bytes;
    };
    QTRY_VERIFY(!lifetime->image()->globalSelection()->pixelSelection()->outlineCache().isEmpty());
    const auto softMirror = mirror();
    QCOMPARE(quint8(softMirror[128 * 256 + 128]), 255);
    QCOMPARE(quint8(softMirror[128 * 256 + hardBounds.left()]), edge);
    QVERIFY(quint8(softMirror[128 * 256 + hardBounds.left() - 4]) > 0);

    // Filling through the soft selection keeps the fractional alpha.
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(
        QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 3);
    const QRect region(QPoint(), QSize(256, 256));
    const auto pixel = [&](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * 256 + x) * 4, 4);
    };
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(filled.bytes, 128, 128), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(quint8(pixel(filled.bytes, hardBounds.left(), 128).at(3)), edge);
    QCOMPARE(pixel(filled.bytes, 8, 8), QByteArray(4, char(0)));

    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto unfilled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(unfilled.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(unfilled.bytes, 128, 128), QByteArray(4, char(0)));
    QVERIFY(owner->selectionMask()->isAlpha());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!owner->selectionMask()->isAlpha());
    QTRY_COMPARE(quint8(mirror()[128 * 256 + hardBounds.left()]), 255);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(owner->selectionMask()->isAlpha());
    QTRY_COMPARE(quint8(mirror()[128 * 256 + hardBounds.left()]), edge);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().undoCount(), 3);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("feathered-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false),
             qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.selectionKind, Gpu::SelectionKind::Freehand);
    QVERIFY(loaded.document.image.selectionMask);
    QVERIFY(loaded.document.image.selectionMask->isAlpha());
    QVERIFY(*loaded.document.image.selectionMask == *soft);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QVERIFY(reopened);
    QVERIFY(reopened->selectionMask() && reopened->selectionMask()->isAlpha());
    QVERIFY(*reopened->selectionMask() == *soft);
    const auto reopenedPixels = reopened->readRgbaPixels(reopened->operations().activeLayer(), region,
                                                         256 * 256 * 4);
    QCOMPARE(reopenedPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(reopenedPixels.bytes, filled.bytes);
}

void KisGpuDocumentViewTest::pasteClipboardImageCreatesGpuLayer_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("sourceWords");
    for (const bool words : {false, true}) for (const bool sourceWords : {false, true})
        QTest::newRow(qPrintable(QStringLiteral("u%1-from-u%2").arg(words ? 16 : 8).arg(sourceWords ? 16 : 8))) << words << sourceWords;
}

void KisGpuDocumentViewTest::pasteClipboardImageCreatesGpuLayer()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(bool, sourceWords);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = format;
    raster.layers.append({QStringLiteral("Paint"), true, QUuid::createUuid(), {}});
    const auto owner = paintImageWithEncoding(execution, raster);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Paste image"), cs));
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
    const auto profile = QColorSpace::fromIccProfile(cs->profile()->rawData());
    QVERIFY(profile.isValid());
    QImage pasted(16, 16, sourceWords ? QImage::Format_RGBA64 : QImage::Format_RGBA8888);
    if (sourceWords) pasted.fill(QColor::fromRgba64(4353, 33282, 61955, 32769));
    else pasted.fill(QColor(17, 129, 241, 191));
    pasted.setColorSpace(profile);
    const auto sourceColorSpace = sourceWords ? KoColorSpaceRegistry::instance()->rgb16(cs->profile()) :
                                                KoColorSpaceRegistry::instance()->rgb8(cs->profile());
    QByteArray nativeColor(sourceWords ? 8 : 4, Qt::Uninitialized);
    if (sourceWords) {
        const quint16 bgra[]{61955, 33282, 4353, 32769};
        std::memcpy(nativeColor.data(), bgra, sizeof(bgra));
    } else nativeColor = QByteArray::fromHex("f18111bf");
    KoColor expectedColor(reinterpret_cast<const quint8 *>(nativeColor.constData()), sourceColorSpace);
    expectedColor.convertTo(cs);
    Gpu::RgbaPixel rgba;
    if (words) {
        quint16 bgra[4];
        std::memcpy(bgra, expectedColor.data(), sizeof(bgra));
        rgba = {bgra[2], bgra[1], bgra[0], bgra[3]};
    } else {
        const auto *bgra = expectedColor.data();
        rgba = {bgra[2], bgra[1], bgra[0], bgra[3]};
    }
    if (words && sourceWords) QCOMPARE(rgba, (Gpu::RgbaPixel{4353, 33282, 61955, 32769}));
    const auto expectedPixel = Gpu::rgbaBytes(rgba, format);
    const auto clearClipboard = qScopeGuard([] { QGuiApplication::clipboard()->clear(); });
    QGuiApplication::clipboard()->setImage(pasted);
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->paste();
    QTRY_COMPARE(owner->operations().undoCount(), 1);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    const auto pixels = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
    int matches = 0;
    for (qsizetype offset = 0; offset < pixels.bytes.size(); offset += expectedPixel.size())
        if (pixels.bytes.mid(offset, expectedPixel.size()) == expectedPixel) ++matches;
    QCOMPARE(matches, 16 * 16);
    QCOMPARE(owner->operations().snapshot()->colorProfile, cs->profile()->rawData());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, pixels.bytes);
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const auto path = files.filePath(QStringLiteral("pasted.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile archive(path);
    QVERIFY(archive.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(archive);
    QVERIFY(loaded.code == LibrePaint::ImportExport::Kra::Status::Success);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, pixels.bytes);
    QCOMPARE(reopened->fillLayerColor(reopened->operations().activeLayer(), rgba), Gpu::Result::Success);
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, pixels.bytes);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    QImage large(128, 128, QImage::Format_RGBA8888);
    large.fill(QColor(241, 129, 17));
    large.setColorSpace(profile);
    QGuiApplication::clipboard()->setImage(large);
    manager->paste();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto cropped = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cropped.code, Gpu::ExecutionCode::Success);
    int croppedMatches = 0;
    const auto largePixel = Gpu::rgbaBytes(words ? Gpu::RgbaPixel{61937, 33153, 4369, 65535} : Gpu::RgbaPixel{241, 129, 17, 255}, format);
    for (qsizetype offset = 0; offset < cropped.bytes.size(); offset += largePixel.size())
        if (cropped.bytes.mid(offset, largePixel.size()) == largePixel) ++croppedMatches;
    QCOMPARE(croppedMatches, 64 * 64);
}

void KisGpuDocumentViewTest::reselectRestoresGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Reselect")));
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
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    const QRect selected(16, 24, 64, 48);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    manager->deselect();
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    manager->reselect();
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Rectangle);
    const QRect ellipse(32, 40, 48, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipse, true));
    QTRY_VERIFY(owner->selectionInverted());
    manager->deselect();
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    manager->reselect();
    QTRY_COMPARE(owner->selectionRect(), ellipse);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::arrowKeysNudgeGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Nudge")));
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
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
    view->canvasBase()->canvasWidget()->setFocus();
    const QRect selected(16, 24, 48, 40);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Right);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(1, 0));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Down, Qt::ShiftModifier);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(1, 10));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Right, Qt::ControlModifier);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(101, 10));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Right,
                    Qt::ControlModifier | Qt::ShiftModifier);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(1101, 10));
    const QRect ellipse(24, 32, 40, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipse, true));
    QTRY_VERIFY(owner->selectionInverted());
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Left);
    QTRY_COMPARE(owner->selectionRect(), ellipse.translated(-1, 0));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::copySelectionToNewLayerUsesGpuPixels_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8") << false;
    QTest::newRow("u16") << true;
}

void KisGpuDocumentViewTest::copySelectionToNewLayerUsesGpuPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.layers.append({QStringLiteral("Paint"), true, QUuid::createUuid(), {}});
    const auto owner = paintImageWithEncoding(execution, raster);
    const Gpu::RgbaPixel color = words ? Gpu::RgbaPixel{4353, 33282, 61955, 65535} : Gpu::RgbaPixel{17, 129, 241, 255};
    const auto colorBytes = Gpu::rgbaBytes(color, raster.format);
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    QCOMPARE(owner->fillLayerColor(owner->operations().activeLayer(), color), Gpu::Result::Success);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    const auto baseLayer = saved->activeLayer;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Copy selection layer"), cs));
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
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_selection_to_new_layer"));
    QVERIFY(action);
    QTRY_VERIFY(!action->isEnabled());
    QSignalSpy selectionChanges(window->viewManager()->selectionManager(),
                                &KisSelectionManager::currentSelectionChanged);
    QVERIFY(selectionChanges.isValid());
    const QRect selected(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QVERIFY(lifetime->image()->globalSelection());
    QVERIFY(lifetime->image()->globalSelection()->hasNonEmptyPixelSelection());
    QTRY_VERIFY(!selectionChanges.isEmpty());
    QTRY_VERIFY(action->isEnabled());
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    LibrePaint::Image::Gpu::LayerId copiedLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) copiedLayer = layer.id;
    QVERIFY(copiedLayer);
    QCOMPARE(owner->operations().activeLayer(), copiedLayer);
    const auto copiedPixels = owner->readRgbaPixels(copiedLayer, region, 8 * 1024 * 1024);
    QCOMPARE(copiedPixels.code, Gpu::ExecutionCode::Success);
    const auto expectedSelection = [&](bool ellipse, bool inverted) {
        QByteArray source(64 * 64 * colorBytes.size(), '\0');
        QByteArray coverage(64 * 64, '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
            const int dx = 2 * x + 1 - 64, dy = 2 * y + 1 - 64;
            const bool inside = ellipse ? dx * dx + dy * dy <= 32 * 32 : selected.contains(x, y);
            if (inverted || selected.contains(x, y))
                std::memcpy(source.data() + (y * 64 + x) * colorBytes.size(), colorBytes.constData(), colorBytes.size());
            coverage[y * 64 + x] = inside != inverted ? char(255) : char(0);
        }
        return nativeClipboardMask(cs, source, coverage, region.size());
    };
    QCOMPARE(copiedPixels.bytes, expectedSelection(false, false));
    int copiedMatches = 0;
    for (qsizetype offset = 0; offset < copiedPixels.bytes.size(); offset += colorBytes.size())
        if (copiedPixels.bytes.mid(offset, colorBytes.size()) == colorBytes) ++copiedMatches;
    QCOMPARE(copiedMatches, 32 * 32);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    QVERIFY(lifetime->image()->setGpuSelectionRect(selected, true));
    QTRY_VERIFY(owner->selectionInverted());
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    LibrePaint::Image::Gpu::LayerId invertedLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) invertedLayer = layer.id;
    QVERIFY(invertedLayer);
    QCOMPARE(owner->operations().activeLayer(), invertedLayer);
    const auto invertedPixels = owner->readRgbaPixels(invertedLayer, region, 8 * 1024 * 1024);
    QCOMPARE(invertedPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(invertedPixels.bytes, expectedSelection(false, true));
    int invertedMatches = 0;
    for (qsizetype offset = 0; offset < invertedPixels.bytes.size(); offset += colorBytes.size())
        if (invertedPixels.bytes.mid(offset, colorBytes.size()) == colorBytes) ++invertedMatches;
    QCOMPARE(invertedMatches, 64 * 64 - 32 * 32);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    const QRect ellipseRect(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipseRect));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    LibrePaint::Image::Gpu::LayerId ellipseLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) ellipseLayer = layer.id;
    QVERIFY(ellipseLayer);
    QCOMPARE(owner->operations().activeLayer(), ellipseLayer);
    const auto ellipsePixels = owner->readRgbaPixels(ellipseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(ellipsePixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(ellipsePixels.bytes, expectedSelection(true, false));
    const auto ellipsePixel = [&](int x, int y) { return ellipsePixels.bytes.mid((y * 64 + x) * colorBytes.size(), colorBytes.size()); };
    QCOMPARE(ellipsePixel(32, 32), colorBytes);
    auto transparentColor = color;
    transparentColor[3] = 0;
    QCOMPARE(ellipsePixel(16, 16), Gpu::rgbaBytes(transparentColor, raster.format));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipseRect, true));
    QTRY_VERIFY(owner->selectionInverted());
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    LibrePaint::Image::Gpu::LayerId invertedEllipseLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) invertedEllipseLayer = layer.id;
    QVERIFY(invertedEllipseLayer);
    QCOMPARE(owner->operations().activeLayer(), invertedEllipseLayer);
    const auto invertedEllipsePixels = owner->readRgbaPixels(invertedEllipseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(invertedEllipsePixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(invertedEllipsePixels.bytes, expectedSelection(true, true));
    const auto invertedEllipsePixel = [&](int x, int y) {
        return invertedEllipsePixels.bytes.mid((y * 64 + x) * colorBytes.size(), colorBytes.size());
    };
    QCOMPARE(invertedEllipsePixel(32, 32), Gpu::rgbaBytes(transparentColor, raster.format));
    QCOMPARE(invertedEllipsePixel(16, 16), colorBytes);
    QTemporaryDir savedFiles;
    QVERIFY(savedFiles.isValid());
    const auto savedPath = savedFiles.filePath(QStringLiteral("copied-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QFile savedFile(savedPath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(savedFile);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.format, raster.format);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, invertedEllipsePixels.bytes.size()).bytes,
             invertedEllipsePixels.bytes);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        LibrePaint::Canvas::Gpu::CanvasWindow *canvas = nullptr;
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *value = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate); value && value->isVisible()) canvas = value;
        QVERIFY(canvas);
        QSignalSpy frames(canvas, &LibrePaint::Canvas::Gpu::CanvasWindow::frameSubmitted);
        const auto visible = owner->operations().snapshot();
        canvas->refresh();
        QTRY_VERIFY(std::any_of(frames.cbegin(), frames.cend(), [&](const auto &frame) {
            return frame[0].toULongLong() == owner->handle() && frame[1].toULongLong() == visible->revision();
        }));
        QVERIFY(execution->device()->finish());
        QVERIFY(QDir().mkpath(results));
        QVERIFY(window->screen()->grabWindow(window->winId()).save(QDir(results).filePath(
                    QStringLiteral("copied-selection-layer-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".png"))));
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    QByteArray alpha(selected.width() * selected.height(), '\0');
    for (int y = 0; y < selected.height(); ++y) for (int x = 0; x < selected.width(); ++x)
        alpha[y * selected.width() + x] = char(std::array<quint8, 6>{0, 1, 127, 128, 254, 255}[(x + 2 * y) % 6]);
    const auto mask = Gpu::SelectionMask::fromAlphaCoverage(selected, alpha, region);
    QVERIFY(mask);
    for (const bool inverted : {false, true}) {
        QVERIFY(lifetime->image()->setGpuSelectionMask(mask, inverted));
        QByteArray coverage(64 * 64, '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
            const quint8 value = mask->coverage(x, y);
            coverage[y * 64 + x] = char(inverted ? 255 - value : value);
        }
        QByteArray source(64 * 64 * colorBytes.size(), '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
            if (inverted || selected.contains(x, y))
                std::memcpy(source.data() + (y * 64 + x) * colorBytes.size(), colorBytes.constData(), colorBytes.size());
        const auto expected = nativeClipboardMask(cs, source, coverage, region.size());
        action->trigger();
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QVERIFY(lifetime->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false));
        lifetime->waitForSavingToComplete();
        QFile partialFile(savedPath);
        QVERIFY(partialFile.open(QIODevice::ReadOnly));
        const auto partial = LibrePaint::ImportExport::Kra::RasterCodec::read(partialFile);
        QVERIFY(partial.code == LibrePaint::ImportExport::Kra::Status::Success);
        const auto reopened = paintImageWithEncoding(execution, partial.document.image);
        QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QCOMPARE(reopened->fillLayerColor(reopened->operations().activeLayer(), color), Gpu::Result::Success);
        QCOMPARE(reopened->undo(), Gpu::Result::Success);
        QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
        QTRY_VERIFY(window->viewManager()->activeNode() && window->viewManager()->activeNode()->parent());
        QTRY_COMPARE(window->viewManager()->activeNode()->uuid(), saved->layers.first().uuid);
        window->viewManager()->selectionManager()->paste();
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
        const auto *clipboard = qobject_cast<const KisMimeData *>(KisClipboard::instance()->layersMimeData());
        QVERIFY(clipboard);
        const auto clipOwner = clipboard->nodes().first()->image()->gpuImage();
        QVERIFY(clipOwner);
        const auto clipRegion = QRect(QPoint(), clipOwner->operations().snapshot()->size);
        const auto pasted = owner->readRgbaPixels(owner->operations().activeLayer(), clipRegion, 8 * 1024 * 1024);
        QCOMPARE(pasted.bytes, clipOwner->readRgbaPixels(clipOwner->operations().activeLayer(), clipRegion, 8 * 1024 * 1024).bytes);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
        auto *partialCut = window->viewManager()->actionManager()->actionByName(QStringLiteral("cut_selection_to_new_layer"));
        QVERIFY(partialCut);
        partialCut->trigger();
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
        QCOMPARE(owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024).bytes,
                 nativeClearSelection(cs, colorBytes.repeated(64 * 64), coverage, region.size()));
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
        QCOMPARE(owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024).bytes, colorBytes.repeated(64 * 64));
    }
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));

    auto *cutAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("cut_selection_to_new_layer"));
    QVERIFY(cutAction);
    cutAction->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto basePixels = owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(basePixels.code, Gpu::ExecutionCode::Success);
    const auto cutPixel = Gpu::rgbaPixel(QByteArrayView(basePixels.bytes).mid((24 * 64 + 24) * colorBytes.size(), colorBytes.size()), raster.format);
    QVERIFY(cutPixel);
    QCOMPARE(cutPixel->at(3), quint16(0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    const auto restored = owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(restored.code, Gpu::ExecutionCode::Success);
    QCOMPARE(restored.bytes.mid((24 * 64 + 24) * colorBytes.size(), colorBytes.size()), colorBytes);
}

void KisGpuDocumentViewTest::ordinaryLayerChangesDuringInput()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
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
    QByteArray stroke(64 * 64 * 4, '\0');
    QVERIFY(!layer->visible());
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(stroke.data()), image->bounds());
    // The interior is opaque; endpoint coverage depends on display-coordinate rounding.
    QCOMPARE(stroke.mid((28 * 64 + 28) * 4, 4), QByteArray::fromHex("f18111ff"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QVERIFY(layer->visible());
    QByteArray pixels(stroke.size(), '\0');
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), image->bounds());
    QCOMPARE(pixels, stroke);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), 0);
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), image->bounds());
    QCOMPARE(pixels, QByteArray(stroke.size(), '\0'));
}

void KisGpuDocumentViewTest::stabilizedPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("failure");
    const QStringList presets{QStringLiteral("b)_Basic-5_Size_default.kpp"),
                              QStringLiteral("d)_Ink-3_Gpen.kpp"), QStringLiteral("u)_Pixel_Art_Dithering.kpp")};
    for (int mode = 0; mode < presets.size(); ++mode) for (bool words : {false, true}) {
        const QByteArray tag = presets[mode].toLatin1() + (words ? "-u16" : "-u8");
        QTest::newRow(tag.constData()) << presets[mode] << words << mode << 0;
    }
    for (const auto &entry : QList<QPair<const char *, int>>{{"cancel-pending", 1}, {"input-capacity", 2},
                                                           {"close-on-capacity", 3}})
        QTest::newRow(entry.first) << presets[1] << false << 2 << entry.second;
}

void KisGpuDocumentViewTest::stabilizedPresetsKeepOrdinaryEditing()
{
    QFETCH(QString, presetName);
    QFETCH(bool, words);
    QFETCH(int, mode);
    QFETCH(int, failure);
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    KisConfig cfg(false);
    const bool previousDelayed = cfg.stabilizerDelayedPaint();
    cfg.setStabilizerDelayedPaint(mode == 2);
    const auto restoreDelayed = qScopeGuard([&] { KisConfig(false).setStabilizerDelayedPaint(previousDelayed); });
    const auto bundle = KisResourceStorageSP::create(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle"));
    auto preset = bundle->resource(QStringLiteral("paintoppresets/") + presetName).dynamicCast<KisPaintOpPreset>();
    QVERIFY(preset);
    KisResourceModel brushes(ResourceType::Brushes);
    if (brushes.rowCount() == 0)
        QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
    KisResourceModel patterns(ResourceType::Patterns);
    if (patterns.rowCount() == 0)
        QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    if (preset->settings()->getBool("Texture/Pattern/Enabled")) {
        const auto file = QFileInfo(preset->settings()->getString("Texture/Pattern/PatternFileName")).fileName();
        auto pattern = bundle->resource(QStringLiteral("patterns/") + file).dynamicCast<KoPattern>();
        QVERIFY(pattern);
        if (!KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns).bestMatch(pattern->md5Sum(), pattern->filename(), pattern->name()))
            QVERIFY(patterns.addResource(pattern));
        preset->settings()->setProperty("Texture/Pattern/isRandomOffsetX", false);
        preset->settings()->setProperty("Texture/Pattern/isRandomOffsetY", false);
    }
    preset->setFilename(QStringLiteral("stabilized-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
    KisResourceModel presets(ResourceType::PaintOpPresets);
    QVERIFY(presets.addResource(preset));
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QSize size(96, 96);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = size;
    raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.layers = {{QStringLiteral("Paint Layer 1")}};
    Gpu::Limits limits;
    if (failure >= 2) limits.samplesPerStroke = 32;
    auto owner = paintImageWithEncoding(execution, raster, limits);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Stabilized editing"), cs));
    document->setAutoSaveActive(false);
    document->setFileBatchMode(true);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> input;
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
    resources->setSize(mode == 2 ? 12 : 8);
    resources->setOpacity(.7);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setEraserMode(false);
    resources->setGlobalAlphaLock(false);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(tool);
    const auto configure = [&] {
        return tool->setProperty("smoothingType", 3) && tool->setProperty("smoothnessQualityMin", 6) &&
            tool->setProperty("smoothnessQualityMax", 6) &&
            tool->setProperty("useDelayDistance", mode == 1) && tool->setProperty("delayDistance", 2) &&
            tool->setProperty("finishStabilizedCurve", true) && tool->setProperty("stabilizeSensors", true);
    };
    QVERIFY(configure());
    QSignalSpy errors(tool, &KoToolBase::statusTextChanged);
    if (failure == 3) connect(tool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
        if (message.isEmpty() || !lifetime) return;
        lifetime->setModified(false);
        window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= size.width() &&
                view->canvasBase()->canvasWidget()->height() >= size.height());
    view->zoomManager()->zoomTo100();
    const auto imageSize = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    view->canvasBase()->canvasWidget()->setFocus();
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = canvas;
    QVERIFY(input);
    QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().snapshot());
    QTRY_VERIFY(input && input->isExposed());
    const auto point = [&](QPointF imagePoint) { return view->canvasBase()->coordinatesConverter()->imageToWidget(imagePoint).toPoint(); };
    const auto start = point({16, 24});
    QTest::mouseMove(input, start);
    QEnterEvent entered(start, start, input->mapToGlobal(start));
    QApplication::sendEvent(input, &entered);
    KisImageSP nativeImage = new KisImage(nullptr, size.width(), size.height(), cs, QStringLiteral("Stabilized reference"));
    KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Reference"), 255);
    nativeImage->addNode(nativeLayer, nativeImage->root());
    const auto pixels = [&] {
        const auto value = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(QPoint(), size), size.width() * size.height() * cs->pixelSize());
        return value.bytes;
    };
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto output = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(
        QStringLiteral("stabilized-%1").arg(QString::fromLatin1(QTest::currentDataTag())));
    QVERIFY(QDir().mkpath(output));
    const auto empty = pixels();
    QCOMPARE(empty.size(), size.width() * size.height() * cs->pixelSize());
    for (int step = 0; step < 3; ++step) {
        QVERIFY(configure());
        resources->setSize(mode == 2 ? 12 : 8);
        resources->setOpacity(.7);
        resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
        resources->setEraserMode(step == 1);
        const KisResourcesSnapshotSP nativeResources(new KisResourcesSnapshot(nativeImage, nativeLayer,
            view->canvasBase()->resourceManager()->canvasResourcesInterface()));
        auto nativeStroke = nativeImage->startStroke(new FreehandStrokeStrategy(nativeResources,
            new KisFreehandStrokeInfo, kundo2_i18n("Stabilized reference")));
        const auto finishNative = [&] {
            if (!nativeStroke) return;
            nativeImage->addJob(nativeStroke, new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
            nativeImage->endStroke(nativeStroke);
            nativeStroke.clear();
            nativeImage->waitForDone();
        };
        const auto finishReference = qScopeGuard(finishNative);
        const auto before = pixels();
        const int history = lifetime->undoStack()->index();
        pressed = true;
        QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, point({16, 24}));
        QVERIFY2(owner->operations().hasActiveStroke(), errors.isEmpty() ? "The stabilized press did not start a stroke" :
                 qPrintable(errors.last().first().toString()));
        // A settings edit during the stroke applies to the next stroke.
        QVERIFY(tool->setProperty("smoothingType", 0));
        QVERIFY(tool->setProperty("useDelayDistance", true));
        QVERIFY(tool->setProperty("delayDistance", 80));
        QVERIFY(tool->setProperty("finishStabilizedCurve", false));
        resources->setSize(24);
        resources->setOpacity(.2);
        resources->setFGColor(KoColor(QColor(241, 17, 129), cs));
        QTest::mouseMove(input, point({40, 24}), 5);
        if (failure == 1) {
            lifetime->image()->requestStrokeCancellation();
        } else if (failure >= 2) {
            QTRY_VERIFY(!owner->operations().hasActiveStroke());
        } else {
            QTest::mouseMove(input, point({56, 40}), 5);
            QTest::mouseMove(input, point({72, 40}), 5);
        }
        if (input) QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, point({72, 40}));
        else QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        pressed = false;
        QVERIFY(!owner->operations().hasActiveStroke());
        if (failure) {
            finishNative();
            QCOMPARE(owner->operations().undoCount(), 0);
            QCOMPARE(pixels(), empty);
            if (failure >= 2) {
                QStringList reasons;
                for (const auto &notification : errors)
                    if (!notification.first().toString().isEmpty()) reasons.append(notification.first().toString());
                QCOMPARE(reasons.size(), 1);
            }
            if (failure == 3) QVERIFY(!lifetime);
            QTest::qWait(50);
            QVERIFY(!owner->operations().hasActiveStroke());
            return;
        }
        QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
        const auto operation = std::get<Gpu::PaintSnapshot>(owner->operations().snapshot()->layers.first().operations.last());
        const auto samples = operation->inputSamples();
        QVERIFY(samples.size() > 2);
        QVERIFY(!operation->brush.basicSmoothing && !operation->brush.weightedSmoothing);
        QCOMPARE(operation->brush.opacity, qreal(.7));
        bool interpolated = false;
        const auto information = [](const Gpu::InputSample &sample) {
            KisPaintInformation result(sample.position, sample.pressure, sample.xTilt, sample.yTilt, sample.rotation,
                                       0, 1, sample.timestamp, sample.speed);
            result.setCanvasRotation(sample.canvasRotation);
            result.setCanvasMirroredH(sample.canvasMirroredH);
            result.setCanvasMirroredV(sample.canvasMirroredV);
            return result;
        };
        qsizetype sampleIndex = 0;
        KisPaintInformation previous;
        for (const auto command : operation->shape.pathCommands) {
            const auto point = information(samples[sampleIndex++]);
            if (command == Gpu::PathCommand::Point)
                nativeImage->addJob(nativeStroke, new FreehandStrokeStrategy::Data(0, point));
            else if (command == Gpu::PathCommand::LineTo)
                nativeImage->addJob(nativeStroke, new FreehandStrokeStrategy::Data(0, previous, point));
            else QVERIFY(command == Gpu::PathCommand::MoveTo);
            previous = point;
            interpolated |= point.pos().x() > 16 && point.pos().x() < 40;
        }
        QVERIFY(interpolated);
        finishNative();
        QByteArray expected(size.width() * size.height() * cs->pixelSize(), Qt::Uninitialized);
        nativeLayer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(expected.data()), QRect(QPoint(), size));
        exchangeRgbaBgra(expected, cs->pixelSize());
        const auto after = pixels();
        for (const auto &entry : {qMakePair(QStringLiteral("native"), expected), qMakePair(QStringLiteral("gpu"), after)}) {
            QFile artifact(QDir(output).filePath(QStringLiteral("step-%1-%2.rgba").arg(step).arg(entry.first)));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(entry.second), entry.second.size());
        }
        QFile inputArtifact(QDir(output).filePath(QStringLiteral("step-%1-input.txt").arg(step)));
        QVERIFY(inputArtifact.open(QIODevice::WriteOnly));
        for (const auto &sample : samples) {
            const auto line = QStringLiteral("%1 %2 %3 %4\n").arg(sample.position.x(), 0, 'g', 17).arg(sample.position.y(), 0, 'g', 17)
                .arg(sample.pressure, 0, 'g', 17).arg(sample.timestamp).toUtf8();
            QCOMPARE(inputArtifact.write(line), line.size());
        }
        QCOMPARE(after, expected);
        QVERIFY(after != before);
        lifetime->undoStack()->undo();
        QCOMPARE(pixels(), before);
        lifetime->undoStack()->redo();
        QCOMPARE(pixels(), after);
        const auto path = QDir(output).filePath(QStringLiteral("step-%1.kra").arg(step));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto decoded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
        QVERIFY2(decoded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(decoded.diagnostic));
        auto imported = paintImageWithEncoding(execution, decoded.document.image);
        QCOMPARE(imported->readRgbaPixels(imported->operations().activeLayer(), QRect(QPoint(), size), after.size()).bytes, after);
        if (step == 1) {
            // The next ordinary pointer stroke targets the reloaded document pixels.
            file.close();
            lifetime->waitForSavingToComplete();
            QVERIFY2(lifetime->openPath(path, KisDocument::DontAddToRecent), qPrintable(lifetime->errorMessage()));
            owner = lifetime->image()->gpuImage();
            QVERIFY(owner);
            window->viewManager()->nodeManager()->slotUiActivatedNode(lifetime->image()->root()->firstChild());
            resources->setPaintOpPreset(preset);
            view->zoomManager()->zoomTo100();
            const auto extent = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
            view->canvasController()->setPreferredCenter({extent.width() / 2, extent.height() / 2});
            view->canvasBase()->canvasWidget()->setFocus();
            KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
            tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
            QVERIFY(tool);
            for (auto *candidate : QGuiApplication::allWindows())
                if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = canvas;
            QTRY_VERIFY(input && input->isExposed());
        }
    }
}

