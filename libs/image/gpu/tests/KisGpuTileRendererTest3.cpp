/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRendererTest.h"

void KisGpuTileRendererTest::selectionLimitsPaintFillAndEraseAcrossHistory()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    OperationDocument document({256, 256});
    Brush brush;
    brush.rgba = {210, 40, 90, 255};
    brush.diameter = 32;
    QCOMPARE(document.setSelectionRect(QRect(64, 64, 64, 64)), Result::Success);
    const auto first = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(first, {{{32, 96}, 1}, {{224, 96}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(pending->selectionRect, QRect(64, 64, 64, 64));
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 1, 1)), Result::EditInProgress);
    QCOMPARE(document.commitStroke(first), Result::Success);

    QCOMPARE(document.setSelectionRect(QRect(160, 64, 64, 64)), Result::Success);
    const auto second = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(second, {{{32, 96}, 1}, {{224, 96}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(second), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    const auto bytes = readTile(painted, document.activeLayer(), {0, 0});
    QCOMPARE(pixel(bytes, 96, 96), QByteArray::fromHex("d2285aff"));
    QCOMPARE(pixel(bytes, 192, 96), QByteArray::fromHex("d2285aff"));
    QCOMPARE(pixel(bytes, 32, 96), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes, 224, 96), QByteArray(4, '\0'));

    // Both operations retain their own selection through replay.

    QCOMPARE(document.selectionRect(), QRect(160, 64, 64, 64));
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);

    QCOMPARE(document.undo(), Result::Success);
    const auto undone = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});
    QCOMPARE(pixel(undone, 96, 96), QByteArray::fromHex("d2285aff"));
    QCOMPARE(pixel(undone, 192, 96), QByteArray(4, '\0'));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);

    // Fill and erase use the same frozen selection.
    OperationDocument fillDocument({256, 256});
    QCOMPARE(fillDocument.setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    QCOMPARE(fillDocument.fillLayer(fillDocument.activeLayer(), {17, 34, 51}), Result::Success);
    const auto filled = readTile(render(renderer, fillDocument.snapshot()), fillDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(filled, 64, 64), QByteArray::fromHex("112233ff"));
    QCOMPARE(pixel(filled, 16, 64), QByteArray(4, '\0'));
    QCOMPARE(pixel(filled, 128, 64), QByteArray(4, '\0'));

    OperationDocument eraseDocument({256, 256});
    QCOMPARE(eraseDocument.fillLayer(eraseDocument.activeLayer(), {20, 80, 160}), Result::Success);
    QCOMPARE(eraseDocument.setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    Brush eraser = brush;
    eraser.rgba[3] = 255;
    eraser.erase = true;
    const auto erase = eraseDocument.beginStroke(eraseDocument.activeLayer(), eraser, 0);
    QCOMPARE(eraseDocument.appendSamples(erase, {{{64, 64}, 1}}), Result::Success);
    QCOMPARE(eraseDocument.commitStroke(erase), Result::Success);
    const auto erased = readTile(render(renderer, eraseDocument.snapshot()), eraseDocument.activeLayer(), {0, 0});
    QVERIFY(pixel(erased, 16, 16)[3] == char(255));
    QCOMPARE(pixel(erased, 64, 64), QByteArray::fromHex("1450a000"));
    QCOMPARE(eraseDocument.clearSelection(), Result::Success);
    QVERIFY(eraseDocument.selectionRect().isEmpty());
}

void KisGpuTileRendererTest::patternFillTilesAcrossSelectionHistory()
{
    const auto patternPixel = [](int x, int y) {
        const int px = ((x % 4) + 4) % 4;
        const int py = ((y % 4) + 4) % 4;
        QByteArray value(4, '\0');
        value[0] = char(px * 60 + 3);
        value[1] = char(py * 60 + 5);
        value[2] = char((px + py) * 20 + 7);
        value[3] = char(255);
        return value;
    };
    PatternFill pattern;
    pattern.size = QSize(4, 4);
    pattern.rgba = QByteArray(4 * 4 * 4, '\0');
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            const auto value = patternPixel(x, y);
            std::memcpy(pattern.rgba.data() + (y * 4 + x) * 4, value.constData(), 4);
        }
    }
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };

    // The selection crosses all four 256-pixel tiles; the pattern repeats from
    // the document origin, so tile boundaries do not shift its phase.
    OperationDocument document({512, 512});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 120, 120)), Result::Success);
    QCOMPARE(document.fillLayerPattern(layer, pattern), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::FillLayer);
    TileRenderer renderer(m_device);
    auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto bytes00 = readTile(rendered, layer, {0, 0});
    const auto bytes10 = readTile(rendered, layer, {1, 0});
    const auto bytes01 = readTile(rendered, layer, {0, 1});
    const auto bytes11 = readTile(rendered, layer, {1, 1});
    QCOMPARE(bytes00.size(), TileRenderer::TileBytes);
    QCOMPARE(bytes10.size(), TileRenderer::TileBytes);
    QCOMPARE(bytes01.size(), TileRenderer::TileBytes);
    QCOMPARE(bytes11.size(), TileRenderer::TileBytes);
    QCOMPARE(pixel(bytes00, 200, 200), patternPixel(200, 200));
    QCOMPARE(pixel(bytes00, 201, 200), patternPixel(201, 200));
    QCOMPARE(pixel(bytes00, 255, 255), patternPixel(255, 255));
    QCOMPARE(pixel(bytes10, 0, 200), patternPixel(256, 200));
    QCOMPARE(pixel(bytes01, 200, 0), patternPixel(200, 256));
    QCOMPARE(pixel(bytes11, 0, 0), patternPixel(256, 256));
    QCOMPARE(pixel(bytes11, 63, 63), patternPixel(319, 319));
    QCOMPARE(pixel(bytes00, 199, 200), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes10, 64, 200), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes01, 200, 64), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes11, 64, 64), QByteArray(4, '\0'));

    // The fill is one history entry; undo removes the filled tiles and redo
    // restores the same pixels.
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot())->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    QCOMPARE(readTile(rendered, layer, {0, 0}), bytes00);
    QCOMPARE(readTile(rendered, layer, {1, 0}), bytes10);
    QCOMPARE(readTile(rendered, layer, {0, 1}), bytes01);
    QCOMPARE(readTile(rendered, layer, {1, 1}), bytes11);
    QCOMPARE(document.undoCount(), 2);
}

void KisGpuTileRendererTest::patternFillClipsAlphaCompositesErasesAndRejectsInvalidInput()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const QRect clip(QPoint(), QSize(64, 64));
    const QByteArray coverage = QByteArray::fromHex(
        "ff804001" "20c80af0" "50a01464" "fa05823c");
    const auto mask = SelectionMask::fromAlphaCoverage(QRect(16, 16, 4, 4), coverage, clip);
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(16, 16, 4, 4));
    QCOMPARE(mask->coverage(16, 16), quint8(255));
    QCOMPARE(mask->coverage(17, 16), quint8(128));

    // Pattern alpha times selection alpha times fill opacity, with straight
    // RGBA over a transparent destination.
    PatternFill pattern;
    pattern.size = QSize(2, 2);
    pattern.rgba = QByteArray(2 * 2 * 4, '\0');
    for (qsizetype i = 0; i < pattern.rgba.size(); i += 4) {
        pattern.rgba[i + 0] = char(10);
        pattern.rgba[i + 1] = char(20);
        pattern.rgba[i + 2] = char(30);
        pattern.rgba[i + 3] = char(200);
    }
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionMask(mask), Result::Success);
    QCOMPARE(document.fillLayerPattern(layer, pattern, 128.0 / 255), Result::Success);
    TileRenderer renderer(m_device);
    const auto clipped = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    QCOMPARE(pixel(clipped, 16, 16), QByteArray::fromHex("0a141e64"));
    QCOMPARE(pixel(clipped, 17, 16), QByteArray::fromHex("0a141e32"));
    QCOMPARE(pixel(clipped, 18, 16), QByteArray::fromHex("0a141e19"));
    // Positive native float alpha rounds to zero while retaining straight RGB.
    QCOMPARE(pixel(clipped, 19, 16), QByteArray::fromHex("0a141e00"));
    QCOMPARE(pixel(clipped, 15, 16), QByteArray(4, '\0'));
    QCOMPARE(pixel(clipped, 20, 16), QByteArray(4, '\0'));

    // Erase mode removes pattern alpha times selection alpha from the retained
    // destination while preserving its RGB values.
    OperationDocument eraseDocument({64, 64});
    const auto eraseLayer = eraseDocument.activeLayer();
    QCOMPARE(eraseDocument.fillLayer(eraseLayer, {200, 30, 70}), Result::Success);
    QCOMPARE(eraseDocument.setSelectionMask(mask), Result::Success);
    const auto undoBefore = eraseDocument.undoCount();
    QCOMPARE(eraseDocument.fillLayerPattern(eraseLayer, pattern, 255.0 / 255, true), Result::Success);
    QCOMPARE(eraseDocument.undoCount(), undoBefore + 1);
    const auto erased = readTile(render(renderer, eraseDocument.snapshot()), eraseLayer, {0, 0});
    QCOMPARE(pixel(erased, 16, 16), QByteArray::fromHex("c81e4637"));
    QCOMPARE(pixel(erased, 17, 16), QByteArray::fromHex("c81e469b"));
    QCOMPARE(pixel(erased, 18, 16), QByteArray::fromHex("c81e46cd"));
    QCOMPARE(pixel(erased, 19, 16), QByteArray::fromHex("c81e46fe"));
    QCOMPARE(pixel(erased, 15, 16), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(eraseDocument.undo(), Result::Success);
    const auto restored = readTile(render(renderer, eraseDocument.snapshot()), eraseLayer, {0, 0});
    QCOMPARE(pixel(restored, 16, 16), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(eraseDocument.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, eraseDocument.snapshot()), eraseLayer, {0, 0}), erased);

    // Malformed frozen pixels are rejected before touching history or pixels.
    const auto before = document.snapshot();
    PatternFill malformed{QSize(2, 2), QByteArray(3, '\0')};
    QCOMPARE(document.fillLayerPattern(layer, malformed), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
}

void KisGpuTileRendererTest::fillBlendModesMatchLayerComposition_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("multiply") << int(LayerBlendMode::Multiply) << QByteArray::fromHex("0f0209ff");
    QTest::newRow("screen") << int(LayerBlendMode::Screen) << QByteArray::fromHex("df3156ff");
    QTest::newRow("darken") << int(LayerBlendMode::Darken) << QByteArray::fromHex("11112cff");
    QTest::newRow("lighten") << int(LayerBlendMode::Lighten) << QByteArray::fromHex("dd2233ff");
    QTest::newRow("addition") << int(LayerBlendMode::Addition) << QByteArray::fromHex("ee335fff");
    QTest::newRow("linear-dodge") << int(LayerBlendMode::LinearDodge) << QByteArray::fromHex("ee335fff");
    QTest::newRow("behind") << int(LayerBlendMode::Behind) << QByteArray::fromHex("112233ff");
    QTest::newRow("copy-red") << int(LayerBlendMode::CopyRed) << QByteArray::fromHex("dd2233ff");
    QTest::newRow("copy-green") << int(LayerBlendMode::CopyGreen) << QByteArray::fromHex("111133ff");
    QTest::newRow("copy-blue") << int(LayerBlendMode::CopyBlue) << QByteArray::fromHex("11222cff");
}

void KisGpuTileRendererTest::fillBlendModesMatchLayerComposition()
{
    QFETCH(int, mode);
    QFETCH(QByteArray, expected);
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    QCOMPARE(document.setSelectionRect(QRect(16, 16, 32, 32)), Result::Success);
    const auto historyBefore = document.undoCount();
    QCOMPARE(document.fillLayer(layer, {221, 17, 44}, 255.0 / 255, false, false, LayerBlendMode(mode)),
             Result::Success);
    QCOMPARE(document.undoCount(), historyBefore + 1);
    QCOMPARE(document.historyAction(historyBefore + 1), HistoryAction::FillLayer);
    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto filled = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(filled, 24, 24), expected);
    QCOMPARE(pixel(filled, 16, 16), expected);
    QCOMPARE(pixel(filled, 47, 47), expected);
    QCOMPARE(pixel(filled, 15, 16), base);
    QCOMPARE(pixel(filled, 48, 48), base);
    QCOMPARE(pixel(filled, 8, 8), base);
    QCOMPARE(quint8(pixel(filled, 24, 24).at(3)), 255);

    QCOMPARE(document.undo(), Result::Success);
    const auto undoneVersion = render(renderer, document.snapshot());
    QVERIFY(undoneVersion);
    const auto undone = readTile(undoneVersion, layer, {0, 0});
    QCOMPARE(pixel(undone, 24, 24), base);
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), filled);
}

void KisGpuTileRendererTest::strokeBlendModesMatchFillBlendComposition_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("multiply") << int(LayerBlendMode::Multiply) << QByteArray::fromHex("0f0209ff");
    QTest::newRow("screen") << int(LayerBlendMode::Screen) << QByteArray::fromHex("df3156ff");
    QTest::newRow("darken") << int(LayerBlendMode::Darken) << QByteArray::fromHex("11112cff");
    QTest::newRow("lighten") << int(LayerBlendMode::Lighten) << QByteArray::fromHex("dd2233ff");
    QTest::newRow("addition") << int(LayerBlendMode::Addition) << QByteArray::fromHex("ee335fff");
    QTest::newRow("linear-dodge") << int(LayerBlendMode::LinearDodge) << QByteArray::fromHex("ee335fff");
    QTest::newRow("behind") << int(LayerBlendMode::Behind) << QByteArray::fromHex("112233ff");
    QTest::newRow("copy-red") << int(LayerBlendMode::CopyRed) << QByteArray::fromHex("dd2233ff");
    QTest::newRow("copy-green") << int(LayerBlendMode::CopyGreen) << QByteArray::fromHex("111133ff");
    QTest::newRow("copy-blue") << int(LayerBlendMode::CopyBlue) << QByteArray::fromHex("11222cff");
}

void KisGpuTileRendererTest::strokeBlendModesMatchFillBlendComposition()
{
    QFETCH(int, mode);
    QFETCH(QByteArray, expected);
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    Brush brush;
    brush.rgba = {221, 17, 44, 255};
    brush.diameter = 32;
    brush.blendMode = LayerBlendMode(mode);
    const auto stroke = document.beginStroke(layer, brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);

    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto painted = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(painted, 32, 32), expected);
    QCOMPARE(pixel(painted, 8, 8), base);
    QCOMPARE(quint8(pixel(painted, 32, 32).at(3)), 255);

    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(pixel(readTile(render(renderer, document.snapshot()), layer, {0, 0}), 32, 32), base);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), painted);
}

void KisGpuTileRendererTest::strokeBlendModePreservesOpacitySelectionAlphaLockAndChannelMask()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    const auto strokeAt = [](OperationDocument &document, const Brush &brush) {
        const auto id = document.beginStroke(document.activeLayer(), brush, 7);
        if (id.isNull()) return Result::InvalidInput;
        const auto result = document.appendSamples(id, {{{32, 32}, 1}});
        return result == Result::Success ? document.commitStroke(id) : result;
    };
    Brush brush;
    brush.rgba = {221, 17, 44, 255};
    brush.diameter = 32;
    brush.blendMode = LayerBlendMode::Multiply;
    TileRenderer renderer(m_device);

    // Brush opacity scales the source alpha before the separable blend.
    OperationDocument opacityDocument(QSize(64, 64));
    const auto opacityLayer = opacityDocument.activeLayer();
    QCOMPARE(opacityDocument.fillLayer(opacityLayer, {17, 34, 51}), Result::Success);
    brush.opacity = 128.0 / 255;
    QCOMPARE(strokeAt(opacityDocument, brush), Result::Success);
    const auto opacityPixels = readTile(render(renderer, opacityDocument.snapshot()), opacityLayer, {0, 0});
    QCOMPARE(pixel(opacityPixels, 32, 32), QByteArray::fromHex("10121eff"));

    // Selection coverage limits the blended source without changing the mode.
    OperationDocument selectionDocument(QSize(64, 64));
    const auto selectionLayer = selectionDocument.activeLayer();
    QCOMPARE(selectionDocument.fillLayer(selectionLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(selectionDocument.setSelectionRect(QRect(16, 16, 32, 32)), Result::Success);
    brush.opacity = 255.0 / 255;
    QCOMPARE(strokeAt(selectionDocument, brush), Result::Success);
    const auto selectionPixels = readTile(render(renderer, selectionDocument.snapshot()), selectionLayer, {0, 0});
    QCOMPARE(pixel(selectionPixels, 32, 32), QByteArray::fromHex("0f0209ff"));
    QCOMPARE(pixel(selectionPixels, 8, 8), base);

    // Alpha lock keeps the destination alpha and hidden RGB contract.
    OperationDocument alphaDocument(QSize(64, 64));
    const auto alphaLayer = alphaDocument.activeLayer();
    QCOMPARE(alphaDocument.fillLayer(alphaLayer, {17, 34, 51}, 128.0 / 255), Result::Success);
    brush.alphaLocked = true;
    QCOMPARE(strokeAt(alphaDocument, brush), Result::Success);
    const auto alphaPixels = readTile(render(renderer, alphaDocument.snapshot()), alphaLayer, {0, 0});
    QCOMPARE(pixel(alphaPixels, 32, 32), QByteArray::fromHex("76091a80"));
    brush.alphaLocked = false;

    // Protected channels keep their destination bytes after blending.
    OperationDocument channelDocument(QSize(64, 64));
    const auto channelLayer = channelDocument.activeLayer();
    QCOMPARE(channelDocument.fillLayer(channelLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(channelDocument.setLayerChannelMask(channelLayer, 14), Result::Success);
    QCOMPARE(strokeAt(channelDocument, brush), Result::Success);
    const auto channelPixels = readTile(render(renderer, channelDocument.snapshot()), channelLayer, {0, 0});
    QCOMPARE(pixel(channelPixels, 32, 32), QByteArray::fromHex("110209ff"));
}

void KisGpuTileRendererTest::gaussianStrokeBlendModeMatchesSeparableComposition()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    Brush brush;
    brush.rgba = {221, 17, 44, 255};
    brush.diameter = 64;
    brush.blendMode = LayerBlendMode::Multiply;
    AutoBrush wash;
    brush.opacity = 1;
    wash.flow = 1;
    brush.sizeResponse = {PressureResponse::Kind::Constant, 1, {}};
    wash.opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    brush.autoBrush = wash;
    const auto stroke = document.beginStroke(layer, brush, 11);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);

    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto painted = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(painted, 32, 32), QByteArray::fromHex("0f0209ff"));
    QCOMPARE(pixel(painted, 8, 8), QByteArray::fromHex("112233ff"));
}

void KisGpuTileRendererTest::patternFillBlendModeMatchesLayerComposition()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    PatternFill pattern;
    pattern.size = QSize(2, 2);
    pattern.rgba = QByteArray(2 * 2 * 4, '\0');
    for (qsizetype index = 0; index < pattern.rgba.size(); index += 4) {
        pattern.rgba[index + 0] = char(34);
        pattern.rgba[index + 1] = char(68);
        pattern.rgba[index + 2] = char(102);
        pattern.rgba[index + 3] = char(200);
    }

    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    QCOMPARE(document.setSelectionRect(QRect(16, 16, 4, 4)), Result::Success);
    QCOMPARE(document.fillLayerPattern(layer, pattern, 255.0 / 255, false, false, LayerBlendMode::Multiply),
             Result::Success);
    TileRenderer renderer(m_device);
    const auto filled = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    // Pattern alpha 200 times selection alpha 255 and Multiply over the opaque base.
    const auto expected = QByteArray::fromHex("050e1bff");
    QCOMPARE(pixel(filled, 16, 16), expected);
    QCOMPARE(pixel(filled, 19, 19), expected);
    QCOMPARE(pixel(filled, 20, 16), base);
    QCOMPARE(pixel(filled, 8, 8), base);
    QCOMPARE(quint8(pixel(filled, 16, 16).at(3)), 255);

    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(pixel(readTile(render(renderer, document.snapshot()), layer, {0, 0}), 16, 16), base);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filled);

    QCOMPARE(pixel(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), 16, 16),
             expected);
}

void KisGpuTileRendererTest::fillBlendModePreservesOpacityAlphaLockAndChannelMask()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    TileRenderer renderer(m_device);

    // Fill opacity multiplies the source alpha before the blend.
    OperationDocument opacityDocument(QSize(64, 64));
    const auto opacityLayer = opacityDocument.activeLayer();
    QCOMPARE(opacityDocument.fillLayer(opacityLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(opacityDocument.fillLayer(opacityLayer, {221, 17, 44}, 128.0 / 255, false, false,
                                       LayerBlendMode::Multiply), Result::Success);
    const auto opacityPixels = readTile(render(renderer, opacityDocument.snapshot()), opacityLayer, {0, 0});
    QCOMPARE(pixel(opacityPixels, 24, 24), QByteArray::fromHex("10121eff"));

    // Alpha lock keeps the destination alpha, including hidden RGB.
    OperationDocument alphaDocument(QSize(64, 64));
    const auto alphaLayer = alphaDocument.activeLayer();
    QCOMPARE(alphaDocument.fillLayer(alphaLayer, {17, 34, 51}, 128.0 / 255), Result::Success);
    QCOMPARE(alphaDocument.fillLayer(alphaLayer, {221, 17, 44}, 255.0 / 255, false, true,
                                     LayerBlendMode::Multiply), Result::Success);
    const auto alphaPixels = readTile(render(renderer, alphaDocument.snapshot()), alphaLayer, {0, 0});
    QCOMPARE(pixel(alphaPixels, 24, 24), QByteArray::fromHex("76091a80"));
    QCOMPARE(alphaDocument.undo(), Result::Success);
    QCOMPARE(pixel(readTile(render(renderer, alphaDocument.snapshot()), alphaLayer, {0, 0}), 24, 24),
             QByteArray::fromHex("11223380"));

    // Channel protection keeps the protected component while the writable
    // components use the same blend as composition.
    OperationDocument channelDocument(QSize(64, 64));
    const auto channelLayer = channelDocument.activeLayer();
    QCOMPARE(channelDocument.fillLayer(channelLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(channelDocument.setLayerChannelMask(channelLayer, 14), Result::Success);
    QCOMPARE(channelDocument.fillLayer(channelLayer, {221, 17, 44}, 255.0 / 255, false, false,
                                       LayerBlendMode::Multiply), Result::Success);
    const auto channelPixels = readTile(render(renderer, channelDocument.snapshot()), channelLayer, {0, 0});
    QCOMPARE(pixel(channelPixels, 24, 24), QByteArray::fromHex("110209ff"));
}

void KisGpuTileRendererTest::invertColorsComplementsRgbPreservesAlphaAndClipsSelection()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("00000000"));
    set(30, 30, QByteArray::fromHex("ff8000ff"));
    set(200, 10, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Invert");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 64, 64)), Result::Success);
    QCOMPARE(document.invertColors(layer), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->kind, FilterKind::InvertColors);
    QCOMPARE(operation->selectionRect, QRect(0, 0, 64, 64));
    QVERIFY(!operation->selectionInverted);

    // Selection changes after acceptance do not rewrite the frozen coverage.
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 8, 8)), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect,
             QRect(0, 0, 64, 64));

    TileRenderer renderer(m_device);
    const auto inverted = render(renderer, document.snapshot());
    QVERIFY(inverted);
    const auto invertedBytes = readTile(inverted, layer, {0, 0});
    QCOMPARE(pixel(invertedBytes, 10, 10), QByteArray::fromHex("f5ebe128"));
    // Transparent pixels keep alpha zero; their hidden RGB follows the same
    // complement as the channel-mask arithmetic used for locked painting.
    QCOMPARE(pixel(invertedBytes, 20, 20), QByteArray::fromHex("ffffff00"));
    QCOMPARE(pixel(invertedBytes, 30, 30), QByteArray::fromHex("007fffff"));
    QCOMPARE(pixel(invertedBytes, 200, 10), QByteArray::fromHex("11223344"));
    QCOMPARE(pixel(invertedBytes, 64, 64), QByteArray(4, '\0'));

    // One history entry: undo restores the exact source pixels, redo the filter.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto original = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    QCOMPARE(original, bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), invertedBytes);

    // Retained input preserves the frozen operation over the current document selection.

    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    const auto retained = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->selectionRect, QRect(0, 0, 64, 64));
}

void KisGpuTileRendererTest::invertColorsInterpolatesFeatheredAndInvertedCoverage()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto mixed = [](int base, int selected) {
        const int complement = 255 - base;
        const int keep = 255 - selected;
        return quint8((base * keep + complement * selected + 127) / 255);
    };
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    coverage[20 * 256 + 20] = char(128);
    coverage[30 * 256 + 30] = char(255);
    const auto soft = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(soft);
    QVERIFY(soft->isAlpha());

    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(20, 20, QByteArray::fromHex("0a141e28"));
    set(30, 30, QByteArray::fromHex("ff8000ff"));
    set(10, 10, QByteArray::fromHex("10203040"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Feather");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionMask(soft), Result::Success);
    QCOMPARE(document.invertColors(layer), Result::Success);
    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    TileRenderer renderer(m_device);
    const auto filtered = render(renderer, document.snapshot());
    QVERIFY(filtered);
    const auto filteredBytes = readTile(filtered, layer, {0, 0});
    QByteArray fractional(4, '\0');
    fractional[0] = char(mixed(10, 128));
    fractional[1] = char(mixed(20, 128));
    fractional[2] = char(mixed(30, 128));
    fractional[3] = char(40);
    QCOMPARE(pixel(filteredBytes, 20, 20), fractional);
    QCOMPARE(pixel(filteredBytes, 30, 30), QByteArray::fromHex("007fffff"));
    // Unselected pixels remain byte-identical.
    QCOMPARE(pixel(filteredBytes, 10, 10), QByteArray::fromHex("10203040"));

    // An inverted rectangular selection complements only the outside.
    OperationDocument invertedDocument(input);
    const auto invertedLayer = invertedDocument.activeLayer();
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.invertColors(invertedLayer), Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()), invertedLayer, {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("10203040"));
    QCOMPARE(pixel(inverted, 200, 10), QByteArray::fromHex("ffffff00"));

    // Elliptical coverage intersects its bounding box before the pixel test.
    OperationDocument ellipseDocument(input);
    const auto ellipseLayer = ellipseDocument.activeLayer();
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.invertColors(ellipseLayer), Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()), ellipseLayer, {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("10203040"));
    QCOMPARE(pixel(ellipse, 128, 128), QByteArray::fromHex("ffffff00"));
}

void KisGpuTileRendererTest::grayscaleUsesRec709CoefficientsWithIntegerRounding()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    // The documented rule: Rec.709 weights applied to stored sRGB-encoded
    // bytes, integer denominator 10000, round half up.
    const auto luma = [](int r, int g, int b) {
        return quint8((2126 * r + 7152 * g + 722 * b + 5000) / 10000);
    };
    const auto gray = [&](int r, int g, int b, int a) {
        const auto value = luma(r, g, b);
        QByteArray result(4, '\0');
        result[0] = result[1] = result[2] = char(value);
        result[3] = char(a);
        return result;
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));  // 10,20,30,40
    set(20, 20, QByteArray::fromHex("000100ff"));  // 0.7152 rounds up to 1
    set(30, 30, QByteArray::fromHex("000001ff"));  // 0.0722 rounds down to 0
    set(40, 40, QByteArray::fromHex("6496c880"));  // 100,150,200,128 rounds up
    set(50, 50, QByteArray::fromHex("ff0000ff"));
    set(60, 60, QByteArray::fromHex("00ff00ff"));
    set(70, 70, QByteArray::fromHex("0000ffff"));
    set(80, 80, QByteArray::fromHex("808080ff"));  // neutral gray is preserved
    set(90, 90, QByteArray::fromHex("ffffff00"));  // hidden RGB, alpha stays 0
    set(200, 10, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Grayscale");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(document.grayscale(layer), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::Grayscale);

    // The filter kind and coverage are frozen in the accepted operation.
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->kind, FilterKind::Grayscale);
    QCOMPARE(operation->selectionRect, QRect(0, 0, 100, 100));
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 8, 8)), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect,
             QRect(0, 0, 100, 100));

    TileRenderer renderer(m_device);
    const auto filtered = render(renderer, document.snapshot());
    QVERIFY(filtered);
    const auto filteredBytes = readTile(filtered, layer, {0, 0});
    QCOMPARE(pixel(filteredBytes, 10, 10), gray(10, 20, 30, 40));
    QCOMPARE(pixel(filteredBytes, 20, 20), gray(0, 1, 0, 255));
    QCOMPARE(pixel(filteredBytes, 30, 30), gray(0, 0, 1, 255));
    QCOMPARE(pixel(filteredBytes, 40, 40), gray(100, 150, 200, 128));
    QCOMPARE(pixel(filteredBytes, 50, 50), gray(255, 0, 0, 255));
    QCOMPARE(pixel(filteredBytes, 60, 60), gray(0, 255, 0, 255));
    QCOMPARE(pixel(filteredBytes, 70, 70), gray(0, 0, 255, 255));
    QCOMPARE(pixel(filteredBytes, 80, 80), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(filteredBytes, 90, 90), gray(255, 255, 255, 0));
    QCOMPARE(pixel(filteredBytes, 200, 10), QByteArray::fromHex("11223344"));
    QCOMPARE(luma(255, 0, 0), quint8(54));
    QCOMPARE(luma(0, 255, 0), quint8(182));
    QCOMPARE(luma(0, 0, 255), quint8(18));
    QCOMPARE(luma(0, 1, 0), quint8(1));
    QCOMPARE(luma(0, 0, 1), quint8(0));

    // One filter history entry: undo restores the source bytes, redo the result.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filteredBytes);

    // Retained input preserves the kind, the frozen coverage and the same pixels.

    QCOMPARE(document.historyAction(2), HistoryAction::Grayscale);
    const auto retained = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->kind, FilterKind::Grayscale);
    QCOMPARE(retained->selectionRect, QRect(0, 0, 100, 100));
}

void KisGpuTileRendererTest::grayscaleClipsEllipseInvertedBinaryAndAlphaCoverage()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto luma = [](int r, int g, int b) {
        return quint8((2126 * r + 7152 * g + 722 * b + 5000) / 10000);
    };
    const auto gray = [&](int r, int g, int b, int a) {
        const auto value = luma(r, g, b);
        QByteArray result(4, '\0');
        result[0] = result[1] = result[2] = char(value);
        result[3] = char(a);
        return result;
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("10203040"));
    set(30, 30, QByteArray::fromHex("ff8000ff"));
    set(100, 100, QByteArray::fromHex("11223344"));
    set(120, 120, QByteArray::fromHex("c86432ff"));
    set(128, 128, QByteArray::fromHex("141e28ff"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Coverage");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    // A binary freehand mask clips exactly to its selected tiles and pixels.
    OperationDocument binaryDocument(input);
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray binaryCoverage(256 * 256, '\0');
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) binaryCoverage[y * 256 + x] = char(255);
    const auto binary = SelectionMask::fromCoverage(clip, binaryCoverage, clip);
    QVERIFY(binary);
    QCOMPARE(binaryDocument.setSelectionMask(binary), Result::Success);
    QCOMPARE(binaryDocument.grayscale(binaryDocument.activeLayer()), Result::Success);
    const auto binaryBytes = readTile(render(renderer, binaryDocument.snapshot()), binaryDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(binaryBytes, 10, 10), QByteArray::fromHex("13131328"));
    QCOMPARE(pixel(binaryBytes, 20, 20), gray(16, 32, 48, 64));
    QCOMPARE(pixel(binaryBytes, 30, 30), QByteArray::fromHex("929292ff"));
    QCOMPARE(pixel(binaryBytes, 100, 100), QByteArray::fromHex("11223344"));

    // Alpha coverage interpolates between the source and the gray value.
    OperationDocument alphaDocument(input);
    QByteArray alphaCoverage(256 * 256, '\0');
    alphaCoverage[20 * 256 + 20] = char(128);
    alphaCoverage[30 * 256 + 30] = char(255);
    const auto alpha = SelectionMask::fromAlphaCoverage(clip, alphaCoverage, clip);
    QVERIFY(alpha);
    QCOMPARE(alphaDocument.setSelectionMask(alpha), Result::Success);
    QCOMPARE(alphaDocument.grayscale(alphaDocument.activeLayer()), Result::Success);
    const auto alphaBytes = readTile(render(renderer, alphaDocument.snapshot()), alphaDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(alphaBytes, 20, 20), QByteArray::fromHex("171f2740"));
    QCOMPARE(pixel(alphaBytes, 30, 30), QByteArray::fromHex("929292ff"));
    QCOMPARE(pixel(alphaBytes, 10, 10), QByteArray::fromHex("0a141e28"));

    // An inverted rectangle filters only the outside.
    OperationDocument invertedDocument(input);
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.grayscale(invertedDocument.activeLayer()), Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()), invertedDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(inverted, 100, 100), QByteArray::fromHex("20202044"));
    QCOMPARE(pixel(inverted, 120, 120), QByteArray::fromHex("767676ff"));

    // Elliptical coverage intersects its bounding box before the pixel test.
    OperationDocument ellipseDocument(input);
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.grayscale(ellipseDocument.activeLayer()), Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()), ellipseDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(ellipse, 128, 128), gray(20, 30, 40, 255));
}

void KisGpuTileRendererTest::hsvAdjustmentUsesProductValuesAndNeutralPixels()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("c81e46ff"));  // 200, 30, 70, 255
    set(20, 20, QByteArray::fromHex("808080ff"));  // neutral gray
    set(30, 30, QByteArray::fromHex("000000ff"));  // black
    set(40, 40, QByteArray::fromHex("646464ff"));  // neutral gray 100
    set(50, 50, QByteArray::fromHex("0a141e28"));  // 10, 20, 30, alpha 40
    set(200, 10, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("HSV");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    // Saturation moves chroma but not value: the product's modern HSV rule.
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const HsvAdjustment saturation{0, 50, 0, HsvAdjustmentType::Hsv, false};
    QCOMPARE(document.hsvAdjustment(layer, saturation), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::HsvAdjustment);
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->kind, FilterKind::HsvAdjustment);
    QCOMPARE(operation->hue, 0);
    QCOMPARE(operation->saturation, 50);
    QCOMPARE(operation->value, 0);
    QCOMPARE(operation->hsvType, HsvAdjustmentType::Hsv);
    QVERIFY(!operation->compatibilityMode);

    const auto filtered = render(renderer, document.snapshot());
    QVERIFY(filtered);
    const auto filteredBytes = readTile(filtered, layer, {0, 0});
    QCOMPARE(pixel(filteredBytes, 10, 10), QByteArray::fromHex("c8002fff"));
    // Neutral pixels ignore hue and saturation; alpha is carried through.
    QCOMPARE(pixel(filteredBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(filteredBytes, 30, 30), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixel(filteredBytes, 40, 40), QByteArray::fromHex("646464ff"));
    QCOMPARE(pixel(filteredBytes, 50, 50), QByteArray::fromHex("000f1e28"));
    QCOMPARE(pixel(filteredBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Frozen settings survive a later selection edit.
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 8, 8)), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filteredBytes);

    // One filter history entry: undo restores the source, redo the result.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filteredBytes);

    // Retained input preserves the frozen settings and the same pixels.

    QCOMPARE(document.historyAction(2), HistoryAction::HsvAdjustment);
    const auto retained = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->saturation, 50);
    QCOMPARE(retained->value, 0);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), filteredBytes);

    // Value moves value and chroma toward the endpoint; saturation is unchanged
    // in the product rule, so a neutral pixel simply gets brighter.
    OperationDocument valueDocument(input);
    const auto valueLayer = valueDocument.activeLayer();
    QCOMPARE(valueDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(valueDocument.hsvAdjustment(valueLayer, {0, 0, 25, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto valueBytes = readTile(render(renderer, valueDocument.snapshot()), valueLayer, {0, 0});
    QCOMPARE(pixel(valueBytes, 10, 10), QByteArray::fromHex("d65674ff"));
    QCOMPARE(pixel(valueBytes, 20, 20), QByteArray::fromHex("a0a0a0ff"));
    QCOMPARE(pixel(valueBytes, 30, 30), QByteArray::fromHex("404040ff"));
    QCOMPARE(pixel(valueBytes, 50, 50), QByteArray::fromHex("474f5628"));

    // The byte conversion rounds half up, matching the CPU rule.
    OperationDocument rounding(input);
    const auto roundingLayer = rounding.activeLayer();
    QCOMPARE(rounding.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(rounding.hsvAdjustment(roundingLayer, {0, 0, 10, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto roundingBytes = readTile(render(renderer, rounding.snapshot()), roundingLayer, {0, 0});
    // 100 + 0.1 * (255 - 100) = 115.5 in byte units, rounded to 116.
    QCOMPARE(pixel(roundingBytes, 40, 40), QByteArray::fromHex("747474ff"));
    QCOMPARE(pixel(roundingBytes, 20, 20), QByteArray::fromHex("8d8d8dff"));

    // Hue rotation leaves neutral pixels unchanged.
    OperationDocument hueDocument(input);
    const auto hueLayer = hueDocument.activeLayer();
    QCOMPARE(hueDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(hueDocument.hsvAdjustment(hueLayer, {60, 0, 0, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto hueBytes = readTile(render(renderer, hueDocument.snapshot()), hueLayer, {0, 0});
    QCOMPARE(pixel(hueBytes, 10, 10), QByteArray::fromHex("c8a01eff"));
    QCOMPARE(pixel(hueBytes, 20, 20), QByteArray::fromHex("808080ff"));

    // The product `type` key selects the HSL policy; the same slider values
    // produce a different result from the HSV policy.
    OperationDocument hslDocument(input);
    const auto hslLayer = hslDocument.activeLayer();
    QCOMPARE(hslDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(hslDocument.hsvAdjustment(hslLayer, {0, 50, 0, HsvAdjustmentType::Hsl, false}),
             Result::Success);
    const auto hslOperation = std::get<FilterSnapshot>(hslDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(hslOperation->hsvType, HsvAdjustmentType::Hsl);
    const auto hslBytes = readTile(render(renderer, hslDocument.snapshot()), hslLayer, {0, 0});
    QCOMPARE(pixel(hslBytes, 10, 10), QByteArray::fromHex("e60036ff"));
    QCOMPARE(pixel(hslBytes, 20, 20), QByteArray::fromHex("808080ff"));

    // HSI (type 2) uses the product's HCI policy.
    OperationDocument hsiDocument(input);
    const auto hsiLayer = hsiDocument.activeLayer();
    QCOMPARE(hsiDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(hsiDocument.hsvAdjustment(hsiLayer, {0, 50, 0, HsvAdjustmentType::Hsi, false}),
             Result::Success);
    const auto hsiBytes = readTile(render(renderer, hsiDocument.snapshot()), hsiLayer, {0, 0});
    QCOMPARE(pixel(hsiBytes, 10, 10), QByteArray::fromHex("ed043bff"));
    QCOMPARE(pixel(hsiBytes, 20, 20), QByteArray::fromHex("808080ff"));

    // The legacy conversions apply to HSL and HSI as well.
    OperationDocument legacyHslDocument(input);
    const auto legacyHslLayer = legacyHslDocument.activeLayer();
    QCOMPARE(legacyHslDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyHslDocument.hsvAdjustment(legacyHslLayer,
                                             {0, 50, 0, HsvAdjustmentType::Hsl, true}), Result::Success);
    const auto legacyHslBytes = readTile(render(renderer, legacyHslDocument.snapshot()),
                                         legacyHslLayer, {0, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("hsv-legacy-hsl.rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(legacyHslBytes), qint64(legacyHslBytes.size()));
    }
    // The actual product hsv_adjustment transformation gives (243, 0, 48)
    // on both ARM64 and x86_64 for these settings and stored input bytes.
    QCOMPARE(pixel(legacyHslBytes, 10, 10), QByteArray::fromHex("f30030ff"));

    OperationDocument legacyHsiDocument(input);
    const auto legacyHsiLayer = legacyHsiDocument.activeLayer();
    QCOMPARE(legacyHsiDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyHsiDocument.hsvAdjustment(legacyHsiLayer,
                                             {0, 50, 0, HsvAdjustmentType::Hsi, true}), Result::Success);
    const auto legacyHsiBytes = readTile(render(renderer, legacyHsiDocument.snapshot()),
                                         legacyHsiLayer, {0, 0});
    QCOMPARE(pixel(legacyHsiBytes, 10, 10), QByteArray::fromHex("ff0035ff"));
}

void KisGpuTileRendererTest::hsvAdjustmentClipsCoverageAndSupportsLegacyConversion()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("0a141e28"));
    set(30, 30, QByteArray::fromHex("0a141e28"));
    set(40, 40, QByteArray::fromHex("646464ff"));
    set(100, 100, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Coverage");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    // Alpha coverage interpolates between the source and the adjusted pixel.
    OperationDocument alphaDocument(input);
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    coverage[20 * 256 + 20] = char(128);
    coverage[30 * 256 + 30] = char(255);
    const auto alpha = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(alpha);
    QCOMPARE(alphaDocument.setSelectionMask(alpha), Result::Success);
    QCOMPARE(alphaDocument.hsvAdjustment(alphaDocument.activeLayer(), {0, 0, 50, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto alphaBytes = readTile(render(renderer, alphaDocument.snapshot()),
                                     alphaDocument.activeLayer(), {0, 0});
    // 10,20,30 adjusted to 133,138,143 at 128/255 coverage.
    QCOMPARE(pixel(alphaBytes, 20, 20), QByteArray::fromHex("484f5728"));
    QCOMPARE(pixel(alphaBytes, 30, 30), QByteArray::fromHex("858a8f28"));
    QCOMPARE(pixel(alphaBytes, 10, 10), QByteArray::fromHex("0a141e28"));

    // An inverted rectangle filters only the outside.
    OperationDocument invertedDocument(input);
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.hsvAdjustment(invertedDocument.activeLayer(),
                                            {0, 0, 50, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()),
                                   invertedDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(inverted, 100, 100), QByteArray::fromHex("88919944"));

    // Elliptical coverage intersects its bounding box before the pixel test.
    OperationDocument ellipseDocument(input);
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.hsvAdjustment(ellipseDocument.activeLayer(),
                                           {0, 0, 50, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()),
                                  ellipseDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("0a141e28"));
    // The unset pixel is transparent; its hidden RGB still follows the filter.
    QCOMPARE(pixel(ellipse, 128, 128), QByteArray::fromHex("80808000"));

    // The legacy conversion can color a neutral pixel when saturation rises;
    // the modern HSV rule cannot. This difference is frozen with the settings.
    OperationDocument modernDocument(input);
    QCOMPARE(modernDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(modernDocument.hsvAdjustment(modernDocument.activeLayer(),
                                          {0, 50, 0, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto modern = readTile(render(renderer, modernDocument.snapshot()),
                                 modernDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(modern, 40, 40), QByteArray::fromHex("646464ff"));

    OperationDocument legacyDocument(input);
    QCOMPARE(legacyDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyDocument.hsvAdjustment(legacyDocument.activeLayer(),
                                          {0, 50, 0, HsvAdjustmentType::Hsv, true}), Result::Success);
    const auto legacyOperation = std::get<FilterSnapshot>(legacyDocument.snapshot()->layers.first().operations.last());
    QVERIFY(legacyOperation->compatibilityMode);
    const auto legacy = readTile(render(renderer, legacyDocument.snapshot()),
                                 legacyDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(legacy, 40, 40), QByteArray::fromHex("643233ff"));

    // The legacy value adjustment differs from the modern rule by more than
    // one byte on the same neutral input.
    OperationDocument legacyValue(input);
    QCOMPARE(legacyValue.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyValue.hsvAdjustment(legacyValue.activeLayer(),
                                       {0, 0, 10, HsvAdjustmentType::Hsv, true}), Result::Success);
    const auto legacyValueBytes = readTile(render(renderer, legacyValue.snapshot()),
                                           legacyValue.activeLayer(), {0, 0});
    QCOMPARE(pixel(legacyValueBytes, 40, 40), QByteArray::fromHex("7e7e7eff"));
    OperationDocument modernValue(input);
    QCOMPARE(modernValue.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(modernValue.hsvAdjustment(modernValue.activeLayer(),
                                       {0, 0, 10, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto modernValueBytes = readTile(render(renderer, modernValue.snapshot()),
                                           modernValue.activeLayer(), {0, 0});
    QCOMPARE(pixel(modernValueBytes, 40, 40), QByteArray::fromHex("747474ff"));

    // Retained input preserves the legacy flag and the same pixels.

    QCOMPARE(legacyDocument.historyAction(legacyDocument.undoCount()), HistoryAction::HsvAdjustment);
    QVERIFY(std::get<FilterSnapshot>(legacyDocument.snapshot()->layers.first().operations.last())->compatibilityMode);
    QCOMPARE(readTile(render(renderer, legacyDocument.snapshot()), legacyDocument.activeLayer(), {0, 0}), legacy);
}

void KisGpuTileRendererTest::yuvResetPreservesStoredPixels()
{
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    for (int y = 0; y < TileRenderer::TileSize; ++y) {
        for (int x = 0; x < TileRenderer::TileSize; ++x) {
            const int offset = (y * TileRenderer::TileSize + x) * 4;
            bytes[offset] = char(x);
            bytes[offset + 1] = char(y);
            bytes[offset + 2] = char((73 * x + 151 * y) % 256);
            bytes[offset + 3] = char((x + y) % 256);
        }
    }
    RasterImage input;
    input.size = {TileRenderer::TileSize, TileRenderer::TileSize};
    RasterLayer source;
    source.name = QStringLiteral("YUV reset");
    source.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000054}"));
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    HsvAdjustment reset{0, 0, 0, HsvAdjustmentType::Yuv, false};
    reset.lumaFrozen = true;
    QCOMPARE(document.hsvAdjustment(layer, reset), Result::Success);
    TileRenderer renderer(m_device);
    const auto actual = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("yuv-reset.rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(actual), qint64(actual.size()));
    }
    // RGBToYUV stores both neutral chroma components at 0.5. With zero
    // slider values, YUVToRGB restores every stored color and alpha byte.
    QCOMPARE(actual, bytes);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::HsvAdjustment);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
}

void KisGpuTileRendererTest::colorizeUsesProductLumaRounding_data()
{
    QTest::addColumn<qreal>("lumaRed");
    QTest::addColumn<qreal>("lumaGreen");
    QTest::addColumn<qreal>("lumaBlue");
    QTest::newRow("rec709") << qreal(0.2126f) << qreal(0.7152f) << qreal(0.0722f);
    QTest::newRow("quarters") << qreal(0.25f) << qreal(0.5f) << qreal(0.25f);
    QTest::newRow("small-coefficients") << qreal(1e-30f) << qreal(0.9f) << qreal(1e-38f);
    QTest::newRow("original-rec709") << qreal(0.2126) << qreal(0.7152) << qreal(0.0722);
    QTest::newRow("rgba-profile") << qreal(0.2224884) << qreal(0.71690369) << qreal(0.06060791);
}

void KisGpuTileRendererTest::colorizeUsesProductLumaRounding()
{
    QFETCH(qreal, lumaRed);
    QFETCH(qreal, lumaGreen);
    QFETCH(qreal, lumaBlue);
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    for (int y = 0; y < TileRenderer::TileSize; ++y) {
        for (int x = 0; x < TileRenderer::TileSize; ++x) {
            const int offset = (y * TileRenderer::TileSize + x) * 4;
            bytes[offset] = char(x);
            bytes[offset + 1] = char(y);
            bytes[offset + 2] = char((73 * x + 151 * y) % 256);
            bytes[offset + 3] = char((x + y) % 256);
        }
    }
    const int opaqueOffset = (43 * TileRenderer::TileSize + 2) * 4;
    const int hiddenOffset = opaqueOffset + 4;
    bytes.replace(opaqueOffset, 4, QByteArray::fromHex("022b2eff"));
    bytes.replace(hiddenOffset, 4, QByteArray::fromHex("022b2e00"));
    RasterImage input;
    input.size = {TileRenderer::TileSize, TileRenderer::TileSize};
    RasterLayer source;
    source.name = QStringLiteral("Colorize rounding");
    source.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000055}"));
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    HsvAdjustment colorize{0, 0, 0, HsvAdjustmentType::Hsv, false};
    colorize.colorize = true;
    colorize.luma = {lumaRed, lumaGreen, lumaBlue};
    colorize.lumaFrozen = true;
    QByteArray expected = bytes;
    for (int offset = 0; offset < bytes.size(); offset += 4) {
        const float red = quint8(bytes.at(offset)) / 255.0f;
        const float green = quint8(bytes.at(offset + 1)) / 255.0f;
        const float blue = quint8(bytes.at(offset + 2)) / 255.0f;
        const float lightness = red * qreal(lumaRed) + green * qreal(lumaGreen) + blue * qreal(lumaBlue);
        const char value = char(int(qBound(0.0f, lightness * 255.0f, 255.0f) + 0.5f));
        expected[offset] = value;
        expected[offset + 1] = value;
        expected[offset + 2] = value;
    }
    QCOMPARE(document.hsvAdjustment(document.activeLayer(), colorize), Result::Success);
    TileRenderer renderer(m_device);
    const auto actual = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("colorize-rounding-%1.rgba")
                                               .arg(QString::fromLatin1(QTest::currentDataTag()))));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(actual), qint64(actual.size()));
    }
    // Compare the f32 result to the native color conversion within #107.
    QCOMPARE_PIXELS_WITHIN_STEP(actual.mid(opaqueOffset, 4), expected.mid(opaqueOffset, 4));
    QCOMPARE_PIXELS_WITHIN_STEP(actual.mid(hiddenOffset, 4), expected.mid(hiddenOffset, 4));
    QCOMPARE_PIXELS_WITHIN_STEP(actual, expected);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
}

void KisGpuTileRendererTest::colorizePreservesDocumentLuma_data()
{
    QTest::addColumn<qreal>("lumaRed");
    QTest::addColumn<qreal>("lumaGreen");
    QTest::addColumn<qreal>("lumaBlue");
    QTest::addColumn<QByteArray>("sourcePixel");
    QTest::addColumn<QByteArray>("expectedPixel");
    // KoDummyColorProfile's colorants, normalised by KoColorSpace, are the
    // unmanaged RGBA8 document coefficients. The product hsv_adjustment
    // transform gives 173; first rounding these coefficients to float gives 172.
    QTest::newRow("rgba-profile") << qreal(0.2224884) << qreal(0.71690369) << qreal(0.06060791)
        << QByteArray::fromHex("80b9bcff") << QByteArray::fromHex("adadadff");
    // The product substitutes its qreal Rec.709 constants for a non-positive
    // coefficient. Its result is 33, while float-rounded constants give 32.
    QTest::newRow("rec709-fallback") << qreal(-1.0) << qreal(0.7152) << qreal(0.0722)
        << QByteArray::fromHex("630477ff") << QByteArray::fromHex("212121ff");
}

void KisGpuTileRendererTest::colorizePreservesDocumentLuma()
{
    QFETCH(qreal, lumaRed);
    QFETCH(qreal, lumaGreen);
    QFETCH(qreal, lumaBlue);
    QFETCH(QByteArray, sourcePixel);
    QFETCH(QByteArray, expectedPixel);
    RasterImage input;
    input.size = {8, 8};
    RasterLayer source;
    source.name = QStringLiteral("Document luma");
    source.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000056}"));
    for (int i = 0; i < 4; ++i) source.defaultPixel[i] = quint8(sourcePixel[i]);
    input.layers.append(source);
    OperationDocument document(input);
    HsvAdjustment settings{0, 0, 0, HsvAdjustmentType::Hsv, false};
    settings.colorize = true;
    QVERIFY(freezeLumaCoefficients({lumaRed, lumaGreen, lumaBlue}, &settings.luma));
    settings.lumaFrozen = true;
    QCOMPARE(document.hsvAdjustment(document.activeLayer(), settings), Result::Success);
    TileRenderer renderer(m_device);
    const auto actual = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});

    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        const auto name = QStringLiteral("colorize-original-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
        QFile file(QDir(directory).filePath(name + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(actual), qint64(actual.size()));
    }
    QCOMPARE_PIXELS_WITHIN_STEP(actual.left(4), expectedPixel);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = project(renderer, render(renderer, document.snapshot()), QRect(0, 0, 8, 8));
    QVERIFY(undone);
    QVERIFY(!undone->tiles().isEmpty());
    QCOMPARE(readBuffer(undone->tiles().first().pixels).left(4), sourcePixel);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
}

void KisGpuTileRendererTest::hsvAdjustmentUsesLumaDependentProductRulesAndFrozenCoefficients()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("c81e46ff"));  // 200,30,70,255
    set(20, 20, QByteArray::fromHex("808080ff"));  // neutral gray
    set(30, 30, QByteArray::fromHex("000000ff"));  // black
    set(40, 40, QByteArray::fromHex("646464ff"));  // neutral gray 100
    set(50, 50, QByteArray::fromHex("0a141e28"));  // 10,20,30, alpha 40
    set(200, 10, QByteArray::fromHex("11223344")); // outside the selection
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Luma");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    const std::array<double, 3> srgb{0.2126f, 0.7152f, 0.0722f};
    const auto lumaSettings = [](qint32 hue, qint32 saturation, qint32 value,
                                 HsvAdjustmentType type, bool legacy, bool colorize,
                                 const std::array<double, 3> &luma) {
        HsvAdjustment settings{hue, saturation, value, type, legacy};
        settings.colorize = colorize;
        settings.luma = luma;
        settings.lumaFrozen = true;
        return settings;
    };

    // HSY (type 3) uses the product HCY policy: the luma coefficients select
    // the value channel and the requested hue and chroma are shifted back to
    // the requested luma. A neutral pixel ignores hue and saturation.
    OperationDocument hsyDocument(input);
    const auto hsyLayer = hsyDocument.activeLayer();
    QCOMPARE(hsyDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto hsy = lumaSettings(0, 50, 0, HsvAdjustmentType::Hsy, false, false, srgb);
    QCOMPARE(hsyDocument.hsvAdjustment(hsyLayer, hsy), Result::Success);
    const auto hsyOperation =
        std::get<FilterSnapshot>(hsyDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(hsyOperation->hsvType, HsvAdjustmentType::Hsy);
    QVERIFY(!hsyOperation->colorize);
    QVERIFY(hsyOperation->luma == srgb);
    const auto hsyBytes = readTile(render(renderer, hsyDocument.snapshot()), hsyLayer, {0, 0});
    QCOMPARE(pixel(hsyBytes, 10, 10), QByteArray::fromHex("ff0a46ff"));
    QCOMPARE(pixel(hsyBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(hsyBytes, 30, 30), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixel(hsyBytes, 40, 40), QByteArray::fromHex("646464ff"));
    QCOMPARE(pixel(hsyBytes, 50, 50), QByteArray::fromHex("01152928"));
    QCOMPARE(pixel(hsyBytes, 200, 10), QByteArray::fromHex("11223344"));

    // The legacy HSY conversion is the product's qreal HCY conversion and
    // produces a different fractional byte on the same input.
    OperationDocument legacyHsyDocument(input);
    const auto legacyHsyLayer = legacyHsyDocument.activeLayer();
    QCOMPARE(legacyHsyDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto legacyHsy =
        lumaSettings(0, 50, 0, HsvAdjustmentType::Hsy, true, false, srgb);
    QCOMPARE(legacyHsyDocument.hsvAdjustment(legacyHsyLayer, legacyHsy), Result::Success);
    const auto legacyHsyBytes =
        readTile(render(renderer, legacyHsyDocument.snapshot()), legacyHsyLayer, {0, 0});
    QCOMPARE(pixel(legacyHsyBytes, 10, 10), QByteArray::fromHex("ff0a46ff"));
    QCOMPARE(pixel(legacyHsyBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(legacyHsyBytes, 50, 50), QByteArray::fromHex("06152428"));
    QCOMPARE(pixel(legacyHsyBytes, 200, 10), QByteArray::fromHex("11223344"));

    // YUV (type 4) scales the blue chroma by the hue slider, the red chroma by
    // the saturation slider and offsets luma. It has no legacy variant.
    // References use RGBToYUV/YUVToRGB with the product's slider scaling.
    OperationDocument yuvDocument(input);
    const auto yuvLayer = yuvDocument.activeLayer();
    QCOMPARE(yuvDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto yuv = lumaSettings(30, 20, -10, HsvAdjustmentType::Yuv, false, false, srgb);
    QCOMPARE(yuvDocument.hsvAdjustment(yuvLayer, yuv), Result::Success);
    const auto yuvOperation =
        std::get<FilterSnapshot>(yuvDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(yuvOperation->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(!yuvOperation->colorize);
    QVERIFY(yuvOperation->luma == srgb);
    const auto yuvBytes = readTile(render(renderer, yuvDocument.snapshot()), yuvLayer, {0, 0});
    QCOMPARE(pixel(yuvBytes, 10, 10), QByteArray::fromHex("f10074ff"));
    QCOMPARE(pixel(yuvBytes, 20, 20), QByteArray::fromHex("8f53adff"));
    QCOMPARE(pixel(yuvBytes, 30, 30), QByteArray::fromHex("0f002dff"));
    QCOMPARE(pixel(yuvBytes, 40, 40), QByteArray::fromHex("733791ff"));
    QCOMPARE(pixel(yuvBytes, 50, 50), QByteArray::fromHex("17004f28"));
    QCOMPARE(pixel(yuvBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Colorize uses the adjusted document luma as HSL lightness. The mode key
    // remains observable only through its slider range.
    OperationDocument colorizeDocument(input);
    const auto colorizeLayer = colorizeDocument.activeLayer();
    QCOMPARE(colorizeDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto colorize =
        lumaSettings(120, 80, 0, HsvAdjustmentType::Hsv, false, true, srgb);
    QCOMPARE(colorizeDocument.hsvAdjustment(colorizeLayer, colorize), Result::Success);
    const auto colorizeOperation =
        std::get<FilterSnapshot>(colorizeDocument.snapshot()->layers.first().operations.last());
    QVERIFY(colorizeOperation->colorize);
    QVERIFY(colorizeOperation->luma == srgb);
    const auto colorizeBytes =
        readTile(render(renderer, colorizeDocument.snapshot()), colorizeLayer, {0, 0});
    QCOMPARE(pixel(colorizeBytes, 10, 10), QByteArray::fromHex("0e7c0eff"));
    QCOMPARE(pixel(colorizeBytes, 20, 20), QByteArray::fromHex("1ae61aff"));
    QCOMPARE(pixel(colorizeBytes, 30, 30), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixel(colorizeBytes, 40, 40), QByteArray::fromHex("14b414ff"));
    QCOMPARE(pixel(colorizeBytes, 50, 50), QByteArray::fromHex("04210428"));
    QCOMPARE(pixel(colorizeBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Colorize with the YUV mode keeps the product's 0..100 Yellow-Blue
    // scaling. Its result differs from the type 0 Colorize rule.
    OperationDocument colorizeYuvDocument(input);
    const auto colorizeYuvLayer = colorizeYuvDocument.activeLayer();
    QCOMPARE(colorizeYuvDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto colorizeYuv =
        lumaSettings(50, 80, 20, HsvAdjustmentType::Yuv, true, true, srgb);
    QCOMPARE(colorizeYuvDocument.hsvAdjustment(colorizeYuvLayer, colorizeYuv), Result::Success);
    const auto colorizeYuvOperation =
        std::get<FilterSnapshot>(colorizeYuvDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(colorizeYuvOperation->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(colorizeYuvOperation->colorize);
    const auto colorizeYuvBytes =
        readTile(render(renderer, colorizeYuvDocument.snapshot()), colorizeYuvLayer, {0, 0});
    QCOMPARE(pixel(colorizeYuvBytes, 10, 10), QByteArray::fromHex("15bfbfff"));
    QCOMPARE(pixel(colorizeYuvBytes, 20, 20), QByteArray::fromHex("48ebebff"));
    QCOMPARE(pixel(colorizeYuvBytes, 50, 50), QByteArray::fromHex("0d777728"));
    QCOMPARE(pixel(colorizeYuvBytes, 200, 10), QByteArray::fromHex("11223344"));

    // A different frozen coefficient set changes the result even though the
    // operation no longer reads a color-space object. The original values
    // remain fixed at admission.
    const std::array<double, 3> custom{0.25f, 0.5f, 0.25f};
    const std::array<qreal, 3> supplied{0.25, 0.5, 0.25};
    std::array<double, 3> rounded{};
    QVERIFY(freezeLumaCoefficients(supplied, &rounded));
    QVERIFY(rounded == custom);
    OperationDocument customDocument(input);
    const auto customLayer = customDocument.activeLayer();
    QCOMPARE(customDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto customHsy =
        lumaSettings(0, 50, 0, HsvAdjustmentType::Hsy, false, false, rounded);
    QCOMPARE(customDocument.hsvAdjustment(customLayer, customHsy), Result::Success);
    const auto customBytes =
        readTile(render(renderer, customDocument.snapshot()), customLayer, {0, 0});
    QCOMPARE(pixel(customBytes, 10, 10), QByteArray::fromHex("ff0440ff"));
    QCOMPARE(pixel(customBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(customBytes, 50, 50), QByteArray::fromHex("00142828"));
    QCOMPARE(pixel(customBytes, 200, 10), QByteArray::fromHex("11223344"));

    // YUV and Colorize consume the same frozen coefficient set, so changing it
    // changes their pixels as well.
    OperationDocument customYuvDocument(input);
    const auto customYuvLayer = customYuvDocument.activeLayer();
    QCOMPARE(customYuvDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(customYuvDocument.hsvAdjustment(
                 customYuvLayer,
                 lumaSettings(30, 20, -10, HsvAdjustmentType::Yuv, false, false, rounded)),
             Result::Success);
    const auto customYuvBytes =
        readTile(render(renderer, customYuvDocument.snapshot()), customYuvLayer, {0, 0});
    QCOMPARE(pixel(customYuvBytes, 10, 10), QByteArray::fromHex("ec0062ff"));
    QCOMPARE(pixel(customYuvBytes, 20, 20), QByteArray::fromHex("8d37a0ff"));
    QCOMPARE(pixel(customYuvBytes, 30, 30), QByteArray::fromHex("0d0020ff"));
    QCOMPARE(pixel(customYuvBytes, 50, 50), QByteArray::fromHex("15004128"));
    QCOMPARE(pixel(customYuvBytes, 200, 10), QByteArray::fromHex("11223344"));

    OperationDocument customColorizeDocument(input);
    const auto customColorizeLayer = customColorizeDocument.activeLayer();
    QCOMPARE(customColorizeDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(customColorizeDocument.hsvAdjustment(
                 customColorizeLayer,
                 lumaSettings(120, 80, 0, HsvAdjustmentType::Hsv, false, true, rounded)),
             Result::Success);
    const auto customColorizeBytes =
        readTile(render(renderer, customColorizeDocument.snapshot()), customColorizeLayer, {0, 0});
    QCOMPARE(pixel(customColorizeBytes, 10, 10), QByteArray::fromHex("119511ff"));
    QCOMPARE(pixel(customColorizeBytes, 20, 20), QByteArray::fromHex("1ae61aff"));
    QCOMPARE(pixel(customColorizeBytes, 50, 50), QByteArray::fromHex("04240428"));
    QCOMPARE(pixel(customColorizeBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Undo restores the source bytes and redo restores the filtered bytes.
    QCOMPARE(yuvDocument.undo(), Result::Success);
    QCOMPARE(yuvDocument.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, yuvDocument.snapshot()), yuvLayer, {0, 0}), bytes);
    QCOMPARE(yuvDocument.redo(), Result::Success);
    QCOMPARE(yuvDocument.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, yuvDocument.snapshot()), yuvLayer, {0, 0}), yuvBytes);

    // Retained input preserves Colorize, the custom coefficients and the same pixels.

    const auto retainedOperation =
        std::get<FilterSnapshot>(customDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(retainedOperation->hsvType, HsvAdjustmentType::Hsy);
    QVERIFY(retainedOperation->luma == custom);

    const auto retainedColorizeOperation =
        std::get<FilterSnapshot>(colorizeYuvDocument.snapshot()->layers.first().operations.last());
    QVERIFY(retainedColorizeOperation->colorize);
    QCOMPARE(retainedColorizeOperation->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(retainedColorizeOperation->luma == srgb);
}

void KisGpuTileRendererTest::hsvAdjustmentLumaModesClipAlphaAndSelectionCoverage()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("0a141e28"));
    set(30, 30, QByteArray::fromHex("0a141e28"));
    set(40, 40, QByteArray::fromHex("646464ff"));
    set(100, 100, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Luma coverage");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);
    const std::array<double, 3> luma{0.2126f, 0.7152f, 0.0722f};
    const auto settings = [&](qint32 hue, qint32 saturation, qint32 value,
                              HsvAdjustmentType type, bool colorize) {
        HsvAdjustment result{hue, saturation, value, type, false};
        result.colorize = colorize;
        result.luma = luma;
        result.lumaFrozen = true;
        return result;
    };

    // A feathered alpha selection interpolates the YUV result; alpha itself is
    // carried through unchanged.
    OperationDocument alphaDocument(input);
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    coverage[20 * 256 + 20] = char(128);
    coverage[30 * 256 + 30] = char(255);
    const auto alpha = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(alpha);
    QCOMPARE(alphaDocument.setSelectionMask(alpha), Result::Success);
    QCOMPARE(alphaDocument.hsvAdjustment(alphaDocument.activeLayer(),
                                        settings(0, 0, 50, HsvAdjustmentType::Yuv, false)),
             Result::Success);
    const auto alphaBytes = readTile(render(renderer, alphaDocument.snapshot()),
                                     alphaDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(alphaBytes, 20, 20), QByteArray::fromHex("4a545e28"));
    QCOMPARE(pixel(alphaBytes, 30, 30), QByteArray::fromHex("8a949e28"));
    QCOMPARE(pixel(alphaBytes, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(alphaBytes, 40, 40), QByteArray::fromHex("646464ff"));

    // Colorize also filters only the complement of an inverted rectangle.
    OperationDocument invertedDocument(input);
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.hsvAdjustment(invertedDocument.activeLayer(),
                                           settings(120, 80, 0, HsvAdjustmentType::Hsv, true)),
             Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()),
                                   invertedDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(inverted, 100, 100), QByteArray::fromHex("06390644"));

    // YUV follows the elliptical coverage pixel test.
    OperationDocument ellipseDocument(input);
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.hsvAdjustment(ellipseDocument.activeLayer(),
                                          settings(0, 0, 50, HsvAdjustmentType::Yuv, false)),
             Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()),
                                  ellipseDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(ellipse, 100, 100), QByteArray::fromHex("91a2b344"));

    // The operation owns the coefficients used by the renderer.
    const auto retainedOperation =
        std::get<FilterSnapshot>(ellipseDocument.snapshot()->layers.first().operations.last());
    QVERIFY(retainedOperation->luma == luma);
    QCOMPARE(retainedOperation->hsvType, HsvAdjustmentType::Yuv);
}

void KisGpuTileRendererTest::featheredSelectionClipsPaintFillEraseHistory()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    for (int y = 64; y < 192; ++y)
        for (int x = 64; x < 192; ++x) coverage[y * 256 + x] = char(255);
    const auto hard = SelectionMask::fromCoverage(QRect(QPoint(), QSize(256, 256)), coverage, clip);
    QVERIFY(hard);
    const auto soft = hard->feathered(8, clip);
    QVERIFY(soft);
    QVERIFY(soft->isAlpha());
    const quint8 edge = soft->coverage(64, 128);
    QVERIFY(edge > 0 && edge < 255);
    QCOMPARE(soft->coverage(128, 128), 255);

    OperationDocument document({256, 256});
    Brush brush;
    brush.rgba = {210, 40, 90, 255};
    brush.diameter = 32;
    QCOMPARE(document.setSelectionMask(hard), Result::Success);
    QCOMPARE(document.setSelectionMask(soft), Result::Success);
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{48, 128}, 1}, {{208, 128}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QVERIFY(pending->selectionMask == soft);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    const auto bytes = readTile(painted, document.activeLayer(), {0, 0});
    QCOMPARE(pixel(bytes, 128, 128), QByteArray::fromHex("d2285aff"));
    QCOMPARE(quint8(pixel(bytes, 64, 128).at(3)), edge);
    QCOMPARE(pixel(bytes, 40, 128), QByteArray(4, '\0'));

    // Fill applies the fractional selection as its source alpha.
    TileRenderer fillRenderer(m_device);
    OperationDocument fillDocument({256, 256});
    QCOMPARE(fillDocument.setSelectionMask(soft), Result::Success);
    QCOMPARE(fillDocument.fillLayer(fillDocument.activeLayer(), {17, 34, 51}), Result::Success);
    const auto filled = readTile(render(fillRenderer, fillDocument.snapshot()), fillDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(filled, 128, 128), QByteArray::fromHex("112233ff"));
    QCOMPARE(quint8(pixel(filled, 64, 128).at(3)), edge);
    QCOMPARE(pixel(filled, 40, 128), QByteArray(4, '\0'));

    // Erase removes exactly the fractional share of the existing alpha.
    OperationDocument eraseDocument({256, 256});
    QCOMPARE(eraseDocument.fillLayer(eraseDocument.activeLayer(), {20, 80, 160}), Result::Success);
    QCOMPARE(eraseDocument.setSelectionMask(soft), Result::Success);
    Brush eraser = brush;
    eraser.erase = true;
    const auto erase = eraseDocument.beginStroke(eraseDocument.activeLayer(), eraser, 0);
    QCOMPARE(eraseDocument.appendSamples(erase, {{{48, 128}, 1}, {{208, 128}, 1}}), Result::Success);
    QCOMPARE(eraseDocument.commitStroke(erase), Result::Success);
    const auto erased = readTile(render(renderer, eraseDocument.snapshot()), eraseDocument.activeLayer(), {0, 0});
    QCOMPARE(quint8(pixel(erased, 128, 128).at(3)), 0);
    QCOMPARE(quint8(pixel(erased, 64, 128).at(3)), quint8(255 - edge));
    QCOMPARE(quint8(pixel(erased, 40, 128).at(3)), 255);

    // The feather is one history entry; undo restores the hard coverage and redo
    // restores the same fractional pixels.
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask() == soft);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask() == hard);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.selectionMask()->isAlpha());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);

    QVERIFY(document.selectionMask()->isAlpha());
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);
}

void KisGpuTileRendererTest::layerTransparencyMaskMultipliesCompositionAlpha()
{
    const QRect clip(QPoint(), QSize(64, 64));
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto projectedBytes = [&](const ProjectedSnapshot &projected) {
        for (const auto &tile : projected->tiles())
            if (tile.coordinate == QPoint(0, 0)) return readBuffer(tile.pixels);
        return QByteArray();
    };
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {200, 30, 70}), Result::Success);
    QCOMPARE(document.setSelectionRect(QRect(16, 16, 16, 16)), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    TileRenderer renderer(m_device);
    auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    // The paint layer retains its own pixels; composition applies the mask.
    const auto layerBytes = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(layerBytes, 8, 8), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(layerBytes, 24, 24), QByteArray::fromHex("c81e46ff"));
    auto projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    auto composed = projectedBytes(projected);
    QCOMPARE(composed.size(), TileRenderer::TileBytes);
    QCOMPARE(pixel(composed, 24, 24), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(composed, 8, 8), QByteArray(4, '\0'));
    QCOMPARE(pixel(composed, 40, 40), QByteArray(4, '\0'));

    // Hiding the mask disables its composition effect without deleting it.
    QCOMPARE(document.setTransparencyMaskVisible(layer, false), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 8, 8), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(composed, 24, 24), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(document.setTransparencyMaskVisible(layer, true), Result::Success);

    // Removal restores full coverage; undo reinstates the retained mask.
    QCOMPARE(document.removeTransparencyMask(layer), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 8, 8), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(document.undo(), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 8, 8), QByteArray(4, '\0'));
    QCOMPARE(pixel(composed, 24, 24), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(document.undo(), Result::Success); // hide
    QCOMPARE(document.undo(), Result::Success); // show
    QCOMPARE(document.undo(), Result::Success); // remove mask addition
    QVERIFY(!document.layerTransparencyMask(layer));

    // Fractional selection coverage becomes fractional composition alpha and
    // masks pixels outside its bounds completely.
    QByteArray coverage(64 * 64, '\0');
    for (int y = 32; y < 48; ++y)
        for (int x = 32; x < 48; ++x) coverage[y * 64 + x] = char(128);
    const auto soft = SelectionMask::fromAlphaCoverage(QRect(QPoint(), QSize(64, 64)), coverage, clip);
    QVERIFY(soft && soft->isAlpha());
    QCOMPARE(document.setSelectionMask(soft), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 40, 40), QByteArray::fromHex("c81e4680"));
    QCOMPARE(pixel(composed, 8, 8), QByteArray(4, '\0'));
    QCOMPARE(pixel(composed, 56, 56), QByteArray(4, '\0'));
}

void KisGpuTileRendererTest::maskedCopyKeepsNativeTransparentComponents()
{
    // Copy onto an empty native mask projection preserves hidden components
    // only at full coverage. Partial application with zero resulting alpha
    // clears them, and zero coverage leaves the empty destination unchanged.
    for (const auto format : {PixelFormat::Rgba8, PixelFormat::Rgba16}) {
        const auto pixel = rgbaBytes(format == PixelFormat::Rgba8 ? RgbaPixel{11, 63, 129, 0} :
            RgbaPixel{1111, 6333, 12999, 0}, format);
        for (const auto alpha : {quint8(0), quint8(1), quint8(128), quint8(255)}) {
            RasterImage input; input.size = {64, 64}; input.format = format;
            input.layers = {{QStringLiteral("Copy")}};
            auto &layer = input.layers.first(); layer.blendMode = LayerBlendMode::Copy;
            layer.tiles = {{{0, 0}, pixel.repeated(256 * 256), format}};
            layer.transparencyMask = alpha ? SelectionMask::fromAlphaCoverage(QRect(0, 0, 64, 64),
                QByteArray(64 * 64, char(alpha)), QRect(0, 0, 64, 64)) : SelectionMask::empty();
            OperationDocument document(input); TileRenderer renderer(m_device);
            const auto rendered = render(renderer, document.snapshot()); QVERIFY(rendered);
            const auto composed = project(renderer, rendered, QRect(0, 0, 64, 64)); QVERIFY(composed);
            const auto expected = alpha == 255 ? pixel : QByteArray(pixel.size(), '\0');
            QVERIFY(!composed->tiles().isEmpty());
            QCOMPARE(readBuffer(composed->tiles().first().pixels).first(pixel.size()), expected);
            QCOMPARE(readTile(rendered, document.activeLayer(), {0, 0}).first(pixel.size()), pixel);
        }
    }
}

void KisGpuTileRendererTest::inputPartitionsAndPreviewProduceTheSamePixels()
{
    QList<InputSample> samples;
    for (int i = 0; i < 37; ++i) samples.append({{20.125 + i * 7.25, 70.5 + (i % 5) * 2}, qreal(i % 8 + 1) / 8});
    OperationDocument single({512, 512}), chunks({512, 512});
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 29.25;
    brush.opacity = 177.0 / 255;
    const auto a = single.beginStroke(single.activeLayer(), brush, 123);
    const auto b = chunks.beginStroke(chunks.activeLayer(), brush, 123);
    QCOMPARE(single.appendSamples(a, samples), Result::Success);
    for (const auto &sample : samples) QCOMPARE(chunks.appendSamples(b, {sample}), Result::Success);
    TileRenderer renderer(m_device);
    const auto left = render(renderer, single.preview());
    const auto right = render(renderer, chunks.preview());
    QVERIFY(left && right);
    for (const auto &tile : left->tiles(single.activeLayer())) {
        QCOMPARE(readTile(left, single.activeLayer(), tile.coordinate), readTile(right, chunks.activeLayer(), tile.coordinate));
    }
}

void KisGpuTileRendererTest::scatteredInputKeepsFrozenSeedAndPartitions_data()
{
    QTest::addColumn<bool>("imageTip");
    QTest::addColumn<bool>("incremental");
    QTest::newRow("generated") << false << false;
    QTest::newRow("imported-image") << true << false;
    QTest::newRow("generated-buildup-tilt") << false << true;
    QTest::newRow("image-buildup-tilt") << true << true;
}

