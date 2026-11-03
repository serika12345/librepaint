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
#include <KisGpuPixelTolerance.h>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QImageReader>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <array>
#include <cstring>
#include <memory>

namespace {
namespace Gpu = LibrePaint::Image::Gpu;
void appendPixel(QByteArray &bytes, quint16 blue, quint16 green, quint16 red, quint16 alpha)
{
    const quint16 values[] = {blue, green, red, alpha};
    bytes.append(reinterpret_cast<const char *>(values), sizeof(values));
}

}

void TestDocument::testWordCurvesKeepMixedLayerPixelsAndHistory()
{
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto *cs = KoColorSpaceRegistry::instance()->rgb16();
    constexpr int side = 16;
    const QStringList modes{QStringLiteral("normal"), QStringLiteral("super_light"),
                            QStringLiteral("pnorm_a"), QStringLiteral("interpolation")};
    const std::array<quint8, 4> opacities{255, 173, 211, 137};
    const quint16 alphas[]{0, 1, 32768, 65535};
    std::array<QByteArray, 4> pixels;
    Gpu::RasterImage input;
    input.size = {side, side}; input.format = Gpu::PixelFormat::Rgba16;
    input.colorProfile = cs->profile()->rawData();
    for (int i = 0; i < 4; ++i) {
        const auto blend = Gpu::layerBlendModeFromCompositeOp(modes[i]);
        QVERIFY(blend && cs->compositeOp(modes[i]));
        Gpu::RasterLayer layer;
        layer.name = modes[i]; layer.blendMode = *blend; layer.opacity = opacities[i];
        layer.uuid = QUuid::createUuidV5(QUuid(QStringLiteral("{99112233-4455-6677-8899-aabbccddeeff}")), QByteArray::number(i));
        input.layers.append(layer);
        for (int y = 0; y < side; ++y) for (int x = 0; x < side; ++x)
            appendPixel(pixels[i], x * 257 + y * 31 + i * 7711, x * 919 + y * 103 + i * 1237,
                        x * 57 + y * 2011 + i * 3019, i ? alphas[(x + y + i) % 4] : quint16(65535));
    }
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mixed Word curves"), cs));
    std::array<std::unique_ptr<Node>, 4> nodes;
    auto child = document->image()->root()->firstChild();
    for (int i = 0; i < 4; ++i, child = child->nextSibling()) {
        QVERIFY(child);
        nodes[i].reset(Node::createNode(document->image(), child));
        QVERIFY(nodes[i]->setPixelData(pixels[i], 0, 0, side, side));
    }
    const auto reference = [&](quint8 normOpacity) {
        QByteArray result(side * side * 8, '\0');
        KoCompositeOp::ParameterInfo p;
        p.dstRowStart = reinterpret_cast<quint8 *>(result.data());
        p.dstRowStride = p.srcRowStride = side * 8; p.rows = p.cols = side;
        for (int i = 0; i < 4; ++i) {
            p.srcRowStart = reinterpret_cast<const quint8 *>(pixels[i].constData());
            p.opacity = float(i == 2 ? normOpacity : opacities[i]) / 255.f;
            cs->compositeOp(modes[i])->composite(p);
        }
        return result;
    };
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto output = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(QStringLiteral("u16-mixed-curves"));
    QVERIFY(QDir().mkpath(output));
    const auto preserveFailure = qScopeGuard([&] { if (QTest::currentTestFailed()) { temporary.setAutoRemove(false); qInfo() << "Mixed curve artifacts" << output; } });
    const auto write = [&](const QString &name, const QByteArray &bytes) {
        QFile file(QDir(output).filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    Document api(document.data(), false);
    const auto before = api.pixelData(0, 0, side, side);
    const auto beforeReference = reference(opacities[2]);
    QVERIFY(write(QStringLiteral("before.bgra16"), before));
    QVERIFY(write(QStringLiteral("before-reference.bgra16"), beforeReference));
    QCOMPARE_PIXELS16_WITHIN_STEP(before, beforeReference);
    const auto normId = owner->operations().snapshot()->layers[2].id;
    QCOMPARE(owner->setLayerOpacity(normId, 61), Gpu::Result::Success);
    const auto after = api.pixelData(0, 0, side, side);
    const auto afterReference = reference(61);
    QVERIFY(write(QStringLiteral("after.bgra16"), after));
    QVERIFY(write(QStringLiteral("after-reference.bgra16"), afterReference));
    QCOMPARE_PIXELS16_WITHIN_STEP(after, afterReference);
    QVERIFY(after != before);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(api.pixelData(0, 0, side, side), before);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(api.pixelData(0, 0, side, side), after);
}
void TestDocument::testIntegerRgba16EditingAndNativeFiles_data()
{
    QTest::addColumn<QString>("mode");
    QTest::newRow("multiply") << QStringLiteral("multiply");
    QTest::newRow("interpolation") << QStringLiteral("interpolation");
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
    QByteArray expected;
    for (const auto [opacity, mask] : {std::pair{255, 15}, std::pair{1, 6}}) {
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
        parameters.srcRowStart = reinterpret_cast<const quint8 *>(topBytes.constData());
        parameters.opacity = float(opacity) / 255; parameters.channelFlags = flags;
        composite->composite(parameters);
        const auto actual = api.pixelData(0, 0, side, side);
        QVERIFY(write(QStringLiteral("projection-o%1-c%2.bgra16").arg(opacity).arg(mask), actual));
        QVERIFY(write(QStringLiteral("reference-o%1-c%2.bgra16").arg(opacity).arg(mask), expected));
        QCOMPARE_PIXELS16_WITHIN_STEP(actual, expected);

    }
    KisPropertiesConfigurationSP saveSettings(new KisPropertiesConfiguration);
    saveSettings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    const auto saved = QDir(output).filePath(QStringLiteral("blend.kra"));
    QVERIFY2(document->saveAs(saved, KisDocument::nativeFormatMimeType(), false, saveSettings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument()); reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(saved, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    Document reopenedApi(reopened.data(), false);
    QCOMPARE_PIXELS16_WITHIN_STEP(reopenedApi.pixelData(0, 0, side, side), expected);
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
        QByteArray encoded;
        appendPixel(encoded, color.blue(), color.green(), color.red(), color.alpha());
        QCOMPARE_PIXELS16_WITHIN_STEP(encoded, expected.mid((y * side + x) * 8, 8));
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
    for (const auto [mask, opacity] : {std::pair{15, 255}, std::pair{6, 1}}) {
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
        QCOMPARE_PIXELS16_WITHIN_STEP(painted, reference);
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
