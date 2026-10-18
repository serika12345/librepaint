/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QTest>
#include <limits>
#include <cmath>
#include <stdexcept>

using namespace LibrePaint::Image::Gpu;

class KisGpuPaintImageTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void colorFillKeepsVersionsHistoryAndWholeRequestFailures();
    void clearedLayersKeepFixedVersionsAndRejectWholeRequests();
    void clearHistoryKeepsItsMeaningAfterDirectWritesAndRecovery();
    void retainedLayerPixelsRemainEditableThroughRemovalAndDiscardedHistory();
    void rejectedRetainedLayerWritesPreserveVersionsAndHistory();
    void retainedImagesAndClonesKeepTheirExecutionAlive();
    void clonesKeepIndependentCurrentStateAndOperationIdentities();
    void retainedLayerCopiesKeepIndependentPixelsAndExecutionLifetime();
    void insertedLayerPixelsPublishAtomicallyAndRemainIndependent();
    void filledFiguresPublishOneVersionAndRejectWholeRequests();
    void sharedEditsPublishBeforeNotificationAndReleaseTheLastAttachment();
    void recoveryPreservesPendingInputAndRedoOnTheSameAttachment();
    void directPixelsReachGpuWithoutEditingNotificationsOrHistory();
    void rectangularWritesPreserveOtherRegionsAcrossHistoryAndRecovery_data();
    void rectangularWritesPreserveOtherRegionsAcrossHistoryAndRecovery();
    void rectangularWritesValidateBeforeMutationAndCrossTileBoundaries();
    void rectangularHistoryEditsPreserveIndependentUntrackedPixels();
    void interiorPixelWritesRemainWithTheirFilledHistory();
    void idleSmoothingPublishesAcceptedVersionsWithoutDuplicateNotifications();
    void derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites_data();
    void derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites();
    void damagedPixelSharingRecoveryPreservesTheLiveImage();
    void historyNavigationPublishesOneVersionAndPreservesSavedState();
};

void KisGpuPaintImageTest::colorFillKeepsVersionsHistoryAndWholeRequestFailures()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(32, 16));
    const auto layer = image.operations().activeLayer();
    const auto other = image.addLayer(QStringLiteral("Other"));
    QCOMPARE(image.selectLayers({other}, other), Result::Success);
    const QRect outside(-4, 5, 1, 1);
    const auto seed = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, outside, seed), Result::Success);
    const auto fixed = image.operations().snapshot();
    QCOMPARE(image.markSaved(fixed), Result::Success);
    const auto count = image.operations().undoCount();
    QSignalSpy changed(&image, &PaintImage::changed);
    bool published = false;
    connect(&image, &PaintImage::changed, this, [&] {
        published = execution.requested(image.handle()) == image.operations().snapshot();
    });
    std::array<quint8, 3> rgb{17, 129, 241};
    QCOMPARE(image.fillLayer(layer, rgb, 128), Result::Success);
    rgb = {0, 255, 0};
    QCOMPARE(changed.size(), 1);
    QVERIFY(published);
    QCOMPARE(image.operations().undoCount(), count + 1);
    QCOMPARE(image.operations().historyAction(count + 1), HistoryAction::FillLayer);
    QCOMPARE(image.operations().snapshot()->selectedLayers, fixed->selectedLayers);
    QCOMPARE(image.operations().activeLayer(), other);
    QCOMPARE(image.readRgbaPixels(layer, {8, 8, 1, 1}, 4).bytes, QByteArray::fromHex("1181f180"));
    QCOMPARE(image.readRgbaPixels(layer, outside, 4).bytes, seed);
    QCOMPARE(image.readRgbaPixels(other, {8, 8, 1, 1}, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), fixed, {8, 8, 1, 1}, layer, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(image.undo(), Result::Success);
    QVERIFY(!image.operations().isModified());
    const auto pending = image.beginStroke(layer, Brush{}, 42);
    QCOMPARE(image.appendSamples(pending, {{{3, 4}, 1}}), Result::Success);
    const auto preview = image.operations().preview();
    const auto pendingRecovery = image.operations().recoveryData();
    changed.clear();
    QCOMPARE(image.fillLayer(layer, rgb), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), preview);
    QCOMPARE(image.operations().recoveryData(), pendingRecovery);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.cancelStroke(pending), Result::Success);
    const auto redo = image.operations().recoveryData();
    QCOMPARE(image.fillLayer(9999, rgb), Result::LayerUnavailable);
    QCOMPARE(image.operations().recoveryData(), redo);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {8, 8, 1, 1}, 4).bytes, QByteArray::fromHex("1181f180"));
    QCOMPARE(image.writeRgbaPixels(layer, {3, 4, 1, 1}, seed), Result::Success);
    PaintImage recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(image.operations().recoveryData()), Result::Success);
    QCOMPARE(recovered.operations().historyAction(count + 1), HistoryAction::FillLayer);
    QCOMPARE(recovered.readRgbaPixels(layer, {3, 4, 1, 1}, 4).bytes, seed);
    QCOMPARE(recovered.readRgbaPixels(layer, {8, 8, 1, 1}, 4).bytes, QByteArray::fromHex("1181f180"));
    QCOMPARE(image.writeLayerLocked(layer, true), Result::Success);
    const auto locked = image.operations().recoveryData();
    changed.clear();
    QCOMPARE(image.fillLayer(layer, rgb), Result::LayerUnavailable);
    QCOMPARE(image.operations().recoveryData(), locked);
    QCOMPARE(changed.size(), 0);
    Limits limits;
    limits.samplesPerStroke = 1;
    PaintImage bounded(execution, QSize(32, 16), limits);
    const auto before = bounded.operations().recoveryData();
    QCOMPARE(bounded.fillLayer(bounded.operations().activeLayer(), rgb), Result::CapacityExceeded);
    QCOMPARE(bounded.operations().recoveryData(), before);
    auto closing = PaintImageSP::create(execution, QSize(32, 16));
    const auto handle = closing->handle();
    connect(closing.data(), &PaintImage::changed, this, [&] { closing.clear(); });
    QCOMPARE(closing->fillLayer(closing->operations().activeLayer(), rgb), Result::Success);
    QVERIFY(!closing);
    QVERIFY(!execution.requested(handle));
}

void KisGpuPaintImageTest::clearHistoryKeepsItsMeaningAfterDirectWritesAndRecovery()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(16, 16));
    const auto layer = image.operations().activeLayer();
    QCOMPARE(image.replaceRgbaPixels(layer, {1, 1, 1, 1}, QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(image.clearLayers({layer}), Result::Success);
    const auto count = image.operations().undoCount();
    QCOMPARE(image.writeRgbaPixels(layer, {8, 8, 1, 1}, QByteArray::fromHex("445566ff")), Result::Success);
    QCOMPARE(image.operations().undoCount(), count);
    // The document's public undo display reads this action after recovery.
    QCOMPARE(image.operations().historyAction(count), HistoryAction::ClearPixels);
    PaintImage recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(image.operations().recoveryData()), Result::Success);
    QCOMPARE(recovered.operations().historyAction(count), HistoryAction::ClearPixels);
}

void KisGpuPaintImageTest::clearedLayersKeepFixedVersionsAndRejectWholeRequests()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster{QSize(64, 64), {{QStringLiteral("Default pixels"), true, QUuid::createUuid(), {}, {5, 6, 7, 255}},
                                      {QStringLiteral("Paint"), true, QUuid::createUuid(), {}}}, 1};
    Limits limits;
    limits.rasterBytes = RgbaTile::ByteCount;
    PaintImage image(execution, raster, limits);
    const auto first = image.operations().snapshot()->layers[0].id;
    const auto second = image.operations().snapshot()->layers[1].id;
    const QRect region(-4, 8, 1, 1);
    const auto pixel = QByteArray::fromHex("112233ff");
    QCOMPARE(image.replaceRgbaPixels(first, region, pixel), Result::Success);
    Brush brush;
    const auto stroke = image.beginStroke(second, brush, 11);
    QCOMPARE(image.appendSamples(stroke, {{{8, 8}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    const auto fixed = image.operations().snapshot();
    const auto reference = image.retainLayerPixels(first);
    QCOMPARE(image.markSaved(fixed), Result::Success);
    const auto count = image.operations().undoCount();
    QSignalSpy changed(&image, &PaintImage::changed);
    bool requestedBeforeNotification = true;
    connect(&image, &PaintImage::changed, this, [&] {
        requestedBeforeNotification &= execution.requested(image.handle()) == image.operations().preview();
    });
    const auto pending = image.beginStroke(second, brush, 12);
    QCOMPARE(image.appendSamples(pending, {{{24, 24}, 1}}), Result::Success);
    const auto preview = image.operations().preview();
    const auto history = image.operations().recoveryData();
    changed.clear();
    QCOMPARE(image.clearLayers({first, second}), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), preview);
    QCOMPARE(image.operations().recoveryData(), history);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.cancelStroke(pending), Result::Success);
    changed.clear();
    QCOMPARE(image.clearLayers({first, 9999}), Result::LayerUnavailable);
    QCOMPARE(image.clearLayers({first, first}), Result::InvalidInput);
    QCOMPARE(image.operations().snapshot(), fixed);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.clearLayers({first, second}), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.operations().undoCount(), count + 1);
    QCOMPARE(image.operations().historyAction(count + 1), HistoryAction::ClearPixels);
    QCOMPARE(image.operations().snapshot()->sampleCount, 0);
    QCOMPARE(image.operations().snapshot()->rasterBytes, 0);
    QCOMPARE(image.operations().snapshot()->selectedLayers, fixed->selectedLayers);
    QCOMPARE(image.operations().activeLayer(), fixed->activeLayer);
    QCOMPARE(reference->version(), image.operations().snapshot());
    QCOMPARE(image.readRgbaPixels(first, region, 4).bytes, QByteArray::fromHex("050607ff"));
    QCOMPARE(image.readRgbaPixels(second, {8, 8, 1, 1}, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), fixed, region, first, 4).bytes, pixel);
    QVERIFY(image.operations().isModified());
    QCOMPARE(image.undo(), Result::Success);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(image.readRgbaPixels(first, region, 4).bytes, pixel);
    QCOMPARE(image.redo(), Result::Success);
    const auto cleared = image.operations().snapshot();
    PaintImage recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(image.operations().recoveryData()), Result::Success);
    QCOMPARE(recovered.readRgbaPixels(first, region, 4).bytes, QByteArray::fromHex("050607ff"));
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.readRgbaPixels(first, region, 4).bytes, pixel);
    QCOMPARE(image.writeLayerLocked(second, true), Result::Success);
    const auto locked = image.operations().snapshot();
    changed.clear();
    QCOMPARE(image.clearLayers({first, second}), Result::LayerUnavailable);
    QCOMPARE(image.operations().snapshot(), locked);
    QCOMPARE(changed.size(), 0);
    QVERIFY(requestedBeforeNotification);
    QCOMPARE(execution.readRgbaPixels(image.handle(), cleared, region, first, 4).bytes, QByteArray::fromHex("050607ff"));
    auto closing = PaintImageSP::create(execution, QSize(32, 32));
    const auto handle = closing->handle();
    connect(closing.data(), &PaintImage::changed, this, [&] { closing.clear(); });
    QCOMPARE(closing->clearLayers({closing->operations().activeLayer()}), Result::Success);
    QVERIFY(!closing);
    QVERIFY(!execution.requested(handle));
}

void KisGpuPaintImageTest::idleSmoothingPublishesAcceptedVersionsWithoutDuplicateNotifications()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    Brush brush;
    brush.basicSmoothing = true;
    QSignalSpy changed(&image, &PaintImage::changed);
    bool requestedAtNotification = true;
    connect(&image, &PaintImage::changed, this, [&] {
        requestedAtNotification &= execution.requested(image.handle()) == image.operations().preview();
    });
    const auto id = image.beginStroke(image.operations().activeLayer(), brush, 7);
    QCOMPARE(image.appendSamples(id, {{{8, 32}, 1}, {{32, 8}, 1}}), Result::Success);
    QCOMPARE(changed.size(), 1);
    const auto beforeIdle = image.operations().preview();
    QCOMPARE(image.flushSmoothing(id), Result::Success);
    QCOMPARE(changed.size(), 2);
    const auto afterIdle = image.operations().preview();
    QVERIFY(beforeIdle != afterIdle);
    QCOMPARE(image.flushSmoothing(id), Result::Success);
    QCOMPARE(changed.size(), 2);
    QCOMPARE(image.operations().preview(), afterIdle);
    QCOMPARE(image.commitStroke(id), Result::Success);
    QCOMPARE(changed.size(), 3);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(requestedAtNotification);
}

void KisGpuPaintImageTest::derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites_data()
{
    QTest::addColumn<bool>("basicSmoothing");
    QTest::addColumn<bool>("weightedSmoothing");
    QTest::newRow("path") << false << false;
    QTest::newRow("basic-smoothing") << true << false;
    QTest::newRow("weighted-smoothing") << true << true;
}

void KisGpuPaintImageTest::derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    QFETCH(bool, basicSmoothing);
    QFETCH(bool, weightedSmoothing);
    Shape shape;
    Brush brush;
    brush.basicSmoothing = basicSmoothing;
    QList<InputSample> samples;
    const qreal limit = OperationDocument::CoordinateLimit;
    if (weightedSmoothing) {
        brush.weightedSmoothing = WeightedSmoothing{24, 48, 0.01, true};
        for (int index = 0; index < 6000; ++index)
            samples.append({{8, 16}, 1, 0, 0, 0, quint64(index * 8)});
    } else if (basicSmoothing) {
        const QList<QPointF> corners{{0, 0}, {limit, 0}, {limit, limit}, {0, limit}};
        for (int index = 0; index < 512; ++index)
            samples.append({corners[index % corners.size()], 1, 0, 0, 0, quint64(index * 8)});
    } else {
        shape.kind = ShapeKind::Path;
        for (int curve = 0; curve < 65; ++curve) {
            shape.pathCommands.append({PathCommand::MoveTo, PathCommand::CubicTo});
            samples.append({{{0, 0}, 1}, {{limit, 0}, 1}, {{limit, limit}, 1}, {{0, limit}, 1}});
        }
    }
    QCOMPARE(image.paintFigure(layer, brush, samples, shape, FigureStyle{}, 42), Result::Success);
    const auto pinned = image.operations().snapshot();
    const auto recovery = image.operations().recoveryData();
    TileRenderer renderer(device);
    auto *commands = device->beginCommands();
    QVERIFY(commands);
    const auto recorded = renderer.record(commands, pinned);
    QVERIFY2(device->submitCommands(true), qPrintable(device->diagnostic()));
    QCOMPARE(recorded.code, RenderOutcome::CapacityExceeded);
    QCOMPARE(renderer.residentBytes(), qsizetype(0));
    const auto read = execution.readLayerPixels(image.handle(), pinned, layer, RgbaTile::ByteCount);
    QCOMPARE(read.code, ExecutionCode::CapacityExceeded);
    QVERIFY(read.tiles.isEmpty());
    QCOMPARE(image.writeRgbaPixels(layer, {8, 8, 1, 1}, QByteArray::fromHex("112233ff")), Result::CapacityExceeded);
    QCOMPARE(image.operations().snapshot(), pinned);
    QCOMPARE(image.operations().recoveryData(), recovery);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(image.operations().activeOperation().isNull());
}

void KisGpuPaintImageTest::interiorPixelWritesRemainWithTheirFilledHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(1024, 1024));
    const auto layer = image.operations().activeLayer();
    FigureStyle style;
    style.outline = false;
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    QCOMPARE(image.paintFigure(layer, Brush{}, {{{0, 0}, 1}, {{1024, 1024}, 1}},
                               {ShapeKind::Rectangle, {0, 0}, {1024, 1024}, 0}, style, 42), Result::Success);
    const QRect region(512, 512, 1, 1);
    const auto written = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, region, written), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1);
    QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes, written);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes, QByteArray(4, char(0)));
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes, written);
}

void KisGpuPaintImageTest::filledFiguresPublishOneVersionAndRejectWholeRequests()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 4;
    FigureStyle style;
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    const Shape shape{ShapeKind::Rectangle, {8, 8}, {56, 56}, 0};
    const QList<InputSample> samples{{{8, 8}, 1}, {{56, 56}, 1}};
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.paintFigure(layer, brush, samples, shape, style, 42), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.operations().undoCount(), 1);
    QCOMPARE(image.readRgbaPixels(layer, {32, 24, 1, 1}, 4).bytes, QByteArray::fromHex("dd112cff"));
    QCOMPARE(image.readRgbaPixels(layer, {32, 8, 1, 1}, 4).bytes, QByteArray::fromHex("dd112cff"));
    QCOMPARE(image.readRgbaPixels(layer, {32, 7, 1, 1}, 4).bytes, QByteArray::fromHex("1181f1ff"));
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {32, 24, 1, 1}, 4).bytes, QByteArray(4, char(0)));
    QCOMPARE(image.redo(), Result::Success);
    const auto before = image.operations().recoveryData();
    changed.clear();
    auto invalid = samples;
    invalid.first().pressure = -1;
    QCOMPARE(image.paintFigure(layer, brush, invalid, shape, style, 43), Result::InvalidInput);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.operations().recoveryData(), before);
    const auto pending = image.beginStroke(layer, brush, 44);
    QVERIFY(!pending.isNull());
    QCOMPARE(image.paintFigure(layer, brush, samples, shape, style, 45), Result::EditInProgress);
    QCOMPARE(image.operations().activeOperation(), pending);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.cancelStroke(pending), Result::Success);
}

void KisGpuPaintImageTest::insertedLayerPixelsPublishAtomicallyAndRemainIndependent()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage source(execution, QSize(16, 16));
    PaintImage destination(execution, QSize(16, 16));
    const auto sourceLayer = source.operations().activeLayer();
    const auto pixels = source.retainLayerPixels(sourceLayer);
    const QRect rect(5, 7, 1, 1);
    const auto bytes = QByteArray::fromHex("112233ff");
    QCOMPARE(source.writeRgbaPixels(sourceLayer, rect, bytes), Result::Success);
    RasterLayer properties;
    properties.name = QStringLiteral("Inserted pixels");
    properties.uuid = QUuid::createUuid();
    QSignalSpy changed(&destination, &PaintImage::changed);
    const auto inserted = destination.insertLayer(source, pixels, properties, 1);
    QVERIFY(inserted);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(destination.operations().undoCount(), 1);
    QCOMPARE(destination.readRgbaPixels(inserted, rect, 4).bytes, bytes);
    const auto fixed = destination.operations().snapshot();
    QCOMPARE(source.writeRgbaPixels(sourceLayer, rect, QByteArray::fromHex("445566ff")), Result::Success);
    QCOMPARE(destination.readRgbaPixels(inserted, rect, 4).bytes, bytes);
    QCOMPARE(destination.undo(), Result::Success);
    QCOMPARE(destination.redo(), Result::Success);
    QCOMPARE(destination.operations().snapshot(), fixed);
    auto rejected = properties;
    rejected.uuid = QUuid::createUuid();
    const auto before = destination.operations().recoveryData();
    QVERIFY(!destination.insertLayer(destination, pixels, rejected, 0));
    QCOMPARE(destination.operations().recoveryData(), before);
    QCOMPARE(source.removeLayer(sourceLayer), Result::Success);
    const auto retained = destination.insertLayer(source, pixels, rejected, 0);
    QVERIFY(retained);
    QCOMPARE(destination.readRgbaPixels(retained, rect, 4).bytes, QByteArray::fromHex("445566ff"));
    OperationDocument empty(QSize(16, 16));
    rejected.uuid = QUuid::createUuid();
    QVERIFY(destination.insertLayer(empty, empty.activeLayer(), rejected, 0));
    QCOMPARE(destination.limits().rasterBytes, Limits{}.rasterBytes);
}

void KisGpuPaintImageTest::retainedLayerPixelsRemainEditableThroughRemovalAndDiscardedHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(16, 16));
    const auto bottom = image.operations().activeLayer();
    const auto layer = image.addLayer(QStringLiteral("Retained"));
    const auto pixels = image.retainLayerPixels(layer);
    QVERIFY(pixels);
    QCOMPARE(image.retainLayerPixels(layer), pixels);
    const auto first = QByteArray::fromHex("112233ff");
    const auto second = QByteArray::fromHex("445566aa");
    const QRect rect(5, 5, 1, 1);
    QCOMPARE(image.writeRgbaPixels(layer, rect, first), Result::Success);
    QCOMPARE(pixels->version(), image.operations().snapshot());
    const auto pinned = pixels->version();
    const auto before = image.operations().undoCount();
    QCOMPARE(image.removeLayer(layer), Result::Success);
    const auto removed = image.operations().snapshot();
    QCOMPARE(image.markSaved(removed), Result::Success);
    QCOMPARE(pixels->version(), pinned);
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.writeRgbaPixels(pixels, rect, second), Result::Success);
    QCOMPARE(image.operations().snapshot(), removed);
    QCOMPARE(image.operations().undoCount(), before + 1);
    QVERIFY(!image.operations().isModified());
    QVERIFY(changed.isEmpty());
    QCOMPARE(execution.readRgbaPixels(image.handle(), pixels->version(), rect, layer, 4).bytes, second);
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, rect, layer, 4).bytes, first);
    PaintImage recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(image.operations().recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.readRgbaPixels(layer, rect, 4).bytes, second);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, rect, 4).bytes, second);
    QCOMPARE(pixels->version(), image.operations().snapshot());
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.renameLayer(bottom, QStringLiteral("Discarded layer branch")), Result::Success);
    QCOMPARE(image.operations().redoCount(), 0);
    const auto branch = image.operations().snapshot();
    const auto history = image.operations().undoCount();
    const auto lastPixels = pixels->version();
    QCOMPARE(image.writeRgbaPixels(pixels, rect, first), Result::Success);
    QCOMPARE(image.operations().snapshot(), branch);
    QCOMPARE(image.operations().undoCount(), history);
    QCOMPARE(image.readRgbaPixels(bottom, rect, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), pixels->version(), rect, layer, 4).bytes, first);
    QCOMPARE(execution.readRgbaPixels(image.handle(), lastPixels, rect, layer, 4).bytes, second);
    QVERIFY(!image.retainLayerPixels(layer));
}

void KisGpuPaintImageTest::rejectedRetainedLayerWritesPreserveVersionsAndHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    Limits limits;
    limits.rasterBytes = 2 * RgbaTile::ByteCount;
    PaintImage image(execution, QSize(16, 16), limits);
    PaintImage other(execution, QSize(16, 16));
    const auto layer = image.operations().activeLayer();
    const auto pixels = image.retainLayerPixels(layer);
    const auto foreign = other.retainLayerPixels(other.operations().activeLayer());
    QVERIFY(pixels && foreign);
    const QRect rect(5, 5, 1, 1);
    const auto bytes = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, rect, bytes), Result::Success);
    QCOMPARE(image.removeLayer(layer), Result::Success);
    QCOMPARE(image.writeRgbaPixels(pixels, rect, bytes), Result::Success);
    const auto pinned = pixels->version();
    const auto before = image.operations().recoveryData();
    QCOMPARE(image.writeRgbaPixels(pixels, rect, bytes), Result::CapacityExceeded);
    QCOMPARE(image.writeRgbaPixels(foreign, rect, bytes), Result::InvalidInput);
    QCOMPARE(pixels->version(), pinned);
    QCOMPARE(image.operations().recoveryData(), before);
}

void KisGpuPaintImageTest::retainedImagesAndClonesKeepTheirExecutionAlive()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Execution>(device);
    std::weak_ptr<Execution> lifetime = execution;
    auto image = PaintImageSP::create(execution, QSize(32, 32));
    const auto layer = image->operations().activeLayer();
    QCOMPARE(image->writeRgbaPixels(layer, QRect(4, 5, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    auto clone = image->clone(true);
    execution.reset();
    device.reset();
    image.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(clone->readRgbaPixels(layer, QRect(4, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));

    auto destinationExecution = std::make_shared<Execution>(Device::createNative(diagnostic));
    std::weak_ptr<Execution> destinationLifetime = destinationExecution;
    auto destination = PaintImageSP::create(destinationExecution, QSize(32, 32));
    auto replacement = clone->cloneTo(*destination, true);
    destinationExecution.reset();
    destination.reset();
    clone.reset();
    QVERIFY(lifetime.expired());
    QVERIFY(!destinationLifetime.expired());
    QCOMPARE(replacement->readRgbaPixels(layer, QRect(4, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    replacement.reset();
    QVERIFY(destinationLifetime.expired());
    QVERIFY_EXCEPTION_THROWN(PaintImage(std::shared_ptr<Execution>{}, QSize(32, 32)), std::invalid_argument);
}

void KisGpuPaintImageTest::clonesKeepIndependentCurrentStateAndOperationIdentities()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    auto source = PaintImageSP::create(execution, QSize(64, 64));
    const auto layer = source->operations().activeLayer();
    const auto stroke = source->beginStroke(layer, Brush{}, 17);
    QCOMPARE(source->appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source->commitStroke(stroke), Result::Success);
    const auto hidden = source->addLayer(QStringLiteral("Hidden"));
    QVERIFY(hidden);
    QCOMPARE(source->writeRgbaPixels(hidden, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Result::Success);
    QCOMPARE(source->setLayerVisible(hidden, false), Result::Success);
    const auto before = source->operations().snapshot();
    const auto history = source->operations().undoCount();
    auto exact = source->clone(true);
    QVERIFY(exact);
    QVERIFY(exact->handle() != source->handle());
    QVERIFY(exact->operations().snapshot()->documentId() != before->documentId());
    QCOMPARE(exact->operations().undoCount(), 0);
    QCOMPARE(exact->operations().redoCount(), 0);
    QVERIFY(!exact->operations().isModified());
    QCOMPARE(exact->operations().snapshot()->layers[0].uuid, before->layers[0].uuid);
    QCOMPARE(exact->readRgbaPixels(layer, QRect(12, 12, 1, 1), 4).bytes, source->readRgbaPixels(layer, QRect(12, 12, 1, 1), 4).bytes);
    QCOMPARE(exact->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(exact->restoreRecovery(exact->operations().recoveryData()), Result::Success);
    QCOMPARE(exact->replaceRgbaPixels(layer, QRect(12, 12, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(source->operations().snapshot(), before);
    QCOMPARE(source->operations().undoCount(), history);
    auto renamed = source->clone(false);
    QVERIFY(renamed);
    for (int index = 0; index < before->layers.size(); ++index)
        QVERIFY(renamed->operations().snapshot()->layers[index].uuid != before->layers[index].uuid);
    Execution destination(device);
    auto transferred = source->cloneTo(destination, true);
    QVERIFY(transferred);
    QCOMPARE(&transferred->execution(), &destination);
    QCOMPARE(destination.requested(transferred->handle()), transferred->operations().snapshot());
    QCOMPARE(transferred->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    Execution full(device, 128 * 1024 * 1024, 1);
    PaintImage oneSlot(full, QSize(64, 64));
    const auto retained = oneSlot.operations().snapshot();
    QVERIFY_EXCEPTION_THROWN(oneSlot.clone(true), std::length_error);
    QVERIFY_EXCEPTION_THROWN(oneSlot.cloneLayerPixels(oneSlot.retainLayerPixels(oneSlot.operations().activeLayer())), std::length_error);
    QCOMPARE(oneSlot.operations().snapshot(), retained);
    QCOMPARE(full.requested(oneSlot.handle()), retained);
}

void KisGpuPaintImageTest::retainedLayerCopiesKeepIndependentPixelsAndExecutionLifetime()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Execution>(device);
    std::weak_ptr<Execution> lifetime = execution;
    auto source = PaintImageSP::create(execution, QSize(32, 32));
    const auto layer = source->addLayer(QStringLiteral("Retained copy"));
    const auto stroke = source->beginStroke(layer, Brush{}, 17);
    QCOMPARE(source->appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source->commitStroke(stroke), Result::Success);
    QCOMPARE(source->writeRgbaPixels(layer, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Result::Success);
    const auto pixels = source->retainLayerPixels(layer);
    const auto retained = pixels->version();
    const auto expected = execution->readRgbaPixels(source->handle(), retained, QRect(0, 0, 32, 32), layer, 4096).bytes;
    QCOMPARE(source->removeLayer(layer), Result::Success);
    const auto before = source->operations().snapshot();
    QVERIFY(!source->cloneLayerPixels({}));
    QVERIFY(!source->cloneLayerPixels(PaintImageSP::create(execution, QSize(1, 1))->retainLayerPixels(1)));
    auto copy = source->cloneLayerPixels(pixels);
    QVERIFY(copy);
    const auto version = copy->operations().snapshot();
    QCOMPARE(version->layers.size(), 1);
    QCOMPARE(version->activeLayer, layer);
    QCOMPARE(version->selectedLayers, QList<LayerId>{layer});
    QCOMPARE(version->sampleCount, retained->sampleCount);
    QCOMPARE(version->rasterBytes, retained->rasterBytes);
    QVERIFY(version->documentId() != retained->documentId());
    QCOMPARE(copy->operations().undoCount(), 0);
    QVERIFY(!copy->operations().isModified());
    QCOMPARE(copy->readRgbaPixels(layer, QRect(0, 0, 32, 32), 4096).bytes, expected);
    QCOMPARE(copy->readRgbaPixels(layer, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(copy->restoreRecovery(copy->operations().recoveryData()), Result::Success);
    QCOMPARE(copy->writeRgbaPixels(layer, QRect(-5, 7, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(execution->readRgbaPixels(source->handle(), retained, QRect(-5, 7, 1, 1), layer, 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(source->operations().snapshot(), before);
    source.reset();
    execution.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(copy->readRgbaPixels(layer, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    copy.reset();
    QVERIFY(lifetime.expired());
}

void KisGpuPaintImageTest::historyNavigationPublishesOneVersionAndPreservesSavedState()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.addLayer(QStringLiteral("Second layer"));
    QVERIFY(layer);
    QCOMPARE(image.renameLayer(layer, QStringLiteral("Saved layer")), Result::Success);
    QSignalSpy saved(&image, &PaintImage::savedStateChanged);
    QCOMPARE(image.markSaved(image.operations().snapshot()), Result::Success);
    QCOMPARE(saved.size(), 1);
    QCOMPARE(image.renameLayer(layer, QStringLiteral("Later layer")), Result::Success);
    const auto versions = image.operations().historyVersions();
    QCOMPARE(versions.size(), 4);
    QCOMPARE(image.operations().savedHistoryIndex(), 2);
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.setHistoryIndex(0), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(execution.requested(image.handle()), versions[0]);
    QCOMPARE(image.operations().redoCount(), 3);
    QVERIFY(image.operations().isModified());
    QCOMPARE(image.setHistoryIndex(-1), Result::InvalidInput);
    QCOMPARE(image.setHistoryIndex(4), Result::InvalidInput);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.setHistoryIndex(2), Result::Success);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(image.operations().historyVersions(), versions);
    const auto stroke = image.beginStroke(layer, {}, 17);
    QCOMPARE(image.appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    const auto pending = image.operations().preview();
    const int notifications = changed.size();
    QCOMPARE(image.setHistoryIndex(0), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), pending);
    QCOMPARE(changed.size(), notifications);
    QCOMPARE(image.cancelStroke(stroke), Result::Success);
    QCOMPARE(image.setHistoryIndex(3), Result::Success);
    QVERIFY(image.operations().isModified());
}

void KisGpuPaintImageTest::sharedEditsPublishBeforeNotificationAndReleaseTheLastAttachment()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    auto owner = PaintImageSP::create(execution, QSize(64, 64));
    auto reader = owner;
    const auto handle = owner->handle();
    QVERIFY(handle);
    QSignalSpy changed(owner.data(), &PaintImage::changed);
    bool currentAtNotification = true;
    connect(owner.data(), &PaintImage::changed, this, [&] {
        currentAtNotification &= execution.requested(handle) == reader->operations().preview();
    });
    owner.clear();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    const auto stroke = reader->beginStroke(reader->operations().activeLayer(), brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(reader->appendSamples(stroke, {{{12, 12}, 1}, {{32, 32}, 0.5}}), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(reader->commitStroke(stroke), Result::Success);
    QCOMPARE(reader->operations().undoCount(), 1);
    QVERIFY(reader->operations().isModified());
    const auto pixels = execution.readPixels(handle, reader->operations().snapshot(), {0, 0, 64, 64});
    QCOMPARE(pixels.code, ExecutionCode::Success);
    QCOMPARE(pixels.tiles.size(), 1);
    QCOMPARE(pixels.tiles.front().rgba.mid((12 * 256 + 12) * 4, 4), QByteArray::fromHex("1181f1ff"));
    const auto count = changed.size();
    QCOMPARE(reader->markSaved(reader->operations().snapshot()), Result::Success);
    QVERIFY(!reader->operations().isModified());
    QCOMPARE(changed.size(), count);
    QCOMPARE(reader->undo(), Result::Success);
    QCOMPARE(reader->redo(), Result::Success);
    QVERIFY(currentAtNotification);
    reader.clear();
    QVERIFY(!execution.requested(handle));
    QCOMPARE(execution.pendingDocuments(), qsizetype(0));
}

void KisGpuPaintImageTest::recoveryPreservesPendingInputAndRedoOnTheSameAttachment()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 128 * 1024 * 1024, 1);
    PaintImage image(execution, QSize(64, 64));
    const auto handle = image.handle();
    const auto previous = image.operations().snapshot();
    QSignalSpy changed(&image, &PaintImage::changed);
    OperationDocument source({64, 64});
    Brush red;
    red.rgba = {255, 0, 0, 255};
    const auto committed = source.beginStroke(source.activeLayer(), red, 1);
    QCOMPARE(source.appendSamples(committed, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source.commitStroke(committed), Result::Success);
    QCOMPARE(source.undo(), Result::Success);
    Brush blue;
    blue.rgba = {0, 0, 255, 255};
    const auto pending = source.beginStroke(source.activeLayer(), blue, 2);
    QCOMPARE(source.appendSamples(pending, {{{32, 32}, 1}}), Result::Success);
    const auto record = source.recoveryData();
    auto corrupt = record;
    corrupt[corrupt.size() - 1] ^= 1;
    QCOMPARE(image.restoreRecovery(corrupt), Result::InvalidRecovery);
    QCOMPARE(image.operations().snapshot(), previous);
    QCOMPARE(execution.requested(handle), previous);
    QVERIFY(changed.isEmpty());
    QCOMPARE(image.restoreRecovery(record), Result::Success);
    QCOMPARE(image.handle(), handle);
    QVERIFY(image.operations().snapshot()->documentId() != previous->documentId());
    QCOMPARE(changed.size(), 1);
    QCOMPARE(execution.requested(handle), image.operations().preview());
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 1);
    const auto preview = execution.readPixels(handle, image.operations().preview(), {0, 0, 64, 64});
    QCOMPARE(preview.code, ExecutionCode::Success);
    QCOMPARE(preview.tiles.size(), 1);
    QCOMPARE(preview.tiles.front().rgba.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(image.cancelStroke(image.operations().activeOperation()), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    image.close();
    QCOMPARE(image.handle(), quint64(0));
    QVERIFY(!execution.requested(handle));
    QCOMPARE(image.undo(), Result::Closed);
}

void KisGpuPaintImageTest::directPixelsReachGpuWithoutEditingNotificationsOrHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto layer = image.operations().activeLayer();
    const auto before = image.operations().snapshot();
    const RgbaTile pixels{{-1, -1}, QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(image.writeRgbaPixels(layer, {-256, -256, 256, 256}, pixels.rgba), Result::Success);
    QVERIFY(changed.isEmpty());
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 0);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(execution.requested(image.handle()), image.operations().snapshot());
    const auto read = execution.readPixels(image.handle(), image.operations().snapshot(), {-1, -1, 1, 1},
                                           {layer}, false, PixelReadArea::LayerCoordinates);
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.tiles.size(), 1);
    QCOMPARE(read.tiles.first().rgba, pixels.rgba);
    const auto old = execution.readPixels(image.handle(), before, {-1, -1, 1, 1},
                                          {layer}, false, PixelReadArea::LayerCoordinates);
    QCOMPARE(old.code, ExecutionCode::Success);
    QVERIFY(old.tiles.isEmpty());
    image.close();
    QCOMPARE(image.writeRgbaPixels(layer, {-256, -256, 256, 256}, pixels.rgba), Result::Closed);
}

void KisGpuPaintImageTest::rectangularWritesPreserveOtherRegionsAcrossHistoryAndRecovery_data()
{
    QTest::addColumn<QPoint>("source");
    QTest::addColumn<QPoint>("point");
    QTest::addColumn<bool>("mirrored");
    QTest::newRow("canvas") << QPoint(8, 12) << QPoint(8, 12) << false;
    QTest::newRow("negative-layer-pixels") << QPoint(-4, -4) << QPoint(-4, -4) << false;
    QTest::newRow("mirrored-outside-canvas") << QPoint(8, 12) << QPoint(-520, 12) << true;
}

void KisGpuPaintImageTest::rectangularWritesPreserveOtherRegionsAcrossHistoryAndRecovery()
{
    QFETCH(QPoint, source);
    QFETCH(QPoint, point);
    QFETCH(bool, mirrored);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage definition;
    definition.size = {128, 128};
    definition.layers = {{QStringLiteral("Edited"), true, {},
                          {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
                         {QStringLiteral("Imported"), true, {}, {}}};
    const QPoint tile(int(std::floor(qreal(point.x()) / RgbaTile::Size)),
                      int(std::floor(qreal(point.y()) / RgbaTile::Size)));
    if (!tile.isNull()) definition.layers[0].tiles.append({tile, definition.layers[0].tiles.first().rgba});
    PaintImage image(execution, definition);
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto pinned = image.operations().snapshot();
    Brush brush;
    brush.rgba = {0, 0, 255, 255};
    brush.diameter = 6;
    brush.mirrorHorizontal = mirrored;
    brush.mirrorCenter = {-256, 12};
    const auto stroke = image.beginStroke(1, brush, 0);
    QCOMPARE(image.appendSamples(stroke, {{QPointF(source), 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    const auto notifications = changed.size();
    const auto raw = QByteArray::fromHex("120a2700");
    const auto green = QByteArray::fromHex("00ff00ff");
    QCOMPARE(image.writeRgbaPixels(2, {-1, -1, 1, 1}, raw), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, QRect(point, QSize(1, 1)), raw), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, raw), Result::Success);
    QCOMPARE(changed.size(), notifications);
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 1);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, QRect(point, QSize(1, 1)), 1, 4).bytes, QByteArray::fromHex("ff0000ff"));
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, green), Result::Success);
    PaintImage restored(execution, QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(image.operations().recoveryData()), Result::Success);
    for (auto *owner : {&image, &restored}) {
        QCOMPARE(owner->redo(), Result::Success);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), QRect(point, QSize(1, 1)), 1, 4).bytes,
                 QByteArray::fromHex("0000ffff"));
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, green);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {-1, -1, 1, 1}, 2, 4).bytes, raw);
        QCOMPARE(owner->undo(), Result::Success);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), QRect(point, QSize(1, 1)), 1, 4).bytes, raw);
        const auto yellow = QByteArray::fromHex("ffff00ff");
        QCOMPARE(owner->writeRgbaPixels(1, {96, 96, 1, 1}, yellow), Result::Success);
        QCOMPARE(owner->redo(), Result::Success);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, yellow);
        QCOMPARE(owner->undo(), Result::Success);
        QVERIFY(!owner->operations().isModified());
    }
}

void KisGpuPaintImageTest::rectangularWritesValidateBeforeMutationAndCrossTileBoundaries()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 3 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
    PaintImage image(execution, QSize(64, 64));
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto layer = image.operations().activeLayer();
    const auto before = image.operations().snapshot();
    const QRect region(-1, -1, 2, 2);
    const auto pixels = QByteArray::fromHex("120a2700197fafe00000ffffff000080");
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels.first(15)), Result::InvalidInput);
    QCOMPARE(image.writeRgbaPixels(layer, {0, 0, -1, 2}, pixels), Result::InvalidInput);
    QCOMPARE(image.writeRgbaPixels(99, region, pixels), Result::LayerUnavailable);
    QCOMPARE(image.writeRgbaPixels(layer, {0, 0, 0, 0}, {}), Result::Success);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(execution.requested(image.handle()), before);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels), Result::Busy);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels + QByteArrayLiteral("ignored suffix")), Result::Success);
    const auto read = execution.readRgbaPixels(image.handle(), image.operations().snapshot(), region, layer, pixels.size());
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.bytes, pixels);
    QCOMPARE(image.operations().undoCount(), 0);
    QVERIFY(!image.operations().isModified());
    QVERIFY(changed.isEmpty());
    image.close();
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels), Result::Closed);
}

void KisGpuPaintImageTest::rectangularHistoryEditsPreserveIndependentUntrackedPixels()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage definition;
    definition.size = {128, 128};
    definition.layers = {{QStringLiteral("Edited"), true, {},
                          {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}}};
    PaintImage image(execution, definition);
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto blue = QByteArray::fromHex("0000ffff");
    const auto raw = QByteArray::fromHex("120a2700");
    QCOMPARE(image.replaceRgbaPixels(1, {8, 12, 1, 1}, blue), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(image.operations().isModified());
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, raw), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes, blue);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, raw);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes,
             QByteArray::fromHex("ff0000ff"));
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, raw);
    QVERIFY(!image.operations().isModified());
}

void KisGpuPaintImageTest::damagedPixelSharingRecoveryPreservesTheLiveImage()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    QCOMPARE(image.writeRgbaPixels(1, {8, 12, 1, 1}, QByteArray::fromHex("120a2700")), Result::Success);
    const auto before = image.operations().snapshot();
    const auto record = image.operations().recoveryData();
    QVERIFY(!record.isEmpty());
    QBuffer input;
    input.setData(record);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QVERIFY(input.seek(46));
    QDataStream reader(&input);
    reader.setVersion(QDataStream::Qt_5_15);
    reader.setByteOrder(QDataStream::BigEndian);
    quint32 tileCount = 0;
    reader >> tileCount;
    for (quint32 i = 0; i < tileCount; ++i) {
        qint32 x = 0, y = 0;
        quint32 size = 0;
        reader >> x >> y >> size;
        QVERIFY(input.seek(input.pos() + size));
    }
    quint32 operationCount = 0, count = 0;
    quint8 kind = 0;
    quint64 sequence = 0, version = 0, layer = 0;
    reader >> operationCount >> kind >> sequence >> version >> layer >> count;
    QCOMPARE(operationCount, quint32(1));
    QCOMPARE(kind, quint8(1));
    for (quint32 i = 0; i < count; ++i) { quint32 index = 0; reader >> index; }
    const auto groupPosition = input.pos();
    quint64 group = 0;
    quint32 areas = 0;
    reader >> group >> areas;
    QCOMPARE(areas, quint32(1));
    const auto areaPosition = input.pos();
    QSignalSpy changed(&image, &PaintImage::changed);
    for (int damage = 0; damage < 5; ++damage) {
        auto payload = record.first(record.size() - 32);
        QBuffer output(&payload);
        QVERIFY(output.open(QIODevice::ReadWrite));
        QVERIFY(output.seek(damage == 0 ? groupPosition : damage == 4 ? groupPosition + 8 : areaPosition));
        QDataStream writer(&output);
        writer.setVersion(QDataStream::Qt_5_15);
        writer.setByteOrder(QDataStream::BigEndian);
        if (damage == 0) writer << quint64(sequence + 1);
        if (damage == 1) writer << qint32(63) << qint32(12) << qint32(2) << qint32(1);
        if (damage == 2) writer << std::numeric_limits<qint32>::max() << std::numeric_limits<qint32>::max()
                                << std::numeric_limits<qint32>::max() << std::numeric_limits<qint32>::max();
        if (damage == 3) writer << qint32(256) << qint32(0) << qint32(1) << qint32(1);
        if (damage == 4) writer << std::numeric_limits<quint32>::max();
        output.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(image.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(image.operations().snapshot(), before);
        QCOMPARE(execution.requested(image.handle()), before);
        QCOMPARE(image.operations().recoveryData(), record);
        QVERIFY(changed.isEmpty());
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisGpuPaintImageTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPaintImageTest.moc"
