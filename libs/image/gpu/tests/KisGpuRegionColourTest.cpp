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
