/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestDocument.h"
#include <Document.h>
#include <InfoObject.h>
#include <Node.h>
#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <KisGpuBlendMode.h>
#include <gpu/KisGpuPaintImage.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoCompositeOp.h>
#include <KoColor.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_marker_painter.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <KisRegion.h>
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QImageReader>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <cstring>

namespace {
namespace Gpu = LibrePaint::Image::Gpu;
void appendPixel(QByteArray &bytes, quint16 blue, quint16 green, quint16 red, quint16 alpha)
{
    const quint16 values[] = {blue, green, red, alpha};
    bytes.append(reinterpret_cast<const char *>(values), sizeof(values));
}
Gpu::RgbaPixel rgbaAt(const QByteArray &bytes, int pixel)
{
    quint16 values[4];
    std::memcpy(values, bytes.constData() + pixel * 8, sizeof(values));
    return {values[2], values[1], values[0], values[3]};
}
}

void TestDocument::testIntegerRgba16EditingAndNativeFiles_data()
{
    QTest::addColumn<QString>("mode");
    for (const auto *mode : {"multiply", "screen", "darken", "lighten", "add", "linear_dodge",
         "diff", "exclusion", "subtract", "linear_burn", "inverse_subtract", "grain_merge", "grain_extract",
         "overlay", "hard_light", "pin_light", "divide", "dodge", "burn", "hard mix", "vivid_light",
         "linear light", "equivalence", "allanon", "hard_mix_photoshop", "xor", "or", "and", "nand", "nor",
         "xnor", "implication", "not_implication", "converse", "not_converse", "reflect", "glow", "freeze", "heat", "glow_heat", "heat_glow", "reflect_freeze", "freeze_reflect",
         "heat_glow_freeze_reflect_hybrid", "parallel", "hard overlay", "soft_light_pegtop_delphi", "negation", "penumbra a", "penumbra b", "flat_light", "hard_mix_softer_photoshop", "erase", "behind", "copy_red", "copy_green", "copy_blue", "destination-in", "destination-atop", "geometric_mean", "modulo", "modulo_continuous", "divisive_modulo", "divisive_modulo_continuous", "modulo_shift", "modulo_shift_continuous", "soft_light_ifs_illusions", "gamma_light", "gamma_illumination", "gamma_dark", "easy dodge", "easy burn", "super_light", "pnorm_a", "pnorm_b", "soft_light_svg", "soft_light", "tint_ifs_illusions", "shade_ifs_illusions", "fog_lighten_ifs_illusions", "fog_darken_ifs_illusions", "additive_subtractive", "marker", "greater", "luminosity_sai", "copy", "color", "hue", "tint", "saturation", "inc_saturation", "dec_saturation", "luminize", "inc_luminosity", "dec_luminosity", "color_hsi", "hue_hsi", "saturation_hsi", "inc_saturation_hsi", "dec_saturation_hsi", "intensity", "inc_intensity", "dec_intensity", "color_hsl", "hue_hsl", "saturation_hsl", "inc_saturation_hsl", "dec_saturation_hsl", "lightness", "inc_lightness", "dec_lightness", "color_hsv", "hue_hsv", "saturation_hsv", "inc_saturation_hsv", "dec_saturation_hsv", "value", "inc_value", "dec_value", "darker color", "lighter color", "tangent_normalmap", "combine_normal", "lambert_lighting", "lambert_lighting_gamma2.2"})
        QTest::newRow(mode) << QString::fromLatin1(mode);
}

void TestDocument::testIntegerRgba16EditingAndNativeFiles()
{
    QFETCH(QString, mode);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto *cs = KoColorSpaceRegistry::instance()->rgb16();
    const auto *composite = cs->compositeOp(mode);
    const auto blend = Gpu::layerBlendModeFromCompositeOp(mode);
    QVERIFY(composite && blend);
    constexpr int side = 14;
    const quint16 values[] = {0, 1, 2, 254, 255, 256, 257, 16383, 16384, 32767, 32768, 49151, 65534, 65535};
    QByteArray bottomBytes, topBytes;
    for (int y = 0; y < side; ++y) for (int x = 0; x < side; ++x) {
        appendPixel(bottomBytes, values[x], values[(x + y) % side], values[(x + 3 * y) % side], values[y]);
        appendPixel(topBytes, values[(x + 2 * y + 3) % side], values[(3 * x + y + 2) % side], values[(x + y + 5) % side], values[x]);
    }
    Gpu::RasterImage input;
    input.size = {side, side}; input.format = Gpu::PixelFormat::Rgba16;
    input.colorProfile = cs->profile()->rawData();
    Gpu::RasterLayer lower, upper;
    lower.name = QStringLiteral("Destination"); upper.name = QStringLiteral("Source");
    lower.uuid = QUuid(QStringLiteral("{11223344-5566-7788-99aa-bbccddeeff00}"));
    upper.uuid = QUuid(QStringLiteral("{22334455-6677-8899-aabb-ccddeeff0011}"));
    input.layers = {lower, upper};
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setCurrentImage(KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("U16 blend contract"), cs));
    document->setColorHistoryColors({});
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint U16 blend fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    Document api(document.data(), false);
    const KisImageSP image = document->image();
    QScopedPointer<Node> bottom(Node::createNode(image, image->root()->firstChild()));
    QScopedPointer<Node> top(Node::createNode(image, image->root()->lastChild()));
    QVERIFY(bottom->setPixelData(bottomBytes, 0, 0, side, side));
    QVERIFY(top->setPixelData(topBytes, 0, 0, side, side));
    top->setBlendingMode(mode);
    QCOMPARE(top->blendingMode(), mode);
    auto *topLayer = dynamic_cast<KisLayer *>(image->root()->lastChild().data());
    QVERIFY(topLayer);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto output = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(QStringLiteral("u16-blend-") + mode);
    QVERIFY(QDir().mkpath(output));
    const auto keepFailure = qScopeGuard([&] { if (QTest::currentTestFailed()) { temporary.setAutoRemove(false); qInfo() << "U16 blend artifacts" << output; } });
    const auto write = [&](const QString &name, const QByteArray &bytes) {
        QFile file(QDir(output).filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    const bool coupled = *blend >= Gpu::LayerBlendMode::ColorHsy && *blend <= Gpu::LayerBlendMode::LambertLightingGamma22;
    const bool power = *blend == Gpu::LayerBlendMode::SoftLightIFSIllusions || *blend == Gpu::LayerBlendMode::GammaLight ||
        *blend == Gpu::LayerBlendMode::GammaIllumination || *blend == Gpu::LayerBlendMode::GammaDark ||
        *blend == Gpu::LayerBlendMode::EasyDodge || *blend == Gpu::LayerBlendMode::EasyBurn || *blend == Gpu::LayerBlendMode::SuperLight ||
        *blend == Gpu::LayerBlendMode::PNormA || *blend == Gpu::LayerBlendMode::PNormB;
    for (int paletteKind = 0; paletteKind < (coupled || power ? 3 : 2); ++paletteKind) {
        const bool alphaPalette = paletteKind == 1;
        const auto kind = alphaPalette ? QStringLiteral("alpha-palette") : paletteKind == 2 ?
            (coupled ? QStringLiteral("chromatic-palette") : QStringLiteral("destination-palette")) : QStringLiteral("channel-palette");
        // Every source color or alpha against the native branch boundaries,
        // including the SVG soft-light curve at one quarter.
        // Opaque inputs isolate channels; alpha inputs exercise U16 products,
        // signed interpolation, union opacity and the final rounded division.
        QByteArray source, destination;
        source.reserve(65536 * side * 8); destination.reserve(65536 * side * 8);
        for (const auto value : values) for (quint32 word = 0; word < 65536; ++word) {
            if (alphaPalette) {
                appendPixel(source, 0xabcd, 0x0101, 0xfefd, word);
                appendPixel(destination, 0x0305, 0x1020, 0x8001, value);
            } else {
                appendPixel(source, word, word * 251u, 65535u - word, 65535);
                if (coupled && paletteKind == 2) appendPixel(destination, value, value * 251u + 257u, 65535u - value, 65535);
                else appendPixel(destination, value, value, value, 65535);
            }
        }
        if (power && paletteKind == 2) source.swap(destination);
        auto palette = input; palette.size = {256, 256 * side};
        palette.layers.last().blendMode = *blend;
        QScopedPointer<KisDocument> paletteDocument(KisPart::instance()->createDocument());
        paletteDocument->setFileBatchMode(true);
        paletteDocument->setCurrentImage(KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, palette), QStringLiteral("U16 channel palette"), cs));
        const KisImageSP paletteImage = paletteDocument->image();
        QScopedPointer<Node> destinationNode(Node::createNode(paletteImage, paletteImage->root()->firstChild()));
        QScopedPointer<Node> sourceNode(Node::createNode(paletteImage, paletteImage->root()->lastChild()));
        QVERIFY(destinationNode->setPixelData(destination, 0, 0, palette.size.width(), palette.size.height()));
        QVERIFY(sourceNode->setPixelData(source, 0, 0, palette.size.width(), palette.size.height()));
        KoCompositeOp::ParameterInfo parameters;
        QByteArray reference(destination.size(), '\0');
        parameters.srcRowStart = reinterpret_cast<const quint8 *>(destination.constData());
        parameters.dstRowStart = reinterpret_cast<quint8 *>(reference.data());
        parameters.srcRowStride = parameters.dstRowStride = palette.size.width() * 8;
        parameters.rows = palette.size.height(); parameters.cols = palette.size.width();
        cs->compositeOp(QStringLiteral("normal"))->composite(parameters);
        parameters.srcRowStart = reinterpret_cast<const quint8 *>(source.constData());
        composite->composite(parameters);
        Document paletteApi(paletteDocument.data(), false); paletteApi.refreshProjection();
        const auto actual = paletteApi.pixelData(0, 0, palette.size.width(), palette.size.height());
        if (actual != reference || qEnvironmentVariableIsSet("LIBREPAINT_GPU_RESULT_DIR")) {
            QVERIFY(write(kind + QStringLiteral(".bgra16"), actual));
            QVERIFY(write(kind + QStringLiteral("-reference.bgra16"), reference));
        }
        if (actual != reference) {
            int first = 0; while (actual[first] == reference[first]) ++first;
            qInfo() << "U16 palette" << mode << kind << "source" << first / 8 % 65536 << "destination" << values[first / 8 / 65536]
                    << "actual" << actual.mid(first / 8 * 8, 8).toHex() << "CPU" << reference.mid(first / 8 * 8, 8).toHex();
        }
        QVERIFY(actual == reference);
    }
    QByteArray expected;
    for (int opacity : {0, 1, 137, 255}) for (int mask : {0, 6, 9, 15}) {
        QBitArray flags(4);
        for (int c = 0; c < 4; ++c) flags.setBit(c, mask & (1 << (c < 3 ? 2 - c : 3)));
        topLayer->setChannelFlags(flags); top->setOpacity(opacity); api.refreshProjection();
        expected = QByteArray(side * side * 8, '\0');
        KoCompositeOp::ParameterInfo parameters;
        parameters.srcRowStart = reinterpret_cast<const quint8 *>(bottomBytes.constData());
        parameters.dstRowStart = reinterpret_cast<quint8 *>(expected.data());
        parameters.srcRowStride = parameters.dstRowStride = side * 8;
        parameters.rows = parameters.cols = side;
        cs->compositeOp(QStringLiteral("normal"))->composite(parameters);
        const QByteArray base(expected.constData(), expected.size());
        parameters.srcRowStart = reinterpret_cast<const quint8 *>(topBytes.constData());
        parameters.opacity = float(opacity) / 255; parameters.channelFlags = flags;
        composite->composite(parameters);
        const auto actual = api.pixelData(0, 0, side, side);
        QVERIFY(write(QStringLiteral("projection-o%1-c%2.bgra16").arg(opacity).arg(mask), actual));
        QVERIFY(write(QStringLiteral("reference-o%1-c%2.bgra16").arg(opacity).arg(mask), expected));
        QVERIFY2(actual == expected, qPrintable(QStringLiteral("%1 opacity=%2 mask=%3").arg(mode).arg(opacity).arg(mask)));
        for (int pixel = 0; pixel < side * side; ++pixel) {
            Gpu::RasterImage uniform = input;
            Gpu::RasterLayer group; group.kind = Gpu::LayerKind::Group;
            group.uuid = QUuid(QStringLiteral("{01020304-1122-3344-5566-778899aabbcc}"));
            auto destination = lower, source = upper;
            destination.parentUuid = source.parentUuid = group.uuid;
            destination.defaultPixel = rgbaAt(bottomBytes, pixel); source.defaultPixel = rgbaAt(topBytes, pixel);
            source.opacity = opacity; source.projectionChannelMask = mask; source.blendMode = *blend;
            uniform.layers = {destination, source, group};
            Gpu::OperationDocument defaults(uniform);
            const auto &reference = (!source.defaultPixel[3] && *blend != Gpu::LayerBlendMode::Copy &&
                *blend != Gpu::LayerBlendMode::DestinationIn && *blend != Gpu::LayerBlendMode::DestinationAtop) || (!(mask & 8) && !destination.defaultPixel[3] && *blend != Gpu::LayerBlendMode::Copy) ? base : expected;
            const auto actualDefault = defaults.snapshot()->projectionDefaultPixels().value(3);
            if (actualDefault != rgbaAt(reference, pixel))
                qInfo() << "U16 default" << mode << "opacity" << opacity << "mask" << mask << "pixel" << pixel
                        << "source" << Gpu::rgbaBytes(source.defaultPixel, input.format).toHex()
                        << "destination" << Gpu::rgbaBytes(destination.defaultPixel, input.format).toHex();
            QCOMPARE(actualDefault, rgbaAt(reference, pixel));
        }
    }
    KisPropertiesConfigurationSP saveSettings(new KisPropertiesConfiguration);
    saveSettings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    const auto saved = QDir(output).filePath(QStringLiteral("blend.kra"));
    QVERIFY2(document->saveAs(saved, KisDocument::nativeFormatMimeType(), false, saveSettings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument()); reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(saved, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    Document reopenedApi(reopened.data(), false);
    QCOMPARE(reopenedApi.pixelData(0, 0, side, side), expected);
    QScopedPointer<Node> edited(Node::createNode(reopened->image(), reopened->image()->root()->lastChild()));
    QCOMPARE(edited->blendingMode(), mode);
    const auto pngPath = QDir(output).filePath(QStringLiteral("blend.png"));
    InfoObject pngSettings;
    pngSettings.setProperty(QStringLiteral("alpha"), true); pngSettings.setProperty(QStringLiteral("forceSRGB"), false);
    pngSettings.setProperty(QStringLiteral("saveSRGBProfile"), true); pngSettings.setProperty(QStringLiteral("interlaced"), false);
    pngSettings.setProperty(QStringLiteral("compression"), 3); pngSettings.setProperty(QStringLiteral("storeMetaData"), false);
    pngSettings.setProperty(QStringLiteral("storeAuthor"), false);
    QVERIFY(reopenedApi.exportImage(pngPath, pngSettings));
    const auto png = QImageReader(pngPath).read().convertToFormat(QImage::Format_RGBA64);
    QCOMPARE(png.size(), QSize(side, side));
    for (int y = 0; y < side; ++y) for (int x = 0; x < side; ++x) {
        const auto color = reinterpret_cast<const QRgba64 *>(png.constScanLine(y))[x];
        QCOMPARE((Gpu::RgbaPixel{color.red(), color.green(), color.blue(), color.alpha()}), rgbaAt(expected, y * side + x));
    }
    const auto owner = reopened->image()->gpuImage();
    const auto id = owner->operations().snapshot()->layers.last().id;
    const auto before = edited->pixelData(0, 0, side, side);
    KisPaintDeviceSP source = new KisPaintDevice(cs);
    const quint16 ink[] = {0x1357, 0x4321, 0x9876, 65535};
    KisMarkerPainter marker(source, KoColor(reinterpret_cast<const quint8 *>(ink), cs));
    marker.fillFullCircle({5.5, 5.5}, 6);
    QByteArray selection(side * side, char(128));
    for (int y = 4; y < 8; ++y) for (int x = 0; x < 4; ++x) selection[y * side + x] = 0;
    KisSelectionSP nativeSelection = new KisSelection;
    nativeSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(selection.constData()), QRect(0, 0, side, side));
    QCOMPARE(owner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(QRect(0, 0, side, side), selection, QRect(0, 0, side, side))), Gpu::Result::Success);
    for (int mask : {6, 9, 15}) for (int opacity : {0, 1, 137, 255}) {
        KisPaintDeviceSP destination = new KisPaintDevice(cs);
        destination->writeBytes(reinterpret_cast<const quint8 *>(before.constData()), QRect(0, 0, side, side));
        KisPainter merge(destination, nativeSelection); merge.setCompositeOpId(mode); merge.setOpacityF(qreal(opacity) / 255);
        QBitArray flags(4);
        for (int c = 0; c < 4; ++c) flags.setBit(c, mask & (1 << (c < 3 ? 2 - c : 3)));
        merge.setChannelFlags(flags);
        for (const auto &rect : source->region().rects()) merge.bitBlt(rect.topLeft(), source, rect);
        QByteArray reference(before.size(), Qt::Uninitialized);
        destination->readBytes(reinterpret_cast<quint8 *>(reference.data()), QRect(0, 0, side, side));
        Gpu::Brush brush; brush.rgba = {ink[2], ink[1], ink[0], ink[3]}; brush.diameter = 12;
        brush.opacity = qreal(opacity) / 255; brush.channelWriteMask = mask; brush.blendMode = *blend;
        const auto operation = owner->beginStroke(id, brush, 7); QVERIFY(!operation.isNull());
        QCOMPARE(owner->appendSamples(operation, {{{6, 6}, 1}}), Gpu::Result::Success);
        QCOMPARE(owner->commitStroke(operation), Gpu::Result::Success);
        const auto painted = edited->pixelData(0, 0, side, side);
        QVERIFY(write(QStringLiteral("painted-o%1-c%2.bgra16").arg(opacity).arg(mask), painted));
        QVERIFY(write(QStringLiteral("painted-reference-o%1-c%2.bgra16").arg(opacity).arg(mask), reference));
        QVERIFY2(painted == reference, qPrintable(QStringLiteral("U16 stroke %1 opacity=%2 mask=%3").arg(mode).arg(opacity).arg(mask)));
        reopened->undoStack()->undo(); QCOMPARE(edited->pixelData(0, 0, side, side), before);
        reopened->undoStack()->redo(); QCOMPARE(edited->pixelData(0, 0, side, side), painted);
        if (mask == 15 && opacity == 255) {
            const auto editedPath = QDir(output).filePath(QStringLiteral("edited.kra"));
            QVERIFY(reopened->saveAs(editedPath, KisDocument::nativeFormatMimeType(), false, saveSettings));
            reopened->waitForSavingToComplete();
            QScopedPointer<KisDocument> after(KisPart::instance()->createDocument()); after->setFileBatchMode(true);
            QVERIFY2(after->openPath(editedPath, KisDocument::DontAddToRecent), qPrintable(after->errorMessage()));
            Document afterApi(after.data(), false);
            QCOMPARE(afterApi.pixelData(0, 0, side, side), reopenedApi.pixelData(0, 0, side, side));
        }
        reopened->undoStack()->undo();
    }
    if (*blend >= Gpu::LayerBlendMode::Marker && *blend <= Gpu::LayerBlendMode::LuminositySai) {
        auto blank = input;
        blank.layers = {upper};
        const auto blankOwner = Gpu::PaintImageSP::create(execution, blank);
        QScopedPointer<KisDocument> blankDocument(KisPart::instance()->createDocument());
        blankDocument->setFileBatchMode(true);
        blankDocument->setCurrentImage(KisImage::fromGpuImage(blankOwner, QStringLiteral("Empty U16 alpha composition"), cs));
        const auto blankId = blankOwner->operations().snapshot()->layers.first().id;
        QScopedPointer<Node> blankNode(Node::createNode(blankDocument->image(), blankDocument->image()->root()->firstChild()));
        QCOMPARE(blankOwner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(QRect(0, 0, side, side), selection, QRect(0, 0, side, side))), Gpu::Result::Success);
        const QByteArray empty(side * side * 8, '\0');
        for (int mask : {6, 9, 15}) for (int opacity : {0, 255}) {
            KisPaintDeviceSP destination = new KisPaintDevice(cs);
            KisPainter merge(destination, nativeSelection);
            merge.setCompositeOpId(mode); merge.setOpacityF(qreal(opacity) / 255);
            QBitArray flags(4);
            for (int c = 0; c < 4; ++c) flags.setBit(c, mask & (1 << (c < 3 ? 2 - c : 3)));
            merge.setChannelFlags(flags);
            for (const auto &rect : source->region().rects()) merge.bitBlt(rect.topLeft(), source, rect);
            QByteArray reference(empty.size(), Qt::Uninitialized);
            destination->readBytes(reinterpret_cast<quint8 *>(reference.data()), QRect(0, 0, side, side));
            Gpu::Brush brush; brush.rgba = {ink[2], ink[1], ink[0], ink[3]}; brush.diameter = 12;
            brush.opacity = qreal(opacity) / 255; brush.channelWriteMask = mask; brush.blendMode = *blend;
            const auto operation = blankOwner->beginStroke(blankId, brush, 7); QVERIFY(!operation.isNull());
            QCOMPARE(blankOwner->appendSamples(operation, {{{6, 6}, 1}}), Gpu::Result::Success);
            QCOMPARE(blankOwner->commitStroke(operation), Gpu::Result::Success);
            const auto painted = blankNode->pixelData(0, 0, side, side);
            QVERIFY(write(QStringLiteral("empty-painted-o%1-c%2.bgra16").arg(opacity).arg(mask), painted));
            QVERIFY(write(QStringLiteral("empty-reference-o%1-c%2.bgra16").arg(opacity).arg(mask), reference));
            QVERIFY2(painted == reference, qPrintable(QStringLiteral("Empty U16 stroke %1 opacity=%2 mask=%3").arg(mode).arg(opacity).arg(mask)));
            blankDocument->undoStack()->undo();
            QCOMPARE(blankNode->pixelData(0, 0, side, side), empty);
        }
    }
}
