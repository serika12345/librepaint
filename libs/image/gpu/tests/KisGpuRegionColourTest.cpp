/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <KoColorSpaceRegistry.h>
#include <KoTestConfig.h>
#include <KoResourcePaths.h>
#include <QStandardPaths>
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>
#include <KoCompositeOpRegistry.h>
#include <QColorSpace>
#include <QGuiApplication>
#include <QTest>
#include <QBitArray>
#include <cstring>

using namespace LibrePaint::Image::Gpu;
namespace {
void exchangeChannels(QByteArray &bytes, int stride)
{
    for (qsizetype offset = 0; offset < bytes.size(); offset += stride)
        for (int component = 0; component < stride / 4; ++component)
            std::swap(bytes[offset + component], bytes[offset + stride / 2 + component]);
}
}
class KisGpuRegionColourTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void similarColourCoverageKeepsNativeDifference_data();
    void similarColourCoverageKeepsNativeDifference();
    void colourSelectionUsesUnclippedMaskedProjection();
    void maskVersionsKeepColdRenderCoverage();
    void colourSelectionRetainsPixelsAfterClosureAndDeviceReplacement();
    void generatedSelectionActionsRetainAlphaAndHistory();
    void generatedSelectionMovesKeepClippedAlphaAndHistory();
};
void KisGpuRegionColourTest::similarColourCoverageKeepsNativeDifference_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("profileKind");
    QTest::addColumn<bool>("merged");
    QTest::addColumn<bool>("boundary");
    for (const bool words : {false, true}) for (int kind = 0; kind < 3; ++kind) {
        const QByteArray name = QByteArray(words ? "word-" : "byte-") + QByteArray::number(kind);
        QTest::newRow(name.constData()) << words << kind << false << false;
    }
    QTest::newRow("byte-merged") << false << 0 << true << false;
    QTest::newRow("word-merged") << true << 1 << true << false;
    QTest::newRow("byte-boundary") << false << 0 << false << true;
    QTest::newRow("word-boundary") << true << 0 << false << true;
    QTest::newRow("byte-linear-boundary") << false << 1 << true << true;
    QTest::newRow("word-linear-boundary") << true << 1 << true << true;
}
void KisGpuRegionColourTest::similarColourCoverageKeepsNativeDifference()
{
    QFETCH(bool, words);
    QFETCH(int, profileKind);
    QFETCH(bool, merged);
    QFETCH(bool, boundary);
    auto *registry = KoColorSpaceRegistry::instance();
    const auto depth = words ? Integer16BitsColorDepthID.id() : Integer8BitsColorDepthID.id();
    const auto *profile = registry->createColorProfileFromData(RGBAColorModelID.id(), depth,
        QColorSpace(profileKind == 0 ? QColorSpace::SRgb : profileKind == 1 ? QColorSpace::SRgbLinear : QColorSpace::DisplayP3).iccProfile());
    QVERIFY(profile);
    const auto *cs = registry->colorSpace(RGBAColorModelID.id(), depth, profile);
    QVERIFY(cs);
    const auto definition = cs->nativeLabConversion();
    QVERIFY2(definition.code == LibrePaint::Pigment::Color::NativeLabConversionResult::Success, qPrintable(definition.diagnostic));
    QVERIFY(definition.value && definition.value->isValid());
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster;
    raster.size = {64, 32}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    raster.colorProfile = profile->rawData();
    raster.layers = {{QStringLiteral("Native colour regions"), true, {}, {}}};
    if (merged) raster.layers.append({QStringLiteral("Reference overlay"), true, {}, {}});
    PaintImage image(execution, raster);
    const QRect area(QPoint(), raster.size);
    const QPoint seed(3, 4);
    const int stride = pixelByteCount(raster.format);
    const quint16 maximum = maximumComponent(raster.format);
    const int seedIndex = seed.y() * area.width() + seed.x();
    QByteArray initial;
    quint32 random = 0x71fc9831;
    for (int i = 0; i < area.width() * area.height(); ++i) {
        RgbaPixel pixel;
        for (int c = 0; c < 4; ++c) { random = random * 1664525u + 1013904223u; pixel[c] = quint16((random >> 8) & maximum); }
        if (i % 5 == 0 || i == seedIndex) pixel[3] = maximum;
        else if (i % 31 == 0) pixel[3] = 0;
        else if (words && i % 37 == 0) pixel[3] = 127;
        else if (words && i % 41 == 0) pixel[3] = 128;
        initial += rgbaBytes(pixel, raster.format);
    }
    QCOMPARE(image.writeRgbaPixels(1, area, initial), Result::Success);
    auto expected = initial; exchangeChannels(expected, stride);
    auto nativeSource = expected;
    if (merged) {
        const auto colour = rgbaBytes({quint16(maximum / 4), quint16(maximum / 2), quint16(maximum / 3), quint16(maximum / 2)}, raster.format);
        QByteArray overlay;
        for (int i = 0; i < area.width() * area.height(); ++i) overlay += colour;
        QCOMPARE(image.writeRgbaPixels(2, area, overlay), Result::Success);
        exchangeChannels(overlay, stride);
        cs->compositeOp(COMPOSITE_OVER)->composite(reinterpret_cast<quint8 *>(nativeSource.data()), nativeSource.size(),
            reinterpret_cast<const quint8 *>(overlay.constData()), overlay.size(), nullptr, 0, 1, area.width() * area.height(), 1);
    }
    QByteArray mask(area.width() * area.height(), '\0');
    const RgbaPixel boundaryPixel{quint16(maximum / 3), quint16(maximum / 5), quint16(maximum / 7), quint16(maximum / 2)};
    auto nativeBoundary = rgbaBytes(boundaryPixel, raster.format);
    exchangeChannels(nativeBoundary, stride);
    const auto *reference = reinterpret_cast<const quint8 *>(boundary ? nativeBoundary.constData() : nativeSource.constData() + seedIndex * stride);
    for (int i = 0; i < mask.size(); ++i) {
        // At threshold255/softness100 the native soft-selection coverage is
        // exactly255-differenceA; boundary filling uses its complement.
        // Erasing exposes the resulting coverage in saved alpha.
        const auto difference = cs->differenceA(reference,
            reinterpret_cast<const quint8 *>(nativeSource.constData()) + i * stride);
        mask[i] = char(boundary ? difference : 255 - difference);
    }
    if (boundary) {
        QVERIFY(quint8(mask.at(seedIndex)) > 0);
        QBitArray connected(mask.size());
        QList<int> pending{seedIndex}; connected.setBit(seedIndex);
        for (qsizetype cursor = 0; cursor < pending.size(); ++cursor) {
            const int index = pending[cursor], x = index % area.width(), y = index / area.width();
            for (const auto neighbor : {QPoint(x - 1, y), QPoint(x + 1, y), QPoint(x, y - 1), QPoint(x, y + 1)}) {
                if (!area.contains(neighbor)) continue;
                const int next = neighbor.y() * area.width() + neighbor.x();
                if (connected.testBit(next) || !quint8(mask.at(next))) continue;
                connected.setBit(next); pending.append(next);
            }
        }
        for (int i = 0; i < mask.size(); ++i) if (!connected.testBit(i)) mask[i] = 0;
    }
    const auto opaque = rgbaBytes({0, 0, 0, maximum}, raster.format);
    cs->compositeOp(COMPOSITE_ERASE)->composite(reinterpret_cast<quint8 *>(expected.data()), expected.size(),
        reinterpret_cast<const quint8 *>(opaque.constData()), 0,
        reinterpret_cast<const quint8 *>(mask.constData()), mask.size(), 1, mask.size(), 1);
    exchangeChannels(expected, stride);
    RegionFillSettings settings;
    settings.seed = seed; settings.mode = boundary ? RegionFillMode::Contiguous : RegionFillMode::Similar;
    if (boundary) settings.boundaryColor = boundaryPixel;
    settings.reference = merged ? RegionFillReference::AllLayers : RegionFillReference::CurrentLayer;
    settings.threshold = 255; settings.opacitySpread = 0; settings.labConversion = definition.value;
    const auto filled = image.fillRegion(1, settings, {0, 0, 0, maximum}, {}, 1, true);
    QVERIFY2(filled == Result::Success, qPrintable(image.lastRequestOutcome().diagnostic));
    const auto pixels = image.readRgbaPixels(1, area, expected.size());
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    if (pixels.bytes != expected) {
        for (int i = 0; i < expected.size(); ++i) if (pixels.bytes[i] != expected[i]) {
            qWarning() << "colour difference mismatch at pixel" << i / stride
                       << "source" << initial.mid((i / stride) * stride, stride).toHex()
                       << "native coverage" << quint8(mask.at(i / stride))
                       << "actual" << pixels.bytes.mid((i / stride) * stride, stride).toHex()
                       << "native" << expected.mid((i / stride) * stride, stride).toHex(); break;
        }
    }
    QCOMPARE(pixels.bytes, expected);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, area, expected.size()).bytes, initial);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, area, expected.size()).bytes, expected);
}
void KisGpuRegionColourTest::colourSelectionUsesUnclippedMaskedProjection()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    for (const bool words : {false, true}) {
        RasterImage raster;
        raster.size = {64, 32}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
        raster.layers = {{QStringLiteral("Locked projection reference"), true, {}, {}}};
        raster.layers.append({QStringLiteral("Selection drawing target"), true, {}, {}});
        PaintImage image(execution, raster);
        const QRect canvas(QPoint(), raster.size), masked(8, 8, 12, 8), previous(10, 10, 2, 2);
        const auto maximum = maximumComponent(raster.format);
        QByteArray pixels;
        const auto value = rgbaBytes({quint16(maximum / 3), quint16(maximum / 2), maximum, maximum}, raster.format);
        for (int i = 0; i < canvas.width() * canvas.height(); ++i) pixels += value;
        QCOMPARE(image.writeRgbaPixels(1, canvas, pixels), Result::Success);
        QCOMPARE(image.setSelectionRect(masked), Result::Success);
        QCOMPARE(image.addTransparencyMaskFromSelection(1), Result::Success);
        QCOMPARE(image.setSelectionRect(previous), Result::Success);
        RegionFillSettings settings;
        settings.seed = {10, 10}; settings.threshold = 1;
        settings.mode = RegionFillMode::Similar;
        QCOMPARE(image.fillRegion(1, settings, {0, 0, 0, maximum}, {}), Result::Success);
        auto painted = pixels;
        const auto black = rgbaBytes({0, 0, 0, maximum}, raster.format);
        for (int y = previous.top(); y <= previous.bottom(); ++y)
            for (int x = previous.left(); x <= previous.right(); ++x)
                painted.replace((y * canvas.width() + x) * black.size(), black.size(), black);
        QCOMPARE(image.readRgbaPixels(1, canvas, pixels.size()).bytes, painted);
        QCOMPARE(image.undo(), Result::Success);
        QCOMPARE(image.setLayerLocked(1, true), Result::Success);
        QCOMPARE(image.setLayerVisible(1, false), Result::Success);
        QCOMPARE(image.fillRegion(1, settings, {0, 0, 0, maximum}, {}), Result::LayerUnavailable);
        const auto before = image.operations().snapshot();
        const int history = image.operations().undoCount();
        for (const auto mode : {RegionFillMode::Similar, RegionFillMode::Contiguous}) {
            settings.mode = mode;
            for (const bool boundary : {false, true}) {
                settings.selectionAsBoundary = boundary;
                for (const QPoint seed : {QPoint(10, 10), QPoint(15, 12)}) {
                    settings.seed = seed;
                    const auto result = image.selectRegion(1, settings, SelectionCombination::Replace);
                    QVERIFY2(result == Result::Success, qPrintable(image.lastRequestOutcome().diagnostic));
                    QCOMPARE(image.selectionRect(), mode == RegionFillMode::Contiguous && boundary && previous.contains(seed) ? previous : masked);
                    QVERIFY(image.selectionMask() && image.selectionMask()->isGpuGenerated());
                    QCOMPARE(image.readRgbaPixels(1, canvas, pixels.size()).bytes, pixels);
                    QCOMPARE(image.operations().undoCount(), history + 1);
                    const auto selected = image.operations().snapshot();
                    const QRect expected = image.selectionRect();
                    QByteArray expectedAlpha(canvas.width() * canvas.height(), '\0');
                    for (int y = expected.top(); y <= expected.bottom(); ++y)
                        for (int x = expected.left(); x <= expected.right(); ++x)
                            expectedAlpha[y * canvas.width() + x] = char(255);
                    const auto alpha = execution.readSelectionPixels(image.selectionMask(), canvas, expectedAlpha.size());
                    QVERIFY2(alpha.code == ExecutionCode::Success, qPrintable(alpha.diagnostic));
                    QCOMPARE(alpha.bytes, expectedAlpha);
                    QCOMPARE(execution.readSelectionPixels(image.selectionMask(), canvas, expectedAlpha.size() - 1).code,
                             ExecutionCode::CapacityExceeded);
                    QCOMPARE(image.operations().snapshot(), selected);
                    QCOMPARE(image.fillLayerColor(2, {0, 0, 0, maximum}), Result::Success);
                    QByteArray target(pixels.size(), '\0');
                    for (int y = expected.top(); y <= expected.bottom(); ++y)
                        for (int x = expected.left(); x <= expected.right(); ++x)
                            target.replace((y * canvas.width() + x) * black.size(), black.size(), black);
                    const auto drawn = image.readRgbaPixels(2, canvas, target.size());
                    QVERIFY2(drawn.code == ExecutionCode::Success, qPrintable(drawn.diagnostic));
                    QCOMPARE(drawn.bytes, target);
                    const auto red = rgbaBytes({maximum, 0, 0, maximum}, raster.format);
                    QCOMPARE(image.fillRegion(2, settings, {maximum, 0, 0, maximum}, {}), Result::Success);
                    auto refilled = target;
                    for (int y = expected.top(); y <= expected.bottom(); ++y)
                        for (int x = expected.left(); x <= expected.right(); ++x)
                            refilled.replace((y * canvas.width() + x) * red.size(), red.size(), red);
                    const auto filled = image.readRgbaPixels(2, canvas, target.size());
                    QVERIFY2(filled.code == ExecutionCode::Success, qPrintable(filled.diagnostic));
                    QCOMPARE(filled.bytes, refilled);
                    QCOMPARE(image.undo(), Result::Success);
                    QCOMPARE(image.readRgbaPixels(2, canvas, target.size()).bytes, target);
                    QCOMPARE(image.undo(), Result::Success);
                    QCOMPARE(image.operations().snapshot(), selected);
                    QCOMPARE(image.undo(), Result::Success);
                    QCOMPARE(image.operations().snapshot(), before);
                    QCOMPARE(image.operations().undoCount(), history);
                }
            }
        }
    }
}

void KisGpuRegionColourTest::generatedSelectionActionsRetainAlphaAndHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    const QRect canvas(0, 0, 64, 32), first(5, 4, 18, 9), softArea(-3, 1, 32, 22);
    const auto clip = canvas.united(softArea);
    QByteArray alpha(softArea.width() * softArea.height(), '\0');
    const quint8 levels[]{0, 31, 64, 127, 128, 201, 255};
    for (int y = 0; y < softArea.height(); ++y) for (int x = 0; x < softArea.width(); ++x)
        alpha[y * softArea.width() + x] = char(levels[(x + 2 * y) % 7]);
    const auto soft = SelectionMask::fromAlphaCoverage(softArea, alpha, clip);
    const auto hard = SelectionMask::fromRectangle(first, canvas);
    QVERIFY(soft && hard);
    for (const auto format : {PixelFormat::Rgba8, PixelFormat::Rgba16}) {
        RasterImage raster;
        raster.size = canvas.size(); raster.format = format;
        raster.layers = {{QStringLiteral("Frozen colour source"), true, {}, {}},
                         {QStringLiteral("Selected painting"), true, {}, {}}};
        PaintImage image(execution, raster);
        const auto maximum = maximumComponent(format);
        QCOMPARE(image.setSelectionRect(first), Result::Success);
        QCOMPARE(image.fillLayerColor(1, {maximum, 0, 0, maximum}), Result::Success);
        RegionFillSettings input;
        input.seed = {8, 8}; input.threshold = 1;
        QCOMPARE(image.selectRegion(1, input, SelectionCombination::Replace), Result::Success);
        const auto generated = image.selectionMask();
        QVERIFY(generated && generated->isGpuGenerated());
        for (const bool incomingGpu : {false, true}) for (const bool inverted : {false, true})
            for (const auto action : {SelectionCombination::Replace, SelectionCombination::Add, SelectionCombination::Subtract,
                                     SelectionCombination::Intersect, SelectionCombination::SymmetricDifference}) {
                const auto previous = incomingGpu ? soft : generated;
                const auto incoming = incomingGpu ? generated : soft;
                const auto previousLiteral = incomingGpu ? soft : hard;
                const auto incomingLiteral = incomingGpu ? hard : soft;
                QCOMPARE(image.setSelectionMask(previous, inverted), Result::Success);
                const auto before = image.operations().snapshot();
                const auto combinationClip = inverted ? canvas : clip;
                const auto expected = previousLiteral->combined(*incomingLiteral, action, combinationClip, inverted);
                QVERIFY(expected);
                const auto combined = image.combineSelectionMask(incoming, action);
                QVERIFY2(combined == Result::Success, qPrintable(image.lastRequestOutcome().diagnostic));
                QCOMPARE(image.selectionRect(), expected->bounds());
                const auto selected = image.operations().snapshot();
                const auto actual = image.selectionMask() ? image.selectionMask() : SelectionMask::empty();
                const auto read = execution.readSelectionPixels(actual, clip, clip.width() * clip.height());
                QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
                QByteArray expectedAlpha(read.bytes.size(), '\0');
                for (int y = clip.top(); y <= clip.bottom(); ++y) for (int x = clip.left(); x <= clip.right(); ++x)
                    expectedAlpha[(y - clip.y()) * clip.width() + x - clip.x()] = char(expected->coverage(x, y));
                QCOMPARE(read.bytes, expectedAlpha);
                QCOMPARE(image.fillLayerColor(2, {0, maximum, 0, maximum}), Result::Success);
                const auto pixels = image.readRgbaPixels(2, canvas, canvas.width() * canvas.height() * pixelByteCount(format));
                QCOMPARE(pixels.code, ExecutionCode::Success);
                QByteArray expectedPixels(pixels.bytes.size(), '\0');
                for (int y = 0; y < canvas.height(); ++y) for (int x = 0; x < canvas.width(); ++x) {
                    const auto weight = expected->isEmpty() ? 255 : expected->coverage(x, y);
                    const auto value = rgbaBytes(weight ? RgbaPixel{0, maximum, 0, quint16(quint32(maximum) * weight / 255)} : RgbaPixel{}, format);
                    expectedPixels.replace((y * canvas.width() + x) * value.size(), value.size(), value);
                }
                QCOMPARE(pixels.bytes, expectedPixels);
                QCOMPARE(image.undo(), Result::Success);
                QCOMPARE(image.operations().snapshot(), selected);
                QCOMPARE(image.undo(), Result::Success);
                QCOMPARE(image.operations().snapshot(), before);
                QCOMPARE(image.redo(), Result::Success);
                QCOMPARE(image.operations().snapshot(), selected);
                QCOMPARE(image.redo(), Result::Success);
                QCOMPARE(image.readRgbaPixels(2, canvas, pixels.bytes.size()).bytes, pixels.bytes);
                QCOMPARE(image.undo(), Result::Success);
                QCOMPARE(image.undo(), Result::Success);
            }
    }
}

void KisGpuRegionColourTest::generatedSelectionMovesKeepClippedAlphaAndHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    const QRect canvas(0, 0, 64, 32), selected(4, 3, 23, 17);
    const auto hard = SelectionMask::fromRectangle(selected, canvas);
    QByteArray alpha(selected.width() * selected.height(), '\0');
    const quint8 levels[]{0, 1, 127, 128, 201, 255};
    for (int y = 0; y < selected.height(); ++y) for (int x = 0; x < selected.width(); ++x)
        alpha[y * selected.width() + x] = char(levels[(x + y * 2) % 6]);
    const auto soft = SelectionMask::fromAlphaCoverage(selected, alpha, canvas);
    QVERIFY(hard && soft);
    SelectionMaskSP replayed;
    QByteArray replayedAlpha;
    for (const auto format : {PixelFormat::Rgba8, PixelFormat::Rgba16}) {
        RasterImage raster;
        raster.size = canvas.size(); raster.format = format;
        raster.layers = {{QStringLiteral("Frozen colour source"), true, {}, {}},
                         {QStringLiteral("Moved selection painting"), true, {}, {}}};
        PaintImage image(execution, raster);
        const auto maximum = maximumComponent(format);
        QCOMPARE(image.setSelectionRect(selected), Result::Success);
        QCOMPARE(image.fillLayerColor(1, {maximum, 0, 0, maximum}), Result::Success);
        RegionFillSettings input;
        input.seed = {8, 8}; input.threshold = 1;
        QCOMPARE(image.selectRegion(1, input, SelectionCombination::Replace), Result::Success);
        QCOMPARE(image.combineSelectionMask(soft, SelectionCombination::Intersect), Result::Success);
        const auto generated = image.selectionMask();
        QVERIFY(generated && generated->isGpuGenerated());
        const auto literal = hard->combined(*soft, SelectionCombination::Intersect, canvas);
        QVERIFY(literal);
        for (const QPoint delta : {QPoint(17, -3), QPoint(-19, 5), QPoint(51, 17), QPoint(80, 0)}) {
            QCOMPARE(image.setSelectionMask(generated), Result::Success);
            const auto before = image.operations().snapshot();
            const auto moved = image.translatedSelectionMask(generated, delta, canvas);
            QVERIFY2(moved, qPrintable(image.lastRequestOutcome().diagnostic));
            QCOMPARE(image.operations().snapshot(), before);
            const auto expected = literal->translated(delta, canvas);
            QCOMPARE(moved->bounds(), expected ? expected->bounds() : QRect());
            const auto read = execution.readSelectionPixels(moved, canvas, canvas.width() * canvas.height());
            QCOMPARE(read.code, ExecutionCode::Success);
            QByteArray coverage(read.bytes.size(), '\0');
            for (int y = 0; expected && y < canvas.height(); ++y) for (int x = 0; x < canvas.width(); ++x)
                coverage[y * canvas.width() + x] = char(expected->coverage(x, y));
            QCOMPARE(read.bytes, coverage);
            if (moved->isEmpty()) {
                QCOMPARE(image.clearSelection(), Result::Success);
            } else {
                QVERIFY(moved->isGpuGenerated());
                QCOMPARE(image.setSelectionMask(moved), Result::Success);
                replayed = moved; replayedAlpha = coverage;
                const auto movedVersion = image.operations().snapshot();
                QCOMPARE(image.fillLayerColor(2, {0, maximum, 0, maximum}), Result::Success);
                const auto pixels = image.readRgbaPixels(2, canvas, canvas.width() * canvas.height() * pixelByteCount(format));
                QCOMPARE(pixels.code, ExecutionCode::Success);
                QByteArray expectedPixels(pixels.bytes.size(), '\0');
                for (int i = 0; i < coverage.size(); ++i) {
                    const auto weight = quint8(coverage[i]);
                    const auto value = rgbaBytes(weight ? RgbaPixel{0, maximum, 0, quint16(quint32(maximum) * weight / 255)} : RgbaPixel{}, format);
                    expectedPixels.replace(i * value.size(), value.size(), value);
                }
                QCOMPARE(pixels.bytes, expectedPixels);
                QCOMPARE(image.undo(), Result::Success);
                QCOMPARE(image.operations().snapshot(), movedVersion);
            }
            QCOMPARE(image.undo(), Result::Success);
            QCOMPARE(image.operations().snapshot(), before);
            QCOMPARE(image.redo(), Result::Success);
            QCOMPARE(image.undo(), Result::Success);
        }
        const auto before = image.operations().snapshot();
        const auto frame = execution.beginReadbackFrame();
        QCOMPARE(frame.code, ExecutionCode::Success);
        QVERIFY(!image.translatedSelectionMask(generated, {1, 1}, canvas));
        QCOMPARE(image.lastRequestOutcome().code, ExecutionCode::Busy);
        QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
        QCOMPARE(image.operations().snapshot(), before);
        image.close();
    }
    QVERIFY(replayed);
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    QCOMPARE(execution.readSelectionPixels(replayed, canvas, replayedAlpha.size()).bytes, replayedAlpha);
}

void KisGpuRegionColourTest::maskVersionsKeepColdRenderCoverage()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    for (const bool words : {false, true}) {
        RasterImage raster;
        raster.size = {32, 16}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
        raster.layers = {{QStringLiteral("Frozen selection operations"), true, {}, {}}};
        PaintImage image(execution, raster);
        const QRect canvas(QPoint(), raster.size), first(0, 0, 8, 8), second(16, 0, 8, 8);
        const auto maximum = maximumComponent(raster.format);
        const auto a = SelectionMask::fromAlphaCoverage(first, QByteArray(64, char(255)), canvas);
        const auto b = SelectionMask::fromAlphaCoverage(second, QByteArray(64, char(128)), canvas);
        QVERIFY(a && b);
        QCOMPARE(image.setSelectionMask(a), Result::Success);
        QCOMPARE(image.fillLayerColor(1, {maximum, 0, 0, maximum}), Result::Success);
        QCOMPARE(image.setSelectionMask(b), Result::Success);
        QCOMPARE(image.fillLayerColor(1, {0, maximum, 0, maximum}), Result::Success);
        QByteArray expected(canvas.width() * canvas.height() * pixelByteCount(raster.format), '\0');
        const auto red = rgbaBytes({maximum, 0, 0, maximum}, raster.format);
        const auto green = rgbaBytes({0, maximum, 0, quint16(maximum * 128u / 255u)}, raster.format);
        for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) {
            expected.replace((y * canvas.width() + x) * red.size(), red.size(), red);
            expected.replace((y * canvas.width() + 16 + x) * green.size(), green.size(), green);
        }
        // The first render replays both operations, rather than inheriting an
        // already rendered first stroke from a warm document version.
        const auto actual = image.readRgbaPixels(1, canvas, expected.size());
        QVERIFY2(actual.code == ExecutionCode::Success, qPrintable(actual.diagnostic));
        QCOMPARE(actual.bytes, expected);
    }
}

void KisGpuRegionColourTest::colourSelectionRetainsPixelsAfterClosureAndDeviceReplacement()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    const QSize size(32, 16);
    const QRect canvas(QPoint(), size), selected(3, 4, 17, 8);
    SelectionMaskSP mask;
    QByteArray expected(canvas.width() * canvas.height(), '\0');
    for (int y = selected.top(); y <= selected.bottom(); ++y)
        for (int x = selected.left(); x <= selected.right(); ++x) expected[y * canvas.width() + x] = char(255);
    {
        PaintImage image(execution, size);
        const auto layer = image.operations().activeLayer();
        QCOMPARE(image.setSelectionRect(selected), Result::Success);
        QCOMPARE(image.fillLayerColor(layer, {17, 93, 201, 255}), Result::Success);
        QCOMPARE(image.clearSelection(), Result::Success);
        RegionFillSettings input;
        input.seed = {6, 6}; input.threshold = 1;
        QCOMPARE(image.selectRegion(layer, input, SelectionCombination::Replace), Result::Success);
        mask = image.selectionMask();
        QVERIFY(mask && mask->isGpuGenerated());
        const auto snapshot = image.operations().snapshot();
        const auto frame = execution.beginReadbackFrame();
        QCOMPARE(frame.code, ExecutionCode::Success);
        QCOMPARE(execution.readSelectionPixels(mask, canvas, expected.size()).code, ExecutionCode::Busy);
        QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
        QCOMPARE(image.operations().snapshot(), snapshot);
        // Replace the live source and close it before the first external mask
        // read. The accepted selection retains its original pixels and recipe.
        QCOMPARE(image.clearSelection(), Result::Success);
        QCOMPARE(image.fillLayerColor(layer, {255, 255, 255, 255}), Result::Success);
        image.close();
    }
    QCOMPARE(execution.pendingDocuments(), qsizetype(0));
    auto pixels = execution.readSelectionPixels(mask, canvas, expected.size());
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixels.bytes, expected);
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    pixels = execution.readSelectionPixels(mask, canvas, expected.size());
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixels.bytes, expected);
    PaintImage continued(execution, size);
    QCOMPARE(continued.setSelectionMask(mask), Result::Success);
    const auto layer = continued.operations().activeLayer();
    QCOMPARE(continued.fillLayerColor(layer, {255, 0, 0, 255}), Result::Success);
    const auto red = rgbaBytes({255, 0, 0, 255}, PixelFormat::Rgba8);
    QByteArray rgba(expected.size() * 4, '\0');
    for (int i = 0; i < expected.size(); ++i) if (quint8(expected[i])) rgba.replace(i * 4, 4, red);
    QCOMPARE(continued.readRgbaPixels(layer, canvas, rgba.size()).bytes, rgba);
}

int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("KRITA_PLUGIN_PATH", QByteArray(KRITA_PLUGINS_DIR_FOR_TESTS));
    qputenv("EXTRA_RESOURCE_DIRS", QByteArray(KRITA_RESOURCE_DIRS_FOR_TESTS));
    QGuiApplication application(argc, argv);
    KoResourcePaths::addAssetType("icc_profiles", "data", "/color/icc");
    KoResourcePaths::addAssetType("icc_profiles", "data", "/profiles/");
    QCoreApplication::setApplicationName(QStringLiteral("krita"));
    KisGpuRegionColourTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuRegionColourTest.moc"
