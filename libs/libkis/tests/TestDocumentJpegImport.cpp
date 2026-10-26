/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestDocument.h"
#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <InfoObject.h>
#include <application/kis_config.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <kis_image.h>
#include <kis_image_config.h>
#include <kis_layer.h>
#include <metadata/kis_meta_data_store.h>
#include <metadata/kis_meta_data_entry.h>
#include <metadata/kis_meta_data_value.h>
#include <metadata/KoDocumentInfo.h>
#include <kis_properties_configuration.h>
#include <kundo2stack.h>
#include <gpu/KisGpuPaintImage.h>
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QDateTime>
#include <QDomDocument>

namespace {
// Fixed 2x1 opaque sRGB JPEG from RGB112233 and405060, encoded once by the
// ordinary exporter at quality90. The retained Photoshop/IPTC title is
// "Fixed PNG import". APP1/APP2/comment metadata was removed; the fixtures
// below add explicit orientation, density and the default color profile.
const QByteArray fixedJpeg = QByteArray::fromHex(
    "ffd8ffe000104a46494600010101004800480000ffed003150686f746f73686f7020332e30003842494d0404000000000015"
    "1c02050010466978656420504e4720696d706f7274ffee099a687474703a2f2f6e732e61646f62652e636f6d2f7861702f31"
    "2e302f3c3f787061636b657420626567696e3d22efbbbf222069643d2257354d304d7043656869487a7265537a4e54637a6b"
    "633964223f3e0a3c783a786d706d65746120786d6c6e733a783d2261646f62653a6e733a6d6574612f2220783a786d70746b"
    "3d22584d5020436f726520342e342e302d4578697632223e0a203c7264663a52444620786d6c6e733a7264663d2268747470"
    "3a2f2f7777772e77332e6f72672f313939392f30322f32322d7264662d73796e7461782d6e7323223e0a20203c7264663a44"
    "65736372697074696f6e207264663a61626f75743d22220a20202020786d6c6e733a64633d22687474703a2f2f7075726c2e"
    "6f72672f64632f656c656d656e74732f312e312f220a20202064633a646174653d22323030312d30322d30335430343a3035"
    "3a30365a220a20202064633a7469746c653d22466978656420504e4720696d706f7274222f3e0a203c2f7264663a5244463e"
    "0a3c2f783a786d706d6574613e0a202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "20202020202020202020202020200a2020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020200a20202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "202020202020202020202020202020200a202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "20202020202020202020202020202020200a2020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020200a20202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "202020202020202020202020202020202020200a202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "20202020202020202020202020202020202020200a2020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020200a20202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "202020202020202020202020202020202020202020200a202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "20202020202020202020202020202020202020202020200a2020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020200a20202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "202020202020202020202020202020202020202020202020200a202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "20202020202020202020202020202020202020202020202020200a2020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020200a20202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "202020202020202020202020202020202020202020202020202020200a202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "20202020202020202020202020202020202020202020202020202020200a2020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020200a20202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "202020202020202020202020202020202020202020202020202020202020200a202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "20202020202020202020202020202020202020202020202020202020202020200a2020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020"
    "2020202020202020202020202020202020202020202020202020202020202020200a20202020202020202020202020202020"
    "20202020202020202020200a3c3f787061636b657420656e643d2277223f3effdb0043000302020302020303030304030304"
    "050805050404050a070706080c0a0c0c0b0a0b0b0d0e12100d0e110e0b0b1016101113141515150c0f171816141812141514"
    "ffdb00430103040405040509050509140d0b0d14141414141414141414141414141414141414141414141414141414141414"
    "14141414141414141414141414141414141414ffc00011080001000203012200021101031101ffc400150001010000000000"
    "0000000000000000000008ffc4001b100100000700000000000000000000000000010305063574b2ffc40014010100000000"
    "000000000000000000000005ffc40014110100000000000000000000000000000000ffda000c03010002110311003f009a2d"
    "fc0d375a571000e877ffd9");

// Existing JPEG fixture, retaining its coded20x20pixels:
// plugins/impex/jpeg/tests/data/sources/transform_rotate_test_1.jpg.
const QByteArray orientationJpeg = QByteArray::fromHex(
    "ffd8ffe000104a46494600010100000100010000ffe100224578696600004d4d002a00000008000101120003000000010001"
    "000000000000ffdb004300080606070605080707070909080a0c140d0c0b0b0c1912130f141d1a1f1e1d1a1c1c20242e2720"
    "222c231c1c2837292c30313434341f27393d38323c2e333432ffdb0043010909090c0b0c180d0d1832211c21323232323232"
    "3232323232323232323232323232323232323232323232323232323232323232323232323232323232323232ffc000110800"
    "14001403012200021101031101ffc4001f0000010501010101010100000000000000000102030405060708090a0bffc400b5"
    "100002010303020403050504040000017d01020300041105122131410613516107227114328191a1082342b1c11552d1f024"
    "33627282090a161718191a25262728292a3435363738393a434445464748494a535455565758595a636465666768696a7374"
    "75767778797a838485868788898a92939495969798999aa2a3a4a5a6a7a8a9aab2b3b4b5b6b7b8b9bac2c3c4c5c6c7c8c9ca"
    "d2d3d4d5d6d7d8d9dae1e2e3e4e5e6e7e8e9eaf1f2f3f4f5f6f7f8f9faffc4001f0100030101010101010101010000000000"
    "000102030405060708090a0bffc400b511000201020404030407050404000102770001020311040521310612415107617113"
    "22328108144291a1b1c109233352f0156272d10a162434e125f11718191a262728292a35363738393a434445464748494a53"
    "5455565758595a636465666768696a737475767778797a82838485868788898a92939495969798999aa2a3a4a5a6a7a8a9aa"
    "b2b3b4b5b6b7b8b9bac2c3c4c5c6c7c8c9cad2d3d4d5d6d7d8d9dae2e3e4e5e6e7e8e9eaf2f3f4f5f6f7f8f9faffda000c03"
    "010002110311003f00dcd1f44b6d02d84b3287ba61c03da9752d4a2b385af6fe4000e5549eb516abab41a5dbb5e5f480be32"
    "a99af39bbbbbbf125d9b9bb62968a7e54f5acb85f845d57fda1996896ba9f3d56b665c658ee485d52b935f78a757d42e9e7b"
    "452b07441d38a29bf691100912ed41c0028afb2971565749fb38c2e969b1fa4d0f0e7010a718cb748ab757536b3e20916f24"
    "2ea8df2a8e9566e58a911ae020e8051451c61270c0c630765a1d5c034a9c7009c6297c8af451457e5c7df9ffd9");
const QByteArray orientationPixelHashes[] = {
    QByteArrayLiteral("d872b12ebb7dae67c59b6491e7b7fd00246467d601bcce5de146c59b333d348a"),
    QByteArrayLiteral("3cb9d6d8d161b8a495d752edad8734a38e33d5f7dbd1f4c68fbc4a86af9b0da1"),
    QByteArrayLiteral("6fce7dc593a94182a6ba406a2ac88435896c3ba96a75ec0a4b1a74a8320043c7"),
    QByteArrayLiteral("1b2359405fee542985aeec6f9ed135d36464bb2237da0cf628cfcfeee93d0e30"),
    QByteArrayLiteral("77240085193e972b63048cd0c1624c223e748fcbc3ed40335a1f59a69dcea43e"),
    QByteArrayLiteral("e2b8737b97a1f2a792b35d5d7d01966b56ed3f6aef4ce6d19313afcd8b8cf8ee"),
    QByteArrayLiteral("e09f2a83474fe0502efe9255e91e4523f19cf8b05e56f94a2d5a088576e0ba4e"),
    QByteArrayLiteral("34459daa9bc7da9be909a0de22e9f8fd65732ce17d9aced4a628da9aa6663820"),
};

QByteArray little16(quint16 value)
{
    QByteArray bytes(2, char(0));
    qToLittleEndian(value, reinterpret_cast<uchar *>(bytes.data()));
    return bytes;
}

QByteArray little32(quint32 value)
{
    QByteArray bytes(4, char(0));
    qToLittleEndian(value, reinterpret_cast<uchar *>(bytes.data()));
    return bytes;
}

QByteArray prependMarker(const QByteArray &jpeg, quint8 marker, const QByteArray &payload)
{
    QByteArray length(2, char(0));
    qToBigEndian(quint16(payload.size() + 2), reinterpret_cast<uchar *>(length.data()));
    return jpeg.left(2) + QByteArray(1, char(0xff)) + QByteArray(1, char(marker)) + length + payload + jpeg.mid(2);
}

QByteArray jpegInput(int orientation, bool profile = false, int densityUnit = 1,
                     int xDensity = 72, int yDensity = 72, int exifX = 0, int exifY = 0,
                     const QByteArray &source = fixedJpeg)
{
    QByteArray jpeg = source;
    const int jfif = jpeg.indexOf(QByteArrayLiteral("JFIF\0"));
    if (jfif >= 0) {
        jpeg[jfif + 7] = char(densityUnit);
        qToBigEndian(quint16(xDensity), reinterpret_cast<uchar *>(jpeg.data() + jfif + 8));
        qToBigEndian(quint16(yDensity), reinterpret_cast<uchar *>(jpeg.data() + jfif + 10));
    } else if (densityUnit != 1 || xDensity != 72 || yDensity != 72) return {};
    const int count = exifX ? 4 : 1;
    QByteArray tiff = QByteArrayLiteral("II") + little16(42) + little32(8) + little16(count);
    tiff += little16(0x112) + little16(3) + little32(1) + little16(orientation) + little16(0);
    if (exifX) {
        const int offset = 8 + 2 + count * 12 + 4;
        tiff += little16(0x11a) + little16(5) + little32(1) + little32(offset);
        tiff += little16(0x11b) + little16(5) + little32(1) + little32(offset + 8);
        tiff += little16(0x128) + little16(3) + little32(1) + little16(2) + little16(0);
    }
    tiff += little32(0);
    if (exifX) tiff += little32(exifX) + little32(1) + little32(exifY) + little32(1);
    jpeg = prependMarker(jpeg, 0xe1, QByteArrayLiteral("Exif\0\0") + tiff);
    if (profile) {
        const auto bytes = KoColorSpaceRegistry::instance()->rgb8()->profile()->rawData();
        jpeg = prependMarker(jpeg, 0xe2, QByteArrayLiteral("ICC_PROFILE\0\1\1") + bytes);
    }
    return jpeg;
}
}

void TestDocument::testGpuSessionOpensJpegWithGpuPixelOwnership_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<int>("orientation");
    QTest::addColumn<qreal>("expectedX");
    QTest::addColumn<qreal>("expectedY");
    QTest::addColumn<int>("frame");
    // Retained ordinary JPEG resolution values, observed on ARM64 and x86_64.
    for (int orientation = 1; orientation <= 8; ++orientation) {
        const QByteArray name = QByteArrayLiteral("orientation-") + QByteArray::number(orientation);
        QTest::newRow(name.constData()) << jpegInput(orientation) << orientation
            << qreal(1.00000000000008) << qreal(1.00000000000008) << 0;
    }
    QTest::newRow("embedded-default-profile") << jpegInput(6, true) << 6
        << qreal(1.00000000000008) << qreal(1.00000000000008) << 0;
    // The existing decoder recovers the final pixels when only EOI is absent.
    QTest::newRow("recovered-missing-eoi") << jpegInput(1).chopped(2) << 1
        << qreal(1.00000000000008) << qreal(1.00000000000008) << 0;
    QTest::newRow("jfif-inch") << jpegInput(6, false, 1, 300, 150) << 6
        << qreal(4.166666666667) << qreal(2.0833333333335) << 0;
    QTest::newRow("jfif-centimetre") << jpegInput(6, false, 2, 118, 59) << 6
        << qreal(4.162777777778111) << qreal(2.0813888888890557) << 0;
    QTest::newRow("exif-resolution") << jpegInput(6, false, 0, 1, 1, 300, 150) << 6
        << qreal(4.166666666667) << qreal(2.0833333333335) << 0;
    QTest::newRow("exif-low-resolution") << jpegInput(6, false, 0, 1, 1, 12, 36) << 6
        << qreal(0.16666666666668) << qreal(0.50000000000004) << 0;
    for (int orientation = 1; orientation <= 8; ++orientation) {
        const QByteArray name = QByteArrayLiteral("two-dimensional-orientation-") + QByteArray::number(orientation);
        QTest::newRow(name.constData()) << jpegInput(orientation, false, 1, 72, 72, 0, 0, orientationJpeg) << orientation
            << qreal(1.00000000000008) << qreal(1.00000000000008) << 1;
    }
    QFile photo(QFINDTESTDATA("../../../plugins/impex/jpeg/tests/data/sources/HPIM0760.JPG"));
    QVERIFY(photo.open(QIODevice::ReadOnly));
    const auto photoBytes = photo.readAll();
    QTest::newRow("rotated-tile-boundaries") << jpegInput(6, false, 1, 72, 72, 0, 0, photoBytes) << 6
        << qreal(1.00000000000008) << qreal(1.00000000000008) << 2;
    QTest::newRow("transposed-tile-boundaries") << jpegInput(5, false, 1, 72, 72, 0, 0, photoBytes) << 5
        << qreal(1.00000000000008) << qreal(1.00000000000008) << 2;
    QTest::newRow("transverse-tile-boundaries") << jpegInput(7, false, 1, 72, 72, 0, 0, photoBytes) << 7
        << qreal(1.00000000000008) << qreal(1.00000000000008) << 2;
}

void TestDocument::testGpuSessionOpensJpegWithGpuPixelOwnership()
{
    QFETCH(QByteArray, encoded);
    QFETCH(int, orientation);
    QFETCH(qreal, expectedX);
    QFETCH(qreal, expectedY);
    QFETCH(int, frame);
    KisImageConfig imageConfig(false);
    const int previousThreads = imageConfig.maxNumberOfThreads();
    const auto restoreThreads = qScopeGuard([&] { imageConfig.setMaxNumberOfThreads(previousThreads); });
    imageConfig.setMaxNumberOfThreads(4);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto directory = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(
        QStringLiteral("jpeg-import-%1").arg(QString::fromLatin1(QTest::currentDataTag())));
    QVERIFY(QDir().mkpath(directory));
    const auto path = QDir(directory).filePath(QStringLiteral("source.jpg"));
    QFile input(path);
    QVERIFY(input.open(QIODevice::WriteOnly));
    QCOMPARE(input.write(encoded), qint64(encoded.size()));
    input.close();

    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    reference->setFileBatchMode(true);
    reference->setAutoSaveActive(false);
    QVERIFY2(reference->openPath(path, KisDocument::DontAddToRecent), qPrintable(reference->errorMessage()));
    KisImageSP referenceImage = reference->image();
    QVERIFY(referenceImage);
    referenceImage->waitForDone();
    const QSize size = frame == 2 ? QSize(1952, 2592) : frame == 1 ? QSize(20, 20) :
        orientation <= 4 ? QSize(2, 1) : QSize(1, 2);
    QCOMPARE(referenceImage->size(), size);
    QCOMPARE(referenceImage->xRes(), expectedX);
    QCOMPARE(referenceImage->yRes(), expectedY);
    QScopedPointer<Node> referenceNode(Node::createNode(referenceImage, referenceImage->root()->firstChild()));
    const auto expectedPixels = referenceNode->pixelData(0, 0, size.width(), size.height());
    if (frame == 0) {
        QCOMPARE(expectedPixels, QByteArray::fromHex(orientation == 2 || orientation == 3 || orientation == 7 || orientation == 8 ?
            "615141ff322212ff" : "322212ff615141ff"));
    } else {
        // Photo hashes use exact coordinate permutations of its original BGRA pixels.
        const QByteArray expectedHash = frame == 1 ? orientationPixelHashes[orientation - 1] :
            orientation == 5 ? QByteArrayLiteral("abe42bbcaa476ad378b19f27e12b1ee6e2c5d68bb5e26f210abce1de78091aed") :
            orientation == 7 ? QByteArrayLiteral("89c0d25b22de202e6af2838a0b9038c98cf045894007405266916a4de9cdb7aa") :
            QByteArrayLiteral("1f6a35cfc3078a452afbf2be2170a6f0f6e1bad2ca031f60cc24282b62fd9889");
        const auto actualHash = QCryptographicHash::hash(expectedPixels, QCryptographicHash::Sha256).toHex();
        if (actualHash != expectedHash) {
            QFile actual(QDir(directory).filePath(QStringLiteral("cpu-open.bgra")));
            QVERIFY(actual.open(QIODevice::WriteOnly));
            QCOMPARE(actual.write(expectedPixels), qint64(expectedPixels.size()));
        }
        QCOMPARE(actualHash, expectedHash);
    }
    auto *referenceStore = qobject_cast<KisLayer *>(referenceImage->root()->firstChild().data())->metaData();
    QCOMPARE(referenceStore->getValue(QStringLiteral("http://ns.adobe.com/tiff/1.0/"), QStringLiteral("Orientation")).asInteger(), 1);
    if (frame == 0)
    QCOMPARE(referenceStore->getValue(QStringLiteral("http://purl.org/dc/elements/1.1/"), QStringLiteral("title")).asVariant().toString(),
             QStringLiteral("Fixed PNG import"));
    QVERIFY(!reference->isModified());

    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    QVERIFY2(document->openPath(path, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    KisImageSP image = document->image();
    QVERIFY(image);
    QVERIFY2(image->gpuImage(), "Ordinary JPEG Open in a GPU session must use GPU pixel ownership");
    QCOMPARE(image->size(), size);
    QCOMPARE(image->xRes(), expectedX);
    QCOMPARE(image->yRes(), expectedY);
    QCOMPARE(image->colorSpace()->id(), referenceImage->colorSpace()->id());
    QCOMPARE(image->colorSpace()->profile()->rawData(), referenceImage->colorSpace()->profile()->rawData());
    QScopedPointer<Node> node(Node::createNode(image, image->root()->firstChild()));
    QCOMPARE(node->name(), referenceNode->name());
    QCOMPARE(node->position(), referenceNode->position());
    QCOMPARE(node->uniqueId(), referenceNode->uniqueId());
    QCOMPARE(node->pixelData(0, 0, size.width(), size.height()), expectedPixels);
    QCOMPARE(Document(document.data(), false).pixelData(0, 0, size.width(), size.height()), expectedPixels);
    auto *store = qobject_cast<KisLayer *>(image->root()->firstChild().data())->metaData();
    auto keys = store->keys(), referenceKeys = referenceStore->keys();
    keys.sort(); referenceKeys.sort();
    QCOMPARE(keys, referenceKeys);
    for (const auto &key : keys)
        QVERIFY2(store->getEntry(key).value() == referenceStore->getEntry(key).value(), qPrintable(key));
    QVERIFY(!document->isModified());

    Document referenceApi(reference.data(), false), api(document.data(), false);
    const auto firstEdit = QByteArray::fromHex("214263ff");
    QVERIFY(referenceNode->setPixelData(firstEdit, 0, 0, 1, 1));
    QVERIFY(node->setPixelData(firstEdit, 0, 0, 1, 1));
    referenceApi.refreshProjection(); api.refreshProjection();
    reference->image()->waitForDone(); document->image()->waitForDone();
    QByteArray edited = expectedPixels;
    edited.replace(0, 4, firstEdit);
    QCOMPARE(referenceApi.pixelData(0, 0, size.width(), size.height()), edited);
    QCOMPARE(api.pixelData(0, 0, size.width(), size.height()), edited);
    const int referenceHistory = reference->undoStack()->index();
    const int history = document->undoStack()->index();
    const QSize expanded = size + QSize(1, 1);
    referenceApi.resizeImage(0, 0, expanded.width(), expanded.height());
    api.resizeImage(0, 0, expanded.width(), expanded.height());
    reference->image()->waitForDone(); document->image()->waitForDone();
    QCOMPARE(reference->image()->size(), expanded);
    QCOMPARE(document->image()->size(), expanded);
    QCOMPARE(reference->undoStack()->index(), referenceHistory + 1);
    QCOMPARE(document->undoStack()->index(), history + 1);
    const auto expandedPixels = referenceApi.pixelData(0, 0, expanded.width(), expanded.height());
    QCOMPARE(api.pixelData(0, 0, expanded.width(), expanded.height()), expandedPixels);
    reference->undoStack()->undo(); document->undoStack()->undo();
    reference->image()->waitForDone(); document->image()->waitForDone();
    QCOMPARE(reference->image()->size(), size);
    QCOMPARE(document->image()->size(), size);
    QCOMPARE(referenceApi.pixelData(0, 0, size.width(), size.height()), edited);
    QCOMPARE(api.pixelData(0, 0, size.width(), size.height()), edited);
    reference->undoStack()->redo(); document->undoStack()->redo();
    reference->image()->waitForDone(); document->image()->waitForDone();
    QCOMPARE(api.pixelData(0, 0, expanded.width(), expanded.height()), expandedPixels);

    const bool previousCompression = KisConfig(true).compressKra();
    const auto restoreCompression = qScopeGuard([&] { KisConfig(false).setCompressKra(previousCompression); });
    KisConfig(false).setCompressKra(false);
    QDomDocument fixedInfo;
    QVERIFY(fixedInfo.setContent(QByteArrayLiteral("<document-info><about><title>Fixed JPEG import</title>"
        "<creation-date>2000-01-02T03:04:05Z</creation-date><date>2001-02-03T04:05:06Z</date>"
        "<editing-cycles>7</editing-cycles><editing-time>10</editing-time><language>en</language></about>"
        "<author><full-name>LibrePaint fixture</full-name></author></document-info>")));
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    for (auto *item : {reference.data(), document.data()}) {
        QVERIFY(item->documentInfo()->load(fixedInfo));
        item->documentInfo()->setAuthorInfo(QStringLiteral("creator"), QStringLiteral("LibrePaint fixture"));
        item->setColorHistoryColors({});
        item->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
        item->setPreActivatedNode(item->image()->root()->firstChild());
    }
    const auto cpuNative = QDir(directory).filePath(QStringLiteral("cpu.kra"));
    const auto gpuNative = QDir(directory).filePath(QStringLiteral("gpu.kra"));
    QVERIFY2(reference->saveAs(cpuNative, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(reference->errorMessage()));
    reference->waitForSavingToComplete();
    QVERIFY2(document->saveAs(gpuNative, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    const auto read = [](const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    QVERIFY(!read(cpuNative).isEmpty());
    QCOMPARE(read(gpuNative), read(cpuNative));
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> savedReference(KisPart::instance()->createDocument());
    savedReference->setFileBatchMode(true);
    savedReference->setAutoSaveActive(false);
    QVERIFY2(savedReference->openPath(cpuNative, KisDocument::DontAddToRecent), qPrintable(savedReference->errorMessage()));
    savedReference->image()->waitForDone();
    QScopedPointer<Node> savedReferenceNode(Node::createNode(savedReference->image(), savedReference->image()->root()->firstChild()));
    QCOMPARE(savedReferenceNode->pixelData(0, 0, expanded.width(), expanded.height()), expandedPixels);
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    reopened->setAutoSaveActive(false);
    QVERIFY2(reopened->openPath(gpuNative, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QVERIFY(reopened->image()->gpuImage());
    QCOMPARE(reopened->image()->size(), expanded);
    QCOMPARE(reopened->image()->xRes(), document->image()->xRes());
    QCOMPARE(reopened->image()->yRes(), document->image()->yRes());
    QScopedPointer<Node> savedNode(Node::createNode(reopened->image(), reopened->image()->root()->firstChild()));
    QCOMPARE(savedNode->uniqueId(), node->uniqueId());
    QCOMPARE(savedNode->pixelData(0, 0, expanded.width(), expanded.height()), expandedPixels);
    auto *savedStore = qobject_cast<KisLayer *>(reopened->image()->root()->firstChild().data())->metaData();
    auto *savedReferenceStore = qobject_cast<KisLayer *>(savedReference->image()->root()->firstChild().data())->metaData();
    auto savedKeys = savedStore->keys(); savedKeys.sort();
    auto savedReferenceKeys = savedReferenceStore->keys(); savedReferenceKeys.sort();
    QCOMPARE(savedKeys, savedReferenceKeys);
    for (const auto &key : savedKeys)
        QVERIFY2(savedStore->getEntry(key).value() == savedReferenceStore->getEntry(key).value(), qPrintable(key));
    if (frame == 0) {
        // XMP decoding gives the IPTC title its Dublin Core language-array type.
        const auto &title = savedStore->getValue(QStringLiteral("http://purl.org/dc/elements/1.1/"), QStringLiteral("title"));
        QCOMPARE(title.type(), KisMetaData::Value::LangArray);
        QCOMPARE(title.asLangArray().value(QStringLiteral("x-default")).asVariant().toString(), QStringLiteral("Fixed PNG import"));
    }

    // Continue editing the reopened document through the public pixel API.
    Document savedReferenceApi(savedReference.data(), false);
    Document savedApi(reopened.data(), false);
    const auto secondEdit = QByteArray::fromHex("c1a283ff");
    QVERIFY(savedReferenceNode->setPixelData(secondEdit, 0, 1, 1, 1));
    QVERIFY(savedNode->setPixelData(secondEdit, 0, 1, 1, 1));
    savedReferenceApi.refreshProjection(); savedApi.refreshProjection();
    savedReference->image()->waitForDone(); reopened->image()->waitForDone();
    const auto finalPixels = savedReferenceApi.pixelData(0, 0, expanded.width(), expanded.height());
    QCOMPARE(savedApi.pixelData(0, 0, expanded.width(), expanded.height()), finalPixels);
    QVERIFY(savedReference->documentInfo()->load(fixedInfo));
    QVERIFY(reopened->documentInfo()->load(fixedInfo));
    savedReference->documentInfo()->setAuthorInfo(QStringLiteral("creator"), QStringLiteral("LibrePaint fixture"));
    reopened->documentInfo()->setAuthorInfo(QStringLiteral("creator"), QStringLiteral("LibrePaint fixture"));
    savedReference->setColorHistoryColors({}); reopened->setColorHistoryColors({});
    savedReference->setPreActivatedNode(savedReference->image()->root()->firstChild());
    reopened->setPreActivatedNode(reopened->image()->root()->firstChild());
    const auto cpuAfter = QDir(directory).filePath(QStringLiteral("cpu-after.kra"));
    const auto gpuAfter = QDir(directory).filePath(QStringLiteral("gpu-after.kra"));
    QVERIFY2(savedReference->saveAs(cpuAfter, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(savedReference->errorMessage()));
    savedReference->waitForSavingToComplete();
    QVERIFY2(reopened->saveAs(gpuAfter, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(reopened->errorMessage()));
    reopened->waitForSavingToComplete();
    QCOMPARE(read(gpuAfter), read(cpuAfter));
    for (const auto &format : {QStringLiteral("png"), QStringLiteral("jpg")}) {
        InfoObject output;
        output.setProperty("forceSRGB", false);
        output.setProperty("saveSRGBProfile", true);
        output.setProperty("alpha", true);
        output.setProperty("compression", 3);
        output.setProperty("quality", 90);
        output.setProperty("storeMetaData", true);
        output.setProperty("storeAuthor", false);
        output.setProperty("exif", true);
        output.setProperty("iptc", true);
        output.setProperty("xmp", true);
        const auto cpuPath = QDir(directory).filePath(QStringLiteral("cpu.") + format);
        const auto gpuPath = QDir(directory).filePath(QStringLiteral("gpu.") + format);
        QVERIFY(savedReferenceApi.exportImage(cpuPath, output));
        QVERIFY(savedApi.exportImage(gpuPath, output));
        QVERIFY(!read(cpuPath).isEmpty());
        QCOMPARE(read(gpuPath), read(cpuPath));
    }
}

void TestDocument::testJpegRejectedOpenKeepsDocument_data()
{
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<bool>("gpuSession");
    QTest::addColumn<QString>("validCpuModel");
    const auto add = [](const QByteArray &name, const QByteArray &bytes, bool gpuOnly = false,
                        const QString &model = QString()) {
        for (bool gpu : {false, true}) {
            if (!gpu && gpuOnly) continue;
            const QByteArray tag = QByteArray(gpu ? "gpu-" : "cpu-") + name;
            QTest::newRow(tag.constData()) << bytes << gpu << model;
        }
    };
    add("signature", QByteArrayLiteral("not a JPEG"));
    add("truncated-header", fixedJpeg.left(50));
    add("malformed-icc", prependMarker(fixedJpeg, 0xe2, QByteArrayLiteral("ICC_PROFILE\0\1\1not-an-ICC-profile")));
    // Existing ordinary Gray8 and CMYK8 exports. Strip APP1/2/13 and COM
    // metadata while preserving coded pixels, tables and Adobe APP14.
    add("gray8", QByteArray::fromHex(
        "ffd8ffe000104a46494600010101012c00960000ffdb00430006040506050406060506070706080a100a0a09090a140e0f0c"
        "1017141818171416161a1d251f1a1b231c1616202c20232627292a29191f2d302d283025282928ffc0000b08000300210101"
        "2200ffc400160001010100000000000000000000000000060704ffc400291000020102030509000000000000000000010203"
        "000405061112212231510713152341536193d1ffda0008010100003f000b6fe66244bf11daf5aac6448a36ba4d541df532ed"
        "4ee259313c22de472d10b5eff43bc977760cc4f33aec2f3e9f2692e5c8635ba48c2f02e8a07402b478c5f7bc3eb5fcafffd9"),
        true, QStringLiteral("GRAYA"));
    add("cmyk8", QByteArray::fromHex(
        "ffd8ffe000104a46494600010101012c00960000ffee000e41646f626500640000000000ffdb004300060405060504060605"
        "06070706080a100a0a09090a140e0f0c1017141818171416161a1d251f1a1b231c1616202c20232627292a29191f2d302d28"
        "3025282928ffc000140800030021044322004d11005911004b1100ffc4001a00010002030100000000000000000000000006"
        "0702030405ffc4003410000200040303080a030000000000000001020003041105062112153107132241515591d108141624"
        "5354617181949293b1ffda000e0443004d0059004b00003f00eecb32924e0d284a50a020e1152e2131cd1339662edd26626e"
        "493c49308471d474b39e0cadaa9c5285483c0833869131c96ecd9a394876625d569006275009a8bff83c22c6cccedec1cd3b"
        "4d732ee4dfb59ef1657a3fd34918462b5613de1eab9a67b9d55514a8b70d0bb78fda2314f3e654524caa9cdb53e7b19b31"
        "ac06d331b9361a7130846ce5299860b50431bf363ae3d1c1fa5ca562c5b52b365a827a8733526de201fc0860400cbb9b660d"
        "266f5989b5d7b224cab0bf60b9d3ea621bbd2b3e39fe23ca108467b8307ee9c3ff00593ca2def52a5f9691fd62279b930aee"
        "ca1fd74f28ffd9"), true, QStringLiteral("CMYKA"));
    const auto profile = KoColorSpaceRegistry::instance()->p709G10Profile();
    Q_ASSERT(profile);
    add("nondefault-profile", prependMarker(fixedJpeg, 0xe2,
        QByteArrayLiteral("ICC_PROFILE\0\1\1") + profile->rawData()), true, QStringLiteral("RGBA"));
    auto sameName = KoColorSpaceRegistry::instance()->rgb8()->profile()->rawData();
    sameName.replace(80, 4, QByteArrayLiteral("LPnt"));
    add("nondefault-profile-with-default-name", prependMarker(fixedJpeg, 0xe2,
        QByteArrayLiteral("ICC_PROFILE\0\1\1") + sameName), true, QStringLiteral("RGBA"));
    const auto frame = fixedJpeg.indexOf(QByteArray::fromHex("ffc0"));
    Q_ASSERT(frame >= 0);
    auto precision = fixedJpeg;
    precision[frame + 4] = char(12);
    add("unsupported-precision-header", precision, true);
    auto capacity = fixedJpeg;
    qToBigEndian(quint16(32768), reinterpret_cast<uchar *>(capacity.data() + frame + 5));
    qToBigEndian(quint16(32768), reinterpret_cast<uchar *>(capacity.data() + frame + 7));
    add("raster-capacity", capacity, true);
}

void TestDocument::testJpegRejectedOpenKeepsDocument()
{
    QFETCH(QByteArray, encoded);
    QFETCH(bool, gpuSession);
    QFETCH(QString, validCpuModel);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Rejected JPEG artifacts retained at" << directory.path();
        }
    });
    const auto write = [&](const QString &name, const QByteArray &bytes) {
        QFile file(directory.filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    QVERIFY(write(QStringLiteral("source.jpg"), fixedJpeg));
    QVERIFY(write(QStringLiteral("rejected.jpg"), encoded));
    const auto rejected = directory.filePath(QStringLiteral("rejected.jpg"));
    if (!validCpuModel.isEmpty()) {
        qputenv("LIBREPAINT_GPU_ENGINE", "0");
        QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
        reference->setFileBatchMode(true);
        reference->setAutoSaveActive(false);
        QVERIFY2(reference->openPath(rejected, KisDocument::DontAddToRecent), qPrintable(reference->errorMessage()));
        QCOMPARE(reference->image()->colorSpace()->colorModelId().id(), validCpuModel);
    }
    qputenv("LIBREPAINT_GPU_ENGINE", gpuSession ? "1" : "0");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    QVERIFY(document->openPath(directory.filePath(QStringLiteral("source.jpg")), KisDocument::DontAddToRecent));
    KisImageSP image = document->image();
    QCOMPARE(bool(image->gpuImage()), gpuSession);
    QScopedPointer<Node> node(Node::createNode(image, image->root()->firstChild()));
    const auto edit = QByteArray::fromHex("c08040ff");
    QVERIFY(node->setPixelData(edit, 0, 0, 1, 1));
    Document api(document.data(), false);
    api.resizeImage(0, 0, 3, 2);
    image->waitForDone();
    QCOMPARE(image->size(), QSize(3, 2));
    QVERIFY(document->undoStack()->count() > 0);
    document->setModified(true);
    const auto pixels = api.pixelData(0, 0, 3, 2);
    const auto owner = image->gpuImage();
    const auto retainedVersion = owner ? owner->operations().preview() : LibrePaint::Image::Gpu::Snapshot();
    const auto path = document->localFilePath();
    const auto mime = document->mimeType();
    const auto undo = document->undoStack()->count();
    const auto index = document->undoStack()->index();
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Previous JPEG document"));
    QSignalSpy changed(document.data(), &KisDocument::sigCurrentImageChanged);
    QVERIFY(!document->openPath(rejected, KisDocument::DontAddToRecent));
    QCOMPARE(KisImageSP(document->image()), image);
    QCOMPARE(api.pixelData(0, 0, 3, 2), pixels);
    if (owner) QCOMPARE(owner->operations().preview(), retainedVersion);
    QCOMPARE(document->documentInfo()->aboutInfo("title"), QStringLiteral("Previous JPEG document"));
    QCOMPARE(document->localFilePath(), path);
    QCOMPARE(document->mimeType(), mime);
    QCOMPARE(document->undoStack()->count(), undo);
    QCOMPARE(document->undoStack()->index(), index);
    QVERIFY(document->isModified());
    QCOMPARE(changed.count(), 0);
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->size(), QSize(2, 1));
    QCOMPARE(api.pixelData(0, 0, 1, 1), edit);
}
