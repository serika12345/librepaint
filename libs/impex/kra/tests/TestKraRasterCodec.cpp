/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kra/KisKraRasterCodec.h>
#include <QBuffer>
#include <QColorSpace>
#include <QDomDocument>
#include <QDateTime>
#include <QTimeZone>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QTest>
#include <QTemporaryDir>
#include <QScopeGuard>
#include <cmath>
#include <limits>
#include <cstring>
#include <quazip.h>
#include <quazipfile.h>
#include <quazipfileinfo.h>
#include <quazipnewinfo.h>

namespace Kra = LibrePaint::ImportExport::Kra;
using namespace LibrePaint::Image::Gpu;

namespace {
QMap<QString, QByteArray> entries(const QByteArray &bytes)
{
    QBuffer input;
    input.setData(bytes);
    if (!input.open(QIODevice::ReadOnly)) return {};
    QuaZip archive(&input);
    archive.setAutoClose(false);
    if (!archive.open(QuaZip::mdUnzip)) return {};
    QMap<QString, QByteArray> result;
    for (bool more = archive.goToFirstFile(); more; more = archive.goToNextFile()) {
        QuaZipFile file(&archive);
        if (!file.open(QIODevice::ReadOnly)) return {};
        result.insert(archive.getCurrentFileName(), file.readAll());
        file.close();
    }
    archive.close();
    return result;
}

QByteArray pack(const QMap<QString, QByteArray> &files)
{
    QByteArray bytes;
    QBuffer output(&bytes);
    if (!output.open(QIODevice::WriteOnly)) return {};
    QuaZip archive(&output);
    archive.setAutoClose(false);
    if (!archive.open(QuaZip::mdCreate)) return {};
    for (auto item = files.cbegin(); item != files.cend(); ++item) {
        QuaZipFile file(&archive);
        QuaZipNewInfo info(item.key());
        info.dateTime = QDateTime(QDate(1980, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));
        if (!file.open(QIODevice::WriteOnly, info)) return {};
        if (file.write(item.value()) != item.value().size()) return {};
        file.close();
    }
    archive.close();
    return bytes;
}

Kra::LoadOutcome load(const QByteArray &bytes, Kra::Limits limits = {})
{
    QBuffer input;
    input.setData(bytes);
    if (!input.open(QIODevice::ReadOnly)) return {};
    const auto result = Kra::RasterCodec::read(input, limits);
    if (!input.isOpen()) return {};
    return result;
}

QByteArray rgbaAt(const RasterLayer &layer, QPoint point)
{
    for (const auto &tile : layer.tiles) {
        const auto local = point - tile.coordinate * RgbaTile::Size;
        if (local.x() >= 0 && local.y() >= 0 && local.x() < RgbaTile::Size && local.y() < RgbaTile::Size) {
            return tile.rgba.mid((local.y() * RgbaTile::Size + local.x()) * 4, 4);
        }
    }
    QByteArray value;
    for (const auto channel : layer.defaultPixel) value.append(char(channel));
    return value;
}

Kra::Document document()
{
    Kra::Document value;
    value.image.size = {513, 257};
    value.image.activeLayer = 1;
    value.image.layers = {
        {QStringLiteral("Lower red"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")),
         {{{-1, 0}, QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4)},
          {{1, 1}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Hidden blue"), false, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")),
         {{{0, 0}, QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4)}}}
    };
    value.image.layers[0].defaultPixel = {17, 23, 31, 0};
    value.image.layers[1].defaultPixel = {255, 255, 255, 255};
    value.files.insert(QStringLiteral("documentinfo.xml"), QByteArrayLiteral("<document-info><title>Ink</title></document-info>"));
    value.files.insert(QStringLiteral("notes/custom.bin"), QByteArray::fromHex("ff00017f"));
    return value;
}

QImage merged(QSize size)
{
    QImage image(size, QImage::Format_RGBA8888);
    image.fill(QColor(10, 20, 30, 128));
    image.setColorSpace(QColorSpace::SRgb);
    return image;
}
}

class TestKraRasterCodec : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void layerTileEncodingMatchesTheCpuWriter_data();
    void layerTileEncodingMatchesTheCpuWriter();
    void groupedArchivesPreserveHierarchyPixelsAndSelectedLeaf_data();
    void groupedArchivesPreserveHierarchyPixelsAndSelectedLeaf();
    void previewSamplingPreservesEdgesAndWeightedAlpha_data();
    void previewSamplingPreservesEdgesAndWeightedAlpha();
    void previewRowsRejectIncompletePixels();
    void savedPngResolutionComesFromDocumentMetadata_data();
    void savedPngResolutionComesFromDocumentMetadata();
    void archiveMetadataIsIndependentOfClockAndHost();
    void layerOpacityAndLockSurviveEditingAndResaving();
    void channelProtectionSurvivesEditingAndResaving_data();
    void channelProtectionSurvivesEditingAndResaving();
    void passThroughGroupsSurviveEditingAndResaving();
    void layerBlendModesSurviveEditingAndResaving();
    void projectionChannelsSurviveEditingAndResaving_data();
    void projectionChannelsSurviveEditingAndResaving();
    void imageMetadataIsCompleteOrRejectedBeforeRasterImport();
    void documentTitlesMoveAssociatedFilesAtomically();
    void imageProfileIdentitySurvivesMetadataUpdates_data();
    void imageProfileIdentitySurvivesMetadataUpdates();
    void existingPaintFilesPreservePixelsAndDefaults_data();
    void existingPaintFilesPreservePixelsAndDefaults();
    void roundTripRetainsLayersHiddenRgbAndAssociatedFiles();
    void offsetsAndDescriptionMetadataSurviveEditing();
    void unsupportedAndInvalidContentsReturnNoDocument();
    void capacityAndFailedWritesPreserveCallerValues();
    void malformedPixelsAndProfilesAreAtomic_data();
    void malformedPixelsAndProfilesAreAtomic();
    void zipDirectoryCapacityPreservesTheOutput();
    void legacyLayerWithoutUuidGetsOnePersistentIdentity();
    void emptyDocumentsRequireAnExplicitLayerContainer();
};

void TestKraRasterCodec::layerTileEncodingMatchesTheCpuWriter_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QByteArray>("filePixels");
    QTest::addColumn<QByteArray>("expectedStream");
    constexpr int fileTileBytes = 64 * 64 * 4;
    QByteArray pixels(fileTileBytes, '\0');
    pixels.replace(0, 12, QByteArray::fromHex("332211ff000000004d484b80"));
    // CPU KRA save from TestDocument::testSavedKraEmbeddedPngsAgreeAcrossBackends
    // (three-pixels), commit 773b4defee. BGRA pixels above, transparent default,
    // one 64 x 64 tile; the entire encoded stream is the compatibility contract.
    QByteArray expected = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 4\nDATA 1\n0,0,LZF,394\n");
    expected += QByteArray::fromHex(
        "010333004d00e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900"
        "e0f900e0d50002220048e0d5e0e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f9"
        "00e0f900e0f900e0f900030011004be1f905e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900"
        "e0f900e0f900e0f900e0f900e0d60002ff0080e0d6e1e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f900e0f9"
        "00e0f900e0f900e0f900e0f900e0f9002000200020002000200020002000200020002000200020002000200020002000"
        "200020002000200020002000200020002000200020002000200020002000200020002000200020002000200020002000"
        "200020002000200020002000200020002000200020002000200020002000200020002000200020002000200020002000"
        "20002000200020002000200020002000200020001f000000000000000000000000000000000000000000000000000000"
        "00000000000300000000"
    );
    QTest::newRow("compressible") << QSize(3, 1) << pixels << expected;

    quint32 random = 0x13579bdf;
    for (auto &byte : pixels) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        byte = char(random >> 24);
    }
    expected = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 4\nDATA 1\n0,0,LZF,16385\n");
    expected += char(0);
    expected += pixels;
    QTest::newRow("incompressible") << QSize(64, 64) << pixels << expected;
}

void TestKraRasterCodec::layerTileEncodingMatchesTheCpuWriter()
{
    QFETCH(QSize, size);
    QFETCH(QByteArray, filePixels);
    QFETCH(QByteArray, expectedStream);
    RgbaTile tile{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
        const auto source = filePixels.constData() + (y * 64 + x) * 4;
        auto destination = tile.rgba.data() + (y * RgbaTile::Size + x) * 4;
        destination[0] = source[2]; destination[1] = source[1];
        destination[2] = source[0]; destination[3] = source[3];
    }
    Kra::Document value;
    value.image.size = size;
    value.image.layers = {{QStringLiteral("Encoded pixels"), true,
        QUuid(QStringLiteral("{d2000001-1122-3344-5566-778899aabbcc}")), {tile}}};
    QImage projection(size, QImage::Format_RGBA8888);
    projection.setColorSpace(QColorSpace::SRgb);
    for (int y = 0; y < size.height(); ++y)
        std::memcpy(projection.scanLine(y), tile.rgba.constData() + y * RgbaTile::Size * 4, size.width() * 4);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, value, projection).code, Kra::Status::Success);
    const auto files = entries(bytes);
    QDomDocument xml;
    QVERIFY(xml.setContent(files.value(QStringLiteral("maindoc.xml"))));
    const auto image = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    const auto layer = image.firstChildElement(QStringLiteral("layers")).firstChildElement(QStringLiteral("layer"));
    const QString path = image.attribute(QStringLiteral("name")) + QStringLiteral("/layers/") + layer.attribute(QStringLiteral("filename"));
    const auto actual = files.value(path);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "KRA tile stream artifacts retained at" << destination;
        }
    });
    const auto writeArtifact = [&](const QString &suffix, const QByteArray &data) {
        QFile file(QDir(destination).filePath(QStringLiteral("kra-layer-tile-%1-%2")
            .arg(QString::fromLatin1(QTest::currentDataTag()), suffix)));
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
    };
    QVERIFY(writeArtifact(QStringLiteral("actual.tiles"), actual));
    QVERIFY(writeArtifact(QStringLiteral("expected.tiles"), expectedStream));
    QVERIFY(writeArtifact(QStringLiteral("input.bgra"), filePixels));
    QCOMPARE(actual, expectedStream);
    const auto restored = load(bytes);
    QCOMPARE(restored.code, Kra::Status::Success);
    QCOMPARE(restored.document.image.layers.size(), 1);
    QCOMPARE(restored.document.image.layers[0].tiles.size(), 1);
    QCOMPARE(restored.document.image.layers[0].tiles[0].rgba, tile.rgba);
}

void TestKraRasterCodec::groupedArchivesPreserveHierarchyPixelsAndSelectedLeaf_data()
{
    QTest::addColumn<int>("depth");
    QTest::newRow("group") << 1;
    QTest::newRow("nested-group") << 2;
}

void TestKraRasterCodec::groupedArchivesPreserveHierarchyPixelsAndSelectedLeaf()
{
    QFETCH(int, depth);
    const auto original = document();
    QByteArray bytes;
    QBuffer initial(&bytes);
    QVERIFY(initial.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(initial, original, merged(original.image.size)).code, Kra::Status::Success);
    auto files = entries(bytes);
    QDomDocument xml;
    QVERIFY(xml.setContent(files.value(QStringLiteral("maindoc.xml"))));
    auto layers = xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).firstChildElement(QStringLiteral("layers"));
    const QList<QUuid> groups{
        QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")),
        QUuid(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"))};
    for (int level = 0; level < depth; ++level) {
        auto group = layers.firstChildElement(QStringLiteral("layer")).cloneNode(false).toElement();
        group.setAttribute(QStringLiteral("nodetype"), QStringLiteral("grouplayer"));
        group.setAttribute(QStringLiteral("filename"), QStringLiteral("group%1").arg(level));
        group.setAttribute(QStringLiteral("name"), QStringLiteral("Group %1").arg(level));
        group.setAttribute(QStringLiteral("uuid"), groups[level].toString());
        group.setAttribute(QStringLiteral("selected"), QStringLiteral("false"));
        group.setAttribute(QStringLiteral("visible"), QStringLiteral("1"));
        group.setAttribute(QStringLiteral("opacity"), QStringLiteral("128"));
        group.setAttribute(QStringLiteral("colorlabel"), level + 3);
        group.setAttribute(QStringLiteral("passthrough"), QStringLiteral("0"));
        auto children = xml.createElement(QStringLiteral("layers"));
        while (!layers.firstChild().isNull()) children.appendChild(layers.firstChild());
        group.appendChild(children);
        layers.appendChild(group);
    }
    files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
    const auto input = pack(files);
    Kra::Limits tooFewLayers;
    tooFewLayers.layers = original.image.layers.size() + depth - 1;
    QCOMPARE(load(input, tooFewLayers).code, Kra::Status::CapacityExceeded);
    auto conflicting = files;
    QDomDocument duplicate;
    QVERIFY(duplicate.setContent(conflicting.value(QStringLiteral("maindoc.xml"))));
    const auto nodes = duplicate.elementsByTagName(QStringLiteral("layer"));
    nodes.at(nodes.size() - 1).toElement().setAttribute(QStringLiteral("uuid"), groups[depth - 1].toString());
    conflicting[QStringLiteral("maindoc.xml")] = duplicate.toByteArray();
    const auto rejected = load(pack(conflicting));
    QCOMPARE(rejected.code, Kra::Status::InvalidData);
    QVERIFY(rejected.document.image.layers.isEmpty());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile fixture(QDir(results).filePath(QStringLiteral("hierarchy-input-%1.kra").arg(depth)));
        QVERIFY(fixture.open(QIODevice::WriteOnly));
        QCOMPARE(fixture.write(input), input.size());
    }
    auto opened = load(input);
    QVERIFY2(opened.code == Kra::Status::Success, qPrintable(opened.diagnostic));
    QCOMPARE(opened.document.image.layers.size(), original.image.layers.size() + depth);
    const auto selected = original.image.layers[original.image.activeLayer].uuid;
    QCOMPARE(opened.document.image.layers[opened.document.image.activeLayer].uuid, selected);
    for (auto &layer : opened.document.image.layers) {
        if (layer.uuid == original.image.layers[0].uuid) layer.name = QStringLiteral("Edited lower red");
        if (groups.contains(layer.uuid)) {
            QCOMPARE(layer.colorLabelIndex, groups.indexOf(layer.uuid) + 3);
            layer.opacity = 64;
            layer.colorLabelIndex = 7;
        }
    }
    QByteArray saved;
    QBuffer output(&saved);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, opened.document, merged(original.image.size)).code, Kra::Status::Success);
    const auto reopened = load(saved);
    QVERIFY2(reopened.code == Kra::Status::Success, qPrintable(reopened.diagnostic));
    QCOMPARE(reopened.document.image.layers[reopened.document.image.activeLayer].uuid, selected);
    for (const auto &layer : reopened.document.image.layers)
        if (groups.contains(layer.uuid)) QCOMPARE(layer.colorLabelIndex, 7);
    for (const auto &expected : original.image.layers) {
        bool found = false;
        for (const auto &layer : reopened.document.image.layers) {
            if (layer.uuid != expected.uuid) continue;
            found = true;
            QCOMPARE(layer.name, expected.uuid == original.image.layers[0].uuid ? QStringLiteral("Edited lower red") : expected.name);
            QCOMPARE(layer.visible, expected.visible);
            QCOMPARE(layer.defaultPixel, expected.defaultPixel);
            for (const auto &tile : expected.tiles) {
                bool tileFound = false;
                for (const auto &actual : layer.tiles) {
                    if (actual.coordinate != tile.coordinate) continue;
                    tileFound = true;
                    QCOMPARE(actual.rgba, tile.rgba);
                }
                QVERIFY(tileFound);
            }
        }
        QVERIFY(found);
    }
    QVERIFY(xml.setContent(reopened.document.description));
    layers = xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).firstChildElement(QStringLiteral("layers"));
    for (int level = depth - 1; level >= 0; --level) {
        const auto group = layers.firstChildElement(QStringLiteral("layer"));
        QVERIFY(group.nextSiblingElement().isNull());
        QCOMPARE(group.attribute(QStringLiteral("nodetype")), QStringLiteral("grouplayer"));
        QCOMPARE(QUuid(group.attribute(QStringLiteral("uuid"))), groups[level]);
        QCOMPARE(group.attribute(QStringLiteral("opacity")), QStringLiteral("64"));
        layers = group.firstChildElement(QStringLiteral("layers"));
    }
    const auto upper = layers.firstChildElement(QStringLiteral("layer"));
    QCOMPARE(QUuid(upper.attribute(QStringLiteral("uuid"))), original.image.layers[1].uuid);
    QCOMPARE(upper.attribute(QStringLiteral("selected")), QStringLiteral("true"));
    QCOMPARE(QUuid(upper.nextSiblingElement().attribute(QStringLiteral("uuid"))), original.image.layers[0].uuid);
    QCOMPARE(reopened.document.files.value(QStringLiteral("notes/custom.bin")), original.files.value(QStringLiteral("notes/custom.bin")));
    if (!results.isEmpty()) {
        QFile file(QDir(results).filePath(QStringLiteral("hierarchy-output-%1.kra").arg(depth)));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(saved), saved.size());
    }
}

void TestKraRasterCodec::previewSamplingPreservesEdgesAndWeightedAlpha_data()
{
    QTest::addColumn<QSize>("sourceSize");
    QTest::addColumn<QSize>("previewSize");
    QTest::addColumn<QColor>("first");
    QTest::addColumn<QColor>("second");
    QTest::addColumn<QColor>("edgeStart");
    QTest::addColumn<QColor>("middleLeft");
    QTest::addColumn<QColor>("middleRight");
    QTest::addColumn<QColor>("edgeEnd");
    const QColor red(255, 0, 0, 255), blue(0, 0, 255, 255), hiddenBlue(0, 0, 255, 0);
    const QColor purple(128, 0, 128, 255), fadedRed(255, 0, 0, 128);
    QTest::newRow("opaque-upscale") << QSize(2, 1) << QSize(256, 128) << red << blue
        << red << QColor(128, 0, 127, 255) << QColor(127, 0, 128, 255) << blue;
    QTest::newRow("portrait-upscale") << QSize(1, 2) << QSize(128, 256) << red << blue
        << red << QColor(128, 0, 127, 255) << QColor(127, 0, 128, 255) << blue;
    QTest::newRow("transparent-upscale") << QSize(2, 1) << QSize(256, 128) << red << hiddenBlue
        << red << fadedRed << QColor(255, 0, 0, 127) << QColor(0, 0, 0, 0);
    QTest::newRow("opaque-area-reduction") << QSize(512, 2) << QSize(256, 1) << red << blue
        << purple << purple << purple << purple;
    QTest::newRow("transparent-area-reduction") << QSize(512, 2) << QSize(256, 1) << red << hiddenBlue
        << fadedRed << fadedRed << fadedRed << fadedRed;
    QTest::newRow("thin-canvas") << QSize(512, 1) << QSize(256, 1) << red << blue
        << purple << purple << purple << purple;
    const QColor dot(17, 33, 81, 127);
    QTest::newRow("one-pixel") << QSize(1, 1) << QSize(256, 256) << dot << dot
        << dot << dot << dot << dot;
}

void TestKraRasterCodec::previewSamplingPreservesEdgesAndWeightedAlpha()
{
    QFETCH(QSize, sourceSize);
    QFETCH(QSize, previewSize);
    QFETCH(QColor, first);
    QFETCH(QColor, second);
    QFETCH(QColor, edgeStart);
    QFETCH(QColor, middleLeft);
    QFETCH(QColor, middleRight);
    QFETCH(QColor, edgeEnd);
    const bool vertical = sourceSize.width() == 1 && sourceSize.height() > 1;
    QImage projection(sourceSize, QImage::Format_RGBA8888);
    projection.setColorSpace(QColorSpace::SRgb);
    for (int y = 0; y < sourceSize.height(); ++y)
        for (int x = 0; x < sourceSize.width(); ++x)
            projection.setPixelColor(x, y, ((vertical ? y : x) % 2) ? second : first);
    Kra::Document source;
    source.image.size = sourceSize;
    RasterLayer layer;
    layer.name = QStringLiteral("Preview samples");
    layer.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}"));
    for (int tileX = 0; tileX * RgbaTile::Size < sourceSize.width(); ++tileX) {
        RgbaTile tile{{tileX, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
        const int copied = qMin(RgbaTile::Size, sourceSize.width() - tileX * RgbaTile::Size) * 4;
        for (int y = 0; y < sourceSize.height(); ++y)
            std::memcpy(tile.rgba.data() + y * RgbaTile::Size * 4,
                        projection.constScanLine(y) + tileX * RgbaTile::Size * 4, copied);
        layer.tiles.append(tile);
    }
    source.image.layers.append(layer);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    const auto result = Kra::RasterCodec::write(output, source, projection);
    QVERIFY2(result.code == Kra::Status::Success, qPrintable(result.diagnostic));
    const auto saved = entries(bytes);
    const auto mergedPixels = QImage::fromData(saved.value(QStringLiteral("mergedimage.png"))).convertToFormat(QImage::Format_RGBA8888);
    QCOMPARE(mergedPixels.size(), projection.size());
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mergedPixels.constBits()), mergedPixels.sizeInBytes()),
             QByteArray(reinterpret_cast<const char *>(projection.constBits()), projection.sizeInBytes()));
    const auto preview = QImage::fromData(saved.value(QStringLiteral("preview.png")));
    QCOMPARE(preview.size(), previewSize);
    const auto at = [&](int index) {
        return vertical ? preview.pixelColor(preview.width() / 2, index)
                        : preview.pixelColor(index, preview.height() / 2);
    };
    QCOMPARE(at(0), edgeStart);
    QCOMPARE(at(127), middleLeft);
    QCOMPARE(at(128), middleRight);
    QCOMPARE(at(255), edgeEnd);
    qsizetype rowsRead = 0;
    const auto streamed = Kra::RasterCodec::createPreview(sourceSize, [&](int y) {
        ++rowsRead;
        return QByteArray(reinterpret_cast<const char *>(projection.constScanLine(y)), sourceSize.width() * 4);
    });
    const auto decoded = preview.convertToFormat(QImage::Format_RGBA8888);
    QCOMPARE(streamed.size(), decoded.size());
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(streamed.constBits()), streamed.sizeInBytes()),
             QByteArray(reinterpret_cast<const char *>(decoded.constBits()), decoded.sizeInBytes()));
    QVERIFY(rowsRead <= sourceSize.height() + 2 * previewSize.height());
    const auto restored = load(bytes);
    QCOMPARE(restored.code, Kra::Status::Success);
    QCOMPARE(rgbaAt(restored.document.image.layers.first(), QPoint()), QByteArray(reinterpret_cast<const char *>(projection.constBits()), 4));
}

void TestKraRasterCodec::previewRowsRejectIncompletePixels()
{
    QVERIFY(Kra::RasterCodec::createPreview({2, 2}, [](int y) { return QByteArray(y ? 7 : 8, '\0'); }).isNull());
    bool called = false;
    QVERIFY(Kra::RasterCodec::createPreview({RgbaTile::CoordinateLimit + 1, 1}, [&](int) {
        called = true;
        return QByteArray();
    }).isNull());
    QVERIFY(!called);
}

void TestKraRasterCodec::savedPngResolutionComesFromDocumentMetadata_data()
{
    QTest::addColumn<QPointF>("resolution");
    QTest::addColumn<QSize>("density");
    QTest::newRow("document-resolution") << QPointF(300, 150) << QSize(11811, 5906);
    QTest::newRow("png-density-range") << QPointF(1e-300, 1e300) << QSize(1, std::numeric_limits<int>::max());
}

void TestKraRasterCodec::savedPngResolutionComesFromDocumentMetadata()
{
    QFETCH(QPointF, resolution);
    QFETCH(QSize, density);
    auto source = document();
    Kra::ImageMetadata metadata;
    metadata.resolution = resolution;
    metadata.iccProfile = QColorSpace(QColorSpace::SRgb).iccProfile();
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, metadata).code, Kra::Status::Success);
    QList<QByteArray> archives;
    for (int hostDensity : {2835, 3780}) {
        auto projection = merged(source.image.size);
        projection.setDotsPerMeterX(hostDensity);
        projection.setDotsPerMeterY(hostDensity);
        QByteArray bytes;
        QBuffer output(&bytes);
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(Kra::RasterCodec::write(output, source, projection).code, Kra::Status::Success);
        const auto saved = entries(bytes);
        for (const auto &name : {QStringLiteral("mergedimage.png"), QStringLiteral("preview.png")}) {
            const auto image = QImage::fromData(saved.value(name));
            QVERIFY(!image.isNull());
            QCOMPARE(image.dotsPerMeterX(), density.width());
            QCOMPARE(image.dotsPerMeterY(), density.height());
        }
        archives.append(bytes);
    }
    QCOMPARE(archives[0], archives[1]);
}

void TestKraRasterCodec::archiveMetadataIsIndependentOfClockAndHost()
{
    const auto source = document();
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    QBuffer input(&bytes);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QuaZip archive(&input);
    archive.setAutoClose(false);
    QVERIFY(archive.open(QuaZip::mdUnzip));
    int count = 0;
    for (bool more = archive.goToFirstFile(); more; more = archive.goToNextFile()) {
        QuaZipFileInfo64 info;
        QVERIFY(archive.getCurrentFileInfo(&info));
        QCOMPARE(info.dateTime.date(), QDate(1980, 1, 1));
        QCOMPARE(info.dateTime.time(), QTime(0, 0));
        QCOMPARE(info.versionCreated >> 8, 3);
        ++count;
    }
    QVERIFY(count > 0);
}

void TestKraRasterCodec::layerOpacityAndLockSurviveEditingAndResaving()
{
    auto source = document();
    source.image.layers[0].opacity = 128;
    source.image.projectionBackground = {17, 34, 51, 128};
    source.image.layers[0].userLocked = true;
    source.image.layers[0].pinnedToTimeline = true;
    source.image.layers[0].colorLabelIndex = 5;
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto imported = load(bytes);
    QVERIFY2(imported.code == Kra::Status::Success, qPrintable(imported.diagnostic));
    QCOMPARE(imported.document.image.projectionBackground, source.image.projectionBackground);
    QCOMPARE(imported.document.image.layers[0].opacity, quint8(128));
    QVERIFY(imported.document.image.layers[0].userLocked);
    QVERIFY(imported.document.image.layers[0].pinnedToTimeline);
    QCOMPARE(imported.document.image.layers[0].colorLabelIndex, 5);
    auto edited = imported.document;
    edited.image.layers[0].opacity = 64;
    edited.image.layers[0].userLocked = false;
    edited.image.layers[0].colorLabelIndex = 0;
    QByteArray saved;
    QBuffer destination(&saved);
    QVERIFY(destination.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(destination, edited, merged(edited.image.size)).code, Kra::Status::Success);
    const auto reopened = load(saved);
    QCOMPARE(reopened.code, Kra::Status::Success);
    QCOMPARE(reopened.document.image.projectionBackground, source.image.projectionBackground);
    QCOMPARE(reopened.document.image.layers[0].opacity, quint8(64));
    QVERIFY(!reopened.document.image.layers[0].userLocked);
    QVERIFY(reopened.document.image.layers[0].pinnedToTimeline);
    QCOMPARE(reopened.document.image.layers[0].colorLabelIndex, 0);
    QCOMPARE(reopened.document.image.layers[0].tiles.first().rgba, source.image.layers[0].tiles.first().rgba);
}

void TestKraRasterCodec::channelProtectionSurvivesEditingAndResaving_data()
{
    QTest::addColumn<quint8>("mask");
    QTest::addColumn<QString>("serializedFlags");
    QTest::newRow("all-writable") << quint8(15) << QString();
    QTest::newRow("alpha-protected") << quint8(7) << QStringLiteral("1110");
    QTest::newRow("red-only") << quint8(1) << QStringLiteral("0010");
    QTest::newRow("alpha-only") << quint8(8) << QStringLiteral("0001");
    QTest::newRow("all-protected") << quint8(0) << QStringLiteral("0000");
}

void TestKraRasterCodec::channelProtectionSurvivesEditingAndResaving()
{
    QFETCH(quint8, mask);
    QFETCH(QString, serializedFlags);
    auto source = document();
    source.image.layers[0].channelWriteMask = mask;
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto imported = load(bytes);
    QVERIFY2(imported.code == Kra::Status::Success, qPrintable(imported.diagnostic));
    QCOMPARE(imported.document.image.layers[0].channelWriteMask, mask);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("channel-protection-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kra")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), bytes.size());
    }
    QDomDocument xml;
    QVERIFY(xml.setContent(entries(bytes).value(QStringLiteral("maindoc.xml"))));
    const auto layers = xml.elementsByTagName(QStringLiteral("layer"));
    for (int i = 0; i < layers.size(); ++i) {
        const auto element = layers.item(i).toElement();
        if (element.attribute(QStringLiteral("uuid")) == source.image.layers[0].uuid.toString())
            QCOMPARE(element.attribute(QStringLiteral("channellockflags")), serializedFlags);
    }
    QByteArray saved;
    QBuffer destination(&saved);
    QVERIFY(destination.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(destination, imported.document, merged(source.image.size)).code, Kra::Status::Success);
    const auto reopened = load(saved);
    QCOMPARE(reopened.code, Kra::Status::Success);
    QCOMPARE(reopened.document.image.layers[0].channelWriteMask, mask);
    QCOMPARE(reopened.document.image.layers[0].tiles.first().rgba, source.image.layers[0].tiles.first().rgba);
    auto unlocked = reopened.document;
    unlocked.image.layers[0].channelWriteMask = 15;
    QByteArray cleared;
    QBuffer clearedOutput(&cleared);
    QVERIFY(clearedOutput.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(clearedOutput, unlocked, merged(source.image.size)).code, Kra::Status::Success);
    const auto clearedDocument = load(cleared);
    QCOMPARE(clearedDocument.code, Kra::Status::Success);
    QCOMPARE(clearedDocument.document.image.layers[0].channelWriteMask, quint8(15));
}

void TestKraRasterCodec::layerBlendModesSurviveEditingAndResaving()
{
    for (auto mode : {LayerBlendMode::Multiply, LayerBlendMode::Screen, LayerBlendMode::Darken, LayerBlendMode::Lighten}) {
        auto source = document();
        RasterLayer group{QStringLiteral("Group"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}};
        group.kind = LayerKind::Group;
        group.blendMode = mode;
        source.image.layers[0].parentUuid = group.uuid;
        source.image.layers[0].blendMode = mode;
        source.image.layers.append(group);
        QByteArray bytes;
        QBuffer output(&bytes);
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
        const auto imported = load(bytes);
        QCOMPARE(imported.code, Kra::Status::Success);
        for (const auto &expected : source.image.layers) {
            const auto found = std::find_if(imported.document.image.layers.cbegin(), imported.document.image.layers.cend(),
                [&](const RasterLayer &layer) { return layer.uuid == expected.uuid; });
            QVERIFY(found != imported.document.image.layers.cend());
            QCOMPARE(found->blendMode, expected.blendMode);
        }
        auto edited = imported.document;
        for (auto &layer : edited.image.layers) layer.blendMode = LayerBlendMode::Normal;
        QByteArray saved;
        QBuffer destination(&saved);
        QVERIFY(destination.open(QIODevice::WriteOnly));
        QCOMPARE(Kra::RasterCodec::write(destination, edited, merged(source.image.size)).code, Kra::Status::Success);
        const auto reopened = load(saved);
        QCOMPARE(reopened.code, Kra::Status::Success);
        for (const auto &layer : reopened.document.image.layers) QCOMPARE(layer.blendMode, LayerBlendMode::Normal);
        edited.image.layers[0].blendMode = LayerBlendMode(5);
        const auto retained = saved;
        QCOMPARE(Kra::RasterCodec::write(destination, edited, merged(source.image.size)).code, Kra::Status::InvalidData);
        QCOMPARE(saved, retained);
    }
}

void TestKraRasterCodec::passThroughGroupsSurviveEditingAndResaving()
{
    auto source = document();
    RasterLayer group{QStringLiteral("Group"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}};
    group.kind = LayerKind::Group;
    group.passThrough = true;
    source.image.layers[0].parentUuid = group.uuid;
    source.image.layers.append(group);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto imported = load(bytes);
    QVERIFY2(imported.code == Kra::Status::Success, qPrintable(imported.diagnostic));
    QVERIFY(imported.document.image.layers.last().passThrough);
    const RasterLayer *child = nullptr;
    for (const auto &layer : imported.document.image.layers)
        if (layer.uuid == source.image.layers[0].uuid) child = &layer;
    QVERIFY(child);
    QCOMPARE(child->parentUuid, group.uuid);
    QCOMPARE(child->tiles.first().rgba, source.image.layers[0].tiles.first().rgba);
    auto edited = imported.document;
    edited.image.layers.last().passThrough = false;
    QByteArray saved;
    QBuffer destination(&saved);
    QVERIFY(destination.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(destination, edited, merged(source.image.size)).code, Kra::Status::Success);
    const auto reopened = load(saved);
    QCOMPARE(reopened.code, Kra::Status::Success);
    QVERIFY(!reopened.document.image.layers.last().passThrough);
    QDomDocument xml;
    QVERIFY(xml.setContent(entries(bytes).value(QStringLiteral("maindoc.xml"))));
    const auto elements = xml.elementsByTagName(QStringLiteral("layer"));
    for (int index = 0; index < elements.size(); ++index) {
        auto element = elements.item(index).toElement();
        const auto old = element.attribute(QStringLiteral("passthrough"));
        element.setAttribute(QStringLiteral("passthrough"), element.attribute(QStringLiteral("nodetype")) == QLatin1String("grouplayer") ? QStringLiteral("2") : QStringLiteral("1"));
        auto files = entries(bytes);
        files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
        const auto invalid = load(pack(files));
        QCOMPARE(invalid.code, Kra::Status::InvalidData);
        QVERIFY(invalid.document.image.layers.isEmpty());
        if (old.isEmpty()) element.removeAttribute(QStringLiteral("passthrough"));
        else element.setAttribute(QStringLiteral("passthrough"), old);
    }
}

void TestKraRasterCodec::projectionChannelsSurviveEditingAndResaving_data()
{
    QTest::addColumn<quint8>("mask");
    QTest::addColumn<QString>("serializedFlags");
    const char *flags[] = {"0000", "0010", "0100", "0110", "1000", "1010", "1100", "1110",
                           "0001", "0011", "0101", "0111", "1001", "1011", "1101", ""};
    for (int mask = 0; mask < 16; ++mask)
        QTest::newRow(QByteArray::number(mask, 16).constData()) << quint8(mask) << QString::fromLatin1(flags[mask]);
}

void TestKraRasterCodec::projectionChannelsSurviveEditingAndResaving()
{
    QFETCH(quint8, mask);
    QFETCH(QString, serializedFlags);
    auto source = document();
    RasterLayer group{QStringLiteral("Group"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}};
    group.kind = LayerKind::Group;
    source.image.layers.append(group);
    for (auto &layer : source.image.layers) layer.projectionChannelMask = mask;
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto imported = load(bytes);
    QVERIFY2(imported.code == Kra::Status::Success, qPrintable(imported.diagnostic));
    for (const auto &layer : imported.document.image.layers) QCOMPARE(layer.projectionChannelMask, mask);
    QCOMPARE(imported.document.image.layers[0].tiles.first().rgba, source.image.layers[0].tiles.first().rgba);
    QDomDocument xml;
    QVERIFY(xml.setContent(entries(bytes).value(QStringLiteral("maindoc.xml"))));
    const auto layers = xml.elementsByTagName(QStringLiteral("layer"));
    QCOMPARE(layers.size(), 3);
    for (int i = 0; i < layers.size(); ++i)
        QCOMPARE(layers.item(i).toElement().attribute(QStringLiteral("channelflags")), serializedFlags);
    auto edited = imported.document;
    edited.image.layers[0].projectionChannelMask = 15;
    QByteArray saved;
    QBuffer destination(&saved);
    QVERIFY(destination.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(destination, edited, merged(source.image.size)).code, Kra::Status::Success);
    const auto reopened = load(saved);
    QCOMPARE(reopened.code, Kra::Status::Success);
    QCOMPARE(reopened.document.image.layers[0].projectionChannelMask, quint8(15));
    QCOMPARE(reopened.document.image.layers[1].projectionChannelMask, mask);
    QCOMPARE(reopened.document.image.layers[2].projectionChannelMask, mask);
    for (const auto bad : {QStringLiteral("10"), QStringLiteral("1112"), QStringLiteral("11111")}) {
        auto files = entries(bytes);
        layers.item(0).toElement().setAttribute(QStringLiteral("channelflags"), bad);
        files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
        const auto rejected = load(pack(files));
        QCOMPARE(rejected.code, Kra::Status::InvalidData);
        QVERIFY(rejected.document.image.layers.isEmpty());
    }
    auto invalid = source;
    invalid.image.layers[0].projectionChannelMask = 16;
    QByteArray rejected;
    QBuffer rejectedOutput(&rejected);
    QVERIFY(rejectedOutput.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(rejectedOutput, invalid, merged(source.image.size)).code, Kra::Status::InvalidData);
}

void TestKraRasterCodec::imageProfileIdentitySurvivesMetadataUpdates_data()
{
    QTest::addColumn<bool>("retained");
    QTest::newRow("new-description") << false;
    QTest::newRow("retained-description") << true;
}

void TestKraRasterCodec::imageProfileIdentitySurvivesMetadataUpdates()
{
    QFETCH(bool, retained);
    auto source = document();
    if (retained) {
        QByteArray bytes;
        QBuffer output(&bytes);
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
        const auto opened = load(bytes);
        QCOMPARE(opened.code, Kra::Status::Success);
        source = opened.document;
    }
    Kra::ImageMetadata metadata;
    metadata.iccProfile = QColorSpace(QColorSpace::SRgb).iccProfile();
    metadata.profileName = QStringLiteral("sRGB <&> 名前");
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, metadata).code, Kra::Status::Success);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto opened = load(bytes);
    QCOMPARE(opened.code, Kra::Status::Success);
    QDomDocument xml;
    QVERIFY(xml.setContent(opened.document.description));
    QCOMPARE(xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).attribute(QStringLiteral("profile")), metadata.profileName);
    const auto extracted = Kra::RasterCodec::readImageMetadata(opened.document.description, opened.document.files);
    QCOMPARE(extracted.code, Kra::Status::Success);
    QCOMPARE(extracted.metadata.profileName, metadata.profileName);
    QCOMPARE(extracted.metadata.iccProfile, metadata.iccProfile);
    const auto name = metadata.profileName;
    metadata.profileName.clear();
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, metadata).code, Kra::Status::Success);
    QCOMPARE(Kra::RasterCodec::readImageMetadata(source.description, source.files).metadata.profileName, name);
    const auto unchanged = source;
    const QStringList invalidNames{QString(QChar::Null), QString(QChar(0xd800)), QString(65537, QLatin1Char('x'))};
    for (const auto &invalid : invalidNames) {
        metadata.profileName = invalid;
        QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, metadata).code, Kra::Status::InvalidData);
        QCOMPARE(source.description, unchanged.description);
        QCOMPARE(source.files, unchanged.files);
    }
}

void TestKraRasterCodec::documentTitlesMoveAssociatedFilesAtomically()
{
    auto source = document();
    QByteArray initial;
    QBuffer output(&initial);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto opened = load(initial);
    QCOMPARE(opened.code, Kra::Status::Success);
    source = opened.document;
    source.files.insert(QStringLiteral("image/extra.bin"), QByteArrayLiteral("retained associated bytes"));
    const auto originalFiles = source.files;
    Kra::ImageMetadata metadata;
    metadata.iccProfile = QColorSpace(QColorSpace::SRgb).iccProfile();
    metadata.annotations = {{QStringLiteral("project"), QStringLiteral("Kept annotation"), QByteArrayLiteral("annotation bytes")}};
    const auto updateTitle = [&](const QString &title) {
        QDomDocument info;
        auto root = info.createElement(QStringLiteral("document-info"));
        info.appendChild(root);
        auto about = info.createElement(QStringLiteral("about"));
        root.appendChild(about);
        auto name = info.createElement(QStringLiteral("title"));
        name.appendChild(info.createTextNode(title));
        about.appendChild(name);
        metadata.documentInfo = info.toByteArray();
        return Kra::RasterCodec::updateImageMetadata(source, metadata);
    };
    const QString title = QStringLiteral("保存 <&> title");
    QCOMPARE(updateTitle(title).code, Kra::Status::Success);
    QDomDocument xml;
    QVERIFY(xml.setContent(source.description));
    QCOMPARE(xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).attribute(QStringLiteral("name")), title);
    for (auto entry = originalFiles.cbegin(); entry != originalFiles.cend(); ++entry) {
        if (!entry.key().startsWith(QLatin1String("image/"))) continue;
        QVERIFY(!source.files.contains(entry.key()));
        QCOMPARE(source.files.value(title + entry.key().mid(5)), entry.value());
    }
    QCOMPARE(source.files.value(title + QStringLiteral("/annotations/project")), QByteArrayLiteral("annotation bytes"));
    QCOMPARE(source.files.value(QStringLiteral("notes/custom.bin")), originalFiles.value(QStringLiteral("notes/custom.bin")));
    QCOMPARE(updateTitle(QStringLiteral("Renamed document")).code, Kra::Status::Success);
    for (const auto &name : source.files.keys()) QVERIFY(!name.startsWith(title + QLatin1Char('/')));
    for (const auto &unsafe : {QStringLiteral("../outside"), QStringLiteral("with/slash"), QStringLiteral("with\\backslash"), QStringLiteral(".."), QString(), QString(1010, QLatin1Char('x'))}) {
        QCOMPARE(updateTitle(unsafe).code, Kra::Status::Success);
        QVERIFY(xml.setContent(source.description));
        QCOMPARE(xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).attribute(QStringLiteral("name")), QStringLiteral("Renamed document"));
        QCOMPARE(source.files.value(QStringLiteral("documentinfo.xml")), metadata.documentInfo);
    }
    QByteArray saved;
    QBuffer savedOutput(&saved);
    QVERIFY(savedOutput.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(savedOutput, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto restored = load(saved);
    QCOMPARE(restored.code, Kra::Status::Success);
    QCOMPARE(restored.document.image.layers.size(), source.image.layers.size());
    for (int i = 0; i < source.image.layers.size(); ++i) {
        QCOMPARE(restored.document.image.layers[i].uuid, source.image.layers[i].uuid);
        QCOMPARE(restored.document.image.layers[i].defaultPixel, source.image.layers[i].defaultPixel);
        const auto &expected = source.image.layers[i].tiles;
        const auto &actual = restored.document.image.layers[i].tiles;
        QCOMPARE(actual.size(), expected.size());
        for (int tile = 0; tile < expected.size(); ++tile) {
            QCOMPARE(actual[tile].coordinate, expected[tile].coordinate);
            QCOMPARE(actual[tile].rgba, expected[tile].rgba);
        }
    }
    QCOMPARE(restored.document.files.value(QStringLiteral("Renamed document/extra.bin")), QByteArrayLiteral("retained associated bytes"));
    const auto restoredMetadata = Kra::RasterCodec::readImageMetadata(restored.document.description, restored.document.files);
    QCOMPARE(restoredMetadata.code, Kra::Status::Success);
    QCOMPARE(restoredMetadata.metadata.iccProfile, metadata.iccProfile);
    QCOMPARE(restoredMetadata.metadata.annotations.first().bytes, metadata.annotations.first().bytes);
    const auto beforeCollision = source;
    for (const auto &name : {QStringLiteral("Collision/extra.bin"), QStringLiteral("Collision/annotations/unrelated")}) {
        source = beforeCollision;
        source.files.insert(name, QByteArrayLiteral("unrelated caller data"));
        const auto unchanged = source;
        QCOMPARE(updateTitle(QStringLiteral("Collision")).code, Kra::Status::InvalidData);
        QCOMPARE(source.description, unchanged.description);
        QCOMPARE(source.files, unchanged.files);
    }
}

void TestKraRasterCodec::imageMetadataIsCompleteOrRejectedBeforeRasterImport()
{
    auto source = document();
    Kra::ImageMetadata metadata;
    metadata.resolution = {300, 150};
    QDomDocument axes;
    QVERIFY(axes.setContent(QByteArrayLiteral(
        "<MirrorAxis>"
        "<mirrorHorizontal type=\"value\" value=\"1\"/>"
        "<mirrorVertical type=\"value\" value=\"1\"/>"
        "<lockHorizontal type=\"value\" value=\"1\"/>"
        "<lockVertical type=\"value\" value=\"0\"/>"
        "<hideHorizontalDecoration type=\"value\" value=\"0\"/>"
        "<hideVerticalDecoration type=\"value\" value=\"1\"/>"
        "<handleSize type=\"value\" value=\"19.5\"/>"
        "<horizontalHandlePosition type=\"value\" value=\"8.25\"/>"
        "<verticalHandlePosition type=\"value\" value=\"-4.5\"/>"
        "<axisPosition type=\"pointf\" x=\"21.25\" y=\"13.5\"/>"
        "</MirrorAxis>"
    )));
    metadata.mirrorAxis = axes.toByteArray();
    metadata.documentInfo = source.files.value(QStringLiteral("documentinfo.xml"));
    metadata.iccProfile = QColorSpace(QColorSpace::SRgb).iccProfile();
    metadata.annotations = {{QStringLiteral("notes"), QStringLiteral("Notes"), QByteArrayLiteral("kept")}};
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, metadata).code, Kra::Status::Success);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("mirror-axis-metadata.kra")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), bytes.size());
    }
    const auto valid = entries(bytes);
    const auto extracted = Kra::RasterCodec::readImageMetadata(valid.value(QStringLiteral("maindoc.xml")), valid);
    QCOMPARE(extracted.code, Kra::Status::Success);
    QCOMPARE(extracted.metadata.resolution, metadata.resolution);
    QCOMPARE(extracted.metadata.iccProfile, metadata.iccProfile);
    QCOMPARE(extracted.metadata.documentInfo, metadata.documentInfo);
    for (const auto &name : {QStringLiteral("mergedimage.png"), QStringLiteral("preview.png")})
        QVERIFY(!QImage::fromData(valid.value(name)).colorSpace().isValid());
    QCOMPARE(extracted.metadata.mirrorAxis, metadata.mirrorAxis);
    QCOMPARE(extracted.metadata.annotations.size(), 1);
    QCOMPARE(extracted.metadata.annotations.first().bytes, QByteArrayLiteral("kept"));
    for (int change = 0; change < 14; ++change) {
        auto files = valid;
        QDomDocument xml;
        QVERIFY(xml.setContent(files.value(QStringLiteral("maindoc.xml"))));
        auto image = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
        auto annotations = image.firstChildElement(QStringLiteral("Annotations"));
        auto annotation = annotations.firstChildElement();
        const auto prefix = image.attribute(QStringLiteral("name")) + QStringLiteral("/annotations/");
        if (change == 0) image.setAttribute(QStringLiteral("x-res"), QStringLiteral("nan"));
        if (change == 1) image.setAttribute(QStringLiteral("y-res"), QStringLiteral("0"));
        if (change == 2) files[QStringLiteral("documentinfo.xml")] = QByteArrayLiteral("<invalid/>");
        if (change == 3) files.remove(prefix + QStringLiteral("notes"));
        if (change == 4) annotations.appendChild(annotation.cloneNode(true));
        if (change == 5) annotation.setAttribute(QStringLiteral("type"), QStringLiteral("../outside"));
        if (change == 6) image.appendChild(annotations.cloneNode(true));
        auto firstLayer = image.firstChildElement(QStringLiteral("layers")).firstChildElement();
        if (change == 7) firstLayer.setAttribute(QStringLiteral("opacity"), QStringLiteral("256"));
        if (change == 8) firstLayer.setAttribute(QStringLiteral("locked"), QStringLiteral("2"));
        auto background = image.firstChildElement(QStringLiteral("ProjectionBackgroundColor"));
        if (change == 9) background.setAttribute(QStringLiteral("ColorData"), QStringLiteral("%%%"));
        if (change == 10) background.setAttribute(QStringLiteral("ColorData"), QStringLiteral("AA=="));
        if (change == 11) image.appendChild(background.cloneNode(true));
        if (change == 12) firstLayer.setAttribute(QStringLiteral("intimeline"), QStringLiteral("2"));
        if (change == 13) image.appendChild(image.firstChildElement(QStringLiteral("MirrorAxis")).cloneNode(true));
        files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
        const auto rejected = Kra::RasterCodec::readImageMetadata(files.value(QStringLiteral("maindoc.xml")), files);
        QCOMPARE(rejected.code, Kra::Status::InvalidData);
        QVERIFY(!rejected.diagnostic.isEmpty());
        QVERIFY(rejected.metadata.iccProfile.isEmpty());
        QVERIFY(rejected.metadata.annotations.isEmpty());
        const auto imported = load(pack(files));
        QCOMPARE(imported.code, Kra::Status::InvalidData);
        QVERIFY(imported.document.image.layers.isEmpty());
    }
    auto cleared = extracted.metadata;
    cleared.mirrorAxis.clear();
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, cleared).code, Kra::Status::Success);
    const auto withoutAxes = Kra::RasterCodec::readImageMetadata(source.description, source.files);
    QCOMPARE(withoutAxes.code, Kra::Status::Success);
    QVERIFY(withoutAxes.metadata.mirrorAxis.isEmpty());
    const auto unchanged = source;
    cleared.mirrorAxis = QByteArrayLiteral("<invalid/>");
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, cleared).code, Kra::Status::InvalidData);
    QCOMPARE(source.description, unchanged.description);
    QCOMPARE(source.files, unchanged.files);

}

void TestKraRasterCodec::emptyDocumentsRequireAnExplicitLayerContainer()
{
    Kra::Document source;
    source.image.size = QSize(32, 24);
    QImage projection(source.image.size, QImage::Format_RGBA8888);
    projection.fill(Qt::transparent);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, projection).code, Kra::Status::Success);
    const auto imported = load(bytes);
    QCOMPARE(imported.code, Kra::Status::Success);
    QCOMPARE(imported.document.image.size, source.image.size);
    QVERIFY(imported.document.image.layers.isEmpty());
    QCOMPARE(imported.document.image.activeLayer, 0);
    for (const bool duplicate : {false, true}) {
        auto files = entries(bytes);
        QDomDocument xml;
        QVERIFY(xml.setContent(files[QStringLiteral("maindoc.xml")]));
        auto image = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
        const auto layers = image.firstChildElement(QStringLiteral("layers"));
        if (duplicate) image.appendChild(layers.cloneNode(true));
        else image.removeChild(layers);
        files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
        QCOMPARE(load(pack(files)).code, Kra::Status::InvalidData);
    }
}

void TestKraRasterCodec::existingPaintFilesPreservePixelsAndDefaults_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QByteArray>("defaultRgba");
    QTest::newRow("transparent-projection") << QStringLiteral("single_layer_no_channel_flags_transp_def_pixel.kra") << QByteArray::fromHex("ffffffff");
    QTest::newRow("opaque-default") << QStringLiteral("single_layer_no_channel_flags_nontransp_def_pixel.kra") << QByteArray::fromHex("ffffffff");
}

void TestKraRasterCodec::existingPaintFilesPreservePixelsAndDefaults()
{
    QFETCH(QString, filename);
    QFETCH(QByteArray, defaultRgba);
    QFile file(QStringLiteral(KRA_COMPATIBILITY_DATA_DIR) + QLatin1Char('/') + filename);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto bytes = file.readAll();
    const auto result = load(bytes);
    QVERIFY2(result.code == Kra::Status::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.document.image.size, QSize(100, 100));
    QCOMPARE(result.document.image.layers.size(), 1);
    const auto &layer = result.document.image.layers.first();
    QCOMPARE(layer.name, QStringLiteral("Layer 1"));
    QCOMPARE(layer.uuid, QUuid(QStringLiteral("{405c189d-df09-476e-a202-a9480b57c3e6}")));
    QCOMPARE(rgbaAt(layer, {-1000, -1000}), defaultRgba);
    const auto original = entries(bytes);
    const auto expected = QImage::fromData(original.value(QStringLiteral("mergedimage.png"))).convertToFormat(QImage::Format_RGBA8888);
    QVERIFY(expected.width() >= 100 && expected.height() >= 100);
    // The other fixture's saved preview differs from its layer samples. Its
    // authoritative layer pixels are compared with the existing KRA reader.
    if (filename.contains(QStringLiteral("nontransp"))) {
        for (int y = 0; y < 100; ++y) {
            for (int x = 0; x < 100; ++x) {
                QCOMPARE(rgbaAt(layer, {x, y}), QByteArray(reinterpret_cast<const char *>(expected.constScanLine(y) + x * 4), 4));
            }
        }
    }
    QCOMPARE(result.document.files.value(QStringLiteral("unnamed/annotations/icc")), original.value(QStringLiteral("unnamed/annotations/icc")));
    OperationDocument imported(result.document.image);
    QCOMPARE(imported.undoCount(), 0);
    QVERIFY(!imported.isModified());
}

void TestKraRasterCodec::roundTripRetainsLayersHiddenRgbAndAssociatedFiles()
{
    const auto source = document();
    const auto projection = merged(source.image.size);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    const auto written = Kra::RasterCodec::write(output, source, projection);
    QVERIFY2(written.code == Kra::Status::Success, qPrintable(written.diagnostic));
    QVERIFY(output.isOpen());
    const auto result = load(bytes);
    QVERIFY2(result.code == Kra::Status::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.document.image.size, source.image.size);
    QCOMPARE(result.document.image.activeLayer, source.image.activeLayer);
    QCOMPARE(result.document.image.layers.size(), 2);
    for (int i = 0; i < 2; ++i) {
        const auto &before = source.image.layers[i];
        const auto &after = result.document.image.layers[i];
        QCOMPARE(after.name, before.name);
        QCOMPARE(after.visible, before.visible);
        QCOMPARE(after.uuid, before.uuid);
        QCOMPARE(after.defaultPixel, before.defaultPixel);
        for (const auto &tile : before.tiles) {
            QCOMPARE(rgbaAt(after, tile.coordinate * 256), tile.rgba.first(4));
            QCOMPARE(rgbaAt(after, tile.coordinate * 256 + QPoint(255, 255)), tile.rgba.last(4));
        }
    }
    for (auto item = source.files.cbegin(); item != source.files.cend(); ++item) QCOMPARE(result.document.files.value(item.key()), item.value());
    const auto saved = entries(bytes);
    QCOMPARE(saved.value(QStringLiteral("mimetype")), QByteArrayLiteral("application/x-krita"));
    auto storedProjection = projection;
    storedProjection.setColorSpace({});
    QCOMPARE(QImage::fromData(saved.value(QStringLiteral("mergedimage.png"))).convertToFormat(QImage::Format_RGBA8888), storedProjection);
    QCOMPARE(document().image.layers[0].tiles[0].rgba, source.image.layers[0].tiles[0].rgba);
}

void TestKraRasterCodec::offsetsAndDescriptionMetadataSurviveEditing()
{
    const auto initial = document();
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, initial, merged(initial.image.size)).code, Kra::Status::Success);
    auto source = load(bytes).document;
    QDomDocument xml;
    QVERIFY(xml.setContent(source.description));
    auto image = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    image.setAttribute(QStringLiteral("x-res"), QStringLiteral("144"));
    image.setAttribute(QStringLiteral("description"), QStringLiteral("Retained metadata"));
    source.description = xml.toByteArray();
    source.image.layers[1].position = {17, -9};
    source.image.layers[1].name = QStringLiteral("Renamed");
    QByteArray saved;
    QBuffer destination(&saved);
    QVERIFY(destination.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(destination, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto result = load(saved);
    QVERIFY2(result.code == Kra::Status::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.document.image.layers[1].name, QStringLiteral("Renamed"));
    QCOMPARE(result.document.image.layers[1].position, QPoint(17, -9));
    QCOMPARE(rgbaAt(result.document.image.layers[1], {0, 0}), QByteArray::fromHex("0000ff80"));
    QCOMPARE(rgbaAt(result.document.image.layers[1], {255, 255}), QByteArray::fromHex("0000ff80"));
    QVERIFY(xml.setContent(result.document.description));
    image = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    QCOMPARE(image.attribute(QStringLiteral("x-res")), QStringLiteral("144"));
    QCOMPARE(image.attribute(QStringLiteral("description")), QStringLiteral("Retained metadata"));
    QCOMPARE(image.firstChildElement(QStringLiteral("layers")).firstChildElement(QStringLiteral("layer")).attribute(QStringLiteral("x")), QStringLiteral("17"));
}

void TestKraRasterCodec::unsupportedAndInvalidContentsReturnNoDocument()
{
    QFile fixture(QStringLiteral(KRA_COMPATIBILITY_DATA_DIR) + QStringLiteral("/single_layer_no_channel_flags_transp_def_pixel.kra"));
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    const auto valid = entries(fixture.readAll());
    for (int change = 0; change < 5; ++change) {
        auto files = valid;
        QDomDocument xml;
        QVERIFY(xml.setContent(files.value(QStringLiteral("maindoc.xml"))));
        auto image = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
        auto layer = image.firstChildElement(QStringLiteral("layers")).firstChildElement(QStringLiteral("layer"));
        if (change == 0) layer.setAttribute(QStringLiteral("compositeop"), QStringLiteral("overlay"));
        if (change == 1) layer.setAttribute(QStringLiteral("nodetype"), QStringLiteral("unsupportedlayer"));
        if (change == 2) image.setAttribute(QStringLiteral("colorspacename"), QStringLiteral("LABA"));
        if (change == 3) layer.setAttribute(QStringLiteral("filename"), QStringLiteral("../outside"));
        if (change == 4) files[QStringLiteral("unnamed/layers/layer2.defaultpixel")] = QByteArray(3, '\0');
        files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
        if (change < 4) {
            QCOMPARE(Kra::RasterCodec::validateMetadata(files.value(QStringLiteral("maindoc.xml")), files).code,
                     change < 3 ? Kra::Status::UnsupportedData : Kra::Status::InvalidData);
        }
        const auto result = load(pack(files));
        QCOMPARE(result.code, change < 3 ? Kra::Status::UnsupportedData : Kra::Status::InvalidData);
        QVERIFY(!result.diagnostic.isEmpty());
        QVERIFY(result.document.image.layers.isEmpty());
    }
}

void TestKraRasterCodec::capacityAndFailedWritesPreserveCallerValues()
{
    const auto source = document();
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    Kra::Limits limits;
    limits.archiveBytes = 64;
    const auto tooLarge = load(bytes, limits);
    QCOMPARE(tooLarge.code, Kra::Status::CapacityExceeded);
    QVERIFY(tooLarge.document.image.layers.isEmpty());
    QByteArray untouched = QByteArrayLiteral("original bytes");
    QBuffer rejected(&untouched);
    QVERIFY(rejected.open(QIODevice::ReadWrite));
    auto invalid = source;
    invalid.image.layers[1].uuid = invalid.image.layers[0].uuid;
    QCOMPARE(Kra::RasterCodec::write(rejected, invalid, merged(invalid.image.size)).code, Kra::Status::InvalidData);
    QCOMPARE(untouched, QByteArrayLiteral("original bytes"));
    QCOMPARE(Kra::RasterCodec::write(rejected, source, merged({1, 1})).code, Kra::Status::InvalidData);
    QCOMPARE(untouched, QByteArrayLiteral("original bytes"));
    QBuffer readOnly;
    readOnly.setData(QByteArrayLiteral("original bytes"));
    QVERIFY(readOnly.open(QIODevice::ReadOnly));
    QCOMPARE(Kra::RasterCodec::write(readOnly, source, merged(source.image.size)).code, Kra::Status::IoFailure);
    QCOMPARE(readOnly.data(), QByteArrayLiteral("original bytes"));
    QCOMPARE(source.image.layers[0].tiles[0].rgba, document().image.layers[0].tiles[0].rgba);
    auto foreignProjection = merged(source.image.size);
    foreignProjection.setColorSpace(QColorSpace::DisplayP3);
    QCOMPARE(Kra::RasterCodec::write(rejected, source, foreignProjection).code, Kra::Status::UnsupportedData);
    QCOMPARE(untouched, QByteArrayLiteral("original bytes"));
    limits = {};
    limits.archiveBytes = 256;
    QCOMPARE(Kra::RasterCodec::write(rejected, source, merged(source.image.size), limits).code, Kra::Status::CapacityExceeded);
    QCOMPARE(untouched, QByteArrayLiteral("original bytes"));
    QCOMPARE(Kra::RasterCodec::write(rejected, source, merged(source.image.size)).code, Kra::Status::IoFailure);
    QCOMPARE(untouched, QByteArrayLiteral("original bytes"));
}

void TestKraRasterCodec::malformedPixelsAndProfilesAreAtomic_data()
{
    QTest::addColumn<int>("change");
    QTest::addColumn<Kra::Status>("status");
    QTest::newRow("truncated-compressed-pixels") << 0 << Kra::Status::InvalidData;
    QTest::newRow("tile-stream-trailer") << 1 << Kra::Status::InvalidData;
    QTest::newRow("duplicate-tile") << 2 << Kra::Status::InvalidData;
    QTest::newRow("invalid-profile") << 3 << Kra::Status::InvalidData;
    QTest::newRow("foreign-profile") << 4 << Kra::Status::UnsupportedData;
    QTest::newRow("unsafe-archive-path") << 5 << Kra::Status::InvalidData;
    QTest::newRow("expanded-raster-capacity") << 6 << Kra::Status::CapacityExceeded;
    QTest::newRow("xml-capacity") << 7 << Kra::Status::CapacityExceeded;
    QTest::newRow("non-srgb-table-curve") << 8 << Kra::Status::UnsupportedData;
    QTest::newRow("invalid-color-label") << 9 << Kra::Status::InvalidData;
}

void TestKraRasterCodec::malformedPixelsAndProfilesAreAtomic()
{
    QFETCH(int, change);
    QFETCH(Kra::Status, status);
    QFile fixture(QStringLiteral(KRA_COMPATIBILITY_DATA_DIR) + QStringLiteral("/single_layer_no_channel_flags_transp_def_pixel.kra"));
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    auto files = entries(fixture.readAll());
    const auto header = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 4\nDATA ");
    const auto raw = QByteArrayLiteral("0,0,LZF,16385\n") + char(0) + QByteArray(16384, '\0');
    const auto path = QStringLiteral("unnamed/layers/layer2");
    if (change == 0) files[path] = header + QByteArrayLiteral("1\n0,0,LZF,2\n") + QByteArray::fromHex("0100");
    if (change == 1) files[path] += 'x';
    if (change == 2) files[path] = header + QByteArrayLiteral("2\n") + raw + raw;
    if (change == 3) files[QStringLiteral("unnamed/annotations/icc")] = QByteArrayLiteral("invalid profile");
    if (change == 4) files[QStringLiteral("unnamed/annotations/icc")] = QColorSpace(QColorSpace::DisplayP3).iccProfile();
    if (change == 5) files[QStringLiteral("../outside")] = QByteArrayLiteral("data");
    if (change == 8) {
        QList<uint16_t> curve;
        for (int value = 0; value < 256; ++value)
            curve.append(qRound(std::pow(value / 255.0, 2.2) * 65535));
        files[QStringLiteral("unnamed/annotations/icc")] = QColorSpace(QColorSpace::Primaries::SRgb, curve).iccProfile();
    }
    if (change == 9) {
        QDomDocument xml;
        QVERIFY(xml.setContent(files.value(QStringLiteral("maindoc.xml"))));
        xml.elementsByTagName(QStringLiteral("layer")).at(0).toElement().setAttribute(QStringLiteral("colorlabel"), QStringLiteral("invalid"));
        files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
    }
    Kra::Limits limits;
    if (change == 6) limits.rasterBytes = 1;
    if (change == 7) limits.xmlBytes = 16;
    const auto result = load(pack(files), limits);
    QCOMPARE(result.code, status);
    QVERIFY(!result.diagnostic.isEmpty());
    QVERIFY(result.document.image.layers.isEmpty());
    QVERIFY(result.document.files.isEmpty());
}

void TestKraRasterCodec::zipDirectoryCapacityPreservesTheOutput()
{
    Kra::Document source;
    source.image.size = {1, 1};
    source.image.layers = {{QStringLiteral("Layer"), true, QUuid::createUuid(), {}}};
    for (int i = 0; i < 256; ++i) source.files.insert(QStringLiteral("empty/%1").arg(i), {});
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged({1, 1})).code, Kra::Status::Success);
    qsizetype expanded = 0;
    for (const auto &entry : entries(bytes)) expanded += entry.size();
    QVERIFY(bytes.size() > expanded);
    Kra::Limits limits;
    limits.archiveBytes = expanded;
    QByteArray untouched;
    QBuffer rejected(&untouched);
    QVERIFY(rejected.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(rejected, source, merged({1, 1}), limits).code, Kra::Status::CapacityExceeded);
    QVERIFY(untouched.isEmpty());
}

void TestKraRasterCodec::legacyLayerWithoutUuidGetsOnePersistentIdentity()
{
    QFile fixture(QStringLiteral(KRA_COMPATIBILITY_DATA_DIR) + QStringLiteral("/single_layer_no_channel_flags_transp_def_pixel.kra"));
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    auto files = entries(fixture.readAll());
    QDomDocument xml;
    QVERIFY(xml.setContent(files.value(QStringLiteral("maindoc.xml"))));
    xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).firstChildElement(QStringLiteral("layers"))
        .firstChildElement(QStringLiteral("layer")).removeAttribute(QStringLiteral("uuid"));
    files[QStringLiteral("maindoc.xml")] = xml.toByteArray();
    const auto imported = load(pack(files));
    QVERIFY2(imported.code == Kra::Status::Success, qPrintable(imported.diagnostic));
    const auto uuid = imported.document.image.layers.first().uuid;
    QVERIFY(!uuid.isNull());
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    const auto saved = Kra::RasterCodec::write(output, imported.document, merged(imported.document.image.size));
    QVERIFY2(saved.code == Kra::Status::Success, qPrintable(saved.diagnostic));
    const auto reopened = load(bytes);
    QVERIFY2(reopened.code == Kra::Status::Success, qPrintable(reopened.diagnostic));
    QCOMPARE(reopened.document.image.layers.first().uuid, uuid);
}

QTEST_GUILESS_MAIN(TestKraRasterCodec)
#include "TestKraRasterCodec.moc"
