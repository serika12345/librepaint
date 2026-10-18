/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_kra_loader_test.h"

#include <simpletest.h>

#include <document/KisDocument.h>
#include <metadata/KoDocumentInfo.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpaceEngine.h>

#include "kis_image.h"
#include <testutil.h>
#include "application/ui/orchestration/KisPart.h"

#include <filter/kis_filter_registry.h>
#include <generator/kis_generator_registry.h>

#include "kis_image_animation_interface.h"
#include "kis_keyframe_channel.h"
#include "kis_time_span.h"

#include <filestest.h>

#include <testui.h>
#include <kra/KisKraRasterCodec.h>
#include <QBuffer>
#include <QTemporaryDir>
#include <QColorSpace>
#include <QDir>
#include <QSaveFile>

const QString KraMimetype = "application/x-krita";


void KisKraLoaderTest::initTestCase()
{
    KisFilterRegistry::instance();
    KisGeneratorRegistry::instance();
}

void KisKraLoaderTest::testLoading()
{
    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    QVERIFY(doc->openPath(QString(FILES_DATA_DIR) + '/' + "load_test.kra", KisDocument::DontAddToRecent));
    KisImageSP image = doc->image();
    image->waitForDone();
    QCOMPARE(image->nlayers(), 12);
    QCOMPARE(doc->documentInfo()->aboutInfo("title"), QString("test image for loading"));
    QCOMPARE(image->height(), 753);
    QCOMPARE(image->width(), 1000);
    QCOMPARE(image->colorSpace()->id(), KoColorSpaceRegistry::instance()->rgb8()->id());

    KisNodeSP node = image->root()->firstChild();
    QVERIFY(node);
    QCOMPARE(node->name(), QString("Background"));
    QVERIFY(node->inherits("KisPaintLayer"));

    node = node->nextSibling();
    QVERIFY(node);
    QCOMPARE(node->name(), QString("Group 1"));
    QVERIFY(node->inherits("KisGroupLayer"));
    QCOMPARE((int) node->childCount(), 2);
}

void testObligeSingleChildImpl(bool transpDefaultPixel)
{

    QString id = !transpDefaultPixel ?
        "single_layer_no_channel_flags_nontransp_def_pixel.kra" :
        "single_layer_no_channel_flags_transp_def_pixel.kra";

    QString fileName = TestUtil::fetchDataFileLazy(id);

    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    const bool result = doc->openPath(fileName, KisDocument::DontAddToRecent);
    QVERIFY(result);

    KisImageSP image = doc->image();

    QVERIFY(image);
    QCOMPARE(image->nlayers(), 2);

    KisNodeSP root = image->root();
    KisNodeSP child = root->firstChild();

    QVERIFY(child);

    QCOMPARE(root->original(), root->projection());

    if (transpDefaultPixel) {
        QCOMPARE(root->original(), child->projection());
    } else {
        QVERIFY(root->original() != child->projection());
    }
}

void KisKraLoaderTest::testObligeSingleChild()
{
    testObligeSingleChildImpl(true);
}

void KisKraLoaderTest::testObligeSingleChildNonTranspPixel()
{
    testObligeSingleChildImpl(false);
}

void KisKraLoaderTest::testDirectRasterCompatibility()
{
    namespace Kra = LibrePaint::ImportExport::Kra;
    for (const auto &name : {QStringLiteral("single_layer_no_channel_flags_transp_def_pixel.kra"),
                             QStringLiteral("single_layer_no_channel_flags_nontransp_def_pixel.kra")}) {
        const auto path = QStringLiteral(FILES_DATA_DIR) + QLatin1Char('/') + name;
        QFile input(path);
        QVERIFY(input.open(QIODevice::ReadOnly));
        const auto imported = Kra::RasterCodec::read(input);
        QVERIFY2(imported.code == Kra::Status::Success, qPrintable(imported.diagnostic));
        QScopedPointer<KisDocument> existing(KisPart::instance()->createDocument());
        existing->setFileBatchMode(true);
        QVERIFY(existing->openPath(path, KisDocument::DontAddToRecent));
        existing->image()->waitForDone();
        const auto pixels = existing->image()->root()->firstChild()->paintDevice();
        QVERIFY(pixels);
        const auto &layer = imported.document.image.layers.first();
        for (const auto &tile : layer.tiles) {
            QByteArray bgra(tile.rgba.size(), Qt::Uninitialized);
            pixels->readBytes(reinterpret_cast<quint8 *>(bgra.data()), tile.coordinate.x() * 256, tile.coordinate.y() * 256, 256, 256);
            for (qsizetype i = 0; i < bgra.size(); i += 4) {
                std::swap(bgra[i], bgra[i + 2]);
            }
            QCOMPARE(bgra, tile.rgba);
        }
        const auto deviceDefault = pixels->defaultPixel();
        QCOMPARE(deviceDefault.data()[0], layer.defaultPixel[2]);
        QCOMPARE(deviceDefault.data()[1], layer.defaultPixel[1]);
        QCOMPARE(deviceDefault.data()[2], layer.defaultPixel[0]);
        QCOMPARE(deviceDefault.data()[3], layer.defaultPixel[3]);
    }

    // A host profile can have the embedded ICC's name and different bytes.
    QByteArray hostIcc = QColorSpace(QColorSpace::SRgb).iccProfile();
    hostIcc.replace(80, 4, QByteArray("LPnt"));
    auto *registry = KoColorSpaceRegistry::instance();
    auto *engine = KoColorSpaceEngineRegistry::instance()->get("icc");
    QVERIFY(engine);
    const KoColorProfile *hostProfile = engine->addProfile(hostIcc);
    QVERIFY(hostProfile);
    QCOMPARE(hostProfile->name(), QStringLiteral("sRGB"));
    const KoColorSpace *hostSpace = registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), hostProfile);
    QVERIFY(hostSpace);
    QCOMPARE(hostSpace->profile()->rawData(), hostIcc);

    Kra::Document source;
    source.image.size = {100, 100};
    source.image.activeLayer = 1;
    using LibrePaint::Image::Gpu::RgbaTile;
    source.image.layers = {
        {QStringLiteral("Lower"), true, QUuid(QStringLiteral("{11111111-1111-4111-8111-111111111111}")), {{{-1, 0}, QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Hidden"), false, QUuid(QStringLiteral("{22222222-2222-4222-8222-222222222222}")), {{{0, 0}, QByteArray::fromHex("197fafe0").repeated(RgbaTile::ByteCount / 4)}}}
    };
    source.image.layers[0].defaultPixel = {17, 23, 31, 0};
    source.image.layers[1].defaultPixel = {255, 255, 255, 255};
    QImage projection(source.image.size, QImage::Format_RGBA8888);
    projection.fill(Qt::transparent);
    projection.setColorSpace(QColorSpace::SRgb);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("direct.kra"));
    QFile output(path);
    QVERIFY(output.open(QIODevice::WriteOnly));
    const auto result = Kra::RasterCodec::write(output, source, projection);
    QVERIFY2(result.code == Kra::Status::Success, qPrintable(result.diagnostic));
    output.close();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY(reopened->openPath(path, KisDocument::DontAddToRecent));
    const QString artifactDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    QVERIFY(artifactDirectory.isEmpty() || QDir().mkpath(artifactDirectory));
    auto writeArtifact = [&](const QString &name, const QByteArray &data) {
        if (artifactDirectory.isEmpty()) return true;
        QSaveFile file(QDir(artifactDirectory).filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
    };
    for (int cycle = 0; cycle < 2; ++cycle) {
        if (cycle == 1) {
            const QString originalPath = reopened->localFilePath();
            const bool originalModified = reopened->isModified();
            const QString savedPath = directory.filePath(QStringLiteral("resaved.kra"));
            QVERIFY(reopened->exportDocumentSync(savedPath, KraMimetype.toLatin1()));
            QCOMPARE(reopened->localFilePath(), originalPath);
            QCOMPARE(reopened->isModified(), originalModified);
            QScopedPointer<KisDocument> saved(KisPart::instance()->createDocument());
            saved->setFileBatchMode(true);
            QVERIFY(saved->openPath(savedPath, KisDocument::DontAddToRecent));
            reopened.swap(saved);
        }
        const QString phase = cycle == 0 ? QStringLiteral("opened") : QStringLiteral("resaved");
        QCOMPARE(reopened->image()->profile()->rawData(), QColorSpace(QColorSpace::SRgb).iccProfile());
        QCOMPARE(hostSpace->profile()->rawData(), hostIcc);
        QCOMPARE(registry->profileByName(hostProfile->name())->rawData(), hostIcc);
        QVERIFY(writeArtifact(QStringLiteral("icc-%1-profile.icc").arg(phase), reopened->image()->profile()->rawData()));
        reopened->image()->waitForDone();
        auto node = reopened->image()->root()->firstChild();
        int index = 0;
        for (const auto &layer : source.image.layers) {
            QVERIFY(node);
            QCOMPARE(node->name(), layer.name);
            QCOMPARE(node->visible(), layer.visible);
            QCOMPARE(node->uuid(), layer.uuid);
            const auto pixels = node->paintDevice();
            QVERIFY(pixels);
            QCOMPARE(pixels->colorSpace()->profile()->rawData(), QColorSpace(QColorSpace::SRgb).iccProfile());
            const auto tile = layer.tiles.first();
            QByteArray raw(tile.rgba.size(), Qt::Uninitialized);
            pixels->readBytes(reinterpret_cast<quint8 *>(raw.data()), tile.coordinate.x() * 256, tile.coordinate.y() * 256, 256, 256);
            for (qsizetype i = 0; i < raw.size(); i += 4) std::swap(raw[i], raw[i + 2]);
            QCOMPARE(raw, tile.rgba);
            QVERIFY(writeArtifact(QStringLiteral("icc-%1-layer-%2.rgba").arg(phase).arg(index++), raw));
            const auto fallback = pixels->defaultPixel();
            QCOMPARE(fallback.data()[0], layer.defaultPixel[2]);
            QCOMPARE(fallback.data()[1], layer.defaultPixel[1]);
            QCOMPARE(fallback.data()[2], layer.defaultPixel[0]);
            QCOMPARE(fallback.data()[3], layer.defaultPixel[3]);
            node = node->nextSibling();
        }
        QVERIFY(!node);
    }

    Kra::Document empty;
    empty.image.size = source.image.size;
    const auto emptyPath = directory.filePath(QStringLiteral("direct-empty.kra"));
    QFile emptyOutput(emptyPath);
    QVERIFY(emptyOutput.open(QIODevice::WriteOnly));
    const auto emptyResult = Kra::RasterCodec::write(emptyOutput, empty, projection);
    QVERIFY2(emptyResult.code == Kra::Status::Success, qPrintable(emptyResult.diagnostic));
    emptyOutput.close();
    QFile encodedEmpty(emptyPath);
    QVERIFY(encodedEmpty.open(QIODevice::ReadOnly));
    const auto storedEmpty = Kra::RasterCodec::read(encodedEmpty);
    QCOMPARE(storedEmpty.code, Kra::Status::Success);
    const auto storedMetadata = Kra::RasterCodec::readImageMetadata(storedEmpty.document.description, storedEmpty.document.files);
    QCOMPARE(storedMetadata.code, Kra::Status::Success);
    QVERIFY(!storedMetadata.metadata.iccProfile.isEmpty());
    QScopedPointer<KisDocument> emptyDocument(KisPart::instance()->createDocument());
    emptyDocument->setFileBatchMode(true);
    QVERIFY(emptyDocument->openPath(emptyPath, KisDocument::DontAddToRecent));
    QCOMPARE(emptyDocument->image()->profile()->rawData(), storedMetadata.metadata.iccProfile);
    emptyDocument->image()->waitForDone();
    QCOMPARE(emptyDocument->image()->root()->childCount(), 0U);
    QByteArray emptyPixels(100 * 100 * 4, Qt::Uninitialized);
    emptyDocument->image()->projection()->readBytes(reinterpret_cast<quint8 *>(emptyPixels.data()), 0, 0, 100, 100);
    QCOMPARE(emptyPixels, QByteArray(emptyPixels.size(), '\0'));
}

void KisKraLoaderTest::testLoadAnimated()
{
    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    QVERIFY(doc->openPath(QString(FILES_DATA_DIR) + '/' + "load_test_animation.kra", KisDocument::DontAddToRecent));
    KisImageSP image = doc->image();

    KisNodeSP node1 = image->root()->firstChild();
    KisNodeSP node2 = node1->nextSibling();

    QVERIFY(node1->inherits("KisPaintLayer"));
    QVERIFY(node2->inherits("KisPaintLayer"));

    KisPaintLayerSP layer1 = qobject_cast<KisPaintLayer*>(node1.data());
    KisPaintLayerSP layer2 = qobject_cast<KisPaintLayer*>(node2.data());

    QVERIFY(layer1->isAnimated());
    QVERIFY(!layer2->isAnimated());

    KisKeyframeChannel *channel1 = layer1->getKeyframeChannel(KisKeyframeChannel::Raster.id());
    QVERIFY(channel1);
    QCOMPARE(channel1->keyframeCount(), 3);

    QCOMPARE(image->animationInterface()->framerate(), 17);
    QCOMPARE(image->animationInterface()->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(15, 45));
    QCOMPARE(image->animationInterface()->currentTime(), 19);

    KisPaintDeviceSP dev = layer1->paintDevice();

    const KoColorSpace *cs = dev->colorSpace();
    KoColor transparent(Qt::transparent, cs);
    KoColor white(Qt::white, cs);
    KoColor red(Qt::red, cs);

    image->animationInterface()->switchCurrentTimeAsync(0);
    image->waitForDone();

    QCOMPARE(dev->exactBounds(), QRect(506, 378, 198, 198));
    QCOMPARE(dev->x(), -26);
    QCOMPARE(dev->y(), -128);
    QCOMPARE(dev->defaultPixel(), transparent);

    image->animationInterface()->switchCurrentTimeAsync(20);
    image->waitForDone();

    QCOMPARE(dev->nonDefaultPixelArea(), QRect(615, 416, 129, 129));
    QCOMPARE(dev->x(), 502);
    QCOMPARE(dev->y(), 224);
    QCOMPARE(dev->defaultPixel(), white);

    image->animationInterface()->switchCurrentTimeAsync(30);
    image->waitForDone();

    QCOMPARE(dev->nonDefaultPixelArea(), QRect(729, 452, 45, 44));
    QCOMPARE(dev->x(), 645);
    QCOMPARE(dev->y(), -10);
    QCOMPARE(dev->defaultPixel(), red);
}



void KisKraLoaderTest::testImportFromWriteonly()
{
    TestUtil::testImportFromWriteonly(KraMimetype);
}


void KisKraLoaderTest::testImportIncorrectFormat()
{
    TestUtil::testImportIncorrectFormat(KraMimetype);
}

void KisKraLoaderTest::testLoadingOldHdrProfileNoCicp()
{
    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    QVERIFY(doc->openPath(QString(FILES_DATA_DIR) + '/' + "hdr_gem_old_profile_no_cicp.kra", KisDocument::DontAddToRecent));
    KisImageSP image = doc->image();
    image->waitForDone();
    QCOMPARE(image->colorSpace()->profile()->getColorPrimaries(), PRIMARIES_ITU_R_BT_2020_2_AND_2100_0);
    QCOMPARE(image->colorSpace()->profile()->getTransferCharacteristics(), TRC_ITU_R_BT_2100_0_PQ);
    QCOMPARE(*image->colorSpace()->profile()->hdrReferenceWhite(), 80.0);
}

void KisKraLoaderTest::testLoadingUncommonHdrProfileWithCicp()
{
    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());
    doc->setFileBatchMode(true);
    QVERIFY(doc->openPath(QString(FILES_DATA_DIR) + '/' + "hdr_gem_180nits_rec2100pq_profile.kra", KisDocument::DontAddToRecent));
    KisImageSP image = doc->image();
    image->waitForDone();
    QCOMPARE(image->colorSpace()->profile()->getColorPrimaries(), PRIMARIES_ITU_R_BT_2020_2_AND_2100_0);
    QCOMPARE(image->colorSpace()->profile()->getTransferCharacteristics(), TRC_ITU_R_BT_2100_0_PQ);
    QCOMPARE(*image->colorSpace()->profile()->hdrReferenceWhite(), 180.0);
}



KISTEST_MAIN(KisKraLoaderTest)
