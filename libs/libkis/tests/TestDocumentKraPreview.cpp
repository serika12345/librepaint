/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestDocument.h"

#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColor.h>
#include <KoStore.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
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
    QTest::newRow("defaults") << false;
    QTest::newRow("fixed-settings") << true;
}

void TestDocument::testSavedSourceOverPixelsAgreeAcrossBackends()
try {
    QFETCH(bool, fixedSettings);
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
            image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, raster), QStringLiteral("Fixed source over"));
        } else {
            image = new KisImage(document->createUndoStore(), 17, 1, KoColorSpaceRegistry::instance()->rgb8(), "Fixed source over");
            KisPaintLayerSP bottom = new KisPaintLayer(image, QStringLiteral("Destination"), 255);
            bottom->setUuid(bottomId);
            QVERIFY(image->addNode(bottom));
            KisPaintLayerSP top = new KisPaintLayer(image, QStringLiteral("Source"), 255);
            top->setUuid(topId);
            QVERIFY(image->addNode(top, image->root(), bottom));
        }
        document->setCurrentImage(image);
        Document publicDocument(document.data(), false);
        QScopedPointer<Node> top(Node::createNode(image, image->root()->lastChild()));
        QScopedPointer<Node> bottom(Node::createNode(image, image->root()->firstChild()));
        QCOMPARE(top->uniqueId(), topId);
        QCOMPARE(bottom->uniqueId(), bottomId);
        QVERIFY(top->setPixelData(QByteArray::fromHex("d3640009").repeated(17), 0, 0, 17, 1));
        QVERIFY(bottom->setPixelData(QByteArray::fromHex("1150c92d").repeated(17), 0, 0, 17, 1));
        publicDocument.refreshProjection();
        image->waitForDone();
        // Exact source-over values are B=4981/99, G=8260/99, R=5494/33, A=891/17.
        QCOMPARE(publicDocument.pixelData(0, 0, 17, 1), QByteArray::fromHex("3253a634").repeated(17));
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
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(merged.constBits()), merged.sizeInBytes()), QByteArray::fromHex("a6533234").repeated(17));
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
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(exported.constBits()), exported.sizeInBytes()), QByteArray::fromHex("a6533234").repeated(17));
        pngFiles.append(bytes);
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
