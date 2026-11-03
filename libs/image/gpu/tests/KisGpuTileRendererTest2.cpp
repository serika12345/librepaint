/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRendererTest.h"

void KisGpuTileRendererTest::alphaLockPreservesCoverageHistory()
{
    QFETCH(quint8, baseAlpha);
    QFETCH(bool, erase);
    QByteArray pixel = baseAlpha ? QByteArray::fromHex("0a28b400") : QByteArray(4, '\0');
    pixel[3] = char(baseAlpha);
    const auto background = pixel.repeated(RgbaTile::ByteCount / 4);
    RasterImage image;
    image.size = {64, 64};
    image.layers = {{QStringLiteral("Protected alpha"), true, {}, {{{0, 0}, background}}}};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {200, 70, 30, 255};
    brush.diameter = 8;
    brush.opacity = 87.0 / 255;
    brush.erase = erase;
    brush.alphaLocked = true;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.snapshot());
    QVERIFY(preview);
    const auto pixels = readTile(preview, layer, {0, 0});
    QCOMPARE(pixels.size(), RgbaTile::ByteCount);
    auto expected = pixel;
    const quint32 alpha = 87;
    if (erase) expected[3] = char((baseAlpha * (255u - alpha) + 127u) / 255u);
    else {
        for (int channel = 0; channel < 3; ++channel)
            expected[channel] = char((brush.rgba[channel] * alpha + quint8(pixel[channel]) * (255u - alpha) + 127u) / 255u);
    }
    QCOMPARE(pixels.mid((32 * RgbaTile::Size + 32) * 4, 4), expected);
    QCOMPARE(pixels.left(4), pixel);
    if (!erase) {
        for (qsizetype offset = 3; offset < pixels.size(); offset += 4)
            QCOMPARE(quint8(pixels[offset]), baseAlpha);
    }

    const auto retained = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->brush, brush);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("alpha-lock-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), background);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    if (baseAlpha == 0 && !erase) {
        QVERIFY(document.addLayer(QStringLiteral("Empty upper layer")));
        const auto withUpper = render(renderer, document.snapshot());
        const auto projection = project(renderer, withUpper, QRect(0, 0, 64, 64));
        QVERIFY(projection && !projection->tiles().isEmpty());
        QCOMPARE(readBuffer(projection->tiles().first().pixels).mid((32 * RgbaTile::Size + 32) * 4, 4), QByteArray(4, '\0'));
        QCOMPARE(readTile(withUpper, layer, {0, 0}), pixels);
    }
}

void KisGpuTileRendererTest::straightLinesPreserveIntermediatePressure_data()
{
    QTest::addColumn<bool>("offAxis");
    QTest::newRow("collinear") << false;
    QTest::newRow("projected") << true;
}

void KisGpuTileRendererTest::straightLinesPreserveIntermediatePressure()
{
    QFETCH(bool, offAxis);
    Brush brush;
    brush.diameter = 20;
    brush.rgba = {17, 129, 241, 255};
    OperationDocument document({256, 256});
    const auto layer = document.addLayer(QStringLiteral("Pressure line"));
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 64}, 0.15}, {{128, offAxis ? 100.0 : 64.0}, 1}, {{224, 64}, 0.15}}), Result::Success);
    TileRenderer renderer(m_device);
    for (const int y : {64, 128}) {
        OperationDocument expected({256, 256});
        const auto expectedLayer = expected.addLayer(QStringLiteral("Pressure samples"));
        const auto expectedStroke = expected.beginStroke(expectedLayer, brush, 0);
        QCOMPARE(expected.appendSamples(expectedStroke, {{{32, qreal(y)}, 0.15}, {{128, qreal(y)}, 1}, {{224, qreal(y)}, 0.15}}), Result::Success);
        const auto reference = render(renderer, expected.preview());
        QVERIFY(reference);
        QCOMPARE(document.updateShape(stroke, {ShapeKind::Line, {32, qreal(y)}, {224, qreal(y)}}), Result::Success);
        const auto line = render(renderer, document.preview());
        QVERIFY(line);
        QCOMPARE(readTile(line, layer, {0, 0}), readTile(reference, expectedLayer, {0, 0}));
    }
}

void KisGpuTileRendererTest::transparentLayerPixelsFollowRootProjectionSemantics_data()
{
    QTest::addColumn<bool>("composite");
    QTest::addColumn<bool>("topPixels");
    QTest::addColumn<bool>("whiteBackground");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("direct-layer") << false << false << false << QByteArray::fromHex("11223300");
    QTest::newRow("empty-upper-layer") << true << false << false << QByteArray::fromHex("00000000");
    QTest::newRow("two-transparent-layers") << true << true << false << QByteArray::fromHex("00000000");
    QTest::newRow("transparent-white-background") << true << true << true << QByteArray::fromHex("ffffff00");
}

void KisGpuTileRendererTest::transparentLayerPixelsFollowRootProjectionSemantics()
{
    QFETCH(bool, composite);
    QFETCH(bool, topPixels);
    QFETCH(bool, whiteBackground);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {256, 256};
    if (whiteBackground) input.projectionBackground = {255, 255, 255, 0};
    input.layers = {{QStringLiteral("Lower"), true, {},
                     {{{0, 0}, QByteArray::fromHex("11223300").repeated(RgbaTile::ByteCount / 4)}}}};
    if (composite) input.layers.append({QStringLiteral("Upper"), true, {}, {}});
    if (topPixels) input.layers.last().tiles = {{{0, 0}, QByteArray::fromHex("44556600").repeated(RgbaTile::ByteCount / 4)}};
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    const auto pixels = render(renderer, document.snapshot());
    QVERIFY(pixels);
    const auto projection = project(renderer, pixels, QRect(0, 0, 256, 256));
    QVERIFY(projection);
    QCOMPARE(readBuffer(projection->tiles().first().pixels).left(4), expected);
    QCOMPARE(readTile(pixels, 1, {0, 0}).left(4), QByteArray::fromHex("11223300"));
}

void KisGpuTileRendererTest::layerCountChangesInvalidateTransparentProjectionReuse()
{
    RasterImage input;
    input.size = {256, 256};
    input.layers = {{QStringLiteral("Lower"), true, {},
                     {{{0, 0}, QByteArray::fromHex("11223300").repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(input);
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
    const QRect region(0, 0, 256, 256);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    const auto single = project(renderer, original, region);
    QVERIFY(single);
    QCOMPARE(readBuffer(single->tiles().first().pixels).left(4), QByteArray::fromHex("11223300"));
    QVERIFY(document.addLayer(QStringLiteral("Upper")));
    const auto expanded = render(renderer, document.snapshot(), original);
    QVERIFY(expanded);
    const auto composite = project(renderer, expanded, region, single);
    QVERIFY(composite);
    QCOMPARE(readBuffer(composite->tiles().first().pixels).left(4), QByteArray::fromHex("00000000"));
    QCOMPARE(document.undo(), Result::Success);
    const auto restored = render(renderer, document.snapshot(), expanded);
    QVERIFY(restored);
    const auto direct = project(renderer, restored, region, composite);
    QVERIFY(direct);
    QCOMPARE(readBuffer(direct->tiles().first().pixels).left(4), QByteArray::fromHex("11223300"));
    QCOMPARE(readBuffer(single->tiles().first().pixels).left(4), QByteArray::fromHex("11223300"));
    QCOMPARE(readBuffer(composite->tiles().first().pixels).left(4), QByteArray::fromHex("00000000"));
    QVERIFY(renderer.residentBytes() <= 2 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::normalProjectionStaysWithinOneStep_data()
{
    QTest::addColumn<int>("opacity");
    for (const int opacity : {1, 128, 254, 255}) {
        const QByteArray name = QByteArray::number(opacity);
        QTest::newRow(name.constData()) << opacity;
    }
}

void KisGpuTileRendererTest::normalProjectionStaysWithinOneStep()
{
    QFETCH(int, opacity);
    QByteArray lower(RgbaTile::ByteCount, '\0'), upper(RgbaTile::ByteCount, '\0');
    for (int destinationAlpha = 0; destinationAlpha < 256; ++destinationAlpha) {
        for (int sourceAlpha = 0; sourceAlpha < 256; ++sourceAlpha) {
            const auto offset = (destinationAlpha * 256 + sourceAlpha) * 4;
            lower[offset] = '\x99'; lower[offset + 1] = '\x66'; lower[offset + 2] = '\x33';
            lower[offset + 3] = char(destinationAlpha);
            upper[offset] = '\x56'; upper[offset + 1] = '\x34'; upper[offset + 2] = '\x12';
            upper[offset + 3] = char(sourceAlpha);
        }
    }
    RasterImage source;
    source.size = {256, 256};
    source.layers = {{QStringLiteral("Destination"), true, {}, {{{0, 0}, lower}}},
                     {QStringLiteral("Source"), true, {}, {{{0, 0}, upper}}}};
    source.layers.last().opacity = quint8(opacity);
    OperationDocument document(source);
    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto projected = project(renderer, rendered, QRect(0, 0, 256, 256));
    QVERIFY(projected);
    QByteArray actual;
    for (const auto &tile : projected->tiles()) if (tile.coordinate == QPoint()) actual = readBuffer(tile.pixels);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile artifact(QDir(directory).filePath(QStringLiteral("normalized-opacity-%1.rgba").arg(opacity)));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(actual), actual.size());
    }
    QCOMPARE(actual.size(), RgbaTile::ByteCount);
    // Straight-alpha normal composition evaluated in double precision (#107:
    // within one 8-bit step). Fully transparent results carry no color.
    QByteArray expected(RgbaTile::ByteCount, '\0');
    for (int index = 0; index < 256 * 256; ++index) {
        const auto *d = reinterpret_cast<const quint8 *>(lower.constData()) + index * 4;
        const auto *u = reinterpret_cast<const quint8 *>(upper.constData()) + index * 4;
        const double sourceAlpha = u[3] / 255.0 * opacity / 255.0;
        const double destinationAlpha = d[3] / 255.0;
        const double alpha = sourceAlpha + destinationAlpha * (1.0 - sourceAlpha);
        auto *e = reinterpret_cast<quint8 *>(expected.data()) + index * 4;
        e[3] = quint8(std::lround(alpha * 255.0));
        if (e[3] == 0) {
            std::memcpy(e, actual.constData() + index * 4, 3);
            continue;
        }
        for (int channel = 0; channel < 3; ++channel) {
            const double color = (u[channel] * sourceAlpha + d[channel] * destinationAlpha * (1.0 - sourceAlpha)) / alpha;
            e[channel] = quint8(std::lround(std::clamp(color, 0.0, 255.0)));
        }
    }
    QCOMPARE_PIXELS_WITHIN_STEP(actual, expected);
}

void KisGpuTileRendererTest::normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries_data()
{
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<QByteArray>("sourcePixel");
    QTest::addColumn<QByteArray>("destinationPixel");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<QByteArray>("expectedPixel");
    QTest::addColumn<bool>("nativeFormat");
    for (const bool implicit : {false, true}) {
        QTest::newRow(implicit ? "default-pixels" : "tiles") << implicit
            << QByteArray::fromHex("7e7e7eff") << QByteArray::fromHex("818181ff")
            << 85 << QByteArray::fromHex("808080ff") << false;
        QTest::newRow(implicit ? "negative-half-default-pixels" : "negative-half-tiles") << implicit
            << QByteArray::fromHex("00000026") << QByteArray::fromHex("fdfdfd19")
            << 1 << QByteArray::fromHex("fbfbfb19") << false;
        // Full-opacity references retain binary32 fused color rounding followed
        // by nearest-even byte conversion, for implicit and stored layer pixels.
        QTest::newRow(implicit ? "full-opacity-even-up-default" : "full-opacity-even-up-tile") << implicit
            << QByteArray::fromHex("ffffff02") << QByteArray::fromHex("01010102")
            << 255 << QByteArray::fromHex("80808004") << false;
        QTest::newRow(implicit ? "full-opacity-even-down-default" : "full-opacity-even-down-tile") << implicit
            << QByteArray::fromHex("00000002") << QByteArray::fromHex("fefefe02")
            << 255 << QByteArray::fromHex("7e7e7e04") << false;
        QTest::newRow(implicit ? "full-opacity-quarter-default" : "full-opacity-quarter-tile") << implicit
            << QByteArray::fromHex("a9a9a902") << QByteArray::fromHex("00000006")
            << 255 << QByteArray::fromHex("2a2a2a08") << false;
        QTest::newRow(implicit ? "full-opacity-mixed-boundary-default" : "full-opacity-mixed-boundary-tile") << implicit
            << QByteArray::fromHex("ff005002") << QByteArray::fromHex("01fe1102")
            << 255 << QByteArray::fromHex("807e3104") << false;
        QTest::newRow(implicit ? "full-opacity-mixed-colors-default" : "full-opacity-mixed-colors-tile") << implicit
            << QByteArray::fromHex("1464c880") << QByteArray::fromHex("c8641480")
            << 255 << QByteArray::fromHex("50648cc0") << false;
        QTest::newRow(implicit ? "native-fma-default" : "native-fma-tile") << implicit
            << rgbaBytes({7435, 7030, 44014, 53742}, PixelFormat::Rgba16)
            << rgbaBytes({6428, 56750, 11086, 49993}, PixelFormat::Rgba16) << 255
            << rgbaBytes({7291, 14159, 39292, 62738}, PixelFormat::Rgba16) << true;
        QTest::newRow(implicit ? "native-zero-source-default" : "native-zero-source-tile") << implicit
            << rgbaBytes({1, 257, 65534, 0}, PixelFormat::Rgba16)
            << rgbaBytes({65534, 257, 1, 65535}, PixelFormat::Rgba16) << 128
            << rgbaBytes({65534, 257, 1, 65535}, PixelFormat::Rgba16) << true;
        QTest::newRow(implicit ? "native-copy-default" : "native-copy-tile") << implicit
            << rgbaBytes({65534, 1, 257, 32768}, PixelFormat::Rgba16)
            << rgbaBytes({1, 257, 65534, 0}, PixelFormat::Rgba16) << 255
            << rgbaBytes({65534, 1, 257, 32768}, PixelFormat::Rgba16) << true;
    }
}

void KisGpuTileRendererTest::normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries()
{
    QFETCH(bool, implicit);
    QFETCH(bool, nativeFormat);
    QFETCH(QByteArray, sourcePixel);
    QFETCH(QByteArray, destinationPixel);
    QFETCH(int, opacity);
    QFETCH(QByteArray, expectedPixel);
    const QUuid group(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const auto format = nativeFormat ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    // The extra transparent layer exercises the paired native projection pass.
    for (int extraLayer = 0; extraLayer <= int(nativeFormat); ++extraLayer) {
        RasterImage input;
        input.format = format;
        input.size = {64, 48};
        input.layers = {{QStringLiteral("Destination"), true, {}, {}, {129, 129, 129, 255}},
                        {QStringLiteral("Source"), true, {}, {}, {126, 126, 126, 255}},
                        {QStringLiteral("Group"), true, group, {}}};
        input.layers[0].parentUuid = input.layers[1].parentUuid = group;
        input.layers[1].opacity = quint8(opacity);
        input.layers[2].kind = LayerKind::Group;
        input.layers[0].defaultPixel = rgbaPixel(destinationPixel, format).value();
        input.layers[1].defaultPixel = rgbaPixel(sourcePixel, format).value();
        const auto expected = rgbaPixel(expectedPixel, format).value();
        const int pixelBytes = pixelByteCount(format);
        const int tileBytes = RgbaTile::byteCount(format);
        if (!implicit) {
            input.layers[0].tiles = {{{0, 0}, destinationPixel.repeated(tileBytes / pixelBytes), format}};
            input.layers[1].tiles = {{{0, 0}, sourcePixel.repeated(tileBytes / pixelBytes), format}};
        }
        if (extraLayer) {
            RasterLayer transparent{QStringLiteral("Transparent"), true, {}, {}, {1, 257, 65534, 0}};
            transparent.parentUuid = group;
            if (!implicit) transparent.tiles = {{{0, 0}, rgbaBytes(transparent.defaultPixel, format).repeated(tileBytes / pixelBytes), format}};
            input.layers.insert(2, transparent);
        }
        OperationDocument document(input);
        // Uniform defaults and materialized pixels share both the binary32
        // multiply-add rounding and the subsequent nearest-even byte conversion.
        QCOMPARE(document.snapshot()->projectionDefaultPixels().value(input.layers.size()), expected);
        TileRenderer renderer(m_device);
        const auto raw = render(renderer, document.snapshot());
        QVERIFY(raw);
        const auto projected = project(renderer, raw, {0, 0, 64, 48});
        QVERIFY(projected && projected->tiles().size() == 1);
        const auto actual = readBuffer(projected->tiles().first().pixels);
        QCOMPARE(actual, expectedPixel.repeated(tileBytes / pixelBytes));
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!directory.isEmpty()) {
            QVERIFY(QDir().mkpath(directory));
            QFile artifact(QDir(directory).filePath(QStringLiteral("normal-rounding-%1.rgba")
                .arg(QString::fromLatin1(QTest::currentDataTag()))));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(actual), actual.size());
        }
    }
}

void KisGpuTileRendererTest::explicitPassThroughReadAfterParentKeepsSubtreePixels()
{
    RasterImage input;
    input.size = {4, 4};
    const QUuid inner(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const QUuid outer(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
    input.layers = {{QStringLiteral("First"), true, {},
                     {{{0, 0}, QByteArray::fromHex("c9295d89").repeated(RgbaTile::ByteCount / 4)}}},
                    {QStringLiteral("Second"), true, {},
                     {{{0, 0}, QByteArray::fromHex("11d34dbd").repeated(RgbaTile::ByteCount / 4)}}},
                    {QStringLiteral("Pass-through"), true, inner, {}},
                    {QStringLiteral("Isolated parent"), true, outer, {}}};
    input.layers[0].parentUuid = input.layers[1].parentUuid = inner;
    input.layers[0].opacity = 153;
    input.layers[1].opacity = 211;
    input.layers[2].kind = input.layers[3].kind = LayerKind::Group;
    input.layers[2].parentUuid = outer;
    input.layers[2].passThrough = true;
    input.layers[2].opacity = 137;
    input.layers[2].projectionChannelMask = 9;
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto parent = renderer.recordRegion(commands, document.snapshot(), {0, 0, 4, 4}, {4});
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY2(parent.code == RenderOutcome::Recorded, qPrintable(parent.diagnostic));
    const auto retained = readTile(parent.version, 4, {});
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto both = renderer.recordRegion(commands, document.snapshot(), {0, 0, 4, 4}, {4, 3}, parent.version);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY2(both.code == RenderOutcome::Recorded, qPrintable(both.diagnostic));
    QCOMPARE(readTile(both.version, 3, {}).first(4), QByteArray::fromHex("30b650bc"));
    QCOMPARE(readTile(parent.version, 4, {}), retained);
}

void KisGpuTileRendererTest::passThroughHierarchyMatchesNativeRootPixels_data()
{
    QTest::addColumn<bool>("passThrough");
    QTest::addColumn<int>("nesting");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<int>("mask");
    QTest::addColumn<bool>("uniform");
    QTest::addColumn<QByteArray>("expected");
    // Native RGBA8/sRGB root references at 6e856978f0. Four-pixel square,
    // fixed lower/first/second RGBA:255997ad/c9295d89/11d34dbd;
    // child opacities153/211, inner opacity171 and component mask6.
    struct Case { bool passThrough; int nesting, opacity, mask; const char *rgba; };
    const Case cases[] = {
        {false, 0, 255, 15, "2ea45ee9"},
        {false, 0, 255, 9, "2e5997e9"},
        {false, 0, 137, 15, "2a8774cd"},
        {false, 0, 137, 9, "2a5997cd"},
        {false, 0, 0, 15, "255997ad"},
        {false, 0, 0, 9, "255997ad"},
        {false, 1, 255, 15, "68627cc7"},
        {false, 1, 255, 9, "685997c7"},
        {false, 1, 137, 15, "4c5e88bb"},
        {false, 1, 137, 9, "4c5997bb"},
        {false, 1, 0, 15, "255997ad"},
        {false, 1, 0, 9, "255997ad"},
        {false, 2, 255, 15, "68627cc7"},
        {false, 2, 255, 9, "685997c7"},
        {false, 2, 137, 15, "4c5e88bb"},
        {false, 2, 137, 9, "4c5997bb"},
        {false, 2, 0, 15, "255997ad"},
        {false, 2, 0, 9, "255997ad"},
        {true, 0, 255, 15, "2ea45de9"},
        {true, 0, 255, 9, "2e5997e9"},
        {true, 0, 137, 15, "348371d1"},
        {true, 0, 137, 9, "345997d1"},
        {true, 0, 0, 15, "255997ad"},
        {true, 0, 0, 9, "255997ad"},
        {true, 1, 255, 15, "697f6ac7"},
        {true, 1, 255, 9, "695997c7"},
        {true, 1, 137, 15, "4c6b7cbb"},
        {true, 1, 137, 9, "4c5997bb"},
        {true, 1, 0, 15, "255997ad"},
        {true, 1, 0, 9, "255997ad"},
        {true, 2, 255, 15, "697f6bc7"},
        {true, 2, 255, 9, "698b79c7"},
        {true, 2, 137, 15, "4c6b7cbb"},
        {true, 2, 137, 9, "4c7487bb"},
        {true, 2, 0, 15, "255997ad"},
        {true, 2, 0, 9, "255997ad"},
    };
    for (const auto &c : cases) for (bool uniform : {false, true}) {
        const QByteArray name = QByteArray::number(c.passThrough) + '-' + QByteArray::number(c.nesting) + '-' +
            QByteArray::number(c.opacity) + '-' + QByteArray::number(c.mask) + (uniform ? "-default" : "-tile");
        QTest::newRow(name.constData()) << c.passThrough << c.nesting << c.opacity << c.mask << uniform
                                      << QByteArray::fromHex(c.rgba);
    }
}

void KisGpuTileRendererTest::passThroughHierarchyMatchesNativeRootPixels()
{
    QFETCH(bool, passThrough);
    QFETCH(int, nesting);
    QFETCH(int, opacity);
    QFETCH(int, mask);
    QFETCH(bool, uniform);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {4, 4};
    const QUuid outer(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const QUuid inner(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
    input.layers = {{QStringLiteral("Bottom"), true, {}, {}},
                    {QStringLiteral("First"), true, {}, {}},
                    {QStringLiteral("Second"), true, {}, {}},
                    {QStringLiteral("Outer"), true, outer, {}}};
    input.layers[1].opacity = 153;
    input.layers[1].parentUuid = outer;
    input.layers[2].opacity = 211;
    input.layers[2].parentUuid = nesting ? inner : outer;
    input.layers[3].kind = LayerKind::Group;
    input.layers[3].passThrough = passThrough;
    input.layers[3].opacity = opacity;
    input.layers[3].projectionChannelMask = mask;
    if (nesting) {
        RasterLayer group{QStringLiteral("Inner"), true, inner, {}};
        group.kind = LayerKind::Group;
        group.parentUuid = outer;
        group.passThrough = nesting == 2;
        group.opacity = 171;
        group.projectionChannelMask = 6;
        input.layers.append(group);
    }
    const QByteArray colors[] = {QByteArray::fromHex("255997ad"), QByteArray::fromHex("c9295d89"),
                                 QByteArray::fromHex("11d34dbd")};
    for (int index = 0; index < 3; ++index) {
        if (uniform) std::copy_n(reinterpret_cast<const quint8 *>(colors[index].constData()), 4, input.layers[index].defaultPixel.begin());
        else input.layers[index].tiles = {{{0, 0}, colors[index].repeated(RgbaTile::ByteCount / 4)}};
    }
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    const auto pixels = render(renderer, document.snapshot());
    QVERIFY(pixels);
    const auto projection = project(renderer, pixels, {0, 0, 4, 4});
    QVERIFY(projection);
    QCOMPARE(readBuffer(projection->tiles().first().pixels).first(4), expected);
    QCOMPARE(document.setLayerVisible(4, false), Result::Success);
    const auto hidden = project(renderer, render(renderer, document.snapshot(), pixels), {0, 0, 4, 4}, projection);
    QVERIFY(hidden);
    QCOMPARE(readBuffer(hidden->tiles().first().pixels).first(4), colors[0]);
    QCOMPARE(readBuffer(projection->tiles().first().pixels).first(4), expected);
}

void KisGpuTileRendererTest::separableLayerProjectionMatchesNativePixels_data()
{
    QTest::addColumn<int>("destinationAlpha");
    QTest::addColumn<int>("sourceAlpha");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<int>("mask");
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<int>("blendMode");
    for (int mode : {1, 2, 3, 4, 5, 6}) {
        const auto references = mode == 1 ? LibrePaint::Image::Gpu::Test::multiplyProjectionReferences() :
            mode == 2 ? LibrePaint::Image::Gpu::Test::screenProjectionReferences() :
            mode == 3 ? LibrePaint::Image::Gpu::Test::darkenProjectionReferences() :
            mode == 4 ? LibrePaint::Image::Gpu::Test::lightenProjectionReferences() :
            LibrePaint::Image::Gpu::Test::additionProjectionReferences();
        qsizetype row = 0;
        for (int destinationAlpha : {0, 173, 255}) for (int sourceAlpha : {0, 137, 255})
        for (int opacity : {137, 255}) for (int mask : {15, 9, 6, 0}) {
            for (bool implicit : {false, true}) {
                const QByteArray name = QByteArray::number(mode) + '-' + QByteArray::number(destinationAlpha) + '-' + QByteArray::number(sourceAlpha) + '-' +
                    QByteArray::number(opacity) + '-' + QByteArray::number(mask) + (implicit ? "-default" : "-stored");
                QTest::newRow(name.constData()) << destinationAlpha << sourceAlpha << opacity << mask << implicit << references.mid(row * 4, 4) << mode;
            }
            ++row;
        }
    }
}

void KisGpuTileRendererTest::separableLayerProjectionMatchesNativePixels()
{
    QFETCH(int, blendMode);
    QFETCH(int, destinationAlpha);
    QFETCH(int, sourceAlpha);
    QFETCH(int, opacity);
    QFETCH(int, mask);
    QFETCH(bool, implicit);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {256, 256};
    const RgbaPixel colors[] = {{37, 89, 151, quint8(destinationAlpha)},
        blendMode >= int(LayerBlendMode::Addition) ? RgbaPixel{208, 224, 240, quint8(sourceAlpha)} :
        RgbaPixel{201, 41, 93, quint8(sourceAlpha)}};
    for (int index = 0; index < 2; ++index) {
        RasterLayer layer;
        layer.opacity = index == 0 ? 255 : opacity;
        layer.blendMode = LayerBlendMode::Normal;
        layer.projectionChannelMask = index == 0 ? 15 : mask;
        if (implicit) layer.defaultPixel = colors[index];
        else layer.tiles = {{{0, 0}, rgbaBytes(colors[index], PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4)}};
        input.layers.append(layer);
    }
    OperationDocument document(input);
    QCOMPARE(document.writeLayerBlendMode(2, LayerBlendMode(blendMode)), Result::Success);
    TileRenderer renderer(m_device);
    const auto raw = render(renderer, document.snapshot());
    QVERIFY(raw);
    const auto projected = project(renderer, raw, {0, 0, 256, 256});
    QVERIFY(projected);
    QCOMPARE(readBuffer(projected->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    QCOMPARE(document.setLayerBlendMode(2, LayerBlendMode::Normal), Result::Success);
    const auto normal = project(renderer, render(renderer, document.snapshot(), raw), {0, 0, 256, 256}, projected);
    QVERIFY(normal);
    QCOMPARE(document.undo(), Result::Success);
    const auto restored = project(renderer, render(renderer, document.snapshot(), raw), {0, 0, 256, 256}, normal);
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    QCOMPARE(readBuffer(projected->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
}

void KisGpuTileRendererTest::channelCopyGroupsKeepHiddenRgbAndNormalizeRoot()
{
    for (const bool implicit : {false, true}) {
        for (const auto mode : {LayerBlendMode::CopyRed, LayerBlendMode::CopyGreen, LayerBlendMode::CopyBlue}) {
            RasterImage input;
            input.size = {256, 256};
            RasterLayer destination, source, group;
            group.uuid = QUuid(QStringLiteral("{01020304-1122-3344-5566-778899aabbcc}"));
            group.kind = LayerKind::Group;
            destination.parentUuid = source.parentUuid = group.uuid;
            source.blendMode = mode;
            const RgbaPixel color{17, 63, 129, 255};
            if (implicit) source.defaultPixel = color;
            else source.tiles = {{{0, 0}, rgbaBytes(color, PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4)}};
            input.layers = {destination, source, group};
            OperationDocument document(input);
            TileRenderer renderer(m_device);
            const auto raw = render(renderer, document.snapshot());
            QVERIFY(raw);
            RgbaPixel hidden{};
            const int channel = int(mode) - int(LayerBlendMode::CopyRed);
            hidden[channel] = color[channel];
            const auto projected = project(renderer, raw, {0, 0, 256, 256});
            QVERIFY(projected);
            const auto hiddenBytes = rgbaBytes(hidden, PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4);
            QCOMPARE(readBuffer(projected->tiles().first().pixels), hiddenBytes);
            // An empty sibling changes the root to normal composition. It
            // ignores the group's zero-alpha colors and keeps the pinned view.
            QVERIFY(document.addLayer(QStringLiteral("Empty sibling")));
            const auto changed = render(renderer, document.snapshot(), raw);
            QVERIFY(changed);
            const auto normalized = project(renderer, changed, {0, 0, 256, 256}, projected);
            QVERIFY(normalized);
            QCOMPARE(readBuffer(normalized->tiles().first().pixels), QByteArray(RgbaTile::ByteCount, '\0'));
            QCOMPARE(readBuffer(projected->tiles().first().pixels), hiddenBytes);
        }
    }
}

void KisGpuTileRendererTest::minimumNonlinearPaintOpacityKeepsNativeQuantization()
{
    // KisPainter accepts the stored real opacity as a float. Immediately below
    // the first byte boundary it still executes with an opacity of one byte.
    const qreal opacity = std::nextafter(0.5 / 255.0, 0.0);
    for (const bool fill : {false, true}) {
        RasterImage input;
        input.size = {64, 64};
        RasterLayer paint;
        paint.tiles = {{{0, 0}, QByteArray::fromHex("000000ff").repeated(RgbaTile::ByteCount / 4)}};
        input.layers = {paint};
        OperationDocument document(input);
        const auto layer = document.activeLayer();
        if (fill) {
            QCOMPARE(document.fillLayer(layer, {255, 255, 255}, opacity, false, false,
                                       LayerBlendMode::EasyDodge), Result::Success);
        } else {
            Brush brush;
            brush.rgba = {255, 255, 255, 255};
            brush.opacity = opacity;
            brush.diameter = 4;
            brush.blendMode = LayerBlendMode::EasyDodge;
            const auto stroke = document.beginStroke(layer, brush, 1);
            QVERIFY(!stroke.isNull());
            QCOMPARE(document.appendSamples(stroke, {{{8, 8}, 1}}), Result::Success);
            QCOMPARE(document.commitStroke(stroke), Result::Success);
        }
        TileRenderer renderer(m_device);
        const auto result = render(renderer, document.snapshot());
        QVERIFY(result);
        // Native EasyDodge, byte opacity 1, source BGRA ffffffff and
        // destination BGRA 000000ff: the full channel range exposes one byte.
        QCOMPARE(readTile(result, layer, {0, 0}).mid((8 * TileRenderer::TileSize + 8) * 4, 4),
                 QByteArray::fromHex("010101ff"));
    }
}

void KisGpuTileRendererTest::wordBlendOperationsFitWithoutByteLookupResources()
{
    // Word geometric/modulo operations compute their colors analytically. A
    // bounded document must leave capacity for retained pixels and new edits.
    const auto format = PixelFormat::Rgba16;
    const auto tileBytes = RgbaTile::byteCount(format);
    const auto pixels = rgbaBytes({16384, 16384, 16384, 65535}, format).repeated(tileBytes / 8);
    for (const auto mode : {LayerBlendMode::GeometricMean, LayerBlendMode::Modulo}) {
        for (const bool painting : {false, true}) {
            RasterImage input;
            input.size = {256, 256};
            input.format = format;
            RasterLayer lower, upper;
            lower.tiles = upper.tiles = {{{0, 0}, pixels, format}};
            input.layers = {lower, upper};
            OperationDocument document(input);
            TileRenderer renderer(m_device, 6 * tileBytes);
            const auto original = document.snapshot();
            const auto raw = render(renderer, original);
            QVERIFY(raw);
            const auto projected = project(renderer, raw, {0, 0, 256, 256});
            QVERIFY(projected);
            const auto layer = original->layers.last().id;
            if (painting) {
                Brush brush;
                brush.rgba = {16384, 16384, 16384, 65535};
                brush.diameter = 8;
                brush.blendMode = mode;
                const auto stroke = document.beginStroke(layer, brush, 1);
                QVERIFY(!stroke.isNull());
                QCOMPARE(document.appendSamples(stroke, {{{8, 8}, 1}}), Result::Success);
                QCOMPARE(document.commitStroke(stroke), Result::Success);
            } else {
                QCOMPARE(document.setLayerBlendMode(layer, mode), Result::Success);
            }
            const auto changed = render(renderer, document.snapshot(), raw);
            QVERIFY(changed);
            const auto result = project(renderer, changed, {0, 0, 256, 256}, projected);
            QVERIFY(result);
            QCOMPARE(readBuffer(result->tiles().first().pixels), pixels);
            QCOMPARE(readBuffer(projected->tiles().first().pixels), pixels);
            QVERIFY(renderer.residentBytes() <= 6 * tileBytes);
        }
    }
}

void KisGpuTileRendererTest::coupledProjectionRetainsOtherBlendOperations()
{
    for (const auto format : {PixelFormat::Rgba8, PixelFormat::Rgba16}) {
        const int unit = format == PixelFormat::Rgba16 ? 65535 : 255;
        const int tileBytes = RgbaTile::byteCount(format);
        const int pixelBytes = pixelByteCount(format);
        const auto tile = [&](RgbaPixel pixel) { return rgbaBytes(pixel, format).repeated(tileBytes / pixelBytes); };
        RasterImage input;
        input.size = {256, 256};
        input.format = format;
        RasterLayer base, color, top;
        base.tiles = {{{0, 0}, tile({quint16(unit / 4), quint16(unit / 4), quint16(unit / 4), quint16(unit)}), format}};
        color.tiles = {{{0, 0}, tile({quint16(unit), 0, 0, quint16(unit)}), format}};
        color.blendMode = LayerBlendMode::ColorHsy;
        const auto green = tile({0, quint16(unit), 0, quint16(unit)});
        top.tiles = {{{0, 0}, green, format}};
        input.layers = {base, color, top};
        OperationDocument document(input);
        TileRenderer renderer(m_device);
        const auto original = render(renderer, document.snapshot());
        QVERIFY(original);
        const auto first = project(renderer, original, {0, 0, 256, 256});
        QVERIFY(first);
        QCOMPARE(readBuffer(first->tiles().first().pixels), green);
        // The same coupled color in a pair with Multiply needs the full word
        // compositor. Reusing a color/Normal program would return green here.
        QCOMPARE(document.setLayerBlendMode(document.snapshot()->layers.last().id, LayerBlendMode::Multiply), Result::Success);
        const auto changed = render(renderer, document.snapshot(), original);
        QVERIFY(changed);
        const auto second = project(renderer, changed, {0, 0, 256, 256}, first);
        QVERIFY(second);
        QCOMPARE(readBuffer(second->tiles().first().pixels), tile({0, 0, 0, quint16(unit)}));
        QCOMPARE(readBuffer(first->tiles().first().pixels), green);
        QCOMPARE(document.undo(), Result::Success);
        const auto restored = render(renderer, document.snapshot(), changed);
        QVERIFY(restored);
        const auto third = project(renderer, restored, {0, 0, 256, 256}, second);
        QVERIFY(third);
        QCOMPARE(readBuffer(third->tiles().first().pixels), green);
        QCOMPARE(document.setLayerBlendMode(document.snapshot()->layers[1].id, LayerBlendMode::Normal), Result::Success);
        QCOMPARE(document.setLayerBlendMode(document.snapshot()->layers.last().id, LayerBlendMode::Multiply), Result::Success);
        const auto multiplied = render(renderer, document.snapshot(), restored);
        QVERIFY(multiplied);
        const auto fourth = project(renderer, multiplied, {0, 0, 256, 256}, third);
        QVERIFY(fourth);
        const auto black = tile({0, 0, 0, quint16(unit)});
        QCOMPARE(readBuffer(fourth->tiles().first().pixels), black);
        QCOMPARE(document.setLayerBlendMode(document.snapshot()->layers.last().id, LayerBlendMode::Screen), Result::Success);
        const auto screened = render(renderer, document.snapshot(), multiplied);
        QVERIFY(screened);
        const auto fifth = project(renderer, screened, {0, 0, 256, 256}, fourth);
        QVERIFY(fifth);
        QCOMPARE(readBuffer(fifth->tiles().first().pixels), tile({quint16(unit), quint16(unit), 0, quint16(unit)}));
        QCOMPARE(readBuffer(fourth->tiles().first().pixels), black);
    }
}

void KisGpuTileRendererTest::transparentSourcesReplaceProjectionAlpha_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<bool>("paired");
    QTest::addColumn<bool>("group");
    QTest::addColumn<bool>("locked");
    for (const auto mode : {LayerBlendMode::DestinationIn, LayerBlendMode::DestinationAtop})
    for (bool implicit : {false, true}) for (bool paired : {false, true})
    for (bool group : {false, true}) for (bool locked : {false, true}) {
        const QByteArray name = QByteArray::number(int(mode)) + '-' + QByteArray::number(implicit) + '-' +
            QByteArray::number(paired) + '-' + QByteArray::number(group) + '-' + QByteArray::number(locked);
        QTest::newRow(name.constData()) << int(mode) << implicit << paired << group << locked;
    }
}

void KisGpuTileRendererTest::transparentSourcesReplaceProjectionAlpha()
{
    QFETCH(int, mode);
    QFETCH(bool, implicit);
    QFETCH(bool, paired);
    QFETCH(bool, group);
    QFETCH(bool, locked);
    RasterImage input;
    input.size = {256, 256};
    if (paired) {
        RasterLayer empty;
        empty.tiles = {{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')}};
        input.layers.append(empty);
    }
    const RgbaPixel color{37, 89, 151, 173};
    RasterLayer lower;
    if (implicit) lower.defaultPixel = color;
    else lower.tiles = {{{0, 0}, rgbaBytes(color, PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4)}};
    input.layers.append(lower);
    RasterLayer source;
    source.kind = group ? LayerKind::Group : LayerKind::Paint;
    source.blendMode = LayerBlendMode(mode);
    source.projectionChannelMask = locked ? 7 : 15;
    input.layers.append(source);
    OperationDocument document(input);
    const auto top = document.snapshot()->layers.last().id;
    TileRenderer renderer(m_device);
    const auto raw = render(renderer, document.snapshot());
    QVERIFY(raw);
    const auto projected = project(renderer, raw, {0, 0, 256, 256});
    QVERIFY(projected);
    const auto expected = rgbaBytes({37, 89, 151, quint16(locked ? 173 : 0)}, PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(readBuffer(projected->tiles().first().pixels), expected);
    QCOMPARE(document.setLayerBlendMode(top, LayerBlendMode::Normal), Result::Success);
    const auto normal = project(renderer, render(renderer, document.snapshot(), raw), {0, 0, 256, 256}, projected);
    QVERIFY(normal);
    QCOMPARE(readBuffer(normal->tiles().first().pixels), rgbaBytes(color, PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4));
    QCOMPARE(document.undo(), Result::Success);
    const auto restored = project(renderer, render(renderer, document.snapshot(), raw), {0, 0, 256, 256}, normal);
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels), expected);
    QCOMPARE(readBuffer(projected->tiles().first().pixels), expected);
}

void KisGpuTileRendererTest::threeLayerProjectionFitsOneOutputTile_data()
{
    QTest::addColumn<int>("bottomAlpha");
    QTest::addColumn<int>("middleMask");
    QTest::addColumn<int>("topMask");
    QTest::addColumn<QByteArray>("expected");
    // RGBA8/sRGB CPU normal-composition references from 6e856978f0.
    // Three 256x256 uniform layers, opacity 255/137/211, bottom-to-top.
    QTest::newRow("transparent-normal") << 0 << 15 << 15 << QByteArray::fromHex("2db94fb9");
    QTest::newRow("transparent-middle-channels") << 0 << 9 << 15 << QByteArray::fromHex("2db241b9");
    QTest::newRow("transparent-inherit-alpha") << 0 << 15 << 6 << QByteArray::fromHex("c991534a");
    QTest::newRow("transparent-masked-bottom") << 0 << 0 << 9 << QByteArray::fromHex("1100009c");
    QTest::newRow("normal") << 173 << 15 << 15 << QByteArray::fromHex("2ca55ee9");
    QTest::newRow("middle-channels") << 173 << 9 << 15 << QByteArray::fromHex("2cab65e9");
    QTest::newRow("inherit-alpha") << 173 << 15 << 6 << QByteArray::fromHex("629d61c5");
    QTest::newRow("masked-bottom") << 173 << 0 << 9 << QByteArray::fromHex("175997df");
}

void KisGpuTileRendererTest::threeLayerProjectionFitsOneOutputTile()
{
    QFETCH(int, bottomAlpha);
    QFETCH(int, middleMask);
    QFETCH(int, topMask);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {256, 256};
    QByteArray colors[] = {QByteArray::fromHex("25599700"), QByteArray::fromHex("c9295d89"), QByteArray::fromHex("11d34dbd")};
    colors[0][3] = char(bottomAlpha);
    for (int index = 0; index < 3; ++index) {
        RasterLayer layer;
        layer.opacity = index == 0 ? 255 : index == 1 ? 137 : 211;
        layer.projectionChannelMask = index == 0 ? 15 : index == 1 ? middleMask : topMask;
        layer.tiles = {{{0, 0}, colors[index].repeated(RgbaTile::ByteCount / 4)}};
        input.layers.append(layer);
    }
    OperationDocument document(input);
    TileRenderer reference(m_device);
    const auto full = project(reference, render(reference, document.snapshot()), {0, 0, 256, 256});
    QVERIFY(full);
    QCOMPARE(readBuffer(full->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    TileRenderer limited(m_device, 4 * TileRenderer::TileBytes);
    const auto layers = render(limited, document.snapshot());
    QVERIFY(layers);
    const auto composed = project(limited, layers, {0, 0, 256, 256});
    QVERIFY2(composed, "Three source tiles must leave one output tile for projection");
    QCOMPARE(readBuffer(composed->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    for (int index = 0; index < 3; ++index)
        QCOMPARE(readTile(layers, document.snapshot()->layers[index].id, {}), input.layers[index].tiles.first().rgba);
    QVERIFY(limited.residentBytes() <= 4 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::multiPassGroupsKeepDistinctLayerParameters_data()
{
    QTest::addColumn<bool>("hiddenRgb");
    QTest::newRow("transparent-intermediate") << true;
    QTest::newRow("different-paired-attributes") << false;
}

void KisGpuTileRendererTest::multiPassGroupsKeepDistinctLayerParameters()
{
    QFETCH(bool, hiddenRgb);
    RasterImage image;
    image.size = {768, 256};
    const QUuid groupUuid(QStringLiteral("{6a58d5cc-ef89-49a1-9c67-f36721c173f6}"));
    const int count = hiddenRgb ? 5 : 8;
    const quint8 opacities[] = {255, 137, 211, 255, 61, 255, 193, 59};
    for (int index = 0; index < count; ++index) {
        RasterLayer layer;
        layer.parentUuid = groupUuid;
        layer.opacity = hiddenRgb ? (index == 1 || index == 3 ? 1 : 255) : opacities[index];
        if (!hiddenRgb && index == 4) {
            layer.projectionChannelMask = 9;
            layer.blendMode = LayerBlendMode::Multiply;
        }
        if (!hiddenRgb && index == 6) {
            layer.projectionChannelMask = 6;
            layer.blendMode = LayerBlendMode::Screen;
        }
        for (int x = 0; x < 3; ++x) {
            QByteArray color;
            for (const int value : {17 + index * 23 + x * 5, 239 - index * 19 - x * 7,
                                    41 + index * 11 + x * 13, hiddenRgb ? (index == 1 ? 1 : 0) : 47 + index * 23})
                color.append(char(value));
            layer.tiles.append({{x, 0}, color.repeated(RgbaTile::ByteCount / 4)});
        }
        image.layers.append(layer);
    }
    image.layers.append({QStringLiteral("Multi-pass group"), true, groupUuid, {}});
    image.layers.last().kind = LayerKind::Group;
    const LayerId group = count + 1;
    // Uniform group defaults and materialized tiles have the same RGBA8 contract.
    const auto expected = [&](int x, bool changed) {
        auto uniform = image;
        for (int index = 0; index < count; ++index) {
            auto &layer = uniform.layers[index];
            const auto pixel = layer.tiles[x].rgba.first(4);
            for (int channel = 0; channel < 4; ++channel) layer.defaultPixel[channel] = quint8(pixel[channel]);
            layer.tiles.clear();
        }
        if (changed) uniform.layers[1].opacity = 255;
        const OperationDocument reference(uniform);
        const auto pixel = reference.snapshot()->projectionDefaultPixels().value(group);
        return rgbaBytes(pixel, PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4);
    };
    OperationDocument document(image);
    TileRenderer renderer(m_device);
    const auto first = render(renderer, document.snapshot());
    QVERIFY(first);
    for (int x = 0; x < 3; ++x) QCOMPARE(readTile(first, group, {x, 0}), expected(x, false));
    QCOMPARE(document.setLayerOpacity(2, 255), Result::Success);
    const auto second = render(renderer, document.snapshot(), first);
    QVERIFY(second);
    for (int x = 0; x < 3; ++x) {
        QCOMPARE(readTile(second, group, {x, 0}), expected(x, true));
        QCOMPARE(readTile(first, group, {x, 0}), expected(x, false));
    }
}

void KisGpuTileRendererTest::previewProjectionTracksBackgroundPropertiesAndRetainedPixels()
{
    RasterImage input;
    input.size = {64, 64};
    for (const auto &rgba : {"255997ad", "c9295d89", "11d34dbd"}) {
        RasterLayer layer;
        layer.tiles = {{{0, 0}, QByteArray::fromHex(rgba).repeated(RgbaTile::ByteCount / 4)}};
        input.layers.append(layer);
    }
    input.activeLayer = 2;
    OperationDocument document(input);
    Brush brush;
    brush.rgba = {73, 149, 231, 128};
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 23);
    TileRenderer renderer(m_device);
    RenderedSnapshot rendered;
    ProjectedSnapshot projected, retained;
    QByteArray retainedPixels;
    for (int step = 0; step < 5; ++step) {
        QCOMPARE(document.appendSamples(stroke, {{{16.0 + step * 4, 24}, 1}}), Result::Success);
        if (step == 2) QCOMPARE(document.writeLayerOpacity(document.snapshot()->layers.first().id, 137), Result::Success);
        if (step == 3) QCOMPARE(document.writeLayerProjectionMask(document.snapshot()->layers[1].id, 9), Result::Success);
        rendered = render(renderer, document.preview(), rendered);
        QVERIFY(rendered);
        projected = project(renderer, rendered, {0, 0, 64, 64}, projected);
        QVERIFY(projected);
        const auto actual = readBuffer(projected->tiles().first().pixels);
        TileRenderer independent(m_device);
        const auto fresh = project(independent, render(independent, document.preview()), {0, 0, 64, 64});
        QVERIFY(fresh);
        QCOMPARE(actual, readBuffer(fresh->tiles().first().pixels));
        if (!retained) {
            retained = projected;
            retainedPixels = actual;
        }
        QCOMPARE(readBuffer(retained->tiles().first().pixels), retainedPixels);
    }
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto committed = project(renderer, render(renderer, document.snapshot(), rendered), {0, 0, 64, 64}, projected);
    QVERIFY(committed);
    QCOMPARE(readBuffer(committed->tiles().first().pixels), readBuffer(projected->tiles().first().pixels));
}

void KisGpuTileRendererTest::switchingDocumentsReleasesPreviewDerivedPixelsAtCapacity()
{
    const auto raster = [](int count) {
        RasterImage image;
        image.size = {64, 64};
        image.activeLayer = count - 1;
        for (int index = 0; index < count; ++index) {
            RasterLayer layer;
            layer.tiles = {{{0, 0}, QByteArray::fromHex("255997ad").repeated(RgbaTile::ByteCount / 4)}};
            image.layers.append(layer);
        }
        return image;
    };
    OperationDocument first(raster(3));
    Brush brush;
    const auto stroke = first.beginStroke(first.activeLayer(), brush, 29);
    QCOMPARE(first.appendSamples(stroke, {{{512, 512}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 10 * TileRenderer::TileBytes);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    QList<LayerId> layers;
    for (const auto &layer : first.preview()->layers) layers.append(layer.id);
    const auto preview = renderer.recordRegion(commands, first.preview(), {0, 0, 64, 64}, layers);
    QCOMPARE(preview.code, RenderOutcome::Recorded);
    const auto composed = renderer.compose(commands, preview.version, {0, 0, 64, 64});
    QCOMPARE(composed.code, RenderOutcome::Recorded);
    QVERIFY(m_device->submitCommands(true));
    const auto retainedPixels = readBuffer(composed.version->tiles().first().pixels);
    OperationDocument second(raster(4));
    const auto rendered = render(renderer, second.snapshot());
    QVERIFY(rendered);
    const auto switched = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY2(switched, "Preview-derived pixels must yield capacity to another document");
    QCOMPARE(readBuffer(composed.version->tiles().first().pixels), retainedPixels);
    QVERIFY(renderer.residentBytes() <= 10 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::partialLayerOpacityComposesOverTransparency_data()
{
    QTest::addColumn<QByteArray>("pixel");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("half-opacity") << QByteArray::fromHex("0000ffff") << 128 << false << QByteArray::fromHex("0000ff80");
    QTest::newRow("half-alpha-half-opacity") << QByteArray::fromHex("0000ff80") << 128 << false << QByteArray::fromHex("0000ff40");
    QTest::newRow("hidden-rgb-partial-opacity") << QByteArray::fromHex("0000ff00") << 128 << false << QByteArray::fromHex("00000000");
    QTest::newRow("hidden-rgb-full-opacity") << QByteArray::fromHex("0000ff00") << 255 << false << QByteArray::fromHex("0000ff00");
    QTest::newRow("zero-opacity") << QByteArray::fromHex("0000ffff") << 0 << false << QByteArray::fromHex("00000000");
    QTest::newRow("implicit-background") << QByteArray::fromHex("0000ffff") << 128 << true << QByteArray::fromHex("0000ff80");
}

void KisGpuTileRendererTest::partialLayerOpacityComposesOverTransparency()
{
    QFETCH(QByteArray, pixel);
    QFETCH(int, opacity);
    QFETCH(bool, implicit);
    QFETCH(QByteArray, expected);
    RasterImage source;
    source.size = {256, 256};
    RasterLayer layer;
    layer.name = QStringLiteral("Background");
    layer.opacity = quint8(opacity);
    if (implicit) for (int i = 0; i < 4; ++i) layer.defaultPixel[i] = quint8(pixel[i]);
    else layer.tiles = {{{0, 0}, pixel.repeated(RgbaTile::ByteCount / 4)}};
    source.layers = {layer};
    OperationDocument document(source);
    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto merged = project(renderer, rendered, QRect(0, 0, 256, 256));
    QVERIFY(merged);
    if (!opacity) QVERIFY(merged->tiles().isEmpty());
    else QCOMPARE(readBuffer(merged->tiles().first().pixels).left(4), expected);
}

void KisGpuTileRendererTest::layerOpacityChangesProjectionWithoutChangingSourcePixelsOrRetainedVersions()
{
    RasterImage source;
    source.size = {256, 256};
    source.layers = {
        {QStringLiteral("Red"), true, {}, {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Blue"), true, {}, {{{0, 0}, QByteArray::fromHex("0000ffff").repeated(RgbaTile::ByteCount / 4)}}}
    };
    source.layers[1].opacity = 128;
    OperationDocument document(source);
    const auto blue = document.snapshot()->layers[1].id;
    TileRenderer renderer(m_device);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    const auto merged = project(renderer, original, QRect(0, 0, 256, 256));
    QVERIFY(merged);
    QCOMPARE(readBuffer(merged->tiles().first().pixels).left(4), QByteArray::fromHex("7f0080ff"));
    QCOMPARE(readTile(original, blue, {0, 0}).left(4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document.setLayerOpacity(blue, 64), Result::Success);
    const auto changed = render(renderer, document.snapshot(), original);
    QVERIFY(changed);
    const auto updated = project(renderer, changed, QRect(0, 0, 256, 256), merged);
    QVERIFY(updated);
    QCOMPARE(readBuffer(updated->tiles().first().pixels).left(4), QByteArray::fromHex("bf0040ff"));
    QCOMPARE(readBuffer(merged->tiles().first().pixels).left(4), QByteArray::fromHex("7f0080ff"));
    QCOMPARE(readTile(changed, blue, {0, 0}).left(4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), changed);
    const auto restored = project(renderer, undone, QRect(0, 0, 256, 256), updated);
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels).left(4), QByteArray::fromHex("7f0080ff"));
}

void KisGpuTileRendererTest::projectionChannelsInvalidateGroupReuseAndKeepRetainedPixels_data()
{
    QTest::addColumn<bool>("uniform");
    QTest::newRow("stored-tiles") << false;
    QTest::newRow("uniform-defaults") << true;
}

void KisGpuTileRendererTest::projectionChannelsInvalidateGroupReuseAndKeepRetainedPixels()
{
    QFETCH(bool, uniform);
    RasterImage input;
    input.size = {256, 256};
    const QUuid parent(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    input.layers = {{QStringLiteral("Lower"), true, QUuid::createUuid(), {}},
                    {QStringLiteral("Upper"), true, QUuid::createUuid(), {}},
                    {QStringLiteral("Group"), true, parent, {}}};
    input.layers[2].kind = LayerKind::Group;
    input.layers[1].opacity = 128;
    const QByteArray pixels[] = {QByteArray::fromHex("996633e2"), QByteArray::fromHex("56341286")};
    for (int index = 0; index < 2; ++index) {
        auto &layer = input.layers[index];
        layer.parentUuid = parent;
        if (uniform) {
            for (int channel = 0; channel < 4; ++channel) layer.defaultPixel[channel] = quint8(pixels[index][channel]);
        } else layer.tiles = {{{0, 0}, pixels[index].repeated(RgbaTile::ByteCount / 4)}};
    }
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    auto current = render(renderer, document.snapshot());
    QVERIFY(current);
    auto projection = project(renderer, current, {0, 0, 256, 256});
    QVERIFY(projection);
    const auto fixed = projection;
    QCOMPARE(readBuffer(fixed->tiles().first().pixels), QByteArray::fromHex("865829ea").repeated(RgbaTile::ByteCount / 4));
    for (const auto mask : {quint8(7), quint8(9)}) {
        QCOMPARE(document.setLayerProjectionMask(2, mask), Result::Success);
        auto changed = render(renderer, document.snapshot(), current);
        QVERIFY(changed);
        auto updated = project(renderer, changed, {0, 0, 256, 256}, projection);
        QVERIFY(updated);
        const auto expected = QByteArray::fromHex(mask == 7 ? "87592ae2" : "866633ea");
        QCOMPARE(readBuffer(updated->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
        QCOMPARE(readBuffer(fixed->tiles().first().pixels), QByteArray::fromHex("865829ea").repeated(RgbaTile::ByteCount / 4));
        current = changed;
        projection = updated;
    }
}

void KisGpuTileRendererTest::appendedInputPreservesPixelsOutsideItsChangedTiles()
{
    OperationDocument document({512, 256});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 12;
    const auto stroke = document.beginStroke(layer, brush, 123);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 0.5}, {{280, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes), oracle(m_device);
    auto current = render(renderer, document.preview());
    QVERIFY(current);
    QCOMPARE(current->tiles(layer).size(), 2);
    const auto unchanged = readTile(current, layer, {0, 0});
    const auto previous = readTile(current, layer, {1, 0});
    QCOMPARE(document.appendSamples(stroke, {{{300, 32}, 0.75}}), Result::Success);
    auto updated = render(renderer, document.preview(), current);
    QVERIFY2(updated, "Extending one tile must retain the two-tile preview within a three-tile budget");
    QCOMPARE(readTile(current, layer, {0, 0}), unchanged);
    QCOMPARE(readTile(current, layer, {1, 0}), previous);
    QCOMPARE(readTile(updated, layer, {0, 0}), unchanged);
    QCOMPARE(readTile(updated, layer, {1, 0}), readTile(render(oracle, document.preview()), layer, {1, 0}));
    current = updated;
    updated.clear();
    QCOMPARE(document.appendSamples(stroke, {{{350, 32}, 0.25}}), Result::Success);
    updated = render(renderer, document.preview(), current);
    QVERIFY(updated);
    QCOMPARE(readTile(updated, layer, {0, 0}), unchanged);
    QCOMPARE(readTile(updated, layer, {1, 0}), readTile(render(oracle, document.preview()), layer, {1, 0}));
    current = updated;
    updated.clear();
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Line, {32, 180}, {100, 180}, 0}), Result::Success);
    updated = render(renderer, document.preview(), current);
    QVERIFY(updated);
    QCOMPARE(updated->tiles(layer).size(), 1);
    const auto shaped = readTile(updated, layer, {0, 0});
    QCOMPARE(shaped, readTile(render(oracle, document.preview()), layer, {0, 0}));
    current = updated;
    updated.clear();
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), current);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), shaped);
}

void KisGpuTileRendererTest::appendedInputPreservesCoverageBackgroundAndOldVersions_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("normal") << 0;
    QTest::newRow("erase") << 1;
    QTest::newRow("alpha-locked") << 2;
    QTest::newRow("protected-channels") << 3;
    QTest::newRow("mirrored-overlap") << 4;
    QTest::newRow("hidden-background-rgb") << 5;
    QTest::newRow("large-brush") << 6;
}

void KisGpuTileRendererTest::appendedInputPreservesCoverageBackgroundAndOldVersions()
{
    QFETCH(int, mode);
    const QByteArray pixel = QByteArray::fromHex(mode == 5 ? "4d2b8700" : "4d2b875f");
    const auto background = pixel.repeated(RgbaTile::ByteCount / 4);
    RasterImage input;
    input.size = {512, 256};
    input.layers = {{QStringLiteral("Background"), true, {},
                     {{{0, 0}, background}, {{1, 0}, background}}}};
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {31, 142, 207, 135};
    brush.diameter = mode == 6 ? 512 : 16.75;
    brush.spacing = 0.27;
    brush.opacity = 149.0 / 255;
    brush.erase = mode == 1;
    brush.alphaLocked = mode == 2;
    brush.channelWriteMask = mode == 3 ? 5 : 15;
    brush.mirrorHorizontal = brush.mirrorVertical = mode == 4;
    brush.mirrorCenter = {64, 32};
    const auto stroke = document.beginStroke(layer, brush, 123);
    QCOMPARE(document.appendSamples(stroke, {{{32.25, 32.25}, 0.35}}), Result::Success);
    // The 512px brush spans up to 4x3 layer tiles, including off-canvas pixels.
    constexpr int maximumPaintedTiles = 4 * 3;
    TileRenderer renderer(m_device, (3 * maximumPaintedTiles + 2) * TileRenderer::TileBytes);
    TileRenderer oracle(m_device, (maximumPaintedTiles + 2) * TileRenderer::TileBytes);
    const auto pixels = [&](const RenderedSnapshot &version) {
        QMap<QPair<int, int>, QByteArray> result;
        for (const auto &tile : version->tiles(layer))
            result.insert({tile.coordinate.x(), tile.coordinate.y()}, readTile(version, layer, tile.coordinate));
        return result;
    };
    auto current = render(renderer, document.preview());
    QVERIFY(current);
    const auto first = current;
    const auto firstPixels = pixels(first);
    const QList<InputSample> additions{{{66.75, 34.5}, 0.87}, {{42.25, 31.75}, 0.6},
                                       {{300.5, 48.75}, 0.95}, {{32.5, 31.25}, 0.55}};
    for (const auto &sample : additions) {
        QCOMPARE(document.appendSamples(stroke, {sample}), Result::Success);
        const auto next = render(renderer, document.preview(), current);
        QVERIFY(next);
        const auto rebuilt = render(oracle, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(pixels(next), pixels(rebuilt));
        QCOMPARE(pixels(first), firstPixels);
        current = next;
    }
    const auto finalPixels = pixels(current);
    if (mode == 4) QVERIFY(finalPixels.contains({-1, 0}));
    if (mode == 6) QVERIFY(finalPixels.contains({-1, -1}));
    const auto retainedInput = document.preview();
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    const auto cancelled = render(renderer, document.snapshot(), current);
    QVERIFY(cancelled);
    QCOMPARE(cancelled->tiles(layer).size(), 2);
    for (int x = 0; x < 2; ++x) QCOMPARE(readTile(cancelled, layer, {x, 0}), background);
    QCOMPARE(pixels(current), finalPixels);

    const auto rebuilt = render(oracle, retainedInput);
    QVERIFY(rebuilt);
    QCOMPARE(pixels(rebuilt), finalPixels);
}

void KisGpuTileRendererTest::regionalInputKeepsPixelsAcrossViewportAndCapacityChanges_data()
{
    QTest::addColumn<int>("maximumTiles");
    QTest::newRow("budget-16") << 16;
    QTest::newRow("budget-96") << 96;
}

void KisGpuTileRendererTest::regionalInputKeepsPixelsAcrossViewportAndCapacityChanges()
{
    QFETCH(int, maximumTiles);
    RasterImage image;
    image.size = {1024, 512};
    RasterLayer background;
    background.defaultPixel = {27, 73, 149, 180};
    image.layers = {background};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    Brush brush;
    brush.diameter = 512;
    brush.rgba = {211, 37, 83, 137};
    brush.opacity = 173.0 / 255;
    const auto stroke = document.beginStroke(layer, brush, 73);
    QVERIFY(!stroke.isNull());
    const QList<InputSample> samples = {{{360, 256}, 1}, {{260, 256}, 0.6}, {{150, 200}, 0.3},
                                       {{180, 180}, 0.4}, {{170, 220}, 0.7}};
    const QList<QRect> regions = {{0, 0, 512, 512}, {0, 0, 512, 512}, {0, 0, 512, 512},
                                 {256, 0, 512, 512}, {256, 0, 256, 512}};
    TileRenderer renderer(m_device, maximumTiles * TileRenderer::TileBytes);
    TileRenderer reference(m_device);
    const auto fallback = QByteArray::fromHex("1b4995b4").repeated(TileRenderer::TileBytes / 4);
    const auto pixels = [&](const RenderedSnapshot &version, QRect region) {
        QByteArray result;
        for (int y = region.top(); y <= region.bottom(); y += TileRenderer::TileSize) {
            for (int x = region.left(); x <= region.right(); x += TileRenderer::TileSize) {
                const auto tile = readTile(version, layer, {x / TileRenderer::TileSize, y / TileRenderer::TileSize});
                result += tile.isEmpty() ? fallback : tile;
            }
        }
        return result;
    };
    RenderedSnapshot previous, pinned;
    QByteArray pinnedPixels, observations;
    for (int step = 0; step < samples.size(); ++step) {
        QCOMPARE(document.appendSamples(stroke, {samples[step]}), Result::Success);
        const auto commands = m_device->beginCommands();
        QVERIFY(commands);
        const auto generated = renderer.recordRegion(commands, document.preview(), regions[step], {layer}, previous);
        const bool submitted = m_device->submitCommands(true);
        QCOMPARE(generated.code, RenderOutcome::Recorded);
        QVERIFY2(submitted, qPrintable(m_device->diagnostic()));
        const auto whole = render(reference, document.preview());
        QVERIFY(whole);
        const auto actual = pixels(generated.version, regions[step]);
        QCOMPARE(actual, pixels(whole, regions[step]));
        observations += actual;
        if (!pinned) {
            pinned = generated.version;
            pinnedPixels = actual;
        }
        QCOMPARE(pixels(pinned, regions.first()), pinnedPixels);
        QVERIFY(renderer.residentBytes() <= maximumTiles * TileRenderer::TileBytes);
        previous = generated.version;
    }
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    for (bool undo : {true, false}) {
        QCOMPARE(undo ? document.undo() : document.redo(), Result::Success);
        const auto commands = m_device->beginCommands();
        QVERIFY(commands);
        const auto generated = renderer.recordRegion(commands, document.snapshot(), regions.last(), {layer}, previous);
        const bool submitted = m_device->submitCommands(true);
        QCOMPARE(generated.code, RenderOutcome::Recorded);
        QVERIFY2(submitted, qPrintable(m_device->diagnostic()));
        const auto whole = render(reference, document.snapshot());
        QVERIFY(whole);
        const auto actual = pixels(generated.version, regions.last());
        QCOMPARE(actual, pixels(whole, regions.last()));
        observations += actual;
        QCOMPARE(pixels(pinned, regions.first()), pinnedPixels);
        QVERIFY(renderer.residentBytes() <= maximumTiles * TileRenderer::TileBytes);
        previous = generated.version;
    }
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("regional-stroke-append-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        // Fixed RGBA8 inputs, seed 73, five viewports, undo and redo; row-major tiles.
        QCOMPARE(artifact.write(observations), observations.size());
    }
}

void KisGpuTileRendererTest::pendingStrokeLeavesCapacityForCompositionAndCancellation()
{
    RasterImage input;
    input.size = {64, 64};
    input.activeLayer = 1;
    input.layers = {{QStringLiteral("Lower"), true, {},
                     {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
                    {QStringLiteral("Upper"), true, {}, {}}};
    OperationDocument document(input);
    Brush brush;
    brush.rgba = {0, 0, 255, 255};
    brush.opacity = 128.0 / 255;
    brush.diameter = 16;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes);
    const auto painted = render(renderer, document.preview());
    QVERIFY(painted);
    const auto projection = project(renderer, painted, {0, 0, 64, 64});
    QVERIFY2(projection, "Pending-stroke derived data must leave room for the required projection");
    const auto offset = (32 * TileRenderer::TileSize + 32) * 4;
    QCOMPARE(readBuffer(projection->tiles().first().pixels).mid(offset, 4), QByteArray::fromHex("7f0080ff"));
    QVERIFY(renderer.residentBytes() <= 3 * TileRenderer::TileBytes);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), painted);
    QVERIFY(cancelled);
    const auto restored = project(renderer, cancelled, {0, 0, 64, 64}, projection);
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels).mid(offset, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(readBuffer(projection->tiles().first().pixels).mid(offset, 4), QByteArray::fromHex("7f0080ff"));
    QVERIFY(renderer.residentBytes() <= 3 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::changingPreviewReusesItsBackgroundWithinThreeTiles()
{
    OperationDocument document({256, 256});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {20, 80, 160, 128};
    brush.diameter = 12;
    for (int i = 0; i < OperationDocument::HistoryLimit; ++i) {
        const auto id = document.beginStroke(layer, brush, i);
        QCOMPARE(document.appendSamples(id, {{{qreal(20 + i % 180), 40}, 0.75}}), Result::Success);
        QCOMPARE(document.commitStroke(id), Result::Success);
    }
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes), oracle(m_device);
    const auto background = render(renderer, document.snapshot());
    QVERIFY(background);
    const auto backgroundPixels = readTile(background, layer, {0, 0});
    const auto stroke = document.beginStroke(layer, brush, 1000);
    QCOMPARE(document.appendSamples(stroke, {{{32, 80}, 0.5}}), Result::Success);
    auto preview = render(renderer, document.preview(), background);
    QVERIFY(preview);
    const auto firstPixels = readTile(preview, layer, {0, 0});
    QCOMPARE(document.appendSamples(stroke, {{{180, 120}, 1}}), Result::Success);
    auto changed = render(renderer, document.preview(), preview);
    QVERIFY2(changed, "A preview update must preserve the background and previous view within three tiles");
    QCOMPARE(readTile(changed, layer, {0, 0}), readTile(render(oracle, document.preview()), layer, {0, 0}));
    QCOMPARE(readTile(preview, layer, {0, 0}), firstPixels);
    QCOMPARE(readTile(background, layer, {0, 0}), backgroundPixels);
    preview = changed;
    changed.clear();
    for (const auto &shape : {Shape{ShapeKind::Line, {140, 180}, {200, 200}, 0},
                             Shape{ShapeKind::Ellipse, {60, 80}, {100, 110}, 0.2}}) {
        QCOMPARE(document.updateShape(stroke, shape), Result::Success);
        changed = render(renderer, document.preview(), preview);
        QVERIFY(changed);
        QCOMPARE(readTile(changed, layer, {0, 0}), readTile(render(oracle, document.preview()), layer, {0, 0}));
        preview = changed;
        changed.clear();
    }
    const auto retainedInput = document.preview();
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    auto cancelled = render(renderer, document.snapshot(), preview);
    QVERIFY(cancelled);
    QCOMPARE(readTile(cancelled, layer, {0, 0}), backgroundPixels);
    preview.clear();
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));

    TileRenderer rebuiltRenderer(m_device, 3 * TileRenderer::TileBytes);
    const auto rebuiltBackground = render(rebuiltRenderer, document.snapshot());
    QVERIFY(rebuiltBackground);
    const auto intent = std::get<PaintSnapshot>(retainedInput->layers.first().operations.last());
    const auto next = document.beginStroke(layer, intent->brush, intent->randomSeed);
    QCOMPARE(document.appendSamples(next, intent->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(next, intent->shape, intent->figureStyle), Result::Success);
    auto rebuilt = render(rebuiltRenderer, document.preview(), rebuiltBackground);
    QVERIFY(rebuilt);
    QCOMPARE(document.updateShape(document.activeOperation(), {ShapeKind::Line, {20, 140}, {200, 160}, 0}), Result::Success);
    const auto rebuiltChange = render(rebuiltRenderer, document.preview(), rebuilt);
    QVERIFY(rebuiltChange);
    QCOMPARE(readTile(rebuiltChange, document.activeLayer(), {0, 0}),
             readTile(render(oracle, document.preview()), document.activeLayer(), {0, 0}));
}

void KisGpuTileRendererTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
    QVERIFY(m_device->nativeDevice());
    QVERIFY(m_device->queue());
    WGPUAdapterInfo info{};
    QVERIFY(wgpuAdapterGetInfo(m_device->adapter(), &info) == WGPUStatus_Success);
    qInfo() << "GPU projection adapter:" << QString::fromUtf8(info.device.data, qsizetype(info.device.length))
            << "backend" << int(info.backendType) << "type" << int(info.adapterType);
    wgpuAdapterInfoFreeMembers(info);
}

void KisGpuTileRendererTest::partialRenderingMatchesFullTilesAndRejectsIncompleteComposition()
{
    OperationDocument document(QSize(1024, 512));
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 29.25;
    const auto stroke = document.beginStroke(layer, brush, 123);
    QCOMPARE(document.appendSamples(stroke, {{{20, 70}, 0.5}, {{800, 300}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto hidden = document.addLayer(QStringLiteral("Hidden"));
    QCOMPARE(document.setLayerVisible(hidden, false), Result::Success);
    const RgbaTile offCanvas{{-1, -1}, QByteArray::fromHex("12345678").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(document.replaceTiles(hidden, {offCanvas}), Result::Success);
    TileRenderer fullRenderer(m_device);
    const auto full = render(fullRenderer, document.snapshot());
    QVERIFY(full);
    TileRenderer partialRenderer(m_device, 3 * TileRenderer::TileBytes);
    const QRect region(263, 130, 1, 1);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto partial = partialRenderer.recordRegion(commands, document.snapshot(), region, {layer});
    QCOMPARE(partial.code, RenderOutcome::Recorded);
    const auto composed = partialRenderer.compose(commands, partial.version, region);
    QCOMPARE(composed.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(commands, partial.version, {0, 0, 1024, 512}).code, RenderOutcome::InvalidVersion);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(partial.version->tiles(layer).size(), 1);
    QVERIFY(partial.version->tiles(hidden).isEmpty());
    QCOMPARE(partial.version->tiles(layer)[0].coordinate, QPoint(1, 0));
    QCOMPARE(readTile(partial.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto reused = partialRenderer.recordRegion(commands, document.snapshot(), region, {layer}, partial.version);
    QCOMPARE(reused.version, partial.version);
    const auto shifted = partialRenderer.recordRegion(commands, document.snapshot(), {280, 140, 1, 1}, {layer}, partial.version);
    QCOMPARE(shifted.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
    auto expanded = partialRenderer.recordRegion(commands, document.snapshot(), {256, 0, 512, 256}, {layer}, partial.version);
    QCOMPARE(expanded.code, RenderOutcome::Recorded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(expanded.version->tiles(layer).size(), 2);
    QCOMPARE(partialRenderer.residentBytes(), qsizetype(2 * TileRenderer::TileBytes));
    QCOMPARE(readTile(expanded.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    QCOMPARE(readTile(expanded.version, layer, {2, 0}), readTile(full, layer, {2, 0}));
    expanded.version.clear();
    const auto edit = document.beginStroke(layer, brush, 456);
    QCOMPARE(document.appendSamples(edit, {{{300, 140}, 0.75}}), Result::Success);
    QCOMPARE(document.commitStroke(edit), Result::Success);
    const auto editedFull = render(fullRenderer, document.snapshot(), full);
    QVERIFY(editedFull);
    commands = m_device->beginCommands();
    QVERIFY(commands);
    auto edited = partialRenderer.recordRegion(commands, document.snapshot(), {256, 0, 512, 256}, {layer}, partial.version);
    QCOMPARE(edited.code, RenderOutcome::Recorded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readTile(edited.version, layer, {1, 0}), readTile(editedFull, layer, {1, 0}));
    QCOMPARE(readTile(edited.version, layer, {2, 0}), readTile(editedFull, layer, {2, 0}));
    QCOMPARE(readTile(partial.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    edited.version.clear();
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto raw = partialRenderer.recordRegion(commands, document.snapshot(), {-1, -1, 1, 1}, {hidden}, partial.version);
    QCOMPARE(raw.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(commands, raw.version, {0, 0, 1, 1}).code, RenderOutcome::InvalidVersion);
    const auto incomplete = partialRenderer.recordRegion(commands, document.snapshot(), {0, 0, 1, 1}, {hidden});
    QCOMPARE(incomplete.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(commands, incomplete.version, {0, 0, 1, 1}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(partialRenderer.recordRegion(commands, document.snapshot(), region, {layer, layer}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(partialRenderer.recordRegion(commands, document.snapshot(), region, {999}).code, RenderOutcome::InvalidVersion);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(raw.version->tiles(hidden).size(), 1);
    QCOMPARE(readTile(raw.version, hidden, {-1, -1}), offCanvas.rgba);
}

RenderedSnapshot KisGpuTileRendererTest::render(TileRenderer &renderer, const Snapshot &snapshot,
                                               const RenderedSnapshot &base)
{
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    if (!commands) {
        qWarning() << "GPU command recording unavailable" << m_device->diagnostic();
        return {};
    }
    const auto result = renderer.record(commands, snapshot, base);
    const auto submitted = m_device->submitCommands(true);
    if (result.code != RenderOutcome::Recorded) qWarning() << result.diagnostic;
    if (!submitted) {
        qWarning() << "GPU command submission failed" << m_device->diagnostic();
        return {};
    }
    return result.version;
}

QByteArray KisGpuTileRendererTest::readTile(const RenderedSnapshot &version, LayerId layer, QPoint coordinate)
{
    for (const auto &tile : version->tiles(layer)) {
        if (tile.coordinate != coordinate) continue;
        return readBuffer(tile.pixels);
    }
    return {};
}

QByteArray KisGpuTileRendererTest::readBuffer(WGPUBuffer pixels)
{
    const auto commands = m_device->beginCommands();
    if (!commands) return {};
    const auto reads = Detail::ReadbackBatch::create(*m_device, 1);
    if (!reads || !reads->recordBuffer(*m_device, commands, 0, pixels, wgpuBufferGetSize(pixels))) {
        m_device->submitCommands(true);
        return {};
    }
    if (!m_device->submitCommands(true)) return {};
    return reads->results.front()->data;
}

ProjectedSnapshot KisGpuTileRendererTest::project(TileRenderer &renderer, const RenderedSnapshot &version,
                                                 QRect visible, const ProjectedSnapshot &base)
{
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    if (!commands) return {};
    const auto result = renderer.compose(commands, version, visible, base);
    const auto submitted = m_device->submitCommands(true);
    if (result.code != RenderOutcome::Recorded) qWarning() << result.diagnostic;
    return submitted ? result.version : ProjectedSnapshot{};
}

void KisGpuTileRendererTest::roundBrushProducesRgbaPixelsOnTheGpu()
{
    OperationDocument document({512, 512});
    Brush brush;
    brush.rgba = {200, 30, 70, 255};
    brush.diameter = 16;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    QVERIFY(initial->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.appendSamples(stroke, {{{48, 32}, 1}}), Result::Success);
    const auto preview = render(renderer, document.preview(), initial);
    QVERIFY(preview);
    const auto bytes = readTile(preview, document.activeLayer(), {0, 0});
    QCOMPARE(bytes.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    QCOMPARE(pixel(32, 32), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(0, 0), QByteArray(4, '\0'));
    QCOMPARE(preview->tiles(document.activeLayer()).size(), 1);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), preview), document.activeLayer(), {0, 0}), bytes);
}

