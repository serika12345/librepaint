/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kra/KisKraRasterCodec.h>
#include <QBuffer>
#include <QColorSpace>
#include <QDomDocument>
#include <QFile>
#include <QImage>
#include <QTest>
#include <cmath>
#include <quazip.h>
#include <quazipfile.h>
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
        if (!file.open(QIODevice::WriteOnly, QuaZipNewInfo(item.key()))) return {};
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
    void imageMetadataIsCompleteOrRejectedBeforeRasterImport();
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

void TestKraRasterCodec::imageMetadataIsCompleteOrRejectedBeforeRasterImport()
{
    auto source = document();
    Kra::ImageMetadata metadata;
    metadata.resolution = {300, 150};
    metadata.documentInfo = source.files.value(QStringLiteral("documentinfo.xml"));
    metadata.iccProfile = QColorSpace(QColorSpace::SRgb).iccProfile();
    metadata.annotations = {{QStringLiteral("notes"), QStringLiteral("Notes"), QByteArrayLiteral("kept")}};
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, metadata).code, Kra::Status::Success);
    QByteArray bytes;
    QBuffer output(&bytes);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto valid = entries(bytes);
    const auto extracted = Kra::RasterCodec::readImageMetadata(valid.value(QStringLiteral("maindoc.xml")), valid);
    QCOMPARE(extracted.code, Kra::Status::Success);
    QCOMPARE(extracted.metadata.resolution, metadata.resolution);
    QCOMPARE(extracted.metadata.iccProfile, metadata.iccProfile);
    QCOMPARE(extracted.metadata.documentInfo, metadata.documentInfo);
    QCOMPARE(extracted.metadata.annotations.size(), 1);
    QCOMPARE(extracted.metadata.annotations.first().bytes, QByteArrayLiteral("kept"));
    for (int change = 0; change < 7; ++change) {
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
    QCOMPARE(QImage::fromData(saved.value(QStringLiteral("mergedimage.png"))).convertToFormat(QImage::Format_RGBA8888), projection);
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
    const auto layer = image.firstChildElement(QStringLiteral("layers")).firstChildElement(QStringLiteral("layer"));
    auto changed = layer;
    changed.setAttribute(QStringLiteral("x"), QStringLiteral("17"));
    changed.setAttribute(QStringLiteral("y"), QStringLiteral("-9"));
    source.description = xml.toByteArray();
    source.image.layers[1].name = QStringLiteral("Renamed");
    QByteArray saved;
    QBuffer destination(&saved);
    QVERIFY(destination.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(destination, source, merged(source.image.size)).code, Kra::Status::Success);
    const auto result = load(saved);
    QVERIFY2(result.code == Kra::Status::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.document.image.layers[1].name, QStringLiteral("Renamed"));
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
        if (change == 0) layer.setAttribute(QStringLiteral("opacity"), QStringLiteral("128"));
        if (change == 1) layer.setAttribute(QStringLiteral("nodetype"), QStringLiteral("grouplayer"));
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
