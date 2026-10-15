/*
 * SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_png_test.h"


#include <simpletest.h>
#include <QCoreApplication>
#include <QBuffer>
#include <QPointer>
#include <kis_png_converter.h>
#include <KisExportRasterRows.h>
#include <cstring>
#include <thread>

#include <KoColorProfileQuery.h>
#include "filestest.h"

#include <testui.h>

#ifndef FILES_DATA_DIR
#error "FILES_DATA_DIR not set. A directory with the data used for testing the importing of files in krita"
#endif


const QString PngMimetype = "image/png";

namespace {
class CancelPngBuffer : public QBuffer
{
public:
    KisPNGConverter *converter = nullptr;
    bool cancelled = false;
    int rejectedPhase = -1;
protected:
    qint64 writeData(const char *data, qint64 size) override
    {
        if (rejectedPhase == 0 || (size >= 8 &&
            ((rejectedPhase == 1 && std::memcmp(data + 4, "IDAT", 4) == 0) ||
             (rejectedPhase == 2 && std::memcmp(data + 4, "IEND", 4) == 0)))) return -1;
        const auto written = QBuffer::writeData(data, size);
        if (!cancelled && converter && size >= 8 && std::memcmp(data + 4, "IDAT", 4) == 0) {
            cancelled = true;
            std::thread([this] { converter->cancel(); }).join();
        }
        return written;
    }
};
}

void KisPngTest::testWriteCancellation_data()
{
    QTest::addColumn<bool>("beforeWrite");
    QTest::addColumn<bool>("interlaced");
    QTest::newRow("before") << true << false;
    QTest::newRow("before-interlaced") << true << true;
    QTest::newRow("during") << false << false;
    QTest::newRow("during-interlaced") << false << true;
}

void KisPngTest::testWriteCancellation()
{
    QFETCH(bool, beforeWrite);
    QFETCH(bool, interlaced);
    // Public codec callers must be able to abandon encoding before committing
    // their destination. Fixed xorshift input produces multiple IDAT chunks.
    const QSize size(128, 256);
    QByteArray pixels(size.width() * size.height() * 4, '\0');
    QImage expected(size, QImage::Format_ARGB32);
    quint32 state = 0x12345678;
    for (int y = 0; y < size.height(); ++y) {
        for (int x = 0; x < size.width(); ++x) {
            const auto offset = (y * size.width() + x) * 4;
            for (int c = 0; c < 3; ++c) {
                state ^= state << 13; state ^= state >> 17; state ^= state << 5;
                pixels[offset + c] = char(state & 255);
            }
            pixels[offset + 3] = char(255);
            expected.setPixel(x, y, qRgba(quint8(pixels[offset + 2]), quint8(pixels[offset + 1]),
                                        quint8(pixels[offset]), 255));
        }
    }
    KisPaintDeviceSP device = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    device->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), QRect(QPoint(), size));
    vKisAnnotationSP annotations;
    KisPNGOptions options;
    options.compression = 3;
    options.alpha = true;
    options.tryToSaveAsIndexed = false;
    options.interlace = interlaced;
    QBuffer reference;
    QVERIFY(reference.open(QIODevice::WriteOnly));
    KisPNGConverter complete(nullptr, true);
    QVERIFY(complete.buildFile(&reference, QRect(QPoint(), size), 1, 1, device,
                               annotations.begin(), annotations.end(), options, nullptr).isOk());
    QCOMPARE(QImage::fromData(reference.data()).convertToFormat(QImage::Format_ARGB32), expected);
    CancelPngBuffer output;
    QVERIFY(output.open(QIODevice::WriteOnly));
    KisPNGConverter converter(nullptr, true);
    if (beforeWrite) std::thread([&] { converter.cancel(); }).join();
    else output.converter = &converter;
    const auto result = converter.buildFile(&output, QRect(QPoint(), size), 1, 1, device,
                                            annotations.begin(), annotations.end(), options, nullptr);
    QVERIFY2(result.isCancelled(), qPrintable(result.errorMessage()));
    if (beforeWrite) QVERIFY(output.data().isEmpty());
    else {
        QVERIFY(output.cancelled);
        QVERIFY(output.data().size() < reference.data().size());
    }
}

void KisPngTest::testRowEncodingPreservesPixels_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<bool>("interlaced");
    const QList<QByteArray> names{"rgba8", "rgba16", "graya8", "graya16", "indexed"};
    for (int format = 0; format < names.size(); ++format) {
        for (const bool interlaced : {false, true}) {
            const QByteArray name = names[format] + (interlaced ? "-interlaced" : "-ordinary");
            QTest::newRow(name.constData()) << format << interlaced;
        }
    }
}

void KisPngTest::testRowEncodingPreservesPixels()
{
    QFETCH(int, format);
    QFETCH(bool, interlaced);
    const bool gray = format == 2 || format == 3;
    const bool sixteen = format == 1 || format == 3;
    const auto cs = KoColorSpaceRegistry::instance()->colorSpace(
        gray ? GrayAColorModelID.id() : RGBAColorModelID.id(),
        sixteen ? Integer16BitsColorDepthID.id() : Integer8BitsColorDepthID.id(), nullptr);
    QVERIFY(cs);
    KisPaintDeviceSP device = new KisPaintDevice(cs);
    const QRect region(2, 3, 33, 17);
    for (int y = region.top(); y <= region.bottom(); ++y) {
        for (int x = region.left(); x <= region.right(); ++x) {
            QByteArray pixel(cs->pixelSize(), '\0');
            const auto index = (x + 2 * y) % 8;
            const auto channels = cs->channelCount() - 1;
            if (sixteen) {
                auto *values = reinterpret_cast<quint16 *>(pixel.data());
                for (int c = 0; c < channels; ++c) values[c] = quint16(0x1234 + index * 37 + c * 1234);
                values[channels] = 65535;
            } else {
                for (int c = 0; c < channels; ++c) pixel[c] = char((index * (31 + c * 17)) & 255);
                pixel[channels] = char(255);
            }
            device->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), x, y, 1, 1);
        }
    }
    KisPNGOptions options;
    options.alpha = format != 4;
    options.tryToSaveAsIndexed = format == 4;
    options.saveSRGBProfile = true;
    options.interlace = interlaced;
    options.compression = 3;
    vKisAnnotationSP annotations;
    QBuffer encoded;
    QVERIFY(encoded.open(QIODevice::ReadWrite));
    KisPNGConverter writer(nullptr, true);
    QVERIFY(writer.buildFile(&encoded, region, 1, 1, device, annotations.begin(), annotations.end(), options, nullptr).isOk());
    KisExportRasterRows rows;
    rows.colorSpace = cs;
    rows.size = region.size();
    rows.readRow = [device, region, cs](int row, QByteArray &bytes) {
        bytes.resize(qsizetype(region.width()) * cs->pixelSize());
        device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region.x(), region.y() + row, region.width(), 1);
        return KisImportExportErrorCode(ImportExportCodes::OK);
    };
    QBuffer streamed;
    QVERIFY(streamed.open(QIODevice::WriteOnly));
    KisPNGConverter rowWriter(nullptr, true);
    QVERIFY(rowWriter.buildFile(&streamed, rows, 1, 1, annotations.begin(), annotations.end(), options, nullptr).isOk());
    QCOMPARE(streamed.data(), encoded.data());
    QVERIFY(encoded.seek(0));
    KisPNGConverter reader(nullptr, true);
    const auto result = reader.buildImage(&encoded);
    QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
    QCOMPARE(reader.image()->colorSpace()->id(), cs->id());
    const auto count = region.width() * region.height() * cs->pixelSize();
    QByteArray expected(count, '\0');
    QByteArray actual(count, '\0');
    device->readBytes(reinterpret_cast<quint8 *>(expected.data()), region);
    reader.image()->projection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), QRect(QPoint(), region.size()));
    QCOMPARE(actual, expected);
}

void KisPngTest::testRowEncodingPreservesColorOptions_data()
{
    QTest::addColumn<int>("conversion");
    QTest::addColumn<bool>("alpha");
    QTest::addColumn<bool>("interlaced");
    for (int conversion = 0; conversion < 4; ++conversion) {
        for (bool alpha : {false, true}) {
            for (bool interlaced : {false, true}) {
                const QByteArray name = QByteArray::number(conversion) + (alpha ? "-alpha" : "-background") +
                                  (interlaced ? "-interlaced" : "-ordinary");
                QTest::newRow(name.constData()) << conversion << alpha << interlaced;
            }
        }
    }
}

void KisPngTest::testRowEncodingPreservesColorOptions()
{
    QFETCH(int, conversion);
    QFETCH(bool, alpha);
    QFETCH(bool, interlaced);
    auto *registry = KoColorSpaceRegistry::instance();
    const auto cs = conversion >= 2 ? registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), registry->p709G10Profile()) : registry->rgb8();
    QVERIFY(cs);
    KisPaintDeviceSP device = new KisPaintDevice(cs);
    const QRect region(2, 3, 33, 17);
    for (int y = region.top(); y <= region.bottom(); ++y) {
        for (int x = region.left(); x <= region.right(); ++x) {
            KoColor pixel(QColor((x * 17) % 256, (y * 29) % 256, (x * y) % 256, ((x + y) % 4) * 85), cs);
            device->writeBytes(pixel.data(), x, y, 1, 1);
        }
    }
    KisPNGOptions options;
    options.alpha = alpha;
    options.interlace = interlaced;
    options.forceSRGB = conversion == 1;
    options.saveAsHDR = conversion == 2;
    options.downsample = conversion == 3;
    options.tryToSaveAsIndexed = false;
    options.saveSRGBProfile = true;
    options.transparencyFillColor = QColor(93, 171, 47);
    options.compression = 3;
    vKisAnnotationSP annotations;
    QBuffer reference;
    QVERIFY(reference.open(QIODevice::WriteOnly));
    KisPNGConverter writer(nullptr, true);
    auto result = writer.buildFile(&reference, region, 1, 1, device, annotations.begin(), annotations.end(), options, nullptr);
    QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
    KisExportRasterRows rows;
    rows.colorSpace = cs;
    rows.size = region.size();
    rows.readRow = [device, region, cs](int row, QByteArray &bytes) {
        bytes.resize(qsizetype(region.width()) * cs->pixelSize());
        device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region.x(), region.y() + row, region.width(), 1);
        return KisImportExportErrorCode(ImportExportCodes::OK);
    };
    QBuffer actual;
    QVERIFY(actual.open(QIODevice::WriteOnly));
    KisPNGConverter rowWriter(nullptr, true);
    result = rowWriter.buildFile(&actual, rows, 1, 1, annotations.begin(), annotations.end(), options, nullptr);
    QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
    QCOMPARE(actual.data(), reference.data());
}

void KisPngTest::testRowReadFailureStopsEncoding_data()
{
    QTest::addColumn<int>("failure");
    QTest::newRow("cancelled-source") << 0;
    QTest::newRow("failed-source") << 1;
    QTest::newRow("incomplete-row") << 2;
}

void KisPngTest::testRowReadFailureStopsEncoding()
{
    QFETCH(int, failure);
    KisExportRasterRows rows;
    rows.colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    rows.size = QSize(33, 17);
    int requestedRows = 0;
    rows.readRow = [&](int row, QByteArray &bytes) {
        ++requestedRows;
        bytes.fill('\0', 33 * 4);
        if (row != 3) return KisImportExportErrorCode(ImportExportCodes::OK);
        if (failure == 2) { bytes.chop(1); return KisImportExportErrorCode(ImportExportCodes::OK); }
        return KisImportExportErrorCode(failure == 0 ? ImportExportCodes::Cancelled : ImportExportCodes::Failure);
    };
    KisPNGOptions options;
    options.alpha = true;
    options.tryToSaveAsIndexed = false;
    vKisAnnotationSP annotations;
    QBuffer encoded;
    QVERIFY(encoded.open(QIODevice::WriteOnly));
    KisPNGConverter writer(nullptr, true);
    const auto result = writer.buildFile(&encoded, rows, 1, 1, annotations.begin(), annotations.end(), options, nullptr);
    QVERIFY(!result.isOk());
    QCOMPARE(result.isCancelled(), failure == 0);
    QCOMPARE(requestedRows, 4);
    QVERIFY(!encoded.data().endsWith(QByteArray::fromHex("0000000049454e44ae426082")));
}

void KisPngTest::testWriteFailureReleasesSourcePixels_data()
{
    QTest::addColumn<int>("rejectedPhase");
    QTest::newRow("header") << 0;
    QTest::newRow("pixels") << 1;
    QTest::newRow("footer") << 2;
}

void KisPngTest::testWriteFailureReleasesSourcePixels()
{
    QFETCH(int, rejectedPhase);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP device = new KisPaintDevice(cs);
    QPointer<KisPaintDevice> lifetime(device.data());
    const QRect region(0, 0, 128, 256);
    device->fill(region, KoColor(QColor(17, 129, 241), cs));
    KisPNGOptions options;
    options.alpha = true;
    options.tryToSaveAsIndexed = false;
    options.compression = 0;
    vKisAnnotationSP annotations;
    CancelPngBuffer output;
    output.rejectedPhase = rejectedPhase;
    QVERIFY(output.open(QIODevice::WriteOnly));
    KisPNGConverter writer(nullptr, true);
    const auto failed = writer.buildFile(&output, region, 1, 1, device,
                                         annotations.begin(), annotations.end(), options, nullptr);
    QVERIFY(!failed.isOk());
    QVERIFY(!failed.isCancelled());
    const auto edited = QByteArray::fromHex("332211ff");
    device->writeBytes(reinterpret_cast<const quint8 *>(edited.constData()), 5, 5, 1, 1);
    QBuffer retry;
    QVERIFY(retry.open(QIODevice::WriteOnly));
    QVERIFY(writer.buildFile(&retry, region, 1, 1, device, annotations.begin(), annotations.end(), options, nullptr).isOk());
    QCOMPARE(QImage::fromData(retry.data()).pixelColor(5, 5), QColor(17, 34, 51));
    device.clear();
    QVERIFY(!lifetime);
}

void KisPngTest::testFiles()
{
    // Consumer: people opening standard PNG files in Krita.
    // Operation: import the PNG-suite corpus through the PNG filter.
    // Observable result: each imported image matches its recorded result.
    // Failure impact: users see incorrectly decoded pixels or cannot open a PNG.
    TestUtil::testFiles(QString(FILES_DATA_DIR) + "/sources", QStringList(), QString(), 1);
}

void KisPngTest::testWriteonly()
{
    // Consumer: people importing a PNG from a file that cannot be read.
    // Operation: start a PNG import with a write-only source file.
    // Observable result: the import returns an error instead of a successful document.
    // Failure impact: callers continue with a nonexistent or incomplete imported image.
    TestUtil::testImportFromWriteonly(PngMimetype);
}

void roudTripHdrImage(const KoColorSpace *savingColorSpace)
{
    const KoColorSpace * scRGBF32 =
        KoColorSpaceRegistry::instance()->colorSpace(
            RGBAColorModelID.id(),
            Float32BitsColorDepthID.id(),
            KoColorSpaceRegistry::instance()->p709G10Profile());

    KoColor fillColor(scRGBF32);
    float *pixelPtr = reinterpret_cast<float*>(fillColor.data());

    pixelPtr[0] = 2.7;
    pixelPtr[1] = 1.6;
    pixelPtr[2] = 0.8;
    pixelPtr[3] = 0.9;

    {
        QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());

        KisImageSP image = new KisImage(0, 3, 3, scRGBF32, "png test");
        KisPaintLayerSP paintLayer0 = new KisPaintLayer(image, "paint0", OPACITY_OPAQUE_U8);
        paintLayer0->paintDevice()->fill(image->bounds(), fillColor);
        image->addNode(paintLayer0, image->root());

        // convert image color space before saving
        image->convertImageColorSpace(savingColorSpace, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
        image->waitForDone();

        KisImportExportManager manager(doc.data());
        doc->setFileBatchMode(true);
        doc->setCurrentImage(image);

        KisPropertiesConfigurationSP exportConfiguration = new KisPropertiesConfiguration();
        exportConfiguration->setProperty("saveAsHDR", true);
        exportConfiguration->setProperty("saveSRGBProfile", false);
        exportConfiguration->setProperty("forceSRGB", false);
        doc->exportDocumentSync("test.png", "image/png", exportConfiguration);
    }

    {
        QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
        KisImportExportManager manager(doc.data());
        doc->setFileBatchMode(true);

        KisImportExportErrorCode loadingStatus =
            manager.importDocument("test.png", QString());

        QVERIFY(loadingStatus.isOk());

        KisImageSP image = doc->image();
        image->initialRefreshGraph();

        KoColor resultColor;

//        qDebug() << ppVar(image->colorSpace()) << image->colorSpace()->profile()->name();
//        image->projection()->pixel(1, 1, &resultColor);
//        qDebug() << ppVar(resultColor);

        image->convertImageColorSpace(scRGBF32, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
        image->waitForDone();

        image->projection()->pixel(1, 1, &resultColor);
//        qDebug() << ppVar(resultColor);

        const float tolerance = savingColorSpace->colorDepthId() == Integer8BitsColorDepthID ? 0.02 : 0.01;
        bool resultIsValid = true;
        float *resultPtr = reinterpret_cast<float*>(resultColor.data());
        for (int i = 0; i < 4; i++) {
            resultIsValid &= qAbs(resultPtr[i] - pixelPtr[i]) < tolerance;
        }

        if (!resultIsValid) {
            qDebug() << ppVar(fillColor) << ppVar(resultColor);
        }
        QVERIFY(resultIsValid);
    }
}

void KisPngTest::testSaveHDR()
{
    // Consumer: artists saving HDR PNG documents.
    // Operation: export and import HDR pixels across supported depth/profile pairs.
    // Observable result: the imported pixel values remain within the depth tolerance.
    // Failure impact: saved HDR artwork loses highlight, color, or alpha information.
    QVector<KoID> colorDepthIds;
#ifdef HAVE_OPENEXR
    colorDepthIds << Float16BitsColorDepthID;
#endif
    colorDepthIds << Float32BitsColorDepthID;

    QVector<const KoColorProfile*> profiles;
    const KoColorProfile *profile = KoColorSpaceRegistry::instance()->p709G10Profile();
    if (!profile) {
        qWarning() << "Could not get a p709G10 Profile";
    }
    else {
        profiles << profile;
    }
    profile = KoColorSpaceRegistry::instance()->p2020G10Profile();
    if (!profile) {
        qWarning() << "Could not get a p2020G10 Profile";
    }
    else {
        profiles << profile;
    }
    profile = KoColorSpaceRegistry::instance()->p2020PQProfile();;
    if (!profile) {
        qWarning() << "Could not get a p2020PQ Profile";
    }
    else {
        profiles << profile;
    }

    Q_FOREACH(const KoID &depth, colorDepthIds) {
        Q_FOREACH(const KoColorProfile *profile, profiles) {
            if (profile) {
                roudTripHdrImage(
                    KoColorSpaceRegistry::instance()->colorSpace(
                                RGBAColorModelID.id(),
                                depth.id(),
                                profile));
            }
        }
    }

    roudTripHdrImage(
        KoColorSpaceRegistry::instance()->colorSpace(
                    RGBAColorModelID.id(),
                    Integer16BitsColorDepthID.id(),
                    KoColorSpaceRegistry::instance()->p2020PQProfile()));

    roudTripHdrImage(
        KoColorSpaceRegistry::instance()->colorSpace(
                    RGBAColorModelID.id(),
                    Integer8BitsColorDepthID.id(),
                    KoColorSpaceRegistry::instance()->p2020PQProfile()));
}

Q_DECLARE_METATYPE(ColorPrimaries)
Q_DECLARE_METATYPE(TransferCharacteristics)

void KisPngTest::testRoundtripCicpIccProfile_data()
{
    QTest::addColumn<ColorPrimaries>("primaries");
    QTest::addColumn<TransferCharacteristics>("transfer");

    QTest::addRow("rec2020") << PRIMARIES_ITU_R_BT_2020_2_AND_2100_0 << TRC_ITU_R_BT_2020_2_12bit;
    QTest::addRow("rec2100 PQ") << PRIMARIES_ITU_R_BT_2020_2_AND_2100_0 << TRC_ITU_R_BT_2100_0_PQ;
    QTest::addRow("rec709") << PRIMARIES_ITU_R_BT_709_5 << TRC_ITU_R_BT_709_5;
}

void KisPngTest::testRoundtripCicpIccProfile()
{
    // Consumer: artists preserving PNG color-management information.
    // Operation: export and import a PNG with CICP color metadata.
    // Observable result: the imported profile reports the original primaries and transfer curve.
    // Failure impact: a reopened PNG is interpreted in the wrong color space.
    QFETCH(ColorPrimaries, primaries);
    QFETCH(TransferCharacteristics, transfer);
    QVector<double> colorants;
    const KoColorProfile *profile = KoColorSpaceRegistry::instance()->profileFor(KoColorProfileQuery(primaries, transfer));

    QRect imageRect(0,0,512,512);
    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), profile);

    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    KisImageSP image = new KisImage(new KisSurrogateUndoStore(), imageRect.width(), imageRect.height(), cs, "test image");
    doc->setCurrentImage(image);
    const QString name = "roundtrip_cicp_test_"+QString::number(primaries)+"_"+QString::number(transfer)+".png";

    KisPropertiesConfigurationSP exportConfiguration = new KisPropertiesConfiguration();
    exportConfiguration->setProperty("saveSRGBProfile", false);
    exportConfiguration->setProperty("forceSRGB", false);
    doc->exportDocumentSync(name, "image/png", exportConfiguration);

    //---//

    QScopedPointer<KisDocument> doc2(KisPart::instance()->createDocument());
    KisImportExportManager manager(doc2.data());
    doc2->setFileBatchMode(true);

    KisImportExportErrorCode loadingStatus =
        manager.importDocument(name, QString());

    QVERIFY(loadingStatus.isOk());

    KisImageSP image2 = doc2->image();
    image2->initialRefreshGraph();
    image2->waitForDone();
    QVERIFY(image2->colorSpace()->profile()->getColorPrimaries() == primaries);
    QVERIFY(image2->colorSpace()->profile()->getTransferCharacteristics() == transfer);
}

void KisPngTest::testLoadPngWithLegacyProfile()
{
    // Consumer: artists opening PNG files containing Krita's legacy HDR profile.
    // Operation: import the legacy-profile fixture.
    // Observable result: the image uses the matching PQ profile and its CICP values.
    // Failure impact: the reopened HDR image is interpreted with incorrect color data.
    const QString pngFilePath = TestUtil::fetchDataFileLazy("test-png-with-legacy-hdr-profile.png");
    KIS_ASSERT(QFile::exists(pngFilePath));

    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    KisImportExportManager manager(doc.data());
    doc->setFileBatchMode(true);

    KisImportExportErrorCode loadingStatus =
        manager.importDocument(pngFilePath, QString());

    QVERIFY(loadingStatus.isOk());

    /**
     * A PNG with the legacy profile should use the default Krita profille
     * for a PQ space with all the cicp information set properly. The embedded
     * profile should be ignored.
     */
    const KoColorProfile *profile = doc->image()->colorSpace()->profile();

    QCOMPARE(profile->name(), KoColorSpaceRegistry::instance()->p2020PQProfile()->name());
    QCOMPARE(profile->getColorPrimaries(), PRIMARIES_ITU_R_BT_2020_2_AND_2100_0);
    QCOMPARE(profile->getTransferCharacteristics(), TRC_ITU_R_BT_2100_0_PQ);
}

KISTEST_MAIN(KisPngTest)
