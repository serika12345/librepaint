/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRendererTest.h"

void KisGpuTileRendererTest::scatteredInputKeepsFrozenSeedAndPartitions()
{
    QFETCH(bool, imageTip);
    QFETCH(bool, incremental);
    // The generated Pencil-1/2 family consumes two Taus88 values per painted
    // dab, including corner fans. Rebuilding a preview must replay that order.
    Brush brush;
    brush.rgba = {17, 129, 241, 255}; brush.diameter = 10; brush.spacing = .2;
    brush.autoBrush = AutoBrush{};
    brush.autoBrush->antialiasEdges = false;
    brush.autoBrush->fade = .15; brush.autoBrush->verticalFade = .15;
    brush.autoBrush->flow = .4;
    brush.autoBrush->scatter = PressureResponse{PressureResponse::Kind::Direct, .09, {}};
    brush.autoBrush->drawingAngleRotation = PressureResponse{PressureResponse::Kind::Direct, 1, {}};
    brush.autoBrush->fanCornersStep = M_PI / 2;
    if (incremental) {
        brush.autoBrush->incremental = true;
        brush.opacity = .37;
        brush.autoBrush->drawingAngleRotation.reset();
        brush.autoBrush->fanCornersStep = 0;
        brush.autoBrush->tiltDirectionRotation = PressureResponse{PressureResponse::Kind::Direct, 1, {}};
        brush.sizeSensor = Brush::SizeSensor::TiltElevation;
    }
    if (imageTip) {
        auto tips = QSharedPointer<ImageBrushTips>::create();
        tips->originalSize = {4, 4}; tips->tipCount = 2; tips->randomSelection = true;
        tips->levels = {ImageBrushLevel{{4, 4}, 1, 0, 18}};
        tips->pixels = QByteArray(144, char(0));
        for (int tip = 0; tip < 2; ++tip) for (int y = 1; y <= 4; ++y) for (int x = 1; x <= 4; ++x) {
            const int offset = tip * 72 + (y * 6 + x) * 2;
            tips->pixels[offset] = char(tip * 192); tips->pixels[offset + 1] = char(255);
        }
        brush.autoBrush->mask = AutoBrush::Mask::Image;
        brush.autoBrush->imageTips = tips;
    }

    QList<InputSample> input{{{20.25, 60.75}, .7}, {{110.25, 60.75}, .8},
                            {{35.25, 59.75}, .5}, {{35.25, 100.75}, .9}, {{90.25, 30.75}, .1}};
    if (incremental) for (qsizetype i = 0; i < input.size(); ++i) {
        input[i].xTilt = 35 - i * 10; input[i].yTilt = -15 + i * 9;
        input[i].tiltDirectionOffset = 17;
    }
    OperationDocument whole({128, 128}), chunks({128, 128});
    const auto a = whole.beginStroke(whole.activeLayer(), brush, 77);
    const auto b = chunks.beginStroke(chunks.activeLayer(), brush, 77);
    QVERIFY(!a.isNull() && !b.isNull());
    QCOMPARE(whole.appendSamples(a, input), Result::Success);
    TileRenderer renderer(m_device);
    for (const auto &sample : input) {
        QCOMPARE(chunks.appendSamples(b, {sample}), Result::Success);
        QVERIFY(render(renderer, chunks.preview()));
    }
    QCOMPARE(whole.commitStroke(a), Result::Success);
    QCOMPARE(chunks.commitStroke(b), Result::Success);
    const auto frozen = whole.snapshot();
    const auto expected = render(renderer, frozen), actual = render(renderer, chunks.snapshot());
    QVERIFY(expected && actual);
    const auto pixels = [&](const RenderedSnapshot &version) {
        QHash<QPoint, QByteArray> result;
        if (!version) return result;
        for (const auto &tile : version->tiles(whole.activeLayer()))
            result.insert(tile.coordinate, readTile(version, whole.activeLayer(), tile.coordinate));
        return result;
    };
    const auto original = pixels(expected);
    QVERIFY(!original.isEmpty());
    QCOMPARE(pixels(actual), original);
    QCOMPARE(chunks.undo(), Result::Success);
    QCOMPARE(chunks.redo(), Result::Success);
    QCOMPARE(pixels(render(renderer, chunks.snapshot())), original);
    const auto canceled = chunks.beginStroke(chunks.activeLayer(), brush, 78);
    QVERIFY(!canceled.isNull());
    QCOMPARE(chunks.appendSamples(canceled, input), Result::Success);
    QVERIFY(render(renderer, chunks.preview()));
    QCOMPARE(chunks.cancelStroke(canceled), Result::Success);
    QCOMPARE(pixels(render(renderer, chunks.snapshot())), original);
    QCOMPARE(whole.undo(), Result::Success);
    const auto changedSeed = whole.beginStroke(whole.activeLayer(), brush, 78);
    QVERIFY(!changedSeed.isNull());
    QCOMPARE(whole.appendSamples(changedSeed, input), Result::Success);
    QCOMPARE(whole.commitStroke(changedSeed), Result::Success);
    QVERIFY(pixels(render(renderer, whole.snapshot())) != original);
    TileRenderer replay(m_device);
    QCOMPARE(pixels(render(replay, frozen)), original);
}

void KisGpuTileRendererTest::unchangedTilesAreSharedAndBudgetsRejectAtomically()
{
    OperationDocument document({1024, 1024});
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto first = render(renderer, document.snapshot());
    QVERIFY(first);
    const auto next = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(next, {{{800, 800}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(next), Result::Success);
    const auto second = render(renderer, document.snapshot(), first);
    QVERIFY(second);
    QCOMPARE(second->tiles(document.activeLayer()).first().pixels, first->tiles(document.activeLayer()).first().pixels);
    const auto before = readTile(first, document.activeLayer(), {0, 0});
    const auto third = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(third, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
    QVERIFY(!render(renderer, document.preview(), second));
    QCOMPARE(readTile(first, document.activeLayer(), {0, 0}), before);
    QCOMPARE(renderer.residentBytes(), 2 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::erasingAndLayerVisibilityPreservePixelMeaning()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 16;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    brush.erase = true;
    brush.opacity = 128.0 / 255;
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto erased = render(renderer, document.snapshot(), painted);
    QVERIFY(erased);
    QCOMPARE(readTile(erased, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff00007f"));
    brush.opacity = 255.0 / 255;
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto fullyErased = render(renderer, document.snapshot(), erased);
    QVERIFY(fullyErased);
    const auto pixel = [&](const RenderedSnapshot &version) {
        return readTile(version, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4);
    };
    QCOMPARE(pixel(fullyErased), QByteArray::fromHex("ff000000"));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot());
    QVERIFY(undone);
    QCOMPARE(pixel(undone), QByteArray::fromHex("ff00007f"));
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(pixel(redone), QByteArray::fromHex("ff000000"));

    const auto restored = render(renderer, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(pixel(restored), QByteArray::fromHex("ff000000"));
    QCOMPARE(document.setLayerVisible(document.activeLayer(), false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), fullyErased);
    QVERIFY(hidden);
    QCOMPARE(hidden->tiles(document.activeLayer()).first().pixels, fullyErased->tiles(document.activeLayer()).first().pixels);
    QCOMPARE(readTile(painted, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
}

void KisGpuTileRendererTest::longDiagonalUsesOnlyIntersectingTiles()
{
    OperationDocument document({4096, 4096});
    Brush brush;
    brush.diameter = 4;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{4, 4}, 1}, {{4092, 4092}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 64 * TileRenderer::TileBytes);
    const auto version = render(renderer, document.preview());
    QVERIFY2(version, "A sparse diagonal must fit without allocating its bounding rectangle");
    QVERIFY(version->tiles(document.activeLayer()).size() <= 64);
    QCOMPARE(readTile(version, document.activeLayer(), {0, 0}).mid((4 * TileRenderer::TileSize + 4) * 4, 4), QByteArray::fromHex("000000ff"));
}

void KisGpuTileRendererTest::shapesUndoRestoreTheSamePixels()
{
    OperationDocument document({256, 256});
    Brush brush;
    brush.rgba = {17, 129, 241, 201};
    brush.diameter = 13.75;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{10, 20}, 0.75}, {{180, 140}, 0.5}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto freehand = render(renderer, document.preview());
    QVERIFY(freehand);
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Line, {60, 80}, {200, 140}, 0}), Result::Success);
    const auto line = render(renderer, document.preview(), freehand);
    QVERIFY(line);
    const auto linePixels = readTile(line, document.activeLayer(), {0, 0});
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Ellipse, {30, 40}, {210, 220}, 0.3}), Result::Success);
    const auto ellipse = render(renderer, document.preview(), line);
    QVERIFY(ellipse);
    const auto pixels = readTile(ellipse, document.activeLayer(), {0, 0});
    QVERIFY(pixels != linePixels);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), ellipse), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), ellipse);
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), document.activeLayer(), {0, 0}), pixels);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto cancelled = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(cancelled, {{{128, 128}, 1}}), Result::Success);
    QVERIFY(render(renderer, document.preview(), ellipse));
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), ellipse), document.activeLayer(), {0, 0}), pixels);
}

void KisGpuTileRendererTest::rectangleOutlinesPreserveRotationHistory_data()
{
    QTest::addColumn<qreal>("rotation");
    QTest::addColumn<QPoint>("edge");
    QTest::addColumn<bool>("rounded");
    QTest::addColumn<QSizeF>("radii");
    QTest::newRow("axis-aligned") << qreal(0) << QPoint(16, 32) << false << QSizeF(0, 0);
    QTest::newRow("rotated") << qreal(0.7853981633974483) << QPoint(9, 32) << false << QSizeF(0, 0);
    QTest::newRow("rounded") << qreal(0) << QPoint(16, 32) << true << QSizeF(14, 14);
    QTest::newRow("rounded-rotated") << qreal(0.7853981633974483) << QPoint(21, 21) << true << QSizeF(14, 14);
    QTest::newRow("rounded-clamped") << qreal(0) << QPoint(16, 32) << true << QSizeF(32, 32);
    QTest::newRow("rounded-asymmetric") << qreal(0) << QPoint(16, 32) << true << QSizeF(14, 4);
    QTest::newRow("rounded-zero-axis") << qreal(0) << QPoint(16, 32) << true << QSizeF(0, 14);
}

void KisGpuTileRendererTest::rectangleOutlinesPreserveRotationHistory()
{
    QFETCH(qreal, rotation);
    QFETCH(QPoint, edge);
    QFETCH(bool, rounded);
    QFETCH(QSizeF, radii);
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    const Shape shape{rounded ? ShapeKind::RoundedRectangle : ShapeKind::Rectangle, {16, 16}, {48, 48}, rotation,
                      radii};
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.preview());
    QVERIFY(preview);
    const auto pixels = readTile(preview, layer, {0, 0});
    const auto pixel = [&](const QByteArray &bytes, QPoint point) {
        return bytes.mid((point.y() * TileRenderer::TileSize + point.x()) * 4, 4);
    };
    for (const auto &point : {edge, QPoint(64 - edge.y(), edge.x()),
                              QPoint(64 - edge.x(), 64 - edge.y()), QPoint(edge.y(), 64 - edge.x())}) {
        QCOMPARE(pixel(pixels, point), QByteArray::fromHex("1181f1ff"));
    }
    QCOMPARE(pixel(pixels, {32, 32}), QByteArray(4, '\0'));
    QCOMPARE(pixel(pixels, {8, 8}), QByteArray(4, '\0'));
    if (rounded && rotation == 0 && radii.width() >= 14 && radii.height() >= 14)
        QCOMPARE(pixel(pixels, {16, 16}), QByteArray(4, '\0'));
    if (rounded && radii.width() == 0) QCOMPARE(pixel(pixels, {16, 16}), QByteArray::fromHex("1181f1ff"));
    if (rounded && (radii.width() > 16 || radii.height() > 16)) {
        auto bounded = shape;
        bounded.cornerRadii = {qMin(qreal(16), radii.width()), qMin(qreal(16), radii.height())};
        QCOMPARE(document.updateShape(stroke, bounded), Result::Success);
        QCOMPARE(readTile(render(renderer, document.preview()), layer, {0, 0}), pixels);
        QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    }
    const auto beforeRejectedShape = document.preview();
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Rectangle, {16, 16}, {16, 48}, rotation}), Result::InvalidInput);
    QCOMPARE(document.preview(), beforeRejectedShape);
    if (rounded) {
        auto invalid = shape;
        invalid.cornerRadii.setWidth(-1);
        QCOMPARE(document.updateShape(stroke, invalid), Result::InvalidInput);
        QCOMPARE(document.preview(), beforeRejectedShape);
    }
    QCOMPARE(document.undoCount(), 0);

    QVERIFY(document.hasActiveStroke());
    const auto retained = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(retained->shape.cornerRadii, shape.cornerRadii);
    QCOMPARE(readTile(render(renderer, document.preview()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.historyAction(1), HistoryAction::DrawRectangle);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), preview);
    QVERIFY(undone && undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), layer, {0, 0}), pixels);
    const auto cancelled = document.beginStroke(layer, brush, 8);
    QCOMPARE(document.appendSamples(cancelled, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.updateShape(cancelled, {ShapeKind::Rectangle, {20, 20}, {40, 40}, rotation}), Result::Success);
    QVERIFY(render(renderer, document.preview(), preview));
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    QCOMPARE(readTile(preview, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::polylinesPreserveVerticesClosureHistory_data()
{
    QTest::addColumn<bool>("closed");
    QTest::newRow("open") << false;
    QTest::newRow("closed") << true;
}

void KisGpuTileRendererTest::polylinesPreserveVerticesClosureHistory()
{
    QFETCH(bool, closed);
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    const QList<InputSample> vertices{{{16, 16}, 1}, {{48, 16}, 1}, {{48, 48}, 1}};
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {vertices.first()}), Result::Success);
    const Shape shape{closed ? ShapeKind::Polygon : ShapeKind::Polyline, {16, 16}, {48, 48}, 0};
    const auto single = document.preview();
    QCOMPARE(document.updateShape(stroke, shape), Result::InvalidInput);
    QCOMPARE(document.preview(), single);
    QCOMPARE(document.appendSamples(stroke, vertices.mid(1)), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.preview());
    QVERIFY(preview);
    const auto pixels = readTile(preview, layer, {0, 0});
    const auto pixel = [&](QPoint p) { return pixels.mid((p.y() * TileRenderer::TileSize + p.x()) * 4, 4); };
    QCOMPARE(pixel({32, 16}), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel({48, 32}), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel({32, 32}), closed ? QByteArray::fromHex("1181f1ff") : QByteArray(4, '\0'));
    QCOMPARE(pixel({16, 48}), QByteArray(4, '\0'));
    OperationDocument oracle({64, 64});
    auto path = vertices;
    if (closed) path.append(path.first());
    const auto expected = oracle.beginStroke(oracle.activeLayer(), brush, 7);
    QCOMPARE(oracle.appendSamples(expected, path), Result::Success);
    QCOMPARE(oracle.commitStroke(expected), Result::Success);
    QCOMPARE(readTile(render(renderer, oracle.snapshot()), oracle.activeLayer(), {0, 0}), pixels);
    const auto operation = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), vertices);
    QCOMPARE(document.undoCount(), 0);

    QVERIFY(document.hasActiveStroke());
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers.first().operations.last())->inputSamples(), vertices);
    QCOMPARE(readTile(render(renderer, document.preview()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.historyAction(1), closed ? HistoryAction::DrawPolygon : HistoryAction::DrawPolyline);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), preview);
    QVERIFY(undone && undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), layer, {0, 0}), pixels);
    const auto cancelled = document.beginStroke(layer, brush, 8);
    QCOMPARE(document.appendSamples(cancelled, vertices), Result::Success);
    QCOMPARE(document.updateShape(cancelled, shape), Result::Success);
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::versionsOutliveTheRendererAndRejectAnotherDeviceOwner()
{
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    RenderedSnapshot retained;
    {
        TileRenderer owner(m_device);
        retained = render(owner, document.snapshot());
        QVERIFY(retained);
    }
    QCOMPARE(readTile(retained, document.activeLayer(), {0, 0}).size(), TileRenderer::TileBytes);
    TileRenderer replacement(m_device);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    QCOMPARE(replacement.record(commands, document.snapshot(), retained).code, RenderOutcome::InvalidVersion);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(render(replacement, document.snapshot()));
}

void KisGpuTileRendererTest::visibleLayersComposeOnTheGpuAndReuseUnchangedTiles()
{
    OperationDocument document({1024, 1024});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 16;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto top = document.addLayer(QStringLiteral("Blue"));
    brush.rgba = {0, 0, 255, 255};
    brush.opacity = 128.0 / 255;
    stroke = document.beginStroke(top, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto layers = render(renderer, document.snapshot());
    QVERIFY(layers);
    const auto composed = project(renderer, layers, {0, 0, 256, 256});
    QVERIFY(composed);
    QCOMPARE(composed->tiles().size(), 1);
    const auto pixels = readBuffer(composed->tiles().first().pixels);
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("7f0080ff"));
    const auto outside = project(renderer, layers, {512, 512, 256, 256});
    QVERIFY(outside);
    QVERIFY(outside->tiles().isEmpty());
    QVERIFY(document.addLayer(QStringLiteral("Empty")));
    const auto expanded = render(renderer, document.snapshot(), layers);
    QVERIFY(expanded);
    const auto reused = project(renderer, expanded, {0, 0, 256, 256}, composed);
    QVERIFY(reused);
    QCOMPARE(reused->tiles().first().pixels, composed->tiles().first().pixels);
    QCOMPARE(document.setLayerVisible(top, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), expanded);
    const auto red = project(renderer, hidden, {0, 0, 256, 256}, reused);
    QVERIFY(red);
    QCOMPARE(readBuffer(red->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), pixels);
}

void KisGpuTileRendererTest::compositionPreservesOrderAlphaAndExistingVersionsAtCapacity()
{
    OperationDocument document({64, 64});
    const QList<RgbaPixel> colors{{255, 0, 0, 255}, {0, 0, 255, 128}, {0, 255, 0, 64}, {255, 255, 255, 128}};
    LayerId green = 0;
    for (qsizetype i = 0; i < colors.size(); ++i) {
        const auto layer = i == 0 ? document.activeLayer() : document.addLayer(QString::number(i));
        if (i == 2) green = layer;
        Brush brush;
        brush.rgba = colors[i];
        brush.opacity = qreal(colors[i][3]) / 255;
        brush.rgba[3] = 255;
        brush.diameter = 16;
        const auto stroke = document.beginStroke(layer, brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    TileRenderer renderer(m_device, 6 * TileRenderer::TileBytes);
    const auto layers = render(renderer, document.snapshot());
    QVERIFY(layers);
    auto composed = project(renderer, layers, {0, 0, 64, 64});
    QVERIFY(composed);
    const auto original = readBuffer(composed->tiles().first().pixels);
    QCOMPARE(original.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("afa0b0ff"));
    QCOMPARE(renderer.residentBytes(), 5 * TileRenderer::TileBytes);
    QCOMPARE(document.setLayerVisible(green, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), layers);
    QVERIFY(hidden);
    auto reduced = project(renderer, hidden, {0, 0, 64, 64}, composed);
    QVERIFY(reduced);
    QCOMPARE(readBuffer(reduced->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), original);
    QVERIFY(renderer.residentBytes() <= 6 * TileRenderer::TileBytes);
    const auto lower = document.snapshot()->layers.first().id;
    QCOMPARE(document.setLayerOpacity(lower, 137), Result::Success);
    const auto changed = render(renderer, document.snapshot(), hidden);
    QVERIFY(changed);
    QVERIFY(!project(renderer, changed, {0, 0, 64, 64}, reduced));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), original);
    QCOMPARE(readBuffer(reduced->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
    composed.clear();
    reduced.clear();
    QCOMPARE(document.setLayerOpacity(lower, 255), Result::Success);
    const auto restored = project(renderer, render(renderer, document.snapshot(), changed), {0, 0, 64, 64});
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
}

void KisGpuTileRendererTest::paintingAndCompositionShareOneSubmission()
{
    OperationDocument document({64, 64});
    for (const auto &color : QList<RgbaPixel>{{255, 0, 0, 128}, {0, 0, 255, 128}}) {
        if (!document.snapshot()->layers.first().operations.isEmpty()) QVERIFY(document.addLayer(QStringLiteral("Top")));
        Brush brush;
        brush.rgba = color;
        brush.opacity = qreal(color[3]) / 255;
        brush.rgba[3] = 255;
        brush.diameter = 16;
        const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    TileRenderer renderer(m_device);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto readback = Detail::ReadbackBatch::create(*m_device, 1);
    QVERIFY(readback);
    QVERIFY(readback->recordBuffer(*m_device, commands, 0, projection.version->tiles().first().pixels, TileRenderer::TileBytes));
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readback->results.front()->data.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("5500aac0"));
}

void KisGpuTileRendererTest::longStrokeRebuildKeepsPixelsAndHistory()
{
    Brush brush;
    brush.diameter = 8;
    brush.spacing = 0.25;
    brush.rgba = {71, 139, 211, 255};
    QList<InputSample> samples{{{32, 32}, 1}};
    for (int i = 0; i < 2304; ++i)
        samples.append({{qreal(6 + i % 56), qreal(6 + (i * 7) % 56)}, 1});

    OperationDocument partitioned({64, 64});
    const auto referenceStroke = partitioned.beginStroke(partitioned.activeLayer(), brush, 97);
    TileRenderer reference(m_device);
    RenderedSnapshot incremental;
    for (qsizetype first = 0; first < samples.size(); first += 128) {
        QCOMPARE(partitioned.appendSamples(referenceStroke, samples.mid(first, 128)), Result::Success);
        incremental = render(reference, partitioned.preview(), incremental);
        QVERIFY(incremental);
    }
    QCOMPARE(partitioned.commitStroke(referenceStroke), Result::Success);
    incremental = render(reference, partitioned.snapshot(), incremental);
    QVERIFY(incremental);
    const auto expected = readTile(incremental, partitioned.activeLayer(), {0, 0});

    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, brush, 97);
    QCOMPARE(document.appendSamples(stroke, samples), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);

    TileRenderer bounded(m_device, 2 * TileRenderer::TileBytes);
    const auto original = render(bounded, document.snapshot());
    QVERIFY(original);
    const auto pixels = readTile(original, layer, {0, 0});
    QCOMPARE(pixels, expected);
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("478bd3ff"));
    QCOMPARE(pixels.right(4), QByteArray(4, char(0)));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(bounded, document.snapshot(), original);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(bounded, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);
    QCOMPARE(readTile(original, layer, {0, 0}), pixels);
    QVERIFY(bounded.residentBytes() <= 2 * TileRenderer::TileBytes);

    TileRenderer rebuilt(m_device, TileRenderer::TileBytes);
    const auto restored = render(rebuilt, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, document.activeLayer(), {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("long-stroke-rebuild.rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::retainedPixelsOwnTheNativeDeviceUntilTheirLastReader()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    std::weak_ptr<Device> lifetime = device;
    RenderedSnapshot retained;
    ProjectedSnapshot projected;
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    {
        TileRenderer renderer(device);
        WGPUCommandEncoder commands = nullptr;
        commands = device->beginCommands();
        QVERIFY(commands);
        const auto result = renderer.record(commands, document.snapshot());
        QCOMPARE(result.code, RenderOutcome::Recorded);
        retained = result.version;
        const auto merged = renderer.compose(commands, retained, {0, 0, 64, 64});
        QCOMPARE(merged.code, RenderOutcome::Recorded);
        projected = merged.version;
        QVERIFY(device->submitCommands(true));
    }
    device.reset();
    QVERIFY(!lifetime.expired());
    retained.clear();
    QVERIFY(!lifetime.expired());
    {
        const auto reader = lifetime.lock();
        WGPUCommandEncoder commands = nullptr;
        commands = reader->beginCommands();
        QVERIFY(commands);
        const auto pixels = Detail::ReadbackBatch::create(*reader, 1);
        QVERIFY(pixels);
        QVERIFY(pixels->recordBuffer(*reader, commands, 0, projected->tiles().first().pixels, TileRenderer::TileBytes));
        QVERIFY(reader->submitCommands(true));
        QCOMPARE(pixels->results.front()->data.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("000000ff"));
    }
    projected.clear();
    QVERIFY(lifetime.expired());
}

void KisGpuTileRendererTest::nativeDeviceCreationRequiresTheApplicationThread()
{
    DevicePtr device;
    QString diagnostic;
    std::unique_ptr<QThread> worker(QThread::create([&] { device = Device::createNative(diagnostic); }));
    worker->start();
    QVERIFY(worker->wait(2000));
    QVERIFY(!device);
    QVERIFY(!diagnostic.isEmpty());
}

void KisGpuTileRendererTest::rasterUploadsPreservePixelsThroughPaintingUndo()
{
    RasterImage image;
    image.size = {510, 255};
    const QByteArray invisible = QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4);
    const QByteArray blue = QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4);
    const QByteArray outside = QByteArray::fromHex("00ff0040").repeated(RgbaTile::ByteCount / 4);
    image.layers = {{QStringLiteral("Imported"), true, {}, {{{0, 0}, invisible}, {{1, 0}, blue}, {{-1, 0}, outside}}}};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device, 8 * TileRenderer::TileBytes);
    const auto imported = render(renderer, document.snapshot());
    QVERIFY(imported);
    QCOMPARE(imported->tiles(layer).size(), 3);
    QCOMPARE(readTile(imported, layer, {0, 0}), invisible);
    QCOMPARE(readTile(imported, layer, {1, 0}), blue);
    QCOMPARE(readTile(imported, layer, {-1, 0}), outside);
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto painted = render(renderer, document.snapshot(), imported);
    QVERIFY(painted);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixels.first(4), QByteArray::fromHex("120a2700"));
    QCOMPARE(pixels.last(4), QByteArray::fromHex("120a2700"));
    QCOMPARE(readTile(imported, layer, {0, 0}), invisible);
    for (const auto &tile : painted->tiles(layer)) {
        if (tile.coordinate == QPoint(0, 0)) continue;
        for (const auto &old : imported->tiles(layer)) {
            if (old.coordinate == tile.coordinate) QCOMPARE(tile.pixels, old.pixels);
        }
    }
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), painted);
    QVERIFY(undone);
    QCOMPARE(readTile(undone, layer, {0, 0}), invisible);
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);

}

void KisGpuTileRendererTest::pendingTileWritesUseTheLatestUploadWithinTwoBuffers()
{
    OperationDocument document({256, 256});
    const auto layer = document.activeLayer();
    Brush blue;
    blue.rgba = {0, 0, 255, 255};
    for (int i = 0; i < 2; ++i) {
        const auto color = i % 2 ? QByteArray::fromHex("00ff00ff") : QByteArray::fromHex("ff0000ff");
        QCOMPARE(document.replaceTiles(layer, {{{0, 0}, color.repeated(RgbaTile::ByteCount / 4)}}), Result::Success);
        const auto stroke = document.beginStroke(layer, blue, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    QCOMPARE(document.undoCount(), 4);
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    const auto pixels = readTile(original, layer, {0, 0});
    QCOMPARE(pixels.first(4), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    const auto invisible = QByteArray::fromHex("0a141e00").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, invisible}}), Result::Success);
    const auto replaced = render(renderer, document.snapshot(), original);
    QVERIFY(replaced);
    QCOMPARE(readTile(replaced, layer, {0, 0}), invisible);
    QCOMPARE(readTile(original, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::tileWritesPreservePaintingOnOtherCoordinates()
{
    OperationDocument document({512, 256});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}, {{400, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto green = QByteArray::fromHex("00ff00ff").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, green}}), Result::Success);
    brush.rgba = {0, 0, 255, 255};
    stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{264, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    QCOMPARE(readTile(rendered, layer, {0, 0}), green);
    const auto right = readTile(rendered, layer, {1, 0});
    QCOMPARE(right.mid((32 * 256 + 8) * 4, 4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(right.mid((32 * 256 + 44) * 4, 4), QByteArray::fromHex("ff0000ff"));
}

void KisGpuTileRendererTest::implicitLayerPixelsStaySparseThroughEditsAndProjection_data()
{
    QTest::addColumn<QByteArray>("defaultRgba");
    QTest::newRow("opaque-white") << QByteArray::fromHex("ffffffff");
    QTest::newRow("transparent-rgb") << QByteArray::fromHex("11171f00");
}

void KisGpuTileRendererTest::implicitLayerPixelsStaySparseThroughEditsAndProjection()
{
    QFETCH(QByteArray, defaultRgba);
    RasterImage image;
    image.size = {4096, 4096};
    image.layers = {{QStringLiteral("White"), true, {}, {}}};
    for (int i = 0; i < 4; ++i) image.layers[0].defaultPixel[i] = quint8(defaultRgba[i]);
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device, 6 * TileRenderer::TileBytes);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    QVERIFY(original->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
    const auto corner = project(renderer, original, {4000, 4000, 96, 96});
    QVERIFY(corner);
    QCOMPARE(corner->tiles().size(), 1);
    QCOMPARE(corner->tiles().first().coordinate, QPoint(15, 15));
    QCOMPARE(readBuffer(corner->tiles().first().pixels), defaultRgba.repeated(RgbaTile::ByteCount / 4));
    Brush brush;
    brush.rgba = {0, 0, 255, 255};
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto painted = render(renderer, document.snapshot(), original);
    QVERIFY(painted);
    QCOMPARE(painted->tiles(layer).size(), 1);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.first(4), defaultRgba);
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    const auto visible = project(renderer, painted, {0, 0, 512, 256});
    QVERIFY(visible);
    QCOMPARE(visible->tiles().size(), 2);
    QCOMPARE(visible->tiles()[1].pixels, corner->tiles().first().pixels);
    QCOMPARE(readBuffer(corner->tiles().first().pixels).first(4), defaultRgba);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), painted);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(2 * TileRenderer::TileBytes));

    QCOMPARE(document.redo(), Result::Success);
    TileRenderer rebuilt(m_device, 2 * TileRenderer::TileBytes);
    const auto restored = render(rebuilt, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::implicitLayersRespectCompositionAndCapacity()
{
    RasterImage image;
    image.size = {512, 256};
    image.layers = {{QStringLiteral("Red"), true, {}, {}}, {QStringLiteral("Blue"), true, {}, {}}};
    image.layers[0].defaultPixel = {255, 0, 0, 255};
    image.layers[1].defaultPixel = {0, 0, 255, 128};
    OperationDocument document(image);
    TileRenderer renderer(m_device, 4 * TileRenderer::TileBytes);
    const auto both = render(renderer, document.snapshot());
    QVERIFY(both);
    const auto merged = project(renderer, both, {0, 0, 512, 256});
    QVERIFY(merged);
    QCOMPARE(merged->tiles().size(), 2);
    for (const auto &tile : merged->tiles()) QCOMPARE(readBuffer(tile.pixels).first(4), QByteArray::fromHex("7f0080ff"));
    QCOMPARE(document.setLayerVisible(2, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), both);
    QVERIFY(hidden);
    const auto red = project(renderer, hidden, {0, 0, 512, 256}, merged);
    QVERIFY(red);
    QCOMPARE(red->tiles().size(), 2);
    for (const auto &tile : red->tiles()) QCOMPARE(readBuffer(tile.pixels).first(4), QByteArray::fromHex("ff0000ff"));
    TileRenderer small(m_device, TileRenderer::TileBytes);
    QVERIFY(!render(small, document.snapshot()));
    QCOMPARE(small.residentBytes(), qsizetype(0));
    image.size = {4096, 4096};
    image.layers.removeLast();
    OperationDocument large(image);
    const auto sparse = render(small, large.snapshot());
    QVERIFY(sparse);
    QVERIFY(!project(small, sparse, {0, 0, 4096, 4096}));
    QCOMPARE(small.residentBytes(), qsizetype(TileRenderer::TileBytes));
}

void KisGpuTileRendererTest::convolutionCapacityKeepsReadersAndSparseSelectionRetry_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8") << false;
    QTest::newRow("u16") << true;
}

void KisGpuTileRendererTest::convolutionCapacityKeepsReadersAndSparseSelectionRetry()
{
    QFETCH(bool, words);
    const auto format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const auto tileBytes = RgbaTile::byteCount(format);
    const RgbaPixel color = words ? RgbaPixel{11111, 22222, 33333, 65535} : RgbaPixel{17, 91, 203, 255};
    const auto bytes = rgbaBytes(color, format).repeated(RgbaTile::Size * RgbaTile::Size);
    RasterImage input; input.size = {8192, 8192}; input.format = format;
    input.layers = {{QStringLiteral("Source"), true, {}, {{{0, 0}, bytes, format}}}};
    input.layers.first().defaultPixel = color;
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device, 8 * tileBytes);
    const auto before = render(renderer, document.snapshot()); QVERIFY(before);
    FilterSettings filter; filter.kind = FilterKind::Sharpen;
    QCOMPARE(document.convolutionFilter(layer, filter, {0, 0, 8192, 8192}), Result::Success);
    auto *commands = m_device->beginCommands(); QVERIFY(commands);
    const auto rejected = renderer.record(commands, document.snapshot(), before);
    QCOMPARE(rejected.code, RenderOutcome::CapacityExceeded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readTile(before, layer, {0, 0}), bytes);
    commands = m_device->beginCommands(); QVERIFY(commands);
    const auto viewport = renderer.recordRegion(commands, document.snapshot(), {0, 0, 64, 64}, {layer}, before);
    QVERIFY2(viewport.code == RenderOutcome::Recorded, qPrintable(viewport.diagnostic));
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readTile(viewport.version, layer, {0, 0}), bytes);
    QVERIFY(renderer.residentBytes() <= 8 * tileBytes);
    commands = m_device->beginCommands(); QVERIFY(commands);
    auto transient = renderer.recordRegion(commands, document.snapshot(), {0, 0, 64, 64}, {layer}, before);
    QVERIFY2(transient.code == RenderOutcome::Recorded, qPrintable(transient.diagnostic));
    const auto pendingBytes = renderer.residentBytes();
    transient.version.clear();
    QCOMPARE(renderer.residentBytes(), pendingBytes);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readTile(before, layer, {0, 0}), bytes);

    // Two distant selected pixels require two output tiles, even though their
    // bounding rectangle spans thirty-two tiles.
    OperationDocument sparse(input);
    QByteArray alpha(8192, '\0'); alpha.front() = char(255); alpha.back() = char(255);
    const auto mask = SelectionMask::fromAlphaCoverage({0, 0, 8192, 1}, alpha, {0, 0, 8192, 1}); QVERIFY(mask);
    QCOMPARE(sparse.setSelectionMask(mask), Result::Success);
    QCOMPARE(sparse.convolutionFilter(sparse.activeLayer(), filter, {0, 0, 8192, 8192}), Result::Success);
    TileRenderer selected(m_device, 8 * tileBytes);
    commands = m_device->beginCommands(); QVERIFY(commands);
    const auto result = selected.recordRegion(commands, sparse.snapshot(), {0, 0, 8192, 1}, {sparse.activeLayer()});
    QVERIFY2(result.code == RenderOutcome::Recorded, qPrintable(result.diagnostic));
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readTile(result.version, sparse.activeLayer(), {0, 0}), bytes);
    QCOMPARE(readTile(result.version, sparse.activeLayer(), {31, 0}), bytes);
    QVERIFY(selected.residentBytes() <= 8 * tileBytes);
}

void KisGpuTileRendererTest::removingFigureCoverageRestoresItsPreviewBackground()
{
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    const auto id = document.beginStroke(layer, brush, 42);
    QCOMPARE(document.appendSamples(id, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.preview());
    QVERIFY(painted);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("1181f1ff"));
    FigureStyle style;
    style.outline = false;
    QCOMPARE(document.updateFigure(id, {ShapeKind::Rectangle, {8, 8}, {56, 56}, 0}, style), Result::Success);
    const auto empty = render(renderer, document.preview(), painted);
    QVERIFY(empty);
    QVERIFY(empty->tiles(layer).isEmpty());
    QCOMPARE(readTile(painted, layer, {0, 0}), pixels);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 1);
}

void KisGpuTileRendererTest::collinearFillPreservesPixelsWithinTileBudget_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<int>("vertices");
    for (const auto kind : {ShapeKind::Polyline, ShapeKind::Polygon}) {
        for (int vertices : {2, 3}) {
            const auto name = QString::number(int(kind)) + '-' + QString::number(vertices);
            QTest::newRow(qPrintable(name)) << int(kind) << vertices;
        }
    }
}

void KisGpuTileRendererTest::collinearFillPreservesPixelsWithinTileBudget()
{
    QFETCH(int, kind);
    QFETCH(int, vertices);
    OperationDocument document(QSize(1024, 1024));
    const auto layer = document.activeLayer();
    QList<InputSample> samples{{{0, 0}, 1}};
    if (vertices == 3) samples.append({{512, 512}, 1});
    samples.append({{1024, 1024}, 1});
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{221, 17, 44, 255};
    QCOMPARE(document.paintFigure(layer, Brush{}, samples,
                                    {static_cast<ShapeKind>(kind), samples.first().position,
                                     samples.last().position, 0}, style, 42), Result::Success);
    TileRenderer renderer(m_device, TileRenderer::TileBytes);
    auto *commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto result = renderer.record(commands, document.snapshot());
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(result.code, RenderOutcome::Recorded);
    QVERIFY(result.version->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(0));
    QCOMPARE(document.undoCount(), 1);
}

void KisGpuTileRendererTest::filledInteriorRespectsRegionAndTileBudget()
{
    OperationDocument document(QSize(1024, 1024));
    const auto layer = document.activeLayer();
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{221, 17, 44, 255};
    QCOMPARE(document.paintFigure(layer, Brush{}, {{{0, 0}, 1}, {{1024, 1024}, 1}},
                                    {ShapeKind::Rectangle, {0, 0}, {1024, 1024}, 0}, style, 42), Result::Success);
    TileRenderer bounded(m_device, 9 * TileRenderer::TileBytes);
    auto *commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto whole = bounded.record(commands, document.snapshot());
    QCOMPARE(whole.code, RenderOutcome::CapacityExceeded);
    QCOMPARE(bounded.residentBytes(), qsizetype(0));
    TileRenderer viewport(m_device, TileRenderer::TileBytes);
    const auto cropped = viewport.recordRegion(commands, document.snapshot(), {0, 0, 64, 64}, {layer});
    QCOMPARE(cropped.code, RenderOutcome::Recorded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(viewport.residentBytes(), qsizetype(TileRenderer::TileBytes));
    QCOMPARE(readTile(cropped.version, layer, {0, 0}), QByteArray::fromHex("dd112cff").repeated(TileRenderer::TileBytes / 4));
}

void KisGpuTileRendererTest::solidFiguresPreserveFillOutlineHistory_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<bool>("outline");
    for (const auto kind : {ShapeKind::Rectangle, ShapeKind::Ellipse, ShapeKind::RoundedRectangle,
                            ShapeKind::Polyline, ShapeKind::Polygon}) {
        for (bool outline : {false, true}) {
            const auto name = QString::number(int(kind)) + (outline ? QStringLiteral("-fill-outline") : QStringLiteral("-fill"));
            QTest::newRow(qPrintable(name)) << int(kind) << outline;
        }
    }
}

void KisGpuTileRendererTest::solidFiguresPreserveFillOutlineHistory()
{
    QFETCH(int, kind);
    QFETCH(bool, outline);
    const auto shapeKind = static_cast<ShapeKind>(kind);
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 4;
    const auto id = document.beginStroke(layer, brush, 42);
    QVERIFY(!id.isNull());
    const QList<InputSample> samples = shapeKind == ShapeKind::Polyline || shapeKind == ShapeKind::Polygon ?
        QList<InputSample>{{{8, 8}, 1}, {{56, 8}, 1}, {{32, 56}, 1}} :
        QList<InputSample>{{{8, 8}, 1}, {{56, 56}, 1}};
    QCOMPARE(document.appendSamples(id, samples), Result::Success);
    Shape shape{shapeKind, samples.first().position, samples.last().position, 0};
    if (shapeKind == ShapeKind::RoundedRectangle) shape.cornerRadii = {8, 8};
    FigureStyle style;
    style.outline = outline;
    style.fill = RgbaPixel{221, 17, 44, 255};
    QCOMPARE(document.updateFigure(id, shape, style), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device);
    const auto version = render(renderer, document.snapshot());
    QVERIFY(version);
    const auto pixels = readTile(version, layer, {0, 0});
    QCOMPARE(pixels.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return pixels.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    QCOMPARE(pixel(32, 24), QByteArray::fromHex("dd112cff"));
    const auto topPixel = shapeKind == ShapeKind::Ellipse ?
        (outline ? QByteArray::fromHex("1181f1ff") : QByteArray::fromHex("dd112cfa")) : QByteArray::fromHex("dd112cff");
    QCOMPARE(pixel(32, 8), topPixel);
    QCOMPARE(pixel(32, 7), outline ? QByteArray::fromHex("1181f1ff") : QByteArray(4, char(0)));
    QCOMPARE(pixel(0, 0), QByteArray(4, char(0)));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), version);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);

    const auto restored = render(renderer, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::figureFillMatchesPainterCoverage_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto *kind : {"ellipse", "ellipse-48", "rounded-rectangle", "triangle", "reverse-triangle",
                             "small-curve", "clipped-curve", "odd-even", "winding", "mirrored", "mirrored-half-grid", "mirrored-collapsed",
                             "large-coordinates", "large-vertical", "distant-mirror"})
        QTest::newRow(kind) << QString::fromLatin1(kind);
}

void KisGpuTileRendererTest::figureFillMatchesPainterCoverage()
{
    QFETCH(QString, kind);
    const bool distantMirror = kind == QLatin1String("distant-mirror");
    const QPoint origin(distantMirror ? int(OperationDocument::CoordinateLimit) - 256 : 0, 0);
    QPainterPath path;
    const bool mirrored = kind.startsWith(QLatin1String("mirrored"));
    if (kind == QLatin1String("mirrored-collapsed")) path.addRect(QRectF(16 + 1. / 128, 16, 1. / 128, 16));
    else if (kind == QLatin1String("mirrored-half-grid")) path.addRect(QRectF(16 + 1. / 128, 16 + 1. / 128, 32, 32));
    else if (kind == QLatin1String("ellipse") || mirrored) path.addEllipse(QRectF(16, 16, 32, 32));
    else if (kind == QLatin1String("ellipse-48")) path.addEllipse(QRectF(8, 8, 48, 48));
    else if (kind == QLatin1String("rounded-rectangle")) path.addRoundedRect(QRectF(16, 16, 32, 32), 14, 14);
    else if (kind == QLatin1String("small-curve")) {
        path.moveTo(16, 16);
        path.cubicTo(16.25, 16.0625, 16.5, 16.0625, 16.75, 16);
        path.lineTo(16.75, 17);
        path.lineTo(16, 17);
    } else if (kind == QLatin1String("clipped-curve")) {
        path.addEllipse(QRectF(-9.25, -7.125, 32.5, 40.75));
    } else if (kind == QLatin1String("odd-even") || kind == QLatin1String("winding")) {
        path.addEllipse(QRectF(8.125, 8.25, 48, 48));
        path.addEllipse(QRectF(16.375, 16.5, 32, 32));
        if (kind == QLatin1String("winding")) path.setFillRule(Qt::WindingFill);
    } else if (kind == QLatin1String("large-coordinates")) {
        const auto limit = OperationDocument::CoordinateLimit;
        path.moveTo(-limit, -limit);
        path.lineTo(limit, -limit);
        path.lineTo(limit, limit);
    } else if (kind == QLatin1String("large-vertical")) {
        const auto limit = OperationDocument::CoordinateLimit;
        path.addRect(QRectF(-limit / 2, -limit / 2, limit, limit));
    } else if (distantMirror) {
        path.addRect(QRectF(OperationDocument::CoordinateLimit - .5, 0, .5, 1));
    } else {
        path.moveTo(16, 16);
        path.lineTo(48, 16);
        path.lineTo(48, 48);
        if (kind == QLatin1String("reverse-triangle")) path = path.toReversed();
    }
    path.closeSubpath();
    Brush brush;
    if (mirrored) {
        brush.mirrorHorizontal = brush.mirrorVertical = true;
        brush.mirrorCenter = {32, 32};
    } else if (distantMirror) {
        brush.mirrorHorizontal = true;
        brush.mirrorCenter = {-int(OperationDocument::CoordinateLimit), 0};
    }
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.windingFill = path.fillRule() == Qt::WindingFill;
    QList<InputSample> samples;
    for (int i = 0; i < path.elementCount(); ++i) {
        const auto point = path.elementAt(i);
        shape.pathCommands.append(point.isMoveTo() ? PathCommand::MoveTo :
                                  point.isLineTo() ? PathCommand::LineTo : PathCommand::CubicTo);
        samples.append({point, 1});
        if (point.isCurveTo()) {
            samples.append({path.elementAt(++i), 1});
            samples.append({path.elementAt(++i), 1});
        }
    }
    QImage mask(256, 256, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    {
        QPainter painter(&mask);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(-origin);
        painter.fillPath(path, Qt::white);
        if (mirrored) {
            for (const auto &scale : {QPointF(-1, 1), QPointF(1, -1), QPointF(-1, -1)}) {
                QTransform mirror;
                mirror.translate(32, 32).scale(scale.x(), scale.y()).translate(-32, -32);
                painter.fillPath(mirror.map(path), Qt::white);
            }
        }
    }
    QByteArray expected(TileRenderer::TileBytes, '\0');
    for (int y = 0; y < mask.height(); ++y) {
        for (int x = 0; x < mask.width(); ++x) {
            const int alpha = qAlpha(mask.pixel(x, y));
            if (!alpha) continue;
            const int offset = (y * 256 + x) * 4;
            expected[offset] = 17;
            expected[offset + 1] = char(129);
            expected[offset + 2] = char(241);
            expected[offset + 3] = char(alpha);
        }
    }
    OperationDocument document(QSize(origin.x() + 256, 256));
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{17, 129, 241, 255};
    QCOMPARE(document.paintFigure(document.activeLayer(), brush, samples, shape, style, 42), Result::Success);
    TileRenderer renderer(m_device);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    auto finishFrame = qScopeGuard([&] { m_device->submitCommands(true); });
    const auto rendered = renderer.recordRegion(commands, document.snapshot(), QRect(origin, QSize(256, 256)), {document.activeLayer()});
    QCOMPARE(rendered.code, RenderOutcome::Recorded);
    QVERIFY(m_device->submitCommands(true));
    finishFrame.dismiss();
    const auto actual = readTile(rendered.version, document.activeLayer(), {origin.x() / 256, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        for (const auto &entry : {qMakePair(QStringLiteral("actual"), actual), qMakePair(QStringLiteral("expected"), expected)}) {
            QFile file(QDir(directory).filePath(QStringLiteral("figure-fill-%1-%2.rgba").arg(kind, entry.first)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), qint64(entry.second.size()));
        }
    }
    QCOMPARE(actual, expected);
}

void KisGpuTileRendererTest::emptyFillKeepsLineOutline()
{
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo};
    const QList<InputSample> samples{{{16, 16}, 1}, {{32, 16}, 1}};
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    brush.opacity = 128.0 / 255;
    QByteArray reference;
    for (bool filled : {false, true}) {
        OperationDocument document(QSize(64, 64));
        const auto layer = document.activeLayer();
        FigureStyle style;
        if (filled) style.fill = RgbaPixel{221, 17, 44, 255};
        QCOMPARE(document.paintFigure(layer, brush, samples, shape, style, 42), Result::Success);
        TileRenderer renderer(m_device);
        const auto version = render(renderer, document.snapshot());
        QVERIFY(version);
        const auto pixels = readTile(version, layer, {0, 0});
        QCOMPARE(pixels.size(), TileRenderer::TileBytes);
        QCOMPARE(pixels.mid((16 * TileRenderer::TileSize + 16) * 4, 4), QByteArray::fromHex("1181f180"));
        if (!filled) reference = pixels;
        else QCOMPARE(pixels, reference);
        QCOMPARE(document.undo(), Result::Success);
        const auto undone = render(renderer, document.snapshot(), version);
        QVERIFY(undone);
        QVERIFY(undone->tiles(layer).isEmpty());
        QCOMPARE(document.redo(), Result::Success);
        const auto redone = render(renderer, document.snapshot(), undone);
        QVERIFY(redone);
        QCOMPARE(readTile(redone, layer, {0, 0}), pixels);

        const auto restored = render(renderer, document.snapshot());
        QVERIFY(restored);
        QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
    }
}

void KisGpuTileRendererTest::longFillEdgesKeepPixelsAcrossRegions()
{
    const auto limit = OperationDocument::CoordinateLimit;
    OperationDocument document(QSize(1024, 256));
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::LineTo, PathCommand::LineTo};
    const QList<InputSample> samples{{{-limit, 16.25}, 1}, {{limit, 16.265625}, 1},
                                     {{limit, 48}, 1}, {{-limit, 48}, 1}};
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{17, 129, 241, 255};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, samples, shape, style, 42), Result::Success);
    TileRenderer renderer(m_device);
    QByteArray reference;
    for (const auto &region : {QRect(0, 0, 256, 256), QRect(-256, 0, 768, 256), QRect(0, 0, 768, 256)}) {
        const auto commands = m_device->beginCommands();
        QVERIFY(commands);
        auto finishFrame = qScopeGuard([&] { m_device->submitCommands(true); });
        const auto version = renderer.recordRegion(commands, document.snapshot(), region, {document.activeLayer()});
        QCOMPARE(version.code, RenderOutcome::Recorded);
        QVERIFY(m_device->submitCommands(true));
        finishFrame.dismiss();
        const auto pixels = readTile(version.version, document.activeLayer(), {0, 0});
        QCOMPARE(pixels.size(), TileRenderer::TileBytes);
        // The original 26.6 endpoints span 4/256 pixel vertically. At x=0,
        // integer cell splitting retains y=16*256+66, hence coverage 190.
        // Clipping the line before quantization would discard this precision.
        QCOMPARE(pixels.mid(16 * 256 * 4, 4), QByteArray::fromHex("1181f1be"));
        if (reference.isEmpty()) reference = pixels;
        else QCOMPARE(pixels, reference);
    }
}

void KisGpuTileRendererTest::pathCommandsPreserveCoverageHistory_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto &kind : {QStringLiteral("curve"), QStringLiteral("subpaths"),
                              QStringLiteral("even-odd"), QStringLiteral("winding"), QStringLiteral("empty")})
        QTest::newRow(qPrintable(kind)) << kind;
}

void KisGpuTileRendererTest::pathCommandsPreserveCoverageHistory()
{
    QFETCH(QString, kind);
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    Shape shape;
    shape.kind = ShapeKind::Path;
    QList<InputSample> samples;
    FigureStyle style;
    const bool fill = kind == QLatin1String("even-odd") || kind == QLatin1String("winding");
    if (fill) {
        shape.windingFill = kind == QLatin1String("winding");
        for (const auto &rect : {QRectF(8, 8, 48, 48), QRectF(24, 24, 16, 16)}) {
            shape.pathCommands.append(PathCommand::MoveTo);
            samples.append({rect.topLeft(), 1});
            for (const auto &point : {rect.topRight(), rect.bottomRight(), rect.bottomLeft(), rect.topLeft()}) {
                shape.pathCommands.append(PathCommand::LineTo);
                samples.append({point, 1});
            }
        }
        style.outline = false;
        style.fill = RgbaPixel{221, 17, 44, 255};
    } else if (kind == QLatin1String("curve")) {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
        samples = {{{12, 32}, 1}, {{20, 12}, 1}, {{40, 52}, 1}, {{52, 32}, 1}};
    } else if (kind == QLatin1String("subpaths")) {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo, PathCommand::LineTo};
        samples = {{{12, 12}, 1}, {{52, 12}, 1}, {{12, 52}, 1}, {{52, 52}, 1}};
    }
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    QCOMPARE(document.paintFigure(layer, brush, samples, shape, style, 42), Result::Success);
    TileRenderer renderer(m_device);
    const auto version = render(renderer, document.snapshot());
    QVERIFY(version);
    if (kind == QLatin1String("empty")) {
        QVERIFY(version->tiles(layer).isEmpty());
        QCOMPARE(document.undoCount(), 1);
        return;
    }
    const auto pixels = readTile(version, layer, {0, 0});
    QCOMPARE(pixels.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return pixels.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    if (fill) {
        QCOMPARE(pixel(16, 16), QByteArray::fromHex("dd112cff"));
        QCOMPARE(pixel(32, 32), shape.windingFill ? QByteArray::fromHex("dd112cff") : QByteArray(4, char(0)));
    } else {
        QCOMPARE(pixel(32, kind == QLatin1String("curve") ? 32 : 12), QByteArray::fromHex("1181f1ff"));
        if (kind == QLatin1String("subpaths")) QCOMPARE(pixel(32, 32), QByteArray(4, char(0)));
    }
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), version);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);

    const auto restored = render(renderer, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("path-") + kind + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::pixelCorrectionPreservesCornersSensorsAndIncrementalPixels_data()
{
    QTest::addColumn<QList<InputSample>>("input");
    QTest::addColumn<QList<InputSample>>("corrected");
    QTest::newRow("tap") << QList<InputSample>{{{12.2, 12.7}, 0.8}}
                         << QList<InputSample>{{{12.2, 12.7}, 0.8}};
    QTest::newRow("held-corner") << QList<InputSample>{{{4, 4}, 1}, {{12.2, 12.7}, 0.8},
                                                     {{13.2, 12.3}, 0.4}, {{13.8, 13.8}, 0.6}}
                                 << QList<InputSample>{{{12.5, 12.5}, 0.8}};
    QTest::newRow("flushed-corner") << QList<InputSample>{{{4, 4}, 1}, {{12.2, 12.7}, 0.8},
                                                        {{13.2, 12.3}, 0.4}, {{20.8, 20.8}, 0.6}}
                                    << QList<InputSample>{{{12.5, 12.5}, 0.8}, {{13.5, 12.5}, 0.4}, {{20.5, 20.5}, 0.6}};
    QTest::newRow("initial-tile-removed") << QList<InputSample>{{{300, 300}, 1}, {{12.2, 12.7}, 0.8}}
                                         << QList<InputSample>{{{12.5, 12.5}, 0.8}};
    QTest::newRow("negative-floor") << QList<InputSample>{{{4, 4}, 1}, {{-0.2, 12.7}, 0.8}, {{20.8, 20.8}, 0.6}}
                                    << QList<InputSample>{{{-0.5, 12.5}, 0.8}, {{20.5, 20.5}, 0.6}};
}

void KisGpuTileRendererTest::pixelCorrectionPreservesCornersSensorsAndIncrementalPixels()
{
    QFETCH(QList<InputSample>, input);
    QFETCH(QList<InputSample>, corrected);
    OperationDocument document(QSize(1024, 1024));
    Brush brush;
    brush.diameter = 3;
    brush.rgba = {17, 129, 241, 255};
    brush.pixelPerfect = true;
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 7);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    for (const auto &sample : input) {
        QCOMPARE(document.appendSamples(id, {sample}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(incremental->tiles(layer).size(), rebuilt->tiles(layer).size());
        for (const auto &tile : rebuilt->tiles(layer))
            QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(rebuilt, layer, tile.coordinate));
    }
    QCOMPARE(document.commitStroke(id), Result::Success);
    incremental = render(renderer, document.snapshot(), incremental);
    QVERIFY(incremental);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), input);
    OperationDocument oracle(QSize(1024, 1024));
    brush.pixelPerfect = false;
    const auto expectedId = oracle.beginStroke(oracle.activeLayer(), brush, 7);
    QCOMPARE(oracle.appendSamples(expectedId, corrected), Result::Success);
    QCOMPARE(oracle.commitStroke(expectedId), Result::Success);
    const auto expected = render(renderer, oracle.snapshot());
    QVERIFY(expected);
    QCOMPARE(incremental->tiles(layer).size(), expected->tiles(oracle.activeLayer()).size());
    for (const auto &tile : expected->tiles(oracle.activeLayer()))
        QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(expected, oracle.activeLayer(), tile.coordinate));
    const auto pixels = readTile(incremental, layer, {0, 0});
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), incremental)->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);

    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("pixel-perfect-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::basicSmoothingPreservesCurvesIdleCompletion_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto &kind : {QStringLiteral("curve"), QStringLiteral("idle-then-continue"),
                            QStringLiteral("idle-two"), QStringLiteral("tap"),
                            QStringLiteral("tap-after-two"), QStringLiteral("stationary")})
        QTest::newRow(qPrintable(kind)) << kind;
}

void KisGpuTileRendererTest::basicSmoothingPreservesCurvesIdleCompletion()
{
    QFETCH(QString, kind);
    QList<InputSample> input{{{8, 32}, 1, 0, 0, 0, 0}, {{32, 8}, 1, 0, 0, 0, 8}, {{56, 32}, 1, 0, 0, 0, 16}};
    if (kind == QLatin1String("tap")) input = input.first(1);
    if (kind == QLatin1String("idle-two") || kind == QLatin1String("tap-after-two")) input = input.first(2);
    if (kind == QLatin1String("stationary")) for (auto &sample : input) sample.position = {8, 32};
    const bool idle = kind.startsWith(QLatin1String("idle"));
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.diameter = 4;
    brush.rgba = {17, 129, 241, 255};
    brush.basicSmoothing = true;
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 7);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    for (qsizetype index = 0; index < input.size(); ++index) {
        QCOMPARE(document.appendSamples(id, {input[index]}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        if (index == 0) QVERIFY(incremental->tiles(layer).isEmpty());
        if (index == 1 && idle) {
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            incremental = render(renderer, document.preview(), incremental);
            QVERIFY(incremental);
            QVERIFY(!incremental->tiles(layer).isEmpty());
        }
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(incremental->tiles(layer).size(), rebuilt->tiles(layer).size());
        for (const auto &tile : rebuilt->tiles(layer))
            QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(rebuilt, layer, tile.coordinate));
    }
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto final = render(renderer, document.snapshot(), incremental);
    QVERIFY(final);
    Shape shape;
    QList<InputSample> expected;
    if (kind == QLatin1String("tap") || kind == QLatin1String("tap-after-two") || kind == QLatin1String("stationary")) {
        expected = {input.last()};
    } else {
        shape.kind = ShapeKind::Path;
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
        expected = idle ? QList<InputSample>{{{8, 32}, 1}, {{15.68, 24.32}, 1}, {{24.32, 15.68}, 1}, {{32, 8}, 1}} :
                          QList<InputSample>{{{8, 32}, 1}, {{17.6, 22.4}, 1}, {{22.4, 8}, 1}, {{32, 8}, 1}};
        if (input.size() == 3) {
            shape.pathCommands.append(PathCommand::CubicTo);
            expected.append(idle ? QList<InputSample>{{{39.68, 15.68}, 1}, {{48.32, 24.32}, 1}, {{56, 32}, 1}} :
                                   QList<InputSample>{{{41.6, 8}, 1}, {{46.4, 22.4}, 1}, {{56, 32}, 1}});
        }
    }
    OperationDocument oracle(QSize(64, 64));
    brush.basicSmoothing = false;
    QCOMPARE(oracle.paintFigure(oracle.activeLayer(), brush, expected, shape, FigureStyle{}, 7), Result::Success);
    const auto renderedExpected = render(renderer, oracle.snapshot());
    QVERIFY(renderedExpected);
    const auto pixels = readTile(final, layer, {0, 0});
    QCOMPARE(pixels, readTile(renderedExpected, oracle.activeLayer(), {0, 0}));
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), input);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), final)->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);

    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("basic-smoothing-") + kind + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::weightedSmoothingPreservesPixels_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<QPointF>("fourth");
    QTest::addColumn<QPointF>("fifth");
    QTest::addColumn<qreal>("fourthPressure");
    QTest::addColumn<qreal>("fifthPressure");
    // Fixed results of the existing weighted-input rule in kis_tool_freehand_helper.cpp.
    QTest::newRow("weighted") << QStringLiteral("weighted") << QPointF(21.671799047212705, 17.296375248240203)
        << QPointF(26.190447445238277, 19.37998501314707) << qreal(0.31641004763936476) << qreal(0.16306585611055563);
    QTest::newRow("no-pressure") << QStringLiteral("no-pressure") << QPointF(21.671799047212705, 17.296375248240203)
        << QPointF(26.04262003540347, 19.310955108328365) << qreal(0.2) << qreal(0.1);
    QTest::newRow("no-tail") << QStringLiteral("no-tail") << QPointF(21.087831442146605, 17.427899484954125)
        << QPointF(25.512175466564386, 19.274354334843917) << qreal(0.3456084278926697) << qreal(0.19015482878006573);
    QTest::newRow("slow") << QStringLiteral("slow") << QPointF(20.070453973415706, 17.61942028617956)
        << QPointF(22.863223234608107, 19.016418949226647) << qreal(0.39647730132921466) << qreal(0.30008962757304986);
    QTest::newRow("fast") << QStringLiteral("fast") << QPointF(22.522529933316513, 17.04453976461341)
        << QPointF(26.243823024060887, 19.26420364596108) << qreal(0.2738735033341744) << qreal(0.15800647037967294);
    QTest::newRow("zero-distance") << QStringLiteral("zero-distance") << QPointF(24, 16) << QPointF(28, 20) << qreal(0.2) << qreal(0.1);
}

void KisGpuTileRendererTest::weightedSmoothingPreservesPixels()
{
    QFETCH(QString, kind);
    QFETCH(QPointF, fourth);
    QFETCH(QPointF, fifth);
    QFETCH(qreal, fourthPressure);
    QFETCH(qreal, fifthPressure);
    QList<InputSample> input{{{8, 16}, 1, 0, 0, 0, 0, 0}, {{12, 20}, 0.8, 0, 0, 0, 8, 0.25},
                            {{16, 16}, 0.6, 0, 0, 0, 16, 0.5}, {{20, 20}, 0.4, 0, 0, 0, 24, 0.75},
                            {{24, 16}, 0.2, 0, 0, 0, 32, 1}, {{28, 20}, 0.1, 0, 0, 0, 40, 0}};
    if (kind == QLatin1String("slow") || kind == QLatin1String("fast"))
        for (auto &sample : input) sample.speed = kind == QLatin1String("fast") ? 1 : 0;
    Brush brush;
    brush.diameter = 4;
    brush.rgba = {17, 129, 241, 255};
    brush.basicSmoothing = true;
    brush.weightedSmoothing = WeightedSmoothing{24, 48, kind == QLatin1String("no-tail") ? 0.0 : 0.01,
                                               kind != QLatin1String("no-pressure")};
    if (kind == QLatin1String("zero-distance")) brush.weightedSmoothing->distanceMin = brush.weightedSmoothing->distanceMax = 0;
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 7);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    for (const auto &sample : input) {
        QCOMPARE(document.appendSamples(id, {sample}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(readTile(incremental, layer, {0, 0}), readTile(rebuilt, layer, {0, 0}));
    }
    const auto pinned = incremental;
    const auto pinnedPixels = readTile(pinned, layer, {0, 0});
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto painted = render(renderer, document.snapshot(), incremental);
    QVERIFY(painted);
    auto expected = input;
    expected[4].position = fourth;
    expected[4].pressure = fourthPressure;
    expected[5].position = fifth;
    expected[5].pressure = fifthPressure;
    OperationDocument oracle(QSize(64, 64));
    brush.weightedSmoothing.reset();
    const auto reference = oracle.beginStroke(oracle.activeLayer(), brush, 7);
    QCOMPARE(oracle.appendSamples(reference, expected), Result::Success);
    QCOMPARE(oracle.flushSmoothing(reference), Result::Success);
    QCOMPARE(oracle.commitStroke(reference), Result::Success);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels, readTile(render(renderer, oracle.snapshot()), oracle.activeLayer(), {0, 0}));
    QCOMPARE(readTile(pinned, layer, {0, 0}), pinnedPixels);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), painted)->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);

    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("weighted-smoothing-") + kind + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuTileRendererTest test;
    return QTest::qExec(&test, argc, argv);
}

void KisGpuTileRendererTest::retainedFiltersFollowSourceEditsWithoutBakingPixels_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("mask");
    for (bool words : {false, true}) for (bool mask : {false, true}) {
        const QByteArray name = QByteArray(words ? "word-" : "byte-") + (mask ? "mask" : "adjustment");
        QTest::newRow(name) << words << mask;
    }
}

void KisGpuTileRendererTest::retainedFiltersFollowSourceEditsWithoutBakingPixels()
{
    QFETCH(bool, words);
    QFETCH(bool, mask);
    RasterImage raster;
    raster.size = {32, 24};
    raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    raster.layers = {{QStringLiteral("Paint"), true, {}, {}}};
    OperationDocument document(raster);
    const auto paint = document.activeLayer();
    const quint16 scale = words ? 257 : 1;
    const auto color = [&](quint16 r, quint16 g, quint16 b) -> RgbaPixel {
        return {quint16(r * scale), quint16(g * scale), quint16(b * scale), maximumComponent(raster.format)};
    };
    const auto first = color(11, 83, 207);
    const auto second = color(199, 47, 13);
    QCOMPARE(document.fillLayerColor(paint, first), Result::Success);
    FilterDefinition filter;
    filter.id = QStringLiteral("maximize");
    filter.settings.kind = FilterKind::MaximizeChannel;
    filter.xml = QByteArrayLiteral("<!DOCTYPE params><params/>");
    RasterLayer node;
    node.name = QStringLiteral("Retained maximum");
    node.kind = mask ? LayerKind::FilterMask : LayerKind::Adjustment;
    node.parentUuid = mask ? document.snapshot()->layers.first().uuid : QUuid{};
    node.blendMode = mask ? LayerBlendMode::Normal : LayerBlendMode::Copy;
    node.filter = filter;
    const auto nodeId = document.addFilterNode(node, 1);
    QVERIFY(nodeId);
    TileRenderer renderer(m_device);
    const QRect area(QPoint(), raster.size);
    const int stride = pixelByteCount(raster.format);
    const auto pixel = [&](const QByteArray &bytes) { return bytes.mid((7 * TileRenderer::TileSize + 9) * stride, stride); };
    auto raw = render(renderer, document.snapshot());
    QVERIFY(raw);
    QCOMPARE(pixel(readTile(raw, paint, {0, 0})), rgbaBytes(first, raster.format));
    auto projected = project(renderer, raw, area);
    QVERIFY(projected);
    QVERIFY(!projected->tiles().isEmpty());
    QCOMPARE(pixel(readBuffer(projected->tiles().first().pixels)), rgbaBytes(color(0, 0, 207), raster.format));
    // Editing below the retained node re-evaluates its projection, while the
    // actual paint device continues to hold the unfiltered color.
    QCOMPARE(document.fillLayerColor(paint, second), Result::Success);
    raw = render(renderer, document.snapshot(), raw);
    QCOMPARE(pixel(readTile(raw, paint, {0, 0})), rgbaBytes(second, raster.format));
    projected = project(renderer, raw, area, projected);
    QCOMPARE(pixel(readBuffer(projected->tiles().first().pixels)), rgbaBytes(color(199, 0, 0), raster.format));
    QCOMPARE(document.undo(), Result::Success);
    raw = render(renderer, document.snapshot(), raw);
    projected = project(renderer, raw, area, projected);
    QCOMPARE(pixel(readBuffer(projected->tiles().first().pixels)), rgbaBytes(color(0, 0, 207), raster.format));
    QCOMPARE(document.redo(), Result::Success);
    const auto coverage = SelectionMask::fromAlphaCoverage(area,
        QByteArray(area.width() * area.height(), char(128)), area);
    QVERIFY(coverage);
    QCOMPARE(document.setFilterSelection(nodeId, coverage), Result::Success);
    raw = render(renderer, document.snapshot(), raw);
    projected = project(renderer, raw, area, projected);
    const auto selectedProjection = readBuffer(projected->tiles().first().pixels);
    OperationDocument ordinary(raster);
    const auto ordinaryPaint = ordinary.activeLayer();
    QCOMPARE(ordinary.fillLayerColor(ordinaryPaint, second), Result::Success);
    QCOMPARE(ordinary.setSelectionMask(coverage), Result::Success);
    QCOMPARE(ordinary.colorFilter(ordinaryPaint, FilterKind::MaximizeChannel), Result::Success);
    TileRenderer ordinaryRenderer(m_device);
    const auto ordinaryPixels = render(ordinaryRenderer, ordinary.snapshot());
    const auto ordinaryProjection = project(ordinaryRenderer, ordinaryPixels, area);
    QCOMPARE(selectedProjection, readBuffer(ordinaryProjection->tiles().first().pixels));
    QCOMPARE(pixel(readTile(raw, paint, {0, 0})), rgbaBytes(second, raster.format));
}
