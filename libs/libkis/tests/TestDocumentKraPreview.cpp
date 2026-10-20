/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestDocument.h"

#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <Channel.h>
#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuExecution.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColor.h>
#include <KoStore.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_paint_device.h>
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <exception>

void TestDocument::testSavedSourceOverPixelsAgreeAcrossBackends_data()
{
    QTest::addColumn<bool>("fixedSettings");
    QTest::addColumn<QByteArray>("sourcePixel");
    QTest::addColumn<QByteArray>("destinationPixel");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<QByteArray>("expectedPixel");
    QTest::addColumn<bool>("sourceGroup");
    QTest::addColumn<bool>("hiddenBackdrop");
    QTest::addColumn<int>("projectionMask");
    struct Case { const char *name, *source, *destination; int opacity; const char *expected; };
    const Case cases[] = {
        {"", "d3640009", "1150c92d", 255, "3253a634"},
        {"quotient-102-50", "12345666", "33669932", 255, "1a3f6584"},
        {"quotient-168-136", "123456a8", "33669988", 255, "193f65d6"},
        {"quotient-180-180", "123456b4", "336699b4", 255, "1a3f65e9"},
        {"quotient-170-238", "123456aa", "336699ee", 255, "1c446bf9"},
        {"partial-opacity", "12345686", "336699e2", 128, "295886ea"},
        {"small-alpha-partial-opacity", "12345601", "33669901", 128, "28558301"},
        {"low-opacity-rounds-after-color", "0034567f", "ff6699fe", 1, "fe6699fe"},
    };
    // Scalar and batched production composition agree on these fixed pixels
    // on ARM64 and x86_64; normal saving must retain the same values.
    for (const auto &c : cases) for (const bool fixed : {false, true}) {
        const QByteArray name = QByteArray(c.name) + (c.name[0] ? "-" : "") +
            (fixed ? "fixed-settings" : "defaults");
        QTest::newRow(name.constData()) << fixed << QByteArray::fromHex(c.source) <<
            QByteArray::fromHex(c.destination) << c.opacity << QByteArray::fromHex(c.expected) << false << false << 15;
    }
    for (const bool fixed : {false, true}) {
        const QByteArray name = QByteArrayLiteral("nested-low-opacity-") + (fixed ? "fixed-settings" : "defaults");
        QTest::newRow(name.constData()) << fixed << QByteArray::fromHex("11223301") <<
            QByteArray::fromHex("00000000") << 1 << QByteArray::fromHex("00000000") << true << true << 15;
    }
    for (const bool group : {false, true}) for (const int mask : {0, 1, 7, 8}) for (const bool fixed : {false, true}) {
        const QByteArray name = QByteArrayLiteral("projection-") + QByteArray::number(mask, 16) +
            (group ? "-group-" : "-paint-") + (fixed ? "fixed-settings" : "defaults");
        QTest::newRow(name.constData()) << fixed << QByteArray::fromHex("12345686") <<
            QByteArray::fromHex("336699e2") << 128 << QByteArray() << group << false << mask;
    }
}

void TestDocument::testSavedSourceOverPixelsAgreeAcrossBackends()
try {
    QFETCH(bool, fixedSettings);
    QFETCH(QByteArray, sourcePixel);
    QFETCH(QByteArray, destinationPixel);
    QFETCH(int, opacity);
    QFETCH(QByteArray, expectedPixel);
    QFETCH(bool, sourceGroup);
    QFETCH(bool, hiddenBackdrop);
    QFETCH(int, projectionMask);
    QByteArray referencePixel = expectedPixel;
    namespace Gpu = LibrePaint::Image::Gpu;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Saved source-over comparison artifacts retained at" << destination;
        }
    });
    QList<QByteArray> pngFiles;
    for (const bool gpu : {false, true}) {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        const QUuid bottomId(QStringLiteral("{d3000001-1122-3344-5566-778899aabbcc}"));
        const QUuid topId(QStringLiteral("{d3000002-1122-3344-5566-778899aabbcc}"));
        const QUuid groupId(QStringLiteral("{d3000003-1122-3344-5566-778899aabbcc}"));
        KisImageSP image;
        if (gpu) {
            QString diagnostic;
            const auto execution = KisPart::instance()->gpuExecution(diagnostic);
            QVERIFY2(execution, qPrintable(diagnostic));
            Gpu::RasterImage raster;
            raster.size = {17, 1};
            raster.layers = {{QStringLiteral("Destination"), true, bottomId, {}},
                             {QStringLiteral("Source"), true, topId, {}}};
            raster.activeLayer = 1;
            if (sourceGroup) {
                Gpu::RasterLayer group{QStringLiteral("Source group"), true, groupId, {}};
                group.kind = Gpu::LayerKind::Group;
                raster.layers.insert(1, group);
                raster.layers.last().parentUuid = groupId;
                raster.activeLayer = 2;
            }
            image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, raster), QStringLiteral("Fixed source over"));
        } else {
            image = new KisImage(document->createUndoStore(), 17, 1, KoColorSpaceRegistry::instance()->rgb8(), "Fixed source over");
            KisPaintLayerSP bottom = new KisPaintLayer(image, QStringLiteral("Destination"), 255);
            bottom->setUuid(bottomId);
            QVERIFY(image->addNode(bottom));
            KisPaintLayerSP top = new KisPaintLayer(image, QStringLiteral("Source"), 255);
            top->setUuid(topId);
            if (sourceGroup) {
                KisNodeSP group = new KisGroupLayer(image, QStringLiteral("Source group"), 255);
                group->setUuid(groupId);
                QVERIFY(image->addNode(group, image->root(), bottom));
                QVERIFY(image->addNode(top, group));
            } else QVERIFY(image->addNode(top, image->root(), bottom));
        }
        document->setCurrentImage(image);
        Document publicDocument(document.data(), false);
        QScopedPointer<Node> top(Node::createNode(image, sourceGroup ? image->root()->lastChild()->firstChild() : image->root()->lastChild()));
        QScopedPointer<Node> bottom(Node::createNode(image, image->root()->firstChild()));
        QCOMPARE(top->uniqueId(), topId);
        QCOMPARE(bottom->uniqueId(), bottomId);
        QVERIFY(top->setPixelData(sourcePixel.repeated(17), 0, 0, 17, 1));
        if (hiddenBackdrop) bottom->setVisible(false);
        else QVERIFY(bottom->setPixelData(destinationPixel.repeated(17), 0, 0, 17, 1));
        top->setOpacity(opacity);
        QScopedPointer<Node> projectionNode(Node::createNode(image, image->root()->lastChild()));
        const auto channels = projectionNode->channels();
        const auto releaseChannels = qScopeGuard([&] { qDeleteAll(channels); });
        for (auto *channel : channels) channel->setVisible(projectionMask & (1 << channel->position()));
        publicDocument.refreshProjection();
        image->waitForDone();
        QCOMPARE(top->pixelData(0, 0, 17, 1), sourcePixel.repeated(17));
        if (hiddenBackdrop) {
            QScopedPointer<Node> group(Node::createNode(image, image->root()->lastChild()));
            QCOMPARE(group->projectionPixelData(0, 0, 17, 1), QByteArray::fromHex("11223300").repeated(17));
        }
        if (!gpu && referencePixel.isEmpty()) referencePixel = publicDocument.pixelData(0, 0, 1, 1);
        QCOMPARE(publicDocument.pixelData(0, 0, 17, 1), referencePixel.repeated(17));
        QByteArray expectedRgba;
        expectedRgba.append(referencePixel[2]); expectedRgba.append(referencePixel[1]);
        expectedRgba.append(referencePixel[0]); expectedRgba.append(referencePixel[3]);
        document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed source-over document"));
        document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
        document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
        document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
        document->setFileBatchMode(true);
        const auto prefix = QDir(destination).filePath(QStringLiteral("source-over-rounding-%1-%2").arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? QStringLiteral("gpu") : QStringLiteral("ordinary")));
        KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
        configuration->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
        QVERIFY2(document->saveAs(prefix + QStringLiteral(".kra"), KisDocument::nativeFormatMimeType(), false, configuration), qPrintable(document->errorMessage()));
        document->waitForSavingToComplete();
        QScopedPointer<KoStore> store(KoStore::createStore(prefix + QStringLiteral(".kra"), KoStore::Read));
        QVERIFY(store && !store->bad());
        QVERIFY(store->open(QStringLiteral("mergedimage.png")));
        const auto merged = QImage::fromData(store->device()->readAll()).convertToFormat(QImage::Format_RGBA8888);
        QVERIFY(store->close());
        QCOMPARE(merged.size(), QSize(17, 1));
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(merged.constBits()), merged.sizeInBytes()), expectedRgba.repeated(17));
        InfoObject settings;
        if (fixedSettings) {
            settings.setProperty("alpha", true);
            settings.setProperty("interlaced", false);
            settings.setProperty("compression", 3);
            settings.setProperty("saveSRGBProfile", true);
            settings.setProperty("forceSRGB", false);
            settings.setProperty("storeAuthor", false);
            settings.setProperty("storeMetaData", false);
        }
        QVERIFY(publicDocument.exportImage(prefix + QStringLiteral(".png"), settings));
        QFile png(prefix + QStringLiteral(".png"));
        QVERIFY(png.open(QIODevice::ReadOnly));
        const auto bytes = png.readAll();
        const auto exported = QImage::fromData(bytes).convertToFormat(QImage::Format_RGBA8888);
        QCOMPARE(exported.size(), QSize(17, 1));
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(exported.constBits()), exported.sizeInBytes()), expectedRgba.repeated(17));
        pngFiles.append(bytes);
        if (projectionMask != 15) {
            QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
            reopened->setFileBatchMode(true);
            QVERIFY2(reopened->openPath(prefix + QStringLiteral(".kra"), KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
            reopened->image()->waitForDone();
            Document reopenedApi(reopened.data(), false);
            QCOMPARE(reopenedApi.pixelData(0, 0, 17, 1), referencePixel.repeated(17));
            QScopedPointer<Node> reopenedProjection(Node::createNode(reopened->image(), reopened->image()->root()->lastChild()));
            const auto reopenedChannels = reopenedProjection->channels();
            const auto releaseReopenedChannels = qScopeGuard([&] { qDeleteAll(reopenedChannels); });
            for (auto *channel : reopenedChannels)
                QCOMPARE(channel->visible(), bool(projectionMask & (1 << channel->position())));
            QString diagnostic;
            const auto execution = KisPart::instance()->gpuExecution(diagnostic);
            QVERIFY2(execution, qPrintable(diagnostic));
            namespace Files = LibrePaint::Document::Gpu;
            auto loaded = Files::PaintDocument::openKra(execution, prefix + QStringLiteral(".kra"));
            QVERIFY2(loaded.code == Files::FileCode::Success, qPrintable(loaded.diagnostic));
            const auto loadedOwner = loaded.document->image();
            QCOMPARE(loadedOwner->readRgbaPixels(0, {0, 0, 17, 1}, 17 * 4).bytes, expectedRgba.repeated(17));
            auto loadedView = KisImage::fromGpuImage(loadedOwner, QStringLiteral("Reopened projection channels"));
            const auto loadedTarget = loadedView->root()->lastChild();
            QScopedPointer<Node> loadedSource(Node::createNode(loadedView, sourceGroup ? loadedTarget->firstChild() : loadedTarget));
            QCOMPARE(loadedSource->pixelData(0, 0, 17, 1), sourcePixel.repeated(17));
            KisPart::instance()->removeDocument(reopened.data(), false);
        }
        KisPart::instance()->removeDocument(document.data(), false);
    }
    QCOMPARE(pngFiles[1], pngFiles[0]);
}
catch (const std::exception &error) { QFAIL(error.what()); }

void TestDocument::testSavedKraEmbeddedPngsAgreeAcrossBackends_data()
{
    QTest::addColumn<QSize>("sourceSize");
    QTest::newRow("three-pixels") << QSize(3, 1);
    QTest::newRow("integer-enlargement") << QSize(32, 16);
    QTest::newRow("fractional-enlargement") << QSize(33, 17);
    QTest::newRow("area-reduction") << QSize(513, 257);
    QTest::newRow("thin-document") << QSize(1, 1025);
}

void TestDocument::testSavedKraEmbeddedPngsAgreeAcrossBackends()
try {
    QFETCH(QSize, sourceSize);
    namespace Gpu = LibrePaint::Image::Gpu;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Saved KRA preview comparison artifacts retained at" << destination;
        }
    });
    QByteArray bgra(sourceSize.width() * sourceSize.height() * 4, Qt::Uninitialized);
    for (int y = 0; y < sourceSize.height(); ++y) for (int x = 0; x < sourceSize.width(); ++x) {
        const int alpha = (x + y) % 3 == 1 ? 0 : (x + y) % 3 == 2 ? 128 : 255;
        auto *pixel = bgra.data() + (y * sourceSize.width() + x) * 4;
        pixel[0] = char(alpha ? (x * 13 + y * 7 + 51) % 256 : 0);
        pixel[1] = char(alpha ? (x * 19 + y * 11 + 34) % 256 : 0);
        pixel[2] = char(alpha ? (x * 29 + y * 17 + 17) % 256 : 0);
        pixel[3] = char(alpha);
    }
    QList<QImage> mergedImages, previews;
    QList<QByteArray> mergedPngs, previewPngs;
    for (bool gpu : {false, true}) {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        const QUuid id(QStringLiteral("{d2000001-1122-3344-5566-778899aabbcc}"));
        KisImageSP image;
        if (gpu) {
            QString diagnostic;
            const auto execution = KisPart::instance()->gpuExecution(diagnostic);
            QVERIFY2(execution, qPrintable(diagnostic));
            Gpu::RasterImage raster;
            raster.size = sourceSize;
            raster.layers = {{QStringLiteral("Preview layer"), true, id, {}}};
            image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, raster), QStringLiteral("Preview image"));
        } else {
            image = new KisImage(document->createUndoStore(), sourceSize.width(), sourceSize.height(),
                                 KoColorSpaceRegistry::instance()->rgb8(), "Preview image");
            auto layer = new KisPaintLayer(image, QStringLiteral("Preview layer"), 255);
            layer->setUuid(id);
            image->addNode(layer);
        }
        document->setCurrentImage(image);
        Document publicDocument(document.data(), false);
        QScopedPointer<Node> layer(Node::createNode(image, image->root()->firstChild()));
        QVERIFY(layer->setPixelData(bgra, 0, 0, sourceSize.width(), sourceSize.height()));
        publicDocument.refreshProjection();
        image->waitForDone();
        document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed preview document"));
        document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
        document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
        document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
        document->setFileBatchMode(true);
        const auto path = QDir(destination).filePath(QStringLiteral("saved-preview-%1-%2.kra")
            .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? QStringLiteral("gpu") : QStringLiteral("ordinary")));
        KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
        configuration->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
        QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false, configuration), qPrintable(document->errorMessage()));
        document->waitForSavingToComplete();
        QScopedPointer<KoStore> store(KoStore::createStore(path, KoStore::Read));
        QVERIFY(store && !store->bad());
        for (const auto &name : {QStringLiteral("mergedimage.png"), QStringLiteral("preview.png")}) {
            QVERIFY(store->open(name));
            const auto bytes = store->device()->readAll();
            const auto png = QImage::fromData(bytes).convertToFormat(QImage::Format_RGBA8888);
            QVERIFY(store->close());
            QVERIFY(!png.isNull());
            (name == QLatin1String("mergedimage.png") ? mergedImages : previews).append(png);
            (name == QLatin1String("mergedimage.png") ? mergedPngs : previewPngs).append(bytes);
        }
        KisPart::instance()->removeDocument(document.data(), false);
    }
    const auto rgba = [](const QImage &image) {
        return QByteArray(reinterpret_cast<const char *>(image.constBits()), image.sizeInBytes());
    };
    QCOMPARE(mergedImages[1].size(), mergedImages[0].size());
    QCOMPARE(rgba(mergedImages[1]), rgba(mergedImages[0]));
    QCOMPARE(previews[1].size(), previews[0].size());
    QCOMPARE(rgba(previews[1]), rgba(previews[0]));
    QCOMPARE(mergedPngs[1], mergedPngs[0]);
    QCOMPARE(previewPngs[1], previewPngs[0]);
}
catch (const std::exception &error) { QFAIL(error.what()); }

void TestDocument::testSavedKraPreviewKeepsOtherColorSpaces_data()
{
    QTest::addColumn<bool>("sixteenBits");
    QTest::newRow("linear-rgba8") << false;
    QTest::newRow("srgb-rgba16") << true;
}

void TestDocument::testSavedKraPreviewKeepsOtherColorSpaces()
{
    QFETCH(bool, sixteenBits);
    auto *registry = KoColorSpaceRegistry::instance();
    const KoColorSpace *cs = nullptr;
    if (sixteenBits) cs = registry->rgb16();
    else {
        const auto *profile = registry->profileByName(QStringLiteral("sRGB-elle-V2-g10.icc"));
        QVERIFY(profile);
        cs = registry->rgb8(profile);
    }
    QVERIFY(cs);
    QVERIFY(!(*cs == *registry->rgb8()));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(document->createUndoStore(), 33, 17, cs, "Other preview color space");
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Preview colors"), 255, cs);
    layer->paintDevice()->fill(QRect(0, 0, 16, 17), KoColor(QColor(33, 77, 129, 128), cs));
    layer->paintDevice()->fill(QRect(16, 0, 17, 17), KoColor(QColor(91, 19, 219, 255), cs));
    image->addNode(layer);
    document->setCurrentImage(image);
    image->initialRefreshGraph();
    image->waitForDone();
    const auto expected = document->generatePreviewImage(QSize(256, 256)).convertToFormat(QImage::Format_RGBA8888);
    QVERIFY(!expected.isNull());
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Other color-space preview artifacts retained at" << directory.path();
        }
    });
    QVERIFY(expected.save(directory.filePath(QStringLiteral("expected.png"))));
    document->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("preview.kra"));
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KoStore> store(KoStore::createStore(path, KoStore::Read));
    QVERIFY(store && !store->bad());
    QVERIFY(store->open(QStringLiteral("preview.png")));
    const auto actual = QImage::fromData(store->device()->readAll()).convertToFormat(QImage::Format_RGBA8888);
    QVERIFY(store->close());
    QVERIFY(actual.save(directory.filePath(QStringLiteral("actual.png"))));
    QCOMPARE(actual.size(), expected.size());
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(actual.constBits()), actual.sizeInBytes()),
             QByteArray(reinterpret_cast<const char *>(expected.constBits()), expected.sizeInBytes()));
    KisPart::instance()->removeDocument(document.data(), false);
}
