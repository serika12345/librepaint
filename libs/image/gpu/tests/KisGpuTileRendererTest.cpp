/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRendererTest.h"

void KisGpuTileRendererTest::markerMatchesNormalEngine_data()
{
    QTest::addColumn<bool>("sixteenBits");
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<bool>("erase");
    QTest::addColumn<bool>("moving");
    QTest::newRow("opaque-normal-tap") << false << qreal(1) << false << false;
    QTest::newRow("u16-half-erase-moving") << true << qreal(.5) << true << true;
}

void KisGpuTileRendererTest::markerMatchesNormalEngine()
{
    QFETCH(bool, sixteenBits);
    QFETCH(qreal, opacity);
    const auto format = sixteenBits ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const int pixelSize = pixelByteCount(format);
    const int tileBytes = RgbaTile::byteCount(format);
    QFETCH(bool, erase);
    QFETCH(bool, moving);
    const QDir fixtures(QStringLiteral(FILES_DATA_DIR) + QStringLiteral("/roundmarker"));
    const auto load = [&](const QString &name) {
        QFile file(fixtures.filePath(name));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    constexpr int side = 128;
    const auto initial = load(sixteenBits ? QStringLiteral("u16-initial.rgba") : QStringLiteral("initial.rgba"));
    const auto expected = load(QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba"));
    QCOMPARE(initial.size(), side * side * pixelSize);
    QCOMPARE(expected.size(), initial.size());
    QByteArray tile(tileBytes, '\0');
    for (int y = 0; y < side; ++y)
        std::memcpy(tile.data() + y * RgbaTile::Size * pixelSize, initial.constData() + y * side * pixelSize, side * pixelSize);
    RasterImage raster;
    raster.size = {side, side};
    raster.format = format;
    raster.layers.append({QStringLiteral("Mixed alpha"), true,
        QUuid(QStringLiteral("{1e75cc2a-b43b-4146-9216-ce12bc9ba3ae}")), {{{0, 0}, tile, format}}});
    OperationDocument document(raster);
    const auto layer = document.activeLayer();
    const auto before = document.snapshot();
    TileRenderer renderer(m_device);
    Brush brush;
    brush.rgba = sixteenBits ? RgbaPixel{65534, 257, 1, 65535} : RgbaPixel{17, 34, 51, 255};
    brush.diameter = 10;
    brush.spacing = .02;
    brush.opacity = opacity;
    brush.erase = erase;
    const auto id = document.beginStroke(layer, brush, 0);
    QVERIFY(!id.isNull());
    const QList<QPointF> positions{{12.25, 13.75}, {60.125, 31.5}, {105.5, 91.25}};
    RenderedSnapshot pending;
    for (int i = 0; i < (moving ? positions.size() : 1); ++i) {
        InputSample sample;
        sample.position = positions[i];
        sample.pressure = 1;
        sample.timestamp = i * 16;
        QCOMPARE(document.appendSamples(id, {sample}), Result::Success);
        pending = render(renderer, document.preview(), pending);
        QVERIFY(pending);
    }
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto final = render(renderer, document.snapshot(), pending);
    QVERIFY(final);
    const auto actualTile = readTile(final, layer, {0, 0});
    QCOMPARE(actualTile.size(), tileBytes);
    QByteArray actual;
    for (int y = 0; y < side; ++y) actual.append(actualTile.mid(y * RgbaTile::Size * pixelSize, side * pixelSize));
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), actualTile);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), tile);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), actualTile);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        for (const auto &entry : {std::pair{QStringLiteral("actual"), actual},
                                  std::pair{QStringLiteral("expected"), expected}}) {
            QFile file(QDir(directory).filePath(QStringLiteral("marker-%1-%2.rgba")
                .arg(QString::fromLatin1(QTest::currentDataTag()), entry.first)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), qint64(entry.second.size()));
        }
    }
    QCOMPARE_PIXELS_WITHIN_STEP(actual, expected);
}

void KisGpuTileRendererTest::basicCurveControlsPreserveClosedSegmentDecisions_data()
{
    QTest::addColumn<QPointF>("first");
    QTest::addColumn<QPointF>("last");
    QTest::addColumn<QPointF>("tangent1");
    QTest::addColumn<QPointF>("tangent2");
    QTest::addColumn<bool>("present");
    QTest::addColumn<QPointF>("expected1");
    QTest::addColumn<QPointF>("expected2");
    // Unquantized controls of the fixed Gaussian closed input keep its curve choice.
    QTest::newRow("closed-AB") << QPointF(0x1.88p+3, 0x1.b8p+3) << QPointF(0x1.e1p+5, 0x1.f8p+4) << QPointF(0x1.feaaaaaaaaaabp+0, 0x1.7aaaaaaaaaaabp-1) << QPointF(0x1.f155555555555p+0, 0x1.9d55555555555p+0)
        << true << QPointF(0x1.bb292a54b24b1p+4, 0x1.37a2faf94d778p+4) << QPointF(0x1.68a57010e8adfp+5, 0x1.2ff2bba60ec58p+4);
    QTest::newRow("closed-BC") << QPointF(0x1.e1p+5, 0x1.f8p+4) << QPointF(0x1.a6p+6, 0x1.6dp+6) << QPointF(0x1.f155555555555p+0, 0x1.9d55555555555p+0) << QPointF(-0x1.feaaaaaaaaaabp-1, -0x1.7aaaaaaaaaaabp-2)
        << true << QPointF(0x1.0d73333333346p+7, 0x1.760000000001fp+6) << QPointF(0x1.f2999999999acp+6, 0x1.8966666666676p+6);
    QTest::newRow("closed-CA") << QPointF(0x1.a6p+6, 0x1.6dp+6) << QPointF(0x1.88p+3, 0x1.b8p+3) << QPointF(-0x1.feaaaaaaaaaabp-1, -0x1.7aaaaaaaaaaabp-2) << QPointF(-0x1.f155555555555p+1, -0x1.9d55555555555p+1)
        << true << QPointF(0x1.a6p+6, 0x1.6dp+6) << QPointF(0x1.5b66666666666p+6, 0x1.2efffffffffffp+6);
    QTest::newRow("closed-endpoint") << QPointF(0, 0) << QPointF(10, 0)
        << QPointF(5, 0) << QPointF(3, -4) << true << QPointF(3.2, 0) << QPointF(8.08, 2.56);
    QTest::newRow("closed-endpoint-reversed") << QPointF(10, 0) << QPointF(0, 0)
        << QPointF(-3, 4) << QPointF(-5, 0) << true << QPointF(8.08, 2.56) << QPointF(3.2, 0);
    QTest::newRow("parallel-midpoint") << QPointF(0, 0) << QPointF(10, 0)
        << QPointF(3, 4) << QPointF(-3, -4) << true << QPointF(3.2, 0) << QPointF(6.8, 0);
    QTest::newRow("zero-tangent") << QPointF(0, 0) << QPointF(10, 0)
        << QPointF() << QPointF(3, 4) << false << QPointF() << QPointF();
}

void KisGpuTileRendererTest::basicCurveControlsPreserveClosedSegmentDecisions()
{
    QFETCH(QPointF, first);
    QFETCH(QPointF, last);
    QFETCH(QPointF, tangent1);
    QFETCH(QPointF, tangent2);
    QFETCH(bool, present);
    QFETCH(QPointF, expected1);
    QFETCH(QPointF, expected2);
    const auto controls = basicCurveControls(first, last, tangent1, tangent2);
    QCOMPARE(controls.has_value(), present);
    if (controls) {
        QCOMPARE((*controls)[0], expected1);
        QCOMPARE((*controls)[1], expected2);
    }
}

void KisGpuTileRendererTest::gaussianPixelCorrectionKeepsCpuExtent_data()
{
    QTest::addColumn<int>("index");
    const auto cases = Test::gaussianPixelPerfectCases();
    for (qsizetype i = 0; i < cases.size(); ++i) QTest::newRow(cases[i].name) << int(i);
}

void KisGpuTileRendererTest::gaussianPixelCorrectionKeepsCpuExtent()
{
    QFETCH(int, index);
    qInfo() << "Gaussian pixel correction platform" << QGuiApplication::platformName();
    const auto test = Test::gaussianPixelPerfectCases()[index];
    QCOMPARE(test.rgba.size(), test.extent.width() * test.extent.height() * 4);
    auto brush = Test::defaultGaussianBrush();
    QVERIFY(brush.autoBrush);
    brush.autoBrush->flow = .57;
    brush.pixelPerfect = true;
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer paintLayer; paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{33b00011-1122-3344-5566-778899aabbcc}"));
    canvas.layers = {paintLayer};
    OperationDocument document(canvas);
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 0);
    QVERIFY(!id.isNull());
    TileRenderer renderer(m_device);
    using Tiles = QMap<QPair<int, int>, QByteArray>;
    const auto tileIndex = [](int value) {
        return value >= 0 ? value / RgbaTile::Size : (value - RgbaTile::Size + 1) / RgbaTile::Size;
    };
    const auto tiles = [&](const RenderedSnapshot &version, LayerId target) {
        Tiles result;
        if (version) for (const auto &tile : version->tiles(target))
            result.insert({tile.coordinate.x(), tile.coordinate.y()}, readTile(version, target, tile.coordinate));
        return result;
    };
    const auto region = [&](const Tiles &source, QRect bounds) {
        QByteArray result(bounds.width() * bounds.height() * 4, '\0');
        for (int y = bounds.top(); y <= bounds.bottom(); ++y) for (int x = bounds.left(); x <= bounds.right(); ++x) {
            const int tx = tileIndex(x), ty = tileIndex(y);
            const auto found = source.constFind({tx, ty});
            if (found == source.cend() || found->size() != RgbaTile::ByteCount) continue;
            const qsizetype offset = ((y - ty * RgbaTile::Size) * RgbaTile::Size + x - tx * RgbaTile::Size) * 4;
            std::memcpy(result.data() + ((y - bounds.y()) * bounds.width() + x - bounds.x()) * 4,
                        found->constData() + offset, 4);
        }
        return result;
    };
    Tiles expected;
    for (int y = 0; y < test.extent.height(); ++y) for (int x = 0; x < test.extent.width(); ++x) {
        const auto pixel = test.rgba.mid((y * test.extent.width() + x) * 4, 4);
        if (pixel == QByteArray(4, '\0')) continue;
        const int gx = test.extent.x() + x, gy = test.extent.y() + y;
        const int tx = tileIndex(gx), ty = tileIndex(gy);
        auto found = expected.find({tx, ty});
        if (found == expected.end()) found = expected.insert({tx, ty}, QByteArray(RgbaTile::ByteCount, '\0'));
        const qsizetype offset = ((gy - ty * RgbaTile::Size) * RgbaTile::Size + gx - tx * RgbaTile::Size) * 4;
        std::memcpy(found->data() + offset, pixel.constData(), 4);
    }
    QVERIFY(!expected.isEmpty());
    QMap<QString, QByteArray> artifacts{{QStringLiteral(".expected.full-extent.rgba"), test.rgba}};
    QJsonArray inputs, tileCoordinates, pendingTileCoordinates;
    for (const auto &sample : test.input)
        inputs.append(QJsonObject{{"x", sample.position.x()}, {"y", sample.position.y()},
            {"pressure", sample.pressure}, {"xTilt", sample.xTilt}, {"yTilt", sample.yTilt},
            {"rotation", sample.rotation}, {"elapsed", qint64(sample.timestamp)}, {"speed", sample.speed}});
    QString phase = QStringLiteral("begin");
    qsizetype completedInputs = 0;
    const auto retain = qScopeGuard([&] {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        artifacts[QStringLiteral(".input.json")] = QJsonDocument(QJsonObject{
            {"platform", QGuiApplication::platformName()}, {"phase", phase},
            {"completedInputs", int(completedInputs)}, {"input", inputs},
            {"extent", QJsonArray{test.extent.x(), test.extent.y(), test.extent.width(), test.extent.height()}},
            {"tileCoordinates", tileCoordinates}, {"pendingTileCoordinates", pendingTileCoordinates},
            {"tileSize", RgbaTile::Size}, {"flow", brush.autoBrush->flow},
            {"opacity", brush.opacity}, {"failed", QTest::currentTestFailed()}}).toJson();
        for (auto it = artifacts.cbegin(); it != artifacts.cend(); ++it) {
            QFile file(QDir(directory).filePath(QStringLiteral("gaussian-pixel-perfect-") +
                QString::fromLatin1(test.name) + it.key()));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(it.value()), qint64(it.value().size()));
        }
    });
    const auto packedTiles = [](const Tiles &source, QJsonArray &coordinates) {
        QByteArray result;
        for (auto it = source.cbegin(); it != source.cend(); ++it) {
            coordinates.append(QJsonArray{it.key().first, it.key().second});
            result.append(it.value());
        }
        return result;
    };
    RenderedSnapshot incremental;
    QList<RenderedSnapshot> retained;
    QList<Snapshot> retainedIntents;
    QList<Tiles> retainedPixels;
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        completedInputs = i + 1;

        phase = QStringLiteral("input-%1-render").arg(i);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY2(incremental, "Gaussian pixel-corrected pending rendering must be supported");
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        const auto pending = tiles(incremental, layer);
        QVERIFY(pending == tiles(rebuilt, layer));
        // A raw tap, and every still-single corrected prefix, retain Gaussian's
        // empty stationary preview. A held corner is not flushed on release.
        const auto name = QByteArray(test.name);
        if (i < 2 || name.startsWith("static") || name == "held-corner" ||
            name == "initial-tile-removed" || (name == "flushed-corner" && i < 3))
            QVERIFY(pending.isEmpty());
        retained.append(incremental);
        retainedIntents.append(document.preview());
        retainedPixels.append(pending);
    }
    const auto pending = tiles(incremental, layer);
    artifacts[QStringLiteral(".pending.tiles.rgba")] = packedTiles(pending, pendingTileCoordinates);

    phase = QStringLiteral("pending-rebuild");
    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QVERIFY(tiles(restoredPending, document.activeLayer()) == pending);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto retainedVersion = document.preview();

    phase = QStringLiteral("committed-render");
    const auto final = render(renderer, document.snapshot(), restoredPending);
    QVERIFY(final);
    const auto actual = tiles(final, document.activeLayer());
    artifacts[QStringLiteral(".tiles.rgba")] = packedTiles(actual, tileCoordinates);
    artifacts[QStringLiteral(".full-extent.rgba")] = region(actual, test.extent);
    artifacts[QStringLiteral(".rgba")] = region(actual, {0, 0, 128, 128});
    // Whole native tiles include zeros outside the CPU extent and expose any
    // abandoned first tile or extra off-canvas print, beyond the logical canvas.
    QCOMPARE(actual.keys(), expected.keys());
    for (auto tile = expected.cbegin(); tile != expected.cend(); ++tile)
        QCOMPARE_PIXELS_WITHIN_STEP(actual.value(tile.key()), tile.value());
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    phase = QStringLiteral("committed-history");

    QCOMPARE(document.preview(), retainedVersion);
    const auto replayed = render(renderer, document.snapshot());
    QVERIFY(replayed);
    QVERIFY(tiles(replayed, document.activeLayer()) == actual);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), final);
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QVERIFY(tiles(redone, document.activeLayer()) == actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    const auto cancelled = render(renderer, document.snapshot(), incremental);
    QVERIFY(cancelled);
    QVERIFY(cancelled->tiles(layer).isEmpty());
    for (qsizetype i = 0; i < retained.size(); ++i) {
        QVERIFY(tiles(retained[i], layer) == retainedPixels[i]);
        const auto replay = render(renderer, retainedIntents[i]);
        QVERIFY(replay);
        QVERIFY(tiles(replay, layer) == retainedPixels[i]);
    }
    phase = QStringLiteral("complete");
}

void KisGpuTileRendererTest::gaussianWeightedKeepsCpuPixels_data()
{
    QTest::addColumn<int>("index");
    const auto cases = Test::gaussianWeightedCases();
    for (qsizetype i = 0; i < cases.size(); ++i) QTest::newRow(cases[i].name) << int(i);
}

void KisGpuTileRendererTest::gaussianWeightedKeepsCpuPixels()
{
    QFETCH(int, index);
    qInfo() << "Gaussian Weighted platform" << QGuiApplication::platformName();
    const auto test = Test::gaussianWeightedCases()[index];
    QCOMPARE(test.rgba.size(), 128 * 128 * 4);
    auto brush = Test::defaultGaussianBrush();
    QVERIFY(brush.autoBrush);
    // Same captured Basic-5 resource; the observed CPU recipe changes FlowValue.
    brush.autoBrush->flow = .57;
    brush.basicSmoothing = true;
    brush.weightedSmoothing = test.smoothing;
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer paintLayer; paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{33b00010-1122-3344-5566-778899aabbcc}"));
    canvas.layers = {paintLayer};
    OperationDocument document(canvas);
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 0);
    QVERIFY(!id.isNull());
    TileRenderer renderer(m_device);
    QMap<QString, QByteArray> artifacts{{QStringLiteral(".expected.rgba"), test.rgba}};
    QJsonArray input;
    for (const auto &sample : test.input)
        input.append(QJsonObject{{"x", sample.position.x()}, {"y", sample.position.y()},
            {"pressure", sample.pressure}, {"xTilt", sample.xTilt}, {"yTilt", sample.yTilt},
            {"rotation", sample.rotation}, {"elapsed", qint64(sample.timestamp)}, {"speed", sample.speed}});
    QJsonArray flushes;
    for (const auto count : test.flushes) flushes.append(int(count));
    QString phase = QStringLiteral("begin");
    qsizetype completedInputs = 0;
    bool lastRenderValid = false;
    const auto retain = qScopeGuard([&] {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        artifacts[QStringLiteral(".input.json")] = QJsonDocument(QJsonObject{
            {"platform", QGuiApplication::platformName()}, {"phase", phase},
            {"completedInputs", int(completedInputs)}, {"input", input}, {"flushes", flushes},
            {"distanceMin", test.smoothing.distanceMin}, {"distanceMax", test.smoothing.distanceMax},
            {"tail", test.smoothing.tailAggressiveness}, {"smoothPressure", test.smoothing.smoothPressure},
            {"flow", brush.autoBrush->flow}, {"opacity", brush.opacity},
            {"lastRenderValid", lastRenderValid}, {"failed", QTest::currentTestFailed()},
            {"actualFinalPixelsSaved", artifacts.contains(QStringLiteral(".rgba"))}}).toJson();
        for (auto it = artifacts.cbegin(); it != artifacts.cend(); ++it) {
            QFile file(QDir(directory).filePath(QStringLiteral("gaussian-weighted-") +
                QString::fromLatin1(test.name) + it.key()));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(it.value()), qint64(it.value().size()));
        }
    });
    const auto pixels = [&](const RenderedSnapshot &version, LayerId target) {
        if (!version) return QByteArray();
        const auto tile = readTile(version, target, {0, 0});
        if (tile.isEmpty()) return QByteArray(128 * 128 * 4, '\0');
        if (tile.size() != RgbaTile::ByteCount) return QByteArray();
        QByteArray bytes;
        for (int y = 0; y < 128; ++y) bytes.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
        return bytes;
    };
    RenderedSnapshot incremental;
    QList<RenderedSnapshot> retained;
    QList<Snapshot> retainedIntents;
    QList<QByteArray> retainedPixels;
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        completedInputs = i + 1;

        phase = QStringLiteral("input-%1-render").arg(i);
        incremental = render(renderer, document.preview(), incremental);
        lastRenderValid = bool(incremental);
        QVERIFY2(incremental, "Gaussian Weighted pending rendering must be supported");
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        const auto pendingPixels = pixels(incremental, layer);
        QCOMPARE(pendingPixels.size(), 128 * 128 * 4);
        QCOMPARE(pendingPixels, pixels(rebuilt, layer));
        retainedIntents.append(document.preview());
        retained.append(incremental);
        retainedPixels.append(pendingPixels);
        if (test.flushes.contains(i + 1)) {
            phase = QStringLiteral("input-%1-flush").arg(i);
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            const auto once = document.preview();
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            QCOMPARE(document.preview(), once);

            incremental = render(renderer, document.preview(), incremental);
            QVERIFY(incremental);
            QCOMPARE(pixels(incremental, layer), pixels(render(renderer, document.preview()), layer));
            retainedIntents.append(document.preview());
            retained.append(incremental);
            retainedPixels.append(pixels(incremental, layer));
        }
    }
    const auto pendingPixels = pixels(incremental, layer);
    artifacts[QStringLiteral(".pending.rgba")] = pendingPixels;
    const auto retainedVersion = document.preview();
    phase = QStringLiteral("pending-rebuild");

    QCOMPARE(document.preview(), retainedVersion);
    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QCOMPARE(pixels(restoredPending, document.activeLayer()), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto capturedVersion = document.preview();

    phase = QStringLiteral("committed-render");
    const auto final = render(renderer, document.snapshot(), restoredPending);
    lastRenderValid = bool(final);
    QVERIFY(final);
    const auto actual = pixels(final, document.activeLayer());
    artifacts[QStringLiteral(".rgba")] = actual;
    QCOMPARE_PIXELS_WITHIN_STEP(actual, test.rgba);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    QVERIFY(operation->smoothingFinished);
    QCOMPARE(operation->smoothingFlushes, test.flushes);
    phase = QStringLiteral("committed-history");

    QCOMPARE(document.preview(), capturedVersion);
    QCOMPARE(pixels(render(renderer, document.snapshot()), document.activeLayer()), actual);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(pixels(render(renderer, document.snapshot(), final), document.activeLayer()), QByteArray(actual.size(), '\0'));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(pixels(render(renderer, document.snapshot()), document.activeLayer()), actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(pixels(render(renderer, document.snapshot(), incremental), layer), QByteArray(actual.size(), '\0'));
    for (qsizetype i = 0; i < retained.size(); ++i) {
        QCOMPARE(pixels(retained[i], layer), retainedPixels[i]);
        QCOMPARE(pixels(render(renderer, retainedIntents[i]), layer), retainedPixels[i]);
    }
    phase = QStringLiteral("complete");
}

void KisGpuTileRendererTest::gaussianWeightedOnImportedPixelKeepsCpuMergeByte()
{
    qInfo() << "Gaussian Weighted imported pixel platform" << QGuiApplication::platformName();
    const auto test = Test::gaussianWeightedImportedPixelCase();
    const QPoint coordinate(-1, 0);
    const qsizetype offset = (5 * RgbaTile::Size + 247) * 4;
    const auto beforePixel = QByteArray::fromHex("9d1e656d");
    auto brush = Test::defaultGaussianBrush();
    QVERIFY(brush.autoBrush);
    brush.rgba = {83, 177, 41, 255};
    brush.autoBrush->flow = .57;
    brush.opacity = .57;
    brush.basicSmoothing = true;
    brush.weightedSmoothing = test.smoothing;
    QMap<QString, QByteArray> artifacts{{QStringLiteral(".expected.rgba"), test.rgba},
                                      {QStringLiteral(".before.rgba"), beforePixel}};
    QString phase = QStringLiteral("begin");
    QJsonArray input;
    for (const auto &sample : test.input) input.append(QJsonObject{
        {"x", sample.position.x()}, {"y", sample.position.y()}, {"pressure", sample.pressure},
        {"xTilt", sample.xTilt}, {"yTilt", sample.yTilt}, {"rotation", sample.rotation},
        {"elapsed", qint64(sample.timestamp)}, {"speed", sample.speed}});
    const auto retain = qScopeGuard([&] {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        artifacts[QStringLiteral(".input.json")] = QJsonDocument(QJsonObject{
            {"platform", QGuiApplication::platformName()}, {"phase", phase}, {"input", input},
            {"flushes", QJsonArray{5, 6}}, {"x", -9}, {"y", 5},
            {"brushRgba", QJsonArray{83, 177, 41, 255}}, {"diameter", brush.diameter},
            {"spacing", brush.spacing},
            {"distanceMin", test.smoothing.distanceMin}, {"distanceMax", test.smoothing.distanceMax},
            {"tail", test.smoothing.tailAggressiveness}, {"smoothPressure", test.smoothing.smoothPressure},
            {"flow", brush.autoBrush->flow}, {"opacity", brush.opacity},
            {"creamy", brush.autoBrush->creamy}, {"failed", QTest::currentTestFailed()}}).toJson();
        for (auto it = artifacts.cbegin(); it != artifacts.cend(); ++it) {
            QFile file(QDir(directory).filePath(QStringLiteral("gaussian-weighted-imported-pixel") + it.key()));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(it.value()), qint64(it.value().size()));
        }
    });
    RgbaTile background{coordinate, QByteArray(RgbaTile::ByteCount, '\0')};
    background.rgba.replace(offset, 4, beforePixel);
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer layer; layer.name = QStringLiteral("Paint Layer 1");
    layer.uuid = QUuid(QStringLiteral("{33b00011-1122-3344-5566-778899aabbcc}"));
    layer.tiles = {background}; canvas.layers = {layer};
    OperationDocument document(canvas);
    TileRenderer renderer(m_device);
    const auto imported = render(renderer, document.snapshot());
    QVERIFY(imported);
    QCOMPARE(readTile(imported, document.activeLayer(), coordinate), background.rgba);
    const auto id = document.beginStroke(document.activeLayer(), brush, 0);
    QVERIFY(!id.isNull());
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        if (test.flushes.contains(i + 1)) {
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            const auto once = document.preview();
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            QCOMPARE(document.preview(), once);
        }
    }
    const auto retainedVersion = document.preview();

    phase = QStringLiteral("pending-render");
    const auto pending = render(renderer, document.preview(), imported);
    QVERIFY(pending);
    const auto pendingPixels = readTile(pending, document.activeLayer(), coordinate);
    artifacts[QStringLiteral(".pending.tile.rgba")] = pendingPixels;
    phase = QStringLiteral("pending-rebuild");

    QCOMPARE(document.preview(), retainedVersion);
    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QCOMPARE(readTile(restoredPending, document.activeLayer(), coordinate), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto capturedVersion = document.preview();

    phase = QStringLiteral("committed-render");
    const auto final = render(renderer, document.snapshot(), restoredPending);
    QVERIFY(final);
    const auto pixels = readTile(final, document.activeLayer(), coordinate);
    QCOMPARE(pixels.size(), RgbaTile::ByteCount);
    artifacts[QStringLiteral(".tile.rgba")] = pixels;
    const auto actual = pixels.mid(offset, 4);
    artifacts[QStringLiteral(".rgba")] = actual;
    QCOMPARE_PIXELS_WITHIN_STEP(actual, test.rgba);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    QCOMPARE(operation->smoothingFlushes, test.flushes);
    QVERIFY(operation->smoothingFinished);
    phase = QStringLiteral("committed-history");

    QCOMPARE(document.preview(), capturedVersion);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), coordinate), pixels);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), final), document.activeLayer(), coordinate), background.rgba);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), coordinate), pixels);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(readTile(render(renderer, document.snapshot(), pending), document.activeLayer(), coordinate), background.rgba);
    QCOMPARE(readTile(imported, document.activeLayer(), coordinate), background.rgba);
    QCOMPARE(readTile(pending, document.activeLayer(), coordinate), pendingPixels);
    phase = QStringLiteral("complete");
}

void KisGpuTileRendererTest::gaussianBasicKeepsCpuPixelsFlushes_data()
{
    QTest::addColumn<int>("index");
    const auto cases = Test::gaussianBasicCases();
    for (qsizetype i = 0; i < cases.size(); ++i) QTest::newRow(cases[i].name) << int(i);
}

void KisGpuTileRendererTest::gaussianBasicKeepsCpuPixelsFlushes()
{
    QFETCH(int, index);
    const auto test = Test::gaussianBasicCases()[index];
    auto brush = Test::defaultGaussianBrush();
    brush.basicSmoothing = true;
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer paintLayer; paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{33b00009-1122-3344-5566-778899aabbcc}"));
    canvas.layers = {paintLayer};
    OperationDocument document(canvas);
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 0);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    QList<RenderedSnapshot> retained;
    QList<Snapshot> retainedIntents;
    QList<QByteArray> retainedPixels;
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(readTile(incremental, layer, {0, 0}), readTile(rebuilt, layer, {0, 0}));
        if (i < 2) QVERIFY(incremental->tiles(layer).isEmpty());
        retainedIntents.append(document.preview());
        retained.append(incremental);
        retainedPixels.append(readTile(incremental, layer, {0, 0}));
        if (i + 1 == test.flushAt) {
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            incremental = render(renderer, document.preview(), incremental);
            QVERIFY(incremental && !incremental->tiles(layer).isEmpty());
            retainedIntents.append(document.preview());
            retained.append(incremental);
            retainedPixels.append(readTile(incremental, layer, {0, 0}));
        }
    }
    const auto pendingPixels = readTile(incremental, layer, {0, 0});

    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QCOMPARE(readTile(restoredPending, document.activeLayer(), {0, 0}), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto final = render(renderer, document.snapshot(), restoredPending);
    QVERIFY(final);
    const auto tile = readTile(final, document.activeLayer(), {0, 0});
    QCOMPARE(tile.size(), RgbaTile::ByteCount);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto save = [&](const QString &suffix, const QByteArray &bytes) {
        QFile output(QDir(directory).filePath(QStringLiteral("gaussian-basic-") + QString::fromLatin1(test.name) + suffix));
        return output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size();
    };
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(save(QStringLiteral(".rgba"), pixels));
    }
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), test.rgbaSha256);
    if (QByteArray(test.name).startsWith("stationary")) {
        OperationDocument tap(QSize(128, 128));
        const auto tapId = tap.beginStroke(tap.activeLayer(), brush, 0);
        QCOMPARE(tap.appendSamples(tapId, {test.input.last()}), Result::Success);
        QCOMPARE(tap.commitStroke(tapId), Result::Success);
        QCOMPARE(readTile(render(renderer, tap.snapshot()), tap.activeLayer(), {0, 0}), tile);
    }
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    QCOMPARE(operation->smoothingFlushes, test.flushAt ? QList<qsizetype>{test.flushAt} : QList<qsizetype>{});
    QVERIFY(operation->smoothingFinished);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), final)->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), incremental)->tiles(layer).isEmpty());
    QCOMPARE(document.undoCount(), 0);
    for (qsizetype i = 0; i < retained.size(); ++i) {
        QCOMPARE(readTile(retained[i], layer, {0, 0}), retainedPixels[i]);
        const auto rebuilt = render(renderer, retainedIntents[i]);
        QVERIFY(rebuilt);
        QCOMPARE(readTile(rebuilt, layer, {0, 0}), retainedPixels[i]);
    }
    // Eraser mode consumes the same spatial mask and preserves hidden RGB.
    auto maskBrush = brush;
    maskBrush.opacity = 1;
    OperationDocument mask(canvas);
    const auto maskId = mask.beginStroke(mask.activeLayer(), maskBrush, 0);
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(mask.appendSamples(maskId, {test.input[i]}), Result::Success);
        if (i + 1 == test.flushAt) QCOMPARE(mask.flushSmoothing(maskId), Result::Success);
    }
    QCOMPARE(mask.commitStroke(maskId), Result::Success);
    const auto maskRendered = render(renderer, mask.snapshot()); QVERIFY(maskRendered);
    const auto maskTile = readTile(maskRendered, mask.activeLayer(), {0, 0});
    QCOMPARE(maskTile.size(), RgbaTile::ByteCount);
    // CPU RGBA8 erase rounds .57 * 255 to 145, then multiplies mask and
    // destination alpha separately with round-to-nearest 8-bit products.
    const quint32 opacityByte = quint32(qRound(float(brush.opacity) * 255));
    QCOMPARE(opacityByte, 145u);
    const auto multiplyAlpha = [](quint32 a, quint32 b) { return (a * b + 127) / 255; };
    auto eraseBrush = brush;
    eraseBrush.erase = true;
    RgbaTile background{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        auto *p = background.rgba.data() + 4 * (y * RgbaTile::Size + x);
        p[0] = char(x * 7 + y * 13); p[1] = char(x * 5 + y * 11); p[2] = char(x * 17 + y * 3);
        p[3] = char((x + y) % 256);
    }
    RasterImage raster; raster.size = {128, 128};
    RasterLayer rasterLayer; rasterLayer.name = QStringLiteral("Paint");
    rasterLayer.uuid = QUuid(QStringLiteral("{33b00008-1122-3344-5566-778899aabbcc}"));
    rasterLayer.tiles = {background}; raster.layers = {rasterLayer};
    OperationDocument erase(raster);
    const auto eraseId = erase.beginStroke(erase.activeLayer(), eraseBrush, 0);
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(erase.appendSamples(eraseId, {test.input[i]}), Result::Success);
        if (i + 1 == test.flushAt) QCOMPARE(erase.flushSmoothing(eraseId), Result::Success);
    }

    QCOMPARE(erase.commitStroke(erase.activeOperation()), Result::Success);
    const auto erased = render(renderer, erase.snapshot()); QVERIFY(erased);
    const auto erasedTile = readTile(erased, erase.activeLayer(), {0, 0});
    QCOMPARE(erasedTile.size(), background.rgba.size());
    bool changedAlpha = false;
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        const auto offset = 4 * (y * RgbaTile::Size + x);
        QCOMPARE(erasedTile.mid(offset, 3), background.rgba.mid(offset, 3));
        const auto expectedAlpha = multiplyAlpha(quint8(background.rgba[offset + 3]),
            255 - multiplyAlpha(quint8(maskTile[offset + 3]), opacityByte));
        QCOMPARE(quint32(quint8(erasedTile[offset + 3])), expectedAlpha);
        changedAlpha = changedAlpha || erasedTile[offset + 3] != background.rgba[offset + 3];
    }
    QVERIFY(changedAlpha);
    const auto erasePaint = std::get<PaintSnapshot>(erase.snapshot()->layers.first().operations.last());
    QCOMPARE(erasePaint->inputSamples(), test.input); QVERIFY(erasePaint->brush.erase);
    QCOMPARE(erase.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, erase.snapshot(), erased), erase.activeLayer(), {0, 0}), background.rgba);
    QCOMPARE(erase.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, erase.snapshot()), erase.activeLayer(), {0, 0}), erasedTile);
    if (!directory.isEmpty()) {
        QByteArray pendingRgba, eraseRgba;
        for (int y = 0; y < 128; ++y) {
            pendingRgba.append(pendingPixels.isEmpty() ? QByteArray(128 * 4, '\0') :
                               pendingPixels.mid(y * RgbaTile::Size * 4, 128 * 4));
            eraseRgba.append(erasedTile.mid(y * RgbaTile::Size * 4, 128 * 4));
        }
        QVERIFY(save(QStringLiteral(".pending.rgba"), pendingRgba));
        QVERIFY(save(QStringLiteral(".erase.rgba"), eraseRgba));

    }
}

void KisGpuTileRendererTest::gaussianFreehandKeepsInitialDistanceAndFinalTap_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QByteArray>("expectedSha256");
    // Actual NO_SMOOTHING mouse tool at 62cfadd521, shipped Basic-5,
    // full pressure, flow .5, canvas opacity .57 and the fixed alpha gradient.
    QTest::newRow("tap") << 0 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("two") << 1 << QByteArrayLiteral("4141da91dfe824fa1fe8c468c369e318bb62aac2da13a825aa936f020601513f");
    QTest::newRow("curve") << 2 << QByteArrayLiteral("9768513405d0c7bb276ff79b7a6475bbd15f76122c16cd02d90a857c45e450f2");
    QTest::newRow("closed") << 3 << QByteArrayLiteral("16a05f0a9afe0944736109eb6359ef301c5126040dafb51d4e178f9576032413");
    // Repeated stationary input uses the maintained single-tap baseline.
    QTest::newRow("stationary-two") << -2 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("stationary") << -3 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("stationary-eight") << -8 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("duplicate-first") << 5 << QByteArrayLiteral("6149e497bf814729e64b66d409c9ee605ef32900d4970afc0103080413c99725");
    QTest::newRow("return") << 6 << QByteArrayLiteral("add58eb08c2539e1fa3db420d884a0a21b835dc8b5b6d98a141ee36974879294");
    QTest::newRow("subspacing") << 7 << QByteArrayLiteral("dc1393988897084bb62a1142bc4d7fc6c4d3b03c10fafe253824863cb311901f");
}

void KisGpuTileRendererTest::gaussianFreehandKeepsInitialDistanceAndFinalTap()
{
    QFETCH(int, kind);
    QFETCH(QByteArray, expectedSha256);
    const auto brush = Test::defaultGaussianBrush();
    QList<InputSample> input{{{12.25, 13.75}, 1}, {{60.125, 31.5}, 1}, {{105.5, 91.25}, 1}};
    if (kind == 0) input = input.first(1);
    else if (kind == 1) input = input.first(2);
    else if (kind == 3) input.append(input.first());
    else if (kind < 0) input = QList<InputSample>(-kind, input.first());
    else if (kind == 5) input = {input.first(), input.first(), input.last()};
    else if (kind == 6) input = {input.first(), input[1], input.first()};
    else if (kind == 7) input = {input.first(), {{input.first().position.x() + 1e-12, input.first().position.y()}, 1}};
    for (qsizetype i = 0; i < input.size(); ++i) {
        input[i].timestamp = i * 16;
        if (kind < 0) {
            input[i].pressure = qreal(i + 1) / input.size();
            input[i].xTilt = i * 7 - 28;
            input[i].yTilt = 14 - i * 3;
            input[i].rotation = i * 11;
            input[i].speed = qreal(i) / input.size();
        }
    }
    RgbaTile tile{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
    const std::array<int, 8> alphas{{0, 1, 2, 63, 127, 128, 254, 255}};
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        auto *pixel = tile.rgba.data() + 4 * (y * RgbaTile::Size + x);
        pixel[0] = char((x * 7 + y * 13) % 256);
        pixel[1] = char((x * 5 + y * 11) % 256);
        pixel[2] = char((x * 17 + y * 3) % 256);
        pixel[3] = char(alphas[(x / 16 + y / 16) % alphas.size()]);
    }
    RasterImage image;
    image.size = {128, 128};
    RasterLayer layer;
    layer.name = QStringLiteral("Paint");
    layer.uuid = QUuid(QStringLiteral("{33b00004-1122-3344-5566-778899aabbcc}"));
    layer.tiles = {tile};
    image.layers = {layer};
    OperationDocument document(image);
    const auto target = document.activeLayer();
    const auto stroke = document.beginStroke(target, brush, 0);
    TileRenderer renderer(m_device);
    RenderedSnapshot preview;
    QList<Snapshot> retainedIntents;
    QList<RenderedSnapshot> retainedPreviews;
    QList<QByteArray> retainedPixels;
    for (qsizetype i = 0; i < input.size(); ++i) {
        QCOMPARE(document.appendSamples(stroke, {input[i]}), Result::Success);
        preview = render(renderer, document.preview(), preview);
        QVERIFY(preview);
        if (!i || kind < 0) QCOMPARE(readTile(preview, target, {0, 0}), tile.rgba);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(readTile(preview, target, {0, 0}), readTile(rebuilt, target, {0, 0}));
        retainedIntents.append(document.preview());
        retainedPreviews.append(preview);
        retainedPixels.append(readTile(preview, target, {0, 0}));
    }
    const auto pendingPixels = readTile(preview, target, {0, 0});

    const auto pendingPixelsVersion = render(renderer, document.preview());
    QVERIFY(pendingPixelsVersion);
    QCOMPARE(readTile(pendingPixelsVersion, document.activeLayer(), {0, 0}), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto final = render(renderer, document.snapshot(), pendingPixelsVersion);
    QVERIFY(final);
    const auto actual = readTile(final, document.activeLayer(), {0, 0});
    for (qsizetype i = 0; i < retainedIntents.size(); ++i) {
        QCOMPARE(readTile(retainedPreviews[i], target, {0, 0}), retainedPixels[i]);
        QCOMPARE(readTile(render(renderer, retainedIntents[i]), target, {0, 0}), retainedPixels[i]);
    }
    QByteArray tapPixels;
    if (kind <= 0) {
        OperationDocument tap(image);
        const auto tapStroke = tap.beginStroke(tap.activeLayer(), brush, 0);
        QCOMPARE(tap.appendSamples(tapStroke, {input.last()}), Result::Success);
        QCOMPARE(tap.commitStroke(tapStroke), Result::Success);
        tapPixels = readTile(render(renderer, tap.snapshot()), tap.activeLayer(), {0, 0});
    }

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(actual.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-freehand-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    if (kind <= 0) QCOMPARE(actual, tapPixels);
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), expectedSha256);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile.rgba);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(readTile(render(renderer, document.snapshot(), preview), target, {0, 0}), tile.rgba);
}

void KisGpuTileRendererTest::gaussianEraseKeepsColorsHistory_data()
{
    QTest::addColumn<int>("calls");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<QByteArray>("expectedSha256");
    // Normal CPU figure erase at 62cfadd521, shipped Basic-5 preset,
    // pressure profile (.12,.91,.18), four worker threads and the fixed
    // alpha-gradient background below. Only BGRA/RGBA channel order changes.
    QTest::newRow("line") << 0 << qreal(0.5) << qreal(0.57)
        << QByteArrayLiteral("3c71e36b0b28727bf75c49a3f996f2fd94edd29e4675c70d5cba9b91cae8d0cd");
    QTest::newRow("cubic-open") << 5 << qreal(0.5) << qreal(0.57)
        << QByteArrayLiteral("d825b60fbe1ae9a35115c6cd573260a5b97d438be668c9a78075acdfeac6c5fe");
    QTest::newRow("cubic-closed") << 6 << qreal(0.5) << qreal(1)
        << QByteArrayLiteral("593e84384a8121987d9856c1148fe4e92c70d452d43cc04cd50a95d651f4f1e1");
    QTest::newRow("mixed") << 9 << qreal(1) << qreal(1)
        << QByteArrayLiteral("232813697f63e549ab6af1ede44dbdf42baa4594289e04fd5224145a6e35d951");
    QTest::newRow("zero-flow") << 0 << qreal(0) << qreal(1)
        << QByteArrayLiteral("dc1393988897084bb62a1142bc4d7fc6c4d3b03c10fafe253824863cb311901f");
    QTest::newRow("zero-opacity") << 0 << qreal(1) << qreal(0)
        << QByteArrayLiteral("dc1393988897084bb62a1142bc4d7fc6c4d3b03c10fafe253824863cb311901f");
    QTest::newRow("half-opacity-byte") << 0 << qreal(1) << qreal(0.49607843137254903)
        << QByteArrayLiteral("59238903eb5152baf26cc05eaa97934fefa0c2243ea924692cef0e7a035069d3");
    QTest::newRow("low-opacity-byte") << 0 << qreal(1) << qreal(0.0058823529411764705)
        << QByteArrayLiteral("5498ca9ca9d477ca17eafd7fbcd49cbe9af263dbda817f959a1714acd73f0dbf");
}

void KisGpuTileRendererTest::gaussianEraseKeepsColorsHistory()
{
    QFETCH(int, calls);
    QFETCH(qreal, flow);
    QFETCH(qreal, opacity);
    QFETCH(QByteArray, expectedSha256);
    auto brush = Test::defaultGaussianBrush();
    brush.erase = true;
    brush.opacity = opacity;
    brush.autoBrush->flow = flow;
    Shape shape;
    shape.kind = ShapeKind::Path;
    QList<InputSample> input;
    if (calls == 0) {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo, PathCommand::LineTo};
        input = {{{12.25, 13.75}, .12}, {{60.125, 31.5}, .91}, {{60.125, 31.5}, .91}, {{105.5, 91.25}, .18}};
    } else {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
        shape.pathCalls = {{0, 4}};
        input = {{{12.25, 13.75}, 1}, {{18.5, 48.75}, 1}, {{86.125, 5.25}, 1}, {{105.5, 91.25}, 1}};
        if (calls == 6) {
            input[1].position = {114.125, -15.75};
            input[2].position = {-26.375, 115.25};
            input[3].position = input[0].position;
        } else if (calls == 9) {
            input = {{{12.25, 13.75}, .2}, {{105.5, 25.75}, .8},
                     {{18.125, 91.5}, 1}, {{21.75, 43.125}, 1}, {{108.25, 120.375}, 1}, {{112.5, 78.25}, 1},
                     {{24.25, 48.125}, .9}, {{101.5, 53.75}, .4}};
            shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo,
                                  PathCommand::CubicTo, PathCommand::MoveTo, PathCommand::LineTo};
            shape.pathCalls = {{2, 6}};
        }
    }
    RgbaTile tile{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
    const std::array<int, 8> alphas{{0, 1, 2, 63, 127, 128, 254, 255}};
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        auto *pixel = tile.rgba.data() + 4 * (y * RgbaTile::Size + x);
        pixel[0] = char((x * 7 + y * 13) % 256);
        pixel[1] = char((x * 5 + y * 11) % 256);
        pixel[2] = char((x * 17 + y * 3) % 256);
        pixel[3] = char(alphas[(x / 16 + y / 16) % alphas.size()]);
    }
    RasterImage image;
    image.size = {128, 128};
    RasterLayer layer;
    layer.name = QStringLiteral("Paint");
    layer.uuid = QUuid(QStringLiteral("{33b00003-1122-3344-5566-778899aabbcc}"));
    layer.tiles = {tile};
    image.layers = {layer};
    OperationDocument document(image);
    const auto target = document.activeLayer();
    TileRenderer renderer(m_device);
    const auto background = render(renderer, document.snapshot());
    QVERIFY(background);
    const auto stroke = document.beginStroke(target, brush, 1789);
    QCOMPARE(document.appendSamples(stroke, input), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto preview = render(renderer, document.preview(), background);
    QVERIFY(preview);
    const auto actual = readTile(preview, target, {0, 0});
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(actual.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-erase-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), expectedSha256);
    for (qsizetype i = 0; i < actual.size(); i += 4) {
        QCOMPARE(actual.mid(i, 3), tile.rgba.mid(i, 3));
        QVERIFY(quint8(actual[i + 3]) <= quint8(tile.rgba[i + 3]));
    }
    QCOMPARE(readTile(background, target, {0, 0}), tile.rgba);
    const auto retained = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(retained->inputSamples(), input);
    QCOMPARE(retained->brush, brush);

    QCOMPARE(readTile(render(renderer, document.preview()), document.activeLayer(), {0, 0}), actual);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->brush, brush);
    if (!directory.isEmpty()) {
        const auto prefix = QStringLiteral("gaussian-erase-") + QString::fromLatin1(QTest::currentDataTag());
    }
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile.rgba);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(readTile(render(renderer, document.snapshot(), preview), target, {0, 0}), tile.rgba);
}

void KisGpuTileRendererTest::autoBrushProducesCoverageAndRetainsPixels()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.rgba = {17, 34, 51, 255};
    brush.diameter = 40;
    auto &settings = brush.autoBrush.emplace();
    settings.flow = .5;
    brush.opacity = .57;
    brush.sizeResponse = {PressureResponse::Kind::Constant, 1, {}};
    settings.opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    const auto id = document.beginStroke(document.activeLayer(), brush, 1789);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device, 8 * TileRenderer::TileBytes);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    const auto pixels = readTile(painted, document.activeLayer(), {0, 0});
    // One half-flow stamp has alpha 128 at its center; canvas opacity is
    // applied once at merge. The edge's Gaussian mask alpha is 126.
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("11223349"));
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 51) * 4, 4), QByteArray::fromHex("11223324"));
    QCOMPARE(pixels.mid((12 * TileRenderer::TileSize + 12) * 4, 4), QByteArray(4, '\0'));

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot());
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(readTile(painted, document.activeLayer(), {0, 0}), pixels);
}

// Consumer: ordinary additive Charcoal input and its retained history.
// Mutating the caller's secondary tip must preserve accepted editing pixels.
void KisGpuTileRendererTest::additiveMaskingBrushKeepsFrozenInputAndHistory()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.rgba = {17, 34, 51, 255};
    brush.diameter = 20;
    brush.sizeResponse = {PressureResponse::Kind::Constant, 1, {}};
    brush.autoBrush.emplace().opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    brush.autoBrush->flow = .23;
    auto mask = QSharedPointer<Brush>::create(brush);
    mask->rgba = {255, 255, 255, 255};
    mask->maskPainting = true;
    mask->diameter = 8;
    mask->autoBrush->flow = .37;
    brush.additiveMaskingBrush = mask;
    const QList<InputSample> input{{{24, 24}, 1}, {{40, 32}, .7}};
    const auto id = document.beginStroke(document.activeLayer(), brush, 1789);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, input), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device, 8 * TileRenderer::TileBytes);
    const auto retained = render(renderer, document.snapshot());
    QVERIFY(retained);
    const auto pixels = readTile(retained, document.activeLayer(), {0, 0});
    QVERIFY(pixels != QByteArray(pixels.size(), '\0'));
    OperationDocument primaryOnly(QSize(64, 64));
    auto primary = brush;
    primary.additiveMaskingBrush.clear();
    const auto primaryId = primaryOnly.beginStroke(primaryOnly.activeLayer(), primary, 1789);
    QVERIFY(!primaryId.isNull());
    QCOMPARE(primaryOnly.appendSamples(primaryId, input), Result::Success);
    QCOMPARE(primaryOnly.commitStroke(primaryId), Result::Success);
    QVERIFY(readTile(render(renderer, primaryOnly.snapshot()), primaryOnly.activeLayer(), {0, 0}) != pixels);
    mask->diameter = 40;
    mask->autoBrush->opacityResponse.strength = 0;
    TileRenderer replay(m_device, 8 * TileRenderer::TileBytes);
    QCOMPARE(readTile(render(replay, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(replay, document.snapshot());
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(replay, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(readTile(retained, document.activeLayer(), {0, 0}), pixels);
}

// Consumer: retained brush editing and replay. Resource mutation, cache eviction
// and an exhausted input budget must preserve the accepted pixels and history.
void KisGpuTileRendererTest::paperResourceKeepsFrozenPixelsHistoryAndCapacity()
{
    Limits limits;
    limits.rasterBytes = 4;
    OperationDocument document(QSize(64, 64), limits);
    Brush brush;
    brush.rgba = {17, 34, 51, 255};
    brush.diameter = 40;
    brush.sizeResponse = {PressureResponse::Kind::Constant, 1, {}};
    brush.autoBrush.emplace().mask = AutoBrush::Mask::Circle;
    auto &paper = brush.texture.emplace();
    paper.size = {2, 2};
    paper.alpha = QByteArray::fromHex("004080ff");
    paper.mode = BrushTexture::Mode::Subtract;
    FigureStyle style;
    style.pattern.emplace(); style.pattern->size = {1, 1};
    style.pattern->rgba = QByteArray::fromHex("112233ff");
    const auto initial = document.snapshot();
    QCOMPARE(document.paintFigure(document.activeLayer(), brush, {{{8, 8}, 1}, {{40, 40}, 1}},
                                  {ShapeKind::Rectangle, {8, 8}, {40, 40}}, style, 17), Result::CapacityExceeded);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.undoCount(), 0);
    const auto id = document.beginStroke(document.activeLayer(), brush, 17);
    QVERIFY(!id.isNull());
    paper.alpha.fill(char(255));
    QCOMPARE(document.appendSamples(id, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto accepted = document.snapshot();

    const auto next = document.beginStroke(document.activeLayer(), brush, 18);
    QVERIFY(!next.isNull());
    QCOMPARE(document.cancelStroke(next), Result::Success);
    QCOMPARE(document.snapshot(), accepted);
    QCOMPARE(document.undoCount(), 1);
    TileRenderer renderer(m_device, 8 * TileRenderer::TileBytes);
    const auto version = render(renderer, accepted);
    QVERIFY(version);
    const auto pixels = readTile(version, document.activeLayer(), {0, 0});
    const auto pixel = [&](int x, int y) { return pixels.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    QCOMPARE(pixel(32, 32), QByteArray::fromHex("112233ff"));
    QCOMPARE(pixel(33, 32), QByteArray::fromHex("112233bf"));
    QCOMPARE(pixel(32, 33), QByteArray::fromHex("1122337f"));
    QCOMPARE(pixel(33, 33), QByteArray(4, char(0)));
    // Other documents replace the finite imported-resource cache.
    for (int index = 0; index < 5; ++index) {
        OperationDocument other(QSize(64, 64));
        paper.alpha.fill(char(index + 1));
        const auto stroke = other.beginStroke(other.activeLayer(), brush, index);
        QVERIFY(!stroke.isNull());
        QCOMPARE(other.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(other.commitStroke(stroke), Result::Success);
        QVERIFY(render(renderer, other.snapshot()));
    }
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot())->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(readTile(version, document.activeLayer(), {0, 0}), pixels);
    TileRenderer recovered(m_device, 8 * TileRenderer::TileBytes);
    QCOMPARE(readTile(render(recovered, accepted), document.activeLayer(), {0, 0}), pixels);
}

void KisGpuTileRendererTest::autoBrushSkipsDabsBelowTheBrushMinimum()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.diameter = 40;
    auto &settings = brush.autoBrush.emplace();
    brush.sizeResponse = {PressureResponse::Kind::Direct, 1, {}};
    settings.opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    const auto id = document.beginStroke(document.activeLayer(), brush, 1789);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {{{32, 32}, .0002}}), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    QVERIFY(painted->tiles(document.activeLayer()).isEmpty());
}

void KisGpuTileRendererTest::gaussianLineShapeEditsPreserveInputHistory_data()
{
    QTest::addColumn<int>("inputKind");
    QTest::newRow("endpoints") << 0;
    QTest::newRow("projected-pressure") << 1;
    QTest::newRow("single-sample") << 2;
    QTest::newRow("coincident-input") << 3;
}

void KisGpuTileRendererTest::gaussianLineShapeEditsPreserveInputHistory()
{
    QFETCH(int, inputKind);
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 20;
    auto &settings = brush.autoBrush.emplace();
    settings.flow = .5;
    brush.opacity = .57;
    brush.sizeResponse = {PressureResponse::Kind::Direct, 1, {}};
    settings.opacityResponse = {PressureResponse::Kind::Direct, 1, {}};
    QList<InputSample> samples{{{32, 40}, .2, .1, -.2, .3, 16}, {{224, 40}, .8, -.1, .2, -.3, 48}};
    if (inputKind == 1 || inputKind == 3) samples.insert(1, {{128, 76}, 1, .2, .1, .4, 32});
    if (inputKind == 2) samples = {samples.first()};
    if (inputKind == 3) for (auto &sample : samples) sample.position = {32, 40};

    OperationDocument document({512, 512});
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, brush, 1789);
    QCOMPARE(document.appendSamples(stroke, samples), Result::Success);
    TileRenderer renderer(m_device);
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    const auto initialPixels = readTile(initial, layer, {0, 0});
    RenderedSnapshot previous = initial;
    QByteArray pixels;
    QPoint coordinate;
    for (const auto &shape : {Shape{ShapeKind::Line, {32, 368}, {224, 368}},
                              Shape{ShapeKind::Line, {304, 464}, {464, 384}}}) {
        // A newly entered straight path is the same drawing with explicit
        // positions. Original sensor values remain attached to their samples.
        auto placed = samples;
        if (placed.size() == 1) placed.append(placed.first());
        placed.first().position = shape.first;
        placed.last().position = shape.second;
        if (placed.size() == 3) placed[1].position = (shape.first + shape.second) / 2;
        OperationDocument reference({512, 512});
        const auto referenceStroke = reference.beginStroke(reference.activeLayer(), brush, 1789);
        QCOMPARE(reference.appendSamples(referenceStroke, placed), Result::Success);
        Shape path;
        path.kind = ShapeKind::Path;
        path.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo};
        if (placed.size() == 3) path.pathCommands.append(PathCommand::LineTo);
        QCOMPARE(reference.updateShape(referenceStroke, path), Result::Success);
        const auto expected = render(renderer, reference.preview());
        QVERIFY(expected);

        QCOMPARE(document.updateShape(stroke, shape), Result::Success);
        const auto edited = render(renderer, document.preview(), previous);
        QVERIFY(edited);
        coordinate = {int(shape.first.x()) / RgbaTile::Size, int(shape.first.y()) / RgbaTile::Size};
        QCOMPARE(edited->tiles(layer).size(), 1);
        pixels = readTile(edited, layer, coordinate);
        QCOMPARE(pixels.size(), TileRenderer::TileBytes);
        QCOMPARE(pixels, readTile(expected, reference.activeLayer(), coordinate));
        const auto operation = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
        QCOMPARE(operation->inputSamples(), samples);
        previous = edited;
    }

    const auto rebuilt = render(renderer, document.preview());
    QVERIFY(rebuilt);
    QCOMPARE(readTile(rebuilt, document.activeLayer(), coordinate), pixels);
    const auto retained = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(retained->inputSamples(), samples);
    QCOMPARE(retained->brush, brush);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), previous);
    QVERIFY(cancelled && cancelled->tiles(layer).isEmpty());
    QCOMPARE(document.undoCount(), 0);
    const auto accepted = document.beginStroke(layer, retained->brush, retained->randomSeed);
    QCOMPARE(document.appendSamples(accepted, retained->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(accepted, retained->shape, retained->figureStyle), Result::Success);
    QCOMPARE(document.commitStroke(accepted), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), rebuilt);
    QVERIFY(undone && undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), document.activeLayer(), coordinate), pixels);
    QCOMPARE(readTile(initial, layer, {0, 0}), initialPixels);
    QCOMPARE(readTile(previous, layer, coordinate), pixels);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-line-") +
                   QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
}

void KisGpuTileRendererTest::gaussianPathCallsKeepDistanceHistory_data()
{
    QTest::addColumn<int>("kind");
    QTest::newRow("compound") << 0;
    QTest::newRow("line-path-line") << 2;
}

void KisGpuTileRendererTest::gaussianPathCallsKeepDistanceHistory()
{
    QFETCH(int, kind);
    const auto operation = Test::gaussianCompoundPathInput();
    auto shape = operation.shape;
    auto input = operation.inputSamples();
    if (kind >= 2) {
        shape.pathCommands.append({PathCommand::MoveTo, PathCommand::LineTo});
        input.append({{24.25, 48.125}, 1});
        input.append({{101.5, 53.75}, 1});
        if (kind == 2) {
            input[0].pressure = .2; input[1].pressure = .8;
            input[4].pressure = .9; input[5].pressure = .4;
        } else { input[2].pressure = .2; input[3].pressure = .8; }
    }
    OperationDocument document(QSize(128, 128));
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, operation.brush, 0);
    QCOMPARE(document.appendSamples(stroke, input), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    TileRenderer renderer(m_device);
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    const auto pinnedPixels = readTile(initial, layer, {0, 0});
    if (kind == 0) shape.pathCalls = {{0, 4}};
    else if (kind == 1) shape.pathCalls = {{0, 2}, {2, 4}};
    else if (kind == 2) shape.pathCalls = {{2, 4}};
    else shape.pathCalls = {{0, 2}, {4, 6}};
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto changed = render(renderer, document.preview(), initial);
    QVERIFY(changed);
    const auto tile = readTile(changed, layer, {0, 0});
    QCOMPARE(tile.size(), TileRenderer::TileBytes);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-path-calls-") +
                   QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    QVERIFY(std::any_of(pixels.cbegin(), pixels.cend(), [](char value) { return value != 0; }));
    QCOMPARE(readTile(initial, layer, {0, 0}), pinnedPixels);

    const auto rebuilt = render(renderer, document.preview());
    QVERIFY(rebuilt);
    QCOMPARE(readTile(rebuilt, document.activeLayer(), {0, 0}), tile);
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers.first().operations.last())->inputSamples(), input);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), rebuilt);
    QVERIFY(undone && undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), document.activeLayer(), {0, 0}), tile);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), changed);
    QVERIFY(cancelled && cancelled->tiles(layer).isEmpty());
}

void KisGpuTileRendererTest::gaussianCubicPathsKeepPixelsInputHistory_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QByteArray>("expectedSha256");
    // Unchanged normal CPU figure calls at 2d57883297 with the captured brush.
    // Only BGRA/RGBA byte-channel order was converted for the complete images.
    QTest::newRow("open") << 0 << QByteArrayLiteral("fd9cfb1e45be12615d7764cc8250e9fbfe963a12086c79067c1b6be49f194841");
    QTest::newRow("closed") << 1 << QByteArrayLiteral("6e4827361a867bc92a15fe38c2d8f9f777f8ef15886a682f5d4941d869ada5ae");
    QTest::newRow("compound") << 2 << QByteArrayLiteral("6826252ac05a1a397cefc746c98b4061fe3da7ebffbc66cc04dfb900c3e25a10");
    QTest::newRow("separate") << 3 << QByteArrayLiteral("4500dcdb3d60097b00cbc222370dee64c2458992db7f65cf260701da1860b61e");
    QTest::newRow("line-cubic-line") << 4 << QByteArrayLiteral("efb5ac9576f8202c28e17aeb27296e183ca8f401b4ad820f1d6c05710f81e700");
}

void KisGpuTileRendererTest::gaussianCubicPathsKeepPixelsInputHistory()
{
    QFETCH(int, kind);
    QFETCH(QByteArray, expectedSha256);
    const auto brush = Test::defaultGaussianBrush();
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
    QList<InputSample> input{{{12.25, 13.75}, 1}, {{18.5, 48.75}, 1}, {{86.125, 5.25}, 1}, {{105.5, 91.25}, 1}};
    shape.pathCalls = {{0, 4}};
    if (kind == 1) {
        input[1].position = {114.125, -15.75};
        input[2].position = {-26.375, 115.25};
        input[3].position = input[0].position;
    } else if (kind >= 2) {
        const QList<InputSample> second{{{18.125, 91.5}, 1}, {{21.75, 43.125}, 1},
                                        {{108.25, 120.375}, 1}, {{112.5, 78.25}, 1}};
        if (kind == 4) {
            input = {{{12.25, 13.75}, .2}, {{105.5, 25.75}, .8}};
            input.append(second);
            input.append({{24.25, 48.125}, .9});
            input.append({{101.5, 53.75}, .4});
            shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo,
                                  PathCommand::CubicTo, PathCommand::MoveTo, PathCommand::LineTo};
            shape.pathCalls = {{2, 6}};
        } else {
            input.append(second);
            shape.pathCommands.append({PathCommand::MoveTo, PathCommand::CubicTo});
            shape.pathCalls = kind == 2 ? QList<QPair<qsizetype, qsizetype>>{{0, 8}} :
                                         QList<QPair<qsizetype, qsizetype>>{{0, 4}, {4, 8}};
        }
    }
    auto outline = shape;
    outline.pathCommands.clear();
    for (const auto command : shape.pathCommands) {
        if (command == PathCommand::CubicTo)
            outline.pathCommands.append({PathCommand::LineTo, PathCommand::LineTo, PathCommand::LineTo});
        else outline.pathCommands.append(command);
    }
    OperationDocument document(QSize(128, 128));
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, input), Result::Success);
    QCOMPARE(document.updateShape(stroke, outline), Result::Success);
    TileRenderer renderer(m_device);
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    const auto initialPixels = readTile(initial, layer, {0, 0});
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto preview = document.preview();
    const auto changed = render(renderer, preview, initial);
    QVERIFY(changed);
    const auto tile = readTile(changed, layer, {0, 0});
    QCOMPARE(tile.size(), TileRenderer::TileBytes);
    QVERIFY(tile != initialPixels);
    QCOMPARE(readTile(initial, layer, {0, 0}), initialPixels);
    const auto retained = std::get<PaintSnapshot>(preview->layers.first().operations.last());
    QCOMPARE(retained->inputSamples(), input);
    QCOMPARE(retained->shape.pathCommands, shape.pathCommands);
    QCOMPARE(retained->shape.pathCalls, shape.pathCalls);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-cubic-") +
                   QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), expectedSha256);

    const auto rebuilt = render(renderer, document.preview());
    QVERIFY(rebuilt);
    QCOMPARE(readTile(rebuilt, document.activeLayer(), {0, 0}), tile);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), rebuilt);
    QVERIFY(undone && undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(readTile(redone, document.activeLayer(), {0, 0}), tile);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), changed);
    QVERIFY(cancelled && cancelled->tiles(layer).isEmpty());
}

void KisGpuTileRendererTest::isolatedGroupsKeepProjectionAndRetainedSubtreePixels_data()
{
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<bool>("visible");
    QTest::addColumn<bool>("nested");
    QTest::addColumn<bool>("backdrop");
    QTest::addColumn<QByteArray>("rootPixel");
    QTest::addColumn<QByteArray>("groupPixel");
    QTest::newRow("isolated") << false << true << false << true << QByteArray::fromHex("80007fff") << QByteArray::fromHex("ff0000ff");
    QTest::newRow("uniform") << true << true << false << true << QByteArray::fromHex("80007fff") << QByteArray::fromHex("ff0000ff");
    QTest::newRow("hidden") << false << false << false << true << QByteArray::fromHex("0000ffff") << QByteArray::fromHex("ff0000ff");
    QTest::newRow("nested") << false << true << true << true << QByteArray::fromHex("4000bfff") << QByteArray::fromHex("ff000080");
    QTest::newRow("hidden-rgb") << false << true << false << false << QByteArray::fromHex("11223300") << QByteArray::fromHex("11223300");
    QTest::newRow("implicit-hidden-rgb") << true << true << false << false << QByteArray::fromHex("11223300") << QByteArray::fromHex("11223300");
}

void KisGpuTileRendererTest::isolatedGroupsKeepProjectionAndRetainedSubtreePixels()
{
    QFETCH(bool, implicit);
    QFETCH(bool, visible);
    QFETCH(bool, nested);
    QFETCH(bool, backdrop);
    QFETCH(QByteArray, rootPixel);
    QFETCH(QByteArray, groupPixel);
    RasterImage source;
    source.size = {64, 48};
    const QUuid inner(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const QUuid outer(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
    const auto populate = [&](RasterLayer &layer, const QByteArray &pixel) {
        if (implicit) {
            for (int channel = 0; channel < 4; ++channel) layer.defaultPixel[channel] = quint8(pixel[channel]);
        } else {
            const auto bytes = pixel.repeated(RgbaTile::ByteCount / 4);
            layer.tiles = {{{0, 0}, bytes}, {{1, 0}, bytes}};
        }
    };
    if (backdrop) {
        source.layers.append({QStringLiteral("Blue backdrop"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}});
        populate(source.layers.last(), QByteArray::fromHex("0000ffff"));
    }
    source.activeLayer = source.layers.size();
    source.layers.append({QStringLiteral("Nested pixels"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}});
    populate(source.layers.last(), backdrop ? QByteArray::fromHex("ff0000ff") : groupPixel);
    source.layers.last().parentUuid = inner;
    source.layers.append({QStringLiteral("Inner group"), visible, inner, {}});
    source.layers.last().kind = LayerKind::Group;
    source.layers.last().opacity = backdrop ? 128 : 255;
    if (nested) {
        source.layers.last().parentUuid = outer;
        source.layers.append({QStringLiteral("Outer group"), visible, outer, {}});
        source.layers.last().kind = LayerKind::Group;
        source.layers.last().opacity = 128;
    }
    const LayerId group = source.layers.size();
    OperationDocument document(source);
    TileRenderer renderer(m_device, 10 * TileRenderer::TileBytes);
    const auto raw = render(renderer, document.snapshot());
    QVERIFY(raw);
    const QRect region(0, 0, 64, 48);
    const auto before = project(renderer, raw, region);
    QVERIFY(before);
    const auto projectedPixel = [&](const ProjectedSnapshot &version) {
        for (const auto &tile : version->tiles()) if (tile.coordinate == QPoint()) return readBuffer(tile.pixels).first(4);
        return QByteArray(4, '\0');
    };
    QCOMPARE(projectedPixel(before), rootPixel);
    if (!implicit) {
        QCOMPARE(readTile(raw, group, {0, 0}), groupPixel.repeated(RgbaTile::ByteCount / 4));
        QCOMPARE(readTile(raw, group, {1, 0}), groupPixel.repeated(RgbaTile::ByteCount / 4));
    }
    if (backdrop && visible && !nested) {
        QCOMPARE(document.setLayerOpacity(group, 64), Result::Success);
        const auto changed = render(renderer, document.snapshot(), raw);
        QVERIFY(changed);
        const auto after = project(renderer, changed, region, before);
        QVERIFY(after);
        QCOMPARE(projectedPixel(after), QByteArray::fromHex("4000bfff"));
        QCOMPARE(projectedPixel(before), rootPixel);
        if (!implicit) QCOMPARE(changed->tiles(group).first().pixels, raw->tiles(group).first().pixels);
    }
}

void KisGpuTileRendererTest::discardedAffinePreviewKeepsWorkingBytesUntilSubmissionCompletes()
{
    // A cancelled transform can drop every view reference before the GPU
    // finishes. Capacity must still protect the recorded buffers and scratch.
    const auto pixel = QByteArray::fromHex("112233ff");
    RasterImage input; input.size = {256, 256};
    input.layers = {{QStringLiteral("Transform lifetime"), true, {}, {{{0, 0}, pixel.repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    const auto token = document.beginAffineTransform({{layer, true, false}});
    QVERIFY(!token.isNull());
    auto plan = QSharedPointer<AffinePlan>::create(); plan->filterId = QStringLiteral("NearestNeighbor");
    AffinePass pass; pass.kind = AffinePass::Remap; pass.mapWholeLayer = true; pass.map = {1, 0, 0, 1, 3, -7}; plan->passes.append(pass);
    QCOMPARE(document.updateAffineTransform(token, {{layer, plan, {}}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto commands = m_device->beginCommands(); QVERIFY(commands);
    {
        auto recorded = renderer.recordRegion(commands, document.preview(), {0, 0, 256, 256}, {layer});
        QVERIFY2(recorded.code == RenderOutcome::Recorded, qPrintable(recorded.diagnostic));
        QVERIFY(recorded.version);
    }
    const auto inFlight = renderer.residentBytes();
    QVERIFY(inFlight > 2 * TileRenderer::TileBytes);
    QCOMPARE(document.cancelAffineTransform(token), Result::Success);
    QCOMPARE(renderer.residentBytes(), inFlight);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(renderer.residentBytes(), qsizetype(0));
}

void KisGpuTileRendererTest::layerCropSharesInteriorPreservesBoundaryAndRetainedPixels_data()
{
    QTest::addColumn<QByteArray>("implicit");
    QTest::addColumn<bool>("sixteenBits");
    QTest::newRow("transparent") << QByteArray::fromHex("00000000") << false;
    QTest::newRow("hidden-rgb") << QByteArray::fromHex("19334d00") << false;
    QTest::newRow("partial-alpha") << QByteArray::fromHex("19334d49") << false;
    QTest::newRow("native-transparent") << rgbaBytes({}, PixelFormat::Rgba16) << true;
    QTest::newRow("native-hidden-rgb") << rgbaBytes({1, 257, 65534, 0}, PixelFormat::Rgba16) << true;
    QTest::newRow("native-partial-alpha") << rgbaBytes({1, 257, 65534, 32767}, PixelFormat::Rgba16) << true;
}

void KisGpuTileRendererTest::layerCropSharesInteriorPreservesBoundaryAndRetainedPixels()
{
    QFETCH(QByteArray, implicit);
    QFETCH(bool, sixteenBits);
    const auto format = sixteenBits ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const int pixelSize = pixelByteCount(format);
    const int tileBytes = RgbaTile::byteCount(format);
    const QByteArray pixel = sixteenBits ? rgbaBytes({3, 256, 32768, 0}, format) : QByteArray::fromHex("11223300");
    const auto raw = pixel.repeated(tileBytes / pixelSize);
    RasterImage input;
    input.size = {768, 256};
    input.format = format;
    input.layers = {{QStringLiteral("Crop target"), true, {}, {}}};
    const auto defaultPixel = rgbaPixel(implicit, format);
    QVERIFY(defaultPixel);
    input.layers[0].defaultPixel = *defaultPixel;
    for (int x = -2; x <= 2; ++x) input.layers[0].tiles.append({{x, 0}, raw, format});
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device);
    const auto before = render(renderer, document.snapshot());
    QVERIFY(before);
    const auto allocated = renderer.residentBytes();
    QCOMPARE(document.cropLayer(layer, {-256, 0, 776, 256}), Result::Success);
    const auto after = render(renderer, document.snapshot(), before);
    QVERIFY(after);
    QCOMPARE(after->tiles(layer).size(), 4);
    QCOMPARE(renderer.residentBytes() - allocated, qsizetype(tileBytes));
    for (const auto &tile : after->tiles(layer)) {
        if (tile.coordinate.x() == 2) continue;
        const auto old = before->tiles(layer);
        const auto found = std::find_if(old.cbegin(), old.cend(), [&](const auto &value) { return value.coordinate == tile.coordinate; });
        QVERIFY(found != old.cend());
        QCOMPARE(tile.pixels, found->pixels);
    }
    auto expected = implicit.repeated(tileBytes / pixelSize);
    for (int y = 0; y < 256; ++y) std::memcpy(expected.data() + y * 256 * pixelSize, raw.constData(), 8 * pixelSize);
    QCOMPARE(readTile(after, layer, {2, 0}), expected);
    QCOMPARE(readTile(before, layer, {2, 0}), raw);
    QCOMPARE(readTile(before, layer, {-2, 0}), raw);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), after);
    QVERIFY(undone);
    QCOMPARE(readTile(undone, layer, {2, 0}), raw);
    QCOMPARE(document.redo(), Result::Success);

    TileRenderer rebuilt(m_device);
    const auto restored = render(rebuilt, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {2, 0}), expected);
}

void KisGpuTileRendererTest::layerCropRejectsBoundaryAllocationWithoutChangingRetainedPixels()
{
    const auto raw = QByteArray::fromHex("112233ff").repeated(RgbaTile::ByteCount / 4);
    RasterImage input;
    input.size = {256, 256};
    input.layers = {{QStringLiteral("Crop target"), true, {}, {{{0, 0}, raw}}}};
    OperationDocument document(input);
    TileRenderer renderer(m_device, TileRenderer::TileBytes);
    const auto before = render(renderer, document.snapshot());
    QVERIFY(before);
    QCOMPARE(document.cropLayer(document.activeLayer(), {3, 4, 50, 60}), Result::Success);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto rejected = renderer.record(commands, document.snapshot(), before);
    QCOMPARE(rejected.code, RenderOutcome::CapacityExceeded);
    QVERIFY(!rejected.version);
    QVERIFY(rejected.diagnostic.contains(QStringLiteral("crop")));
    QVERIFY(m_device->submitCommands(true));
    QCOMPARE(readTile(before, document.activeLayer(), {0, 0}), raw);
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
}

void KisGpuTileRendererTest::symmetryKeepsSparseTilesAndIncrementalInputWithinBudget()
{
    OperationDocument document({1024, 1024});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.diameter = 8;
    brush.mirrorHorizontal = brush.mirrorVertical = true;
    brush.mirrorCenter = {512, 512};
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
    const auto fixed = document.preview();
    TileRenderer limited(m_device, 3 * TileRenderer::TileBytes);
    QVERIFY(!render(limited, fixed));
    QCOMPARE(limited.residentBytes(), qsizetype(0));
    QCOMPARE(document.preview(), fixed);
    TileRenderer renderer(m_device);
    const auto first = render(renderer, fixed);
    QCOMPARE(first->tiles(layer).size(), 4);
    for (const QPoint point : {QPoint(0, 0), QPoint(3, 0), QPoint(0, 3), QPoint(3, 3)})
        QVERIFY(!readTile(first, layer, point).isEmpty());
    QCOMPARE(document.appendSamples(stroke, {{{300, 300}, 0.5}}), Result::Success);
    const auto incremental = render(renderer, document.preview(), first);
    const auto rebuilt = render(renderer, document.preview());
    QCOMPARE(incremental->tiles(layer).size(), rebuilt->tiles(layer).size());
    for (const auto &tile : rebuilt->tiles(layer))
        QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(rebuilt, layer, tile.coordinate));
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QVERIFY(render(renderer, document.snapshot())->tiles(layer).isEmpty());
    QCOMPARE(first->document(), fixed);
}

void KisGpuTileRendererTest::symmetryPreservesPixelsOverlapHistory_data()
{
    QTest::addColumn<quint8>("mirrorMode");
    QTest::addColumn<QPoint>("center");
    QTest::addColumn<QPointF>("position");
    QTest::addColumn<bool>("line");
    QTest::newRow("horizontal") << quint8(1) << QPoint(32, 32) << QPointF(16, 16) << false;
    QTest::newRow("vertical") << quint8(2) << QPoint(32, 32) << QPointF(16, 16) << false;
    QTest::newRow("both") << quint8(3) << QPoint(32, 32) << QPointF(16, 16) << false;
    QTest::newRow("moved-axes") << quint8(3) << QPoint(24, 30) << QPointF(16, 16) << false;
    QTest::newRow("outside-canvas") << quint8(1) << QPoint(0, 32) << QPointF(16, 16) << false;
    QTest::newRow("overlapping-axis") << quint8(1) << QPoint(32, 32) << QPointF(32, 16) << false;
    QTest::newRow("retained-line") << quint8(3) << QPoint(32, 32) << QPointF(16, 16) << true;
}

void KisGpuTileRendererTest::symmetryPreservesPixelsOverlapHistory()
{
    QFETCH(quint8, mirrorMode);
    QFETCH(QPoint, center);
    QFETCH(QPointF, position);
    QFETCH(bool, line);
    Brush brush;
    brush.rgba = {200, 70, 30, 255};
    brush.opacity = 128.0 / 255;
    brush.diameter = 8;
    const auto draw = [&](OperationDocument &document, const Brush &settings) {
        const auto stroke = document.beginStroke(document.activeLayer(), settings, 7);
        if (document.appendSamples(stroke, {{position, 1}}) != Result::Success) return OperationId{};
        if (line) {
            if (document.appendSamples(stroke, {{position + QPointF(4, 4), 0.5}}) != Result::Success ||
                document.updateShape(stroke, {ShapeKind::Line, position, position + QPointF(4, 4), 0}) != Result::Success)
                return OperationId{};
        }
        if (document.commitStroke(stroke) != Result::Success) return OperationId{};
        return stroke;
    };
    OperationDocument plain({64, 64});
    QVERIFY(!draw(plain, brush).isNull());
    TileRenderer renderer(m_device);
    const auto original = readTile(render(renderer, plain.snapshot()), plain.activeLayer(), {0, 0});
    QCOMPARE(original.size(), RgbaTile::ByteCount);
    QByteArray expected(RgbaTile::ByteCount, '\0');
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
        const auto pixel = original.mid((y * RgbaTile::Size + x) * 4, 4);
        if (!quint8(pixel[3])) continue;
        for (int reflection = 0; reflection < 4; ++reflection) {
            if ((reflection & mirrorMode) != reflection) continue;
            const int dx = reflection & 1 ? 2 * center.x() - x - 1 : x;
            const int dy = reflection & 2 ? 2 * center.y() - y - 1 : y;
            if (dx >= 0 && dx < 64 && dy >= 0 && dy < 64)
                expected.replace((dy * RgbaTile::Size + dx) * 4, 4, pixel);
        }
    }
    brush.mirrorHorizontal = mirrorMode & 1;
    brush.mirrorVertical = mirrorMode & 2;
    brush.mirrorCenter = center;
    OperationDocument document({64, 64});
    const auto stroke = draw(document, brush);
    QVERIFY(!stroke.isNull());
    const auto pixels = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});
    QCOMPARE(pixels.size(), RgbaTile::ByteCount);
    QCOMPARE(pixels, expected);

    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->brush, brush);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot())->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("symmetry-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
}

void KisGpuTileRendererTest::channelProtectionPreservesPixelsHistory_data()
{
    QTest::addColumn<quint8>("writeMask");
    QTest::addColumn<quint8>("baseAlpha");
    QTest::addColumn<bool>("erase");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("alpha-transparent") << quint8(7) << quint8(0) << false << QByteArray::fromHex("4b328100");
    QTest::newRow("alpha-translucent") << quint8(7) << quint8(128) << false << QByteArray::fromHex("4b328180");
    QTest::newRow("protected-erase") << quint8(7) << quint8(128) << true << QByteArray::fromHex("0a28b454");
    QTest::newRow("red-only") << quint8(1) << quint8(128) << false << QByteArray::fromHex("4b28b480");
    QTest::newRow("alpha-only") << quint8(8) << quint8(128) << false << QByteArray::fromHex("0a28b4ab");
    QTest::newRow("green-and-alpha") << quint8(10) << quint8(128) << false << QByteArray::fromHex("0a37b4ab");
    QTest::newRow("all-protected") << quint8(0) << quint8(255) << false << QByteArray::fromHex("0a28b4ff");
}

void KisGpuTileRendererTest::channelProtectionPreservesPixelsHistory()
{
    QFETCH(quint8, writeMask);
    QFETCH(quint8, baseAlpha);
    QFETCH(bool, erase);
    QFETCH(QByteArray, expected);
    QByteArray pixel = QByteArray::fromHex("0a28b400");
    pixel[3] = char(baseAlpha);
    RasterImage image;
    image.size = {64, 64};
    image.layers = {{QStringLiteral("Protected channels"), true, {},
                     {{{0, 0}, pixel.repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    QCOMPARE(document.writeLayerChannelMask(layer, writeMask), Result::Success);
    Brush brush;
    brush.rgba = {200, 70, 30, 255};
    brush.diameter = 8;
    brush.opacity = 87.0 / 255;
    brush.erase = erase;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto pixels = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    QCOMPARE(pixels.mid((32 * RgbaTile::Size + 32) * 4, 4), expected);

    QCOMPARE(document.snapshot()->layers.first().channelWriteMask, writeMask);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("channel-protection-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}).mid((32 * RgbaTile::Size + 32) * 4, 4), pixel);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::alphaLockPreservesCoverageHistory_data()
{
    QTest::addColumn<quint8>("baseAlpha");
    QTest::addColumn<bool>("erase");
    QTest::newRow("transparent") << quint8(0) << false;
    QTest::newRow("almost-transparent") << quint8(1) << false;
    QTest::newRow("translucent") << quint8(128) << false;
    QTest::newRow("opaque") << quint8(255) << false;
    QTest::newRow("erase-translucent") << quint8(128) << true;
    QTest::newRow("erase-transparent") << quint8(0) << true;
}

