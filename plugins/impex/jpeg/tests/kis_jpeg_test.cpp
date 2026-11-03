/*
 * SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_jpeg_test.h"

#include <simpletest.h>
#include <QCoreApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtEndian>
#include <cstring>
#include <algorithm>

#include <KisExportRasterRows.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include "../kis_jpeg_converter.h"

#include "filestest.h"
#include "jpeglib.h"
#include <kis_meta_data_backend_registry.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <testui.h>

#ifndef FILES_DATA_DIR
#error "FILES_DATA_DIR not set. A directory with the data used for testing the importing of files in krita"
#endif

#ifndef JPEG_LIB_VERSION
#error "JPEG_LIB_VERSION not set. libjpeg should set it."
#endif

const QString JpegMimetype = "image/jpeg";

namespace {
KisJPEGOptions rowOptions()
{
    KisJPEGOptions options{};
    options.quality = 80;
    options.optimize = true;
    options.baseLineJPEG = true;
    options.saveProfile = true;
    options.transparencyFillColor = QColor(31, 117, 209, 128);
    return options;
}
class FailedJpegOutput final : public QIODevice
{
public:
    FailedJpegOutput() { open(QIODevice::WriteOnly); }
    bool isSequential() const override { return true; }
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64) override
    {
        setErrorString(QStringLiteral("JPEG output fixture failure"));
        return -1;
    }
};

class FragmentedJpegInput final : public QIODevice
{
public:
    FragmentedJpegInput(QByteArray bytes, qint64 fragment)
        : m_bytes(std::move(bytes)), m_fragment(fragment)
    { open(QIODevice::ReadOnly | QIODevice::Unbuffered); }
    bool isSequential() const override { return true; }
protected:
    qint64 readData(char *data, qint64 maximum) override
    {
        const auto count = std::min({maximum, m_fragment, qint64(m_bytes.size()) - m_offset});
        if (count > 0) std::memcpy(data, m_bytes.constData() + m_offset, size_t(count));
        m_offset += count;
        return count;
    }
    qint64 writeData(const char *, qint64) override { return -1; }
private:
    QByteArray m_bytes;
    qint64 m_fragment;
    qint64 m_offset = 0;
};
}

void KisJpegTest::testInputFragmentationKeepsImportedLayerIdentity_data()
{
    QTest::addColumn<int>("fragment");
    QTest::addColumn<bool>("trailer");
    QTest::newRow("single-byte") << 1 << false;
    QTest::newRow("short-read-and-trailer") << 7 << true;
    QTest::newRow("buffer-boundary-and-trailer") << 4096 << true;
}

void KisJpegTest::testInputFragmentationKeepsImportedLayerIdentity()
{
    // Native saving uses the imported layer identity. The same JPEG image must
    // keep it across source fragmentation and unread data following its EOI.
    QFETCH(int, fragment);
    QFETCH(bool, trailer);
    KisMetadataBackendRegistry::instance();
    QFile source(QStringLiteral(FILES_DATA_DIR) + QStringLiteral("/sources/transform_rotate_test_1.jpg"));
    QVERIFY(source.open(QIODevice::ReadOnly));
    const auto bytes = source.readAll();
    // Exercise a marker spanning the input buffer boundary as well.
    QByteArray encoded = bytes.left(2) + QByteArray::fromHex("fffe2002") + QByteArray(8192, '*') + bytes.mid(2);
    QBuffer referenceInput(&encoded);
    QVERIFY(referenceInput.open(QIODevice::ReadOnly));
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    KisJPEGConverter referenceConverter(reference.data(), true);
    auto result = referenceConverter.buildImage(&referenceInput);
    QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
    KisImageSP referenceImage = referenceConverter.image();
    QVERIFY(referenceImage);
    referenceImage->waitForDone();
    const auto expected = referenceImage->root()->firstChild()->uuid();
    QVERIFY(!expected.isNull());

    FragmentedJpegInput input(encoded + (trailer ? QByteArrayLiteral("following-stream-data") : QByteArray()), fragment);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisJPEGConverter converter(document.data(), true);
    result = converter.buildImage(&input);
    QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
    KisImageSP image = converter.image();
    QVERIFY(image);
    image->waitForDone();
    QVERIFY2(image->root()->firstChild()->uuid() == expected,
             "The same JPEG image must keep its imported layer identity across fragmented reads and trailing data");
}

void KisJpegTest::testRowsKeepExistingColorAndPrecision_data()
{
    QTest::addColumn<QString>("model");
    QTest::addColumn<QString>("depth");
    QTest::addColumn<bool>("forceSRGB");
    QTest::addColumn<bool>("conversionBoundaries");
    for (const auto &model : {QStringLiteral("RGBA"), QStringLiteral("GRAYA"), QStringLiteral("CMYKA"), QStringLiteral("LABA")}) {
        for (const auto &depth : {QStringLiteral("U8"), QStringLiteral("U16")}) {
            if (model == QStringLiteral("LABA") && depth == QStringLiteral("U8")) continue;
            for (const bool force : {false, true}) {
                const QByteArray name = (model + QLatin1Char('-') + depth +
                    (force ? QStringLiteral("-srgb") : QStringLiteral("-source"))).toLatin1();
                QTest::newRow(name.constData()) << model << depth << force << false;
            }
        }
    }
    QTest::newRow("lab-conversion-boundaries") << QStringLiteral("LABA") << QStringLiteral("U16") << true << true;
}

void KisJpegTest::testRowsKeepExistingColorAndPrecision()
{
    // The ordinary CPU exporter and fixed raster readers must encode the same
    // native pixels, including conversion before transparency background fill.
    QFETCH(QString, model);
    QFETCH(QString, depth);
    QFETCH(bool, forceSRGB);
    QFETCH(bool, conversionBoundaries);
    const auto cs = KoColorSpaceRegistry::instance()->colorSpace(model, depth, QString());
    QVERIFY(cs);
    const QSize size = conversionBoundaries ? QSize(257, 129) : QSize(33, 3);
    const int channelBytes = depth == QStringLiteral("U16") ? 2 : 1;
    QByteArray input(qsizetype(size.width()) * size.height() * cs->pixelSize(), '\0');
    quint32 state = 0x12345678u;
    for (int y = 0; y < size.height(); ++y) for (int x = 0; x < size.width(); ++x) {
        for (quint32 channel = 0; channel < cs->channelCount(); ++channel) {
            const int value = channel == cs->alphaPos() ? (x * 31 + y * 17) % 256 :
                (x * 13 + y * 7 + int(channel) * 71) % 256;
            auto *destination = input.data() + (qsizetype(y) * size.width() + x) * cs->pixelSize() + channel * channelBytes;
            if (channelBytes == 1) *destination = char(value);
            else {
                state = 1664525u * state + 1013904223u;
                const quint16 word = conversionBoundaries ?
                    (channel == cs->alphaPos() ? quint16(65535) : quint16(state >> 16)) :
                    quint16(value * 257 + (x * 11 + int(channel) * 7) % 257);
                std::memcpy(destination, &word, sizeof(word));
            }
        }
    }
    KisImageSP image = new KisImage(nullptr, size.width(), size.height(), cs, QStringLiteral("JPEG rows"));
    image->setResolution(300.0 / 72, 150.0 / 72);
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Fixed pixels"), 255);
    QVERIFY(image->addNode(layer));
    layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(input.constData()), 0, 0, size.width(), size.height());
    auto options = rowOptions();
    options.forceSRGB = forceSRGB;
    KisJPEGConverter legacy(nullptr, true);
    QBuffer expected;
    QVERIFY(expected.open(QIODevice::WriteOnly));
    auto previous = legacy.buildFile(&expected, layer, options, nullptr);
    QVERIFY2(previous.isOk(), qPrintable(previous.errorMessage()));
    KisExportRasterRows rows;
    rows.colorSpace = cs;
    rows.size = size;
    QList<int> reads;
    rows.readRow = [&](int row, QByteArray &bytes) {
        reads.append(row);
        bytes = input.mid(qsizetype(row) * size.width() * cs->pixelSize(), size.width() * cs->pixelSize());
        return KisImportExportErrorCode(ImportExportCodes::OK);
    };
    QBuffer actual;
    QVERIFY(actual.open(QIODevice::WriteOnly));
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path());
    QVERIFY(QDir().mkpath(directory));
    const auto saveArtifact = [&](const QString &suffix, const QByteArray &bytes) {
        QFile file(QDir(directory).filePath(QString::fromLatin1(QTest::currentDataTag()) + suffix));
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    const auto preserve = qScopeGuard([&] {
        if (!QTest::currentTestFailed() && !qEnvironmentVariableIsSet("LIBREPAINT_GPU_RESULT_DIR")) return;
        temporary.setAutoRemove(!QTest::currentTestFailed());
        saveArtifact(QStringLiteral("-legacy.jpg"), expected.data());
        saveArtifact(QStringLiteral("-rows.jpg"), actual.data());
        if (QTest::currentTestFailed()) qInfo() << "JPEG row comparison artifacts retained at" << directory;
    });
    KisJPEGConverter converter(nullptr, true);
    auto result = converter.buildFile(&actual, rows, image->xRes(), image->yRes(), options, nullptr);
    QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
    QCOMPARE(actual.data(), expected.data());
    QCOMPARE(reads.size(), size.height());
    for (int row = 0; row < reads.size(); ++row) QCOMPARE(reads[row], row);
    QVERIFY(!QImage::fromData(actual.data(), "JPEG").isNull());
}

void KisJpegTest::testRowsPropagateReadCancellationAndWriteFailure_data()
{
    QTest::addColumn<int>("failure");
    QTest::newRow("cancel-during-read") << 0;
    QTest::newRow("source-error") << 1;
    QTest::newRow("incomplete-row") << 2;
    QTest::newRow("destination-error") << 3;
}

void KisJpegTest::testRowsPropagateReadCancellationAndWriteFailure()
{
    QFETCH(int, failure);
    KisJPEGConverter converter(nullptr, true);
    KisExportRasterRows rows;
    rows.colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    rows.size = {1, 4};
    int calls = 0;
    rows.readRow = [&](int, QByteArray &bytes) {
        ++calls;
        if (failure == 1) return KisImportExportErrorCode(ImportExportCodes::InsufficientMemory);
        bytes = QByteArray::fromHex(failure == 2 ? "332211" : "332211ff");
        if (failure == 0) converter.cancel();
        return KisImportExportErrorCode(ImportExportCodes::OK);
    };
    auto options = rowOptions();
    options.saveProfile = false;
    QBuffer output;
    QVERIFY(output.open(QIODevice::WriteOnly));
    FailedJpegOutput failed;
    auto result = converter.buildFile(failure == 3 ? static_cast<QIODevice *>(&failed) : &output,
        rows, 1, 1, options, nullptr);
    const ImportExportCodes::ErrorCodeID expected[] = {ImportExportCodes::Cancelled, ImportExportCodes::InsufficientMemory,
        ImportExportCodes::InternalError, ImportExportCodes::ErrorWhileWriting};
    QVERIFY2(result == KisImportExportErrorCode(expected[failure]), qPrintable(result.errorMessage()));
    QVERIFY(calls > 0);
    if (failure != 3) QVERIFY(calls < rows.size.height());
}

void KisJpegTest::testFiles()
{
    KisMetadataBackendRegistry::instance();
    /**
     * Different versions of JPEG library may produce a bit different
     * result, so just compare in a weak way, i.e, only the size for real
     */
    const int fuzziness = 1;
    const int maxNumFailingPixels = 2592 * 1952; // All pixels can be different...
    const bool showDebug = false; // No need to write down all pixels that are different since all of them can be.

    if (JPEG_LIB_VERSION == 80){
        TestUtil::testFiles(QString(FILES_DATA_DIR) + "/sources", QStringList(), "_80", fuzziness, maxNumFailingPixels, showDebug);
    }else {
        TestUtil::testFiles(QString(FILES_DATA_DIR) + "/sources", QStringList(), QString(), fuzziness, maxNumFailingPixels, showDebug);
    }
}

void KisJpegTest::testImportFromWriteonly()
{
    TestUtil::testImportFromWriteonly(JpegMimetype);
}


void KisJpegTest::testExportToReadonly()
{
    TestUtil::testExportToReadonly(JpegMimetype);
}


void KisJpegTest::testImportIncorrectFormat()
{
    TestUtil::testImportIncorrectFormat(JpegMimetype);
}

void KisJpegTest::testInvalidEmbeddedProfileIsRejected()
{
    QFile source(QStringLiteral(FILES_DATA_DIR) + QStringLiteral("/sources/transform_rotate_test_1.jpg"));
    QVERIFY(source.open(QIODevice::ReadOnly));
    const auto bytes = source.readAll();
    const auto payload = QByteArrayLiteral("ICC_PROFILE\0\1\1not-an-ICC-profile");
    QByteArray encoded = bytes.left(2) + QByteArray::fromHex("ffe2") +
        QByteArray(1, char(0)) + QByteArray(1, char(payload.size() + 2)) + payload + bytes.mid(2);
    QBuffer input(&encoded);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisJPEGConverter converter(document.data(), true);
    auto result = converter.buildImage(&input);
    QVERIFY2(result == ImportExportCodes::FileFormatIncorrect, qPrintable(result.errorMessage()));
    QVERIFY(!converter.image());
    QVERIFY(!document->image());
}

void KisJpegTest::testGpuPreparationRejectsRasterCapacity_data()
{
    QTest::addColumn<QSize>("size");
    QTest::newRow("oversized-raster") << QSize(32768, 32768);
    // The unpadded pixels fit in 256MiB, but the required tiles exceed it.
    QTest::newRow("padded-tile-capacity") << QSize(16385, 3841);
}

void KisJpegTest::testGpuPreparationRejectsRasterCapacity()
{
    QFETCH(QSize, size);
    KisMetadataBackendRegistry::instance();
    QFile source(QStringLiteral(FILES_DATA_DIR) + QStringLiteral("/sources/transform_rotate_test_1.jpg"));
    QVERIFY(source.open(QIODevice::ReadOnly));
    auto encoded = source.readAll();
    QBuffer initial(&encoded);
    QVERIFY(initial.open(QIODevice::ReadOnly));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisJPEGConverter converter(document.data(), true);
    auto result = converter.buildImage(&initial);
    QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
    KisImageSP previous = converter.image();
    QVERIFY(previous);
    const auto frame = encoded.indexOf(QByteArray::fromHex("ffc0"));
    QVERIFY(frame >= 0);
    qToBigEndian(quint16(size.height()), reinterpret_cast<uchar *>(encoded.data() + frame + 5));
    qToBigEndian(quint16(size.width()), reinterpret_cast<uchar *>(encoded.data() + frame + 7));
    QBuffer oversized(&encoded);
    QVERIFY(oversized.open(QIODevice::ReadOnly));
    result = converter.prepareGpuImage(&oversized);
    QVERIFY2(result == ImportExportCodes::InsufficientMemory, qPrintable(result.errorMessage()));
    QCOMPARE(converter.image(), previous);
}
KISTEST_MAIN(KisJpegTest)

