/* SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "TestDocument.h"
#include <simpletest.h>

#include <KritaVersionWrapper.h>
#include <QColor>
#include <QDataStream>
#include <QDir>
#include <QBuffer>
#include <QSignalSpy>
#include <QTextStream>
#include <QFile>
#include <QTemporaryDir>
#include <QDomDocument>
#include <QColorSpace>
#include <QScopeGuard>
#include <exception>

#include <Node.h>
#include <Krita.h>
#include <Document.h>
#include <InfoObject.h>
#include <metadata/KoDocumentInfo.h>
#include <kra/KisKraRasterCodec.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>

#include <document/KisDocument.h>
#include <kis_image.h>
#include <kis_annotation.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <kis_fill_painter.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <application/ui/orchestration/KisPart.h>

#include <kis_transform_mask_params_factory_registry.h>
#include <kis_undo_stores.h>
#include <testui.h>

#include <KisPortingUtils.h>

void TestDocument::testOrdinaryCreationUsesGpuAndPreservesInitialDocumentState_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<int>("layerCount");
    QTest::newRow("raster-one-layer") << int(KisConfig::RASTER_LAYER) << 1;
    QTest::newRow("raster-two-layers") << int(KisConfig::RASTER_LAYER) << 2;
    QTest::newRow("canvas-one-layer") << int(KisConfig::CANVAS_COLOR) << 1;
    QTest::newRow("canvas-two-layers") << int(KisConfig::CANVAS_COLOR) << 2;
}

void TestDocument::testOrdinaryCreationUsesGpuAndPreservesInitialDocumentState()
{
    QFETCH(int, style);
    QFETCH(int, layerCount);
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoColor color(QColor(17, 34, 51, 128), cs);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QVERIFY2(document->newImage(QStringLiteral("Ordinary GPU document"), 32, 16, cs, color,
                                KisConfig::BackgroundStyle(style), layerCount, QStringLiteral("Kept description"), 2),
             qPrintable(document->errorMessage()));
    const auto image = KisImageSP(document->image());
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    const auto version = owner->operations().snapshot();
    QCOMPARE(version->size, QSize(32, 16));
    QCOMPARE(version->layers.size(), layerCount);
    QCOMPARE(version->activeLayer, version->layers.last().id);
    QCOMPARE(owner->operations().undoCount(), 0);
    QVERIFY(!document->isModified());
    QCOMPARE(image->xRes(), 2.0);
    QCOMPARE(image->yRes(), 2.0);
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Ordinary GPU document"));
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("abstract")), QStringLiteral("Kept description"));
    QCOMPARE(image->colorSpace(), cs);
    const bool raster = style == int(KisConfig::RASTER_LAYER);
    QCOMPARE(image->root()->firstChild()->userLocked(), raster && layerCount > 1);
    QCOMPARE(image->root()->firstChild()->opacity(), raster ? quint8(128) : quint8(255));
    QCOMPARE(image->root()->firstChild()->name(), QStringLiteral("Background"));
    const bool autoPin = KisConfig(false).autoPinLayersToTimeline();
    QCOMPARE(image->root()->firstChild()->isPinnedToTimeline(), raster && autoPin);
    if (layerCount > 1) QCOMPARE(image->root()->lastChild()->isPinnedToTimeline(), autoPin);
    Document publicDocument(document.data(), false);
    QCOMPARE(publicDocument.pixelData(3, 3, 1, 1), QByteArray::fromHex("33221180"));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    document->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("new.kra"));
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QVERIFY(reopened->image()->gpuImage());
    Document publicReopened(reopened.data(), false);
    QCOMPARE(publicReopened.pixelData(3, 3, 1, 1), publicDocument.pixelData(3, 3, 1, 1));
    QCOMPARE(reopened->image()->defaultProjectionColor(), image->defaultProjectionColor());
    QCOMPARE(reopened->image()->root()->firstChild()->userLocked(), raster && layerCount > 1);
    QCOMPARE(reopened->image()->root()->firstChild()->opacity(), raster ? quint8(128) : quint8(255));
    QCOMPARE(reopened->image()->root()->firstChild()->isPinnedToTimeline(), raster && autoPin);
    if (layerCount > 1) QCOMPARE(reopened->image()->root()->lastChild()->isPinnedToTimeline(), autoPin);
    QCOMPARE(reopened->documentInfo()->aboutInfo(QStringLiteral("abstract")), QStringLiteral("Kept description"));
    const auto original = document->image();
    QVERIFY(!document->newImage(QStringLiteral("Invalid size"), 0, 16, cs, color,
                                KisConfig::BackgroundStyle(style), 1, {}, 1));
    QCOMPARE(document->image(), original);
    QVERIFY(!document->newImage(QStringLiteral("Too many layers"), 32, 16, cs, color,
                                KisConfig::BackgroundStyle(style), 4097, {}, 1));
    QCOMPARE(document->image(), original);
    QVERIFY(!document->isModified());
}

void TestDocument::testBackgroundApiKeepsHistoryAndPinnedPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QVERIFY(document->newImage(QStringLiteral("Background API"), 32, 16, cs,
                                KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    const auto image = KisImageSP(document->image());
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("ff0000ff")), Gpu::Result::Success);
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(6, 6, 1, 1), QByteArray::fromHex("00ff00ff")), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto pinned = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(pinned), Gpu::Result::Success);
    Document publicDocument(document.data(), false);
    QVERIFY(!publicDocument.modified());
    const auto pending = owner->beginStroke(layer, Gpu::Brush{}, 17);
    QVERIFY(!pending.isNull());
    QCOMPARE(owner->appendSamples(pending, {{{8, 8}, 1, 0, 0, 0, 1}}), Gpu::Result::Success);
    QSignalSpy modified(document.data(), &KisDocument::modified);
    const QColor background(17, 34, 51, 128);
    try {
        QVERIFY(publicDocument.setBackgroundColor(background));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QCOMPARE(publicDocument.backgroundColor(), background);
    QCOMPARE(owner->operations().activeOperation(), pending);
    QCOMPARE(owner->operations().preview()->sampleCount, qsizetype(1));
    QCOMPARE(owner->cancelStroke(pending), Gpu::Result::Success);
    QCOMPARE(publicDocument.pixelData(0, 0, 1, 1), QByteArray::fromHex("33221180"));
    QCOMPARE(publicDocument.pixelData(-1, -1, 1, 1), QByteArray::fromHex("33221180"));
    QVERIFY(publicDocument.modified());
    QCOMPARE(modified.size(), 1);
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().redoCount(), 1);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("ff0000ff"));
    QCOMPARE(owner->execution().readRgbaPixels(owner->handle(), pinned, QRect(0, 0, 1, 1), 0, 4).bytes,
             QByteArray(4, '\0'));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(publicDocument.backgroundColor(), background);
    QCOMPARE(publicDocument.pixelData(6, 6, 1, 1), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(publicDocument.pixelData(6, 6, 1, 1), QByteArray::fromHex("33221180"));
    QVERIFY(publicDocument.modified());
    const auto recovery = owner->operations().recoveryData();
    QCOMPARE(owner->restoreRecovery(recovery), Gpu::Result::Success);
    QCOMPARE(publicDocument.backgroundColor(), background);
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().redoCount(), 1);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(publicDocument.backgroundColor(), background);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("background-edited.kra"));
    document->setFileBatchMode(true);
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    QVERIFY(!publicDocument.modified());
    QCOMPARE(owner->operations().undoCount(), 2);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    Document publicReopened(reopened.data(), false);
    QCOMPARE(publicReopened.backgroundColor(), background);
    QCOMPARE(publicReopened.pixelData(0, 0, 1, 1), QByteArray::fromHex("33221180"));
}

void TestDocument::testBackgroundNotificationLifetimes_data()
{
    QTest::addColumn<bool>("replace");
    QTest::newRow("close") << false;
    QTest::newRow("replace") << true;
}

void TestDocument::testBackgroundNotificationLifetimes()
{
    QFETCH(bool, replace);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QPointer<KisDocument> document(KisPart::instance()->createDocument());
    QVERIFY(document->newImage(QStringLiteral("Background notification"), 32, 16, cs,
                               KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    const auto owner = document->image()->gpuImage();
    LibrePaint::Image::Gpu::PaintImageSP replacement;
    const auto cleanup = qScopeGuard([&] { delete document.data(); });
    Document publicDocument(document.data(), false);
    QObject::connect(document.data(), &KisDocument::modified, &publicDocument, [&](bool modified) {
        if (!modified) return;
        if (replace) {
            replacement = owner->clone();
            document->setCurrentImage(KisImage::fromGpuImage(replacement, QStringLiteral("Background replacement")));
        } else {
            delete document.data();
        }
    });
    const QColor background(17, 34, 51, 128);
    try {
        QVERIFY(publicDocument.setBackgroundColor(background));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    if (replace) {
        QVERIFY(document);
        QCOMPARE(document->image()->gpuImage(), replacement);
        QVERIFY(!document->isModified());
    } else {
        QVERIFY(document.isNull());
    }
    const std::array<quint8, 4> expected{17, 34, 51, 128};
    QCOMPARE(owner->operations().snapshot()->projectionBackground, expected);
    QCOMPARE(owner->readRgbaPixels(0, QRect(0, 0, 1, 1), 4).bytes, QByteArray::fromHex("11223380"));
}

void TestDocument::testCanvasBackgroundReferencePixels_data()
{
    QTest::addColumn<QColor>("background");
    QTest::addColumn<bool>("hiddenPixel");
    QTest::addColumn<QByteArray>("inside");
    QTest::addColumn<QByteArray>("outside");
    QTest::addColumn<int>("layerCount");
    QTest::addColumn<int>("pixelAlpha");
    QTest::newRow("opaque") << QColor(17, 34, 51, 255) << false << QByteArray::fromHex("332211ff") << QByteArray::fromHex("332211ff") << 1 << 0;
    QTest::newRow("partial") << QColor(17, 34, 51, 128) << false << QByteArray::fromHex("33221180") << QByteArray::fromHex("33221180") << 1 << 0;
    QTest::newRow("transparent") << QColor(255, 255, 255, 0) << false << QByteArray::fromHex("00000000") << QByteArray::fromHex("00000000") << 1 << 0;
    QTest::newRow("opaque-hidden-rgb") << QColor(17, 34, 51, 255) << true << QByteArray::fromHex("332211ff") << QByteArray::fromHex("332211ff") << 1 << 0;
    QTest::newRow("partial-hidden-rgb") << QColor(17, 34, 51, 128) << true << QByteArray::fromHex("33221180") << QByteArray::fromHex("33221180") << 1 << 0;
    QTest::newRow("transparent-hidden-rgb") << QColor(255, 255, 255, 0) << true << QByteArray::fromHex("66554400") << QByteArray::fromHex("00000000") << 1 << 0;
    QTest::newRow("transparent-two-layers") << QColor(255, 255, 255, 0) << false << QByteArray::fromHex("ffffff00") << QByteArray::fromHex("ffffff00") << 2 << 0;
    QTest::newRow("transparent-hidden-rgb-two-layers") << QColor(255, 255, 255, 0) << true << QByteArray::fromHex("ffffff00") << QByteArray::fromHex("ffffff00") << 2 << 0;
    QTest::newRow("opaque-colored-pixel") << QColor(17, 34, 51, 255) << true << QByteArray::fromHex("4d3c2bff") << QByteArray::fromHex("332211ff") << 1 << 128;
    QTest::newRow("partial-colored-pixel") << QColor(17, 34, 51, 128) << true << QByteArray::fromHex("554433c0") << QByteArray::fromHex("33221180") << 1 << 128;
    QTest::newRow("transparent-colored-pixel") << QColor(255, 255, 255, 0) << true << QByteArray::fromHex("66554480") << QByteArray::fromHex("00000000") << 1 << 128;
}

void TestDocument::testCanvasBackgroundReferencePixels()
{
    QFETCH(QColor, background);
    QFETCH(bool, hiddenPixel);
    QFETCH(QByteArray, inside);
    QFETCH(QByteArray, outside);
    QFETCH(int, layerCount);
    QFETCH(int, pixelAlpha);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const KoColor color(background, cs);
    KisImageSP image = new KisImage(nullptr, 32, 16, cs, QStringLiteral("Canvas background reference"));
    image->setDefaultProjectionColor(color);
    for (int i = 0; i < layerCount; ++i)
        image->addNode(new KisPaintLayer(image, QStringLiteral("Paint Layer %1").arg(i + 1), 255), image->root(), i);
    document->setCurrentImage(image);
    image->initialRefreshGraph();
    if (hiddenPixel) {
        auto pixel = QByteArray::fromHex("66554400");
        pixel[3] = char(pixelAlpha);
        image->root()->firstChild()->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 3, 3, 1, 1);
        image->root()->firstChild()->setDirty(QRect(3, 3, 1, 1));
    }
    image->waitForDone();
    QCOMPARE(image->defaultProjectionColor(), color);
    Document publicDocument(document.data(), false);
    QCOMPARE(publicDocument.pixelData(3, 3, 1, 1), inside);
    QCOMPARE(publicDocument.pixelData(-1, -1, 1, 1), outside);
    const auto referenceBounds = image->projection()->exactBounds();
    const auto referenceDefaultPixel = image->projection()->defaultPixel();
    const auto referenceThumbnail = publicDocument.thumbnail(32, 16);

    namespace Gpu = LibrePaint::Image::Gpu;
    Gpu::RasterImage input;
    input.size = {32, 16};
    for (int i = 0; i < layerCount; ++i) input.layers.append({QStringLiteral("Paint Layer %1").arg(i + 1), true, {}, {}});
    input.projectionBackground = {quint8(background.red()), quint8(background.green()),
                                   quint8(background.blue()), quint8(background.alpha())};
    if (hiddenPixel) {
        Gpu::RgbaTile tile{{0, 0}, QByteArray(Gpu::RgbaTile::ByteCount, '\0')};
        auto pixel = QByteArray::fromHex("44556600");
        pixel[3] = char(pixelAlpha);
        tile.rgba.replace((3 * 256 + 3) * 4, 4, pixel);
        input.layers[0].tiles = {tile};
    }
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    QScopedPointer<KisDocument> target(KisPart::instance()->createDocument());
    auto gpuImage = KisImage::fromGpuImage(owner, QStringLiteral("Canvas background GPU"));
    target->setCurrentImage(gpuImage);
    QCOMPARE(gpuImage->defaultProjectionColor(), color);
    QCOMPARE(gpuImage->rootLayer()->defaultProjectionColor(), color);
    QCOMPARE(gpuImage->projection()->defaultPixel(), referenceDefaultPixel);
    Document publicTarget(target.data(), false);
    QCOMPARE(publicTarget.pixelData(3, 3, 1, 1), inside);
    QCOMPARE(publicTarget.pixelData(-1, -1, 1, 1), outside);
    QCOMPARE(gpuImage->projection()->exactBounds(), referenceBounds);
    QCOMPARE(publicTarget.thumbnail(32, 16), referenceThumbnail);
}


void TestDocument::testSetColorSpace()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    QStringList profiles = Krita().profiles("GRAYA", "U16");
    d.setColorSpace("GRAYA", "U16", profiles.first());

    QVERIFY(layer->colorSpace()->colorModelId().id() == "GRAYA");
    QVERIFY(layer->colorSpace()->colorDepthId().id() == "U16");
    QVERIFY(layer->colorSpace()->profile()->name() == profiles.first());

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testSetColorProfile()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QStringList profiles = Krita().profiles("RGBA", "U8");
    Q_FOREACH(const QString &profileName, profiles) {
        const KoColorProfile *profile = KoColorSpaceRegistry::instance()->profileByName(profileName);

        // skip input-only profiles (e.g. for scanners)
        if (!profile->isSuitableForOutput()) continue;

        d.setColorProfile(profileName);
        QVERIFY(image->colorSpace()->profile()->name() == profileName);
    }
    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testPixelData()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    d.refreshProjection();

    QByteArray ba = d.pixelData(0, 0, 100, 100);
    QDataStream ds(ba);
    do {
        quint8 channelvalue;
        ds >> channelvalue;
        QVERIFY(channelvalue == 0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 255);
        ds >> channelvalue;
        QVERIFY(channelvalue == 255);
    } while (!ds.atEnd());

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testGpuImagePixelDataModifiedStateAndRetainedNode()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    std::weak_ptr<Gpu::Execution> lifetime = execution;
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto layerId = owner->operations().activeLayer();
    QCOMPARE(owner->writeRgbaPixels(layerId, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Public GPU document"));
    kisdoc->setCurrentImage(image);
    Document document(kisdoc.data(), false);
    QCOMPARE(document.width(), 64);
    QCOMPARE(document.height(), 32);
    QCOMPARE(document.pixelData(5, 5, 1, 1), QByteArray::fromHex("332211ff"));
    QVERIFY(!document.modified());
    QScopedPointer<Node> root(document.rootNode());
    const auto children = root->childNodes();
    QCOMPARE(children.size(), 1);
    NodeSP node(children.front());
    root.reset();
    QSignalSpy modified(kisdoc.data(), &KisDocument::modified);
    QVERIFY(node->setPixelData(QByteArray::fromHex("90a4bcff"), 5, 5, 1, 1));
    document.refreshProjection();
    QCOMPARE(document.pixelData(5, 5, 1, 1), QByteArray::fromHex("90a4bcff"));
    QVERIFY(!document.modified());
    QVERIFY(modified.isEmpty());
    QCOMPARE(owner->replaceRgbaPixels(layerId, QRect(5, 5, 1, 1), QByteArray::fromHex("223344ff")), Gpu::Result::Success);
    QVERIFY(document.modified());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!document.modified());
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(document.modified());
    QCOMPARE(modified.size(), 3);
    QCOMPARE(document.pixelData(5, 5, 1, 1), QByteArray::fromHex("443322ff"));
    owner.clear();
    image.clear();
    KisPart::instance()->removeDocument(kisdoc.data(), false);
    kisdoc.reset();
    execution.reset();
    device.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(node->pixelData(5, 5, 1, 1), QByteArray::fromHex("443322ff"));
    node.clear();
    QVERIFY(lifetime.expired());
}

void TestDocument::testGpuOpenKeepsPixelsMetadataAndAssociatedFiles()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    Kra::Document source;
    source.image.size = QSize(64, 32);
    Gpu::RgbaTile tile{{0, 0}, QByteArray(Gpu::RgbaTile::ByteCount, '\0')};
    tile.rgba.replace((5 * 256 + 5) * 4, 4, QByteArray::fromHex("112233ff"));
    source.image.layers = {{QStringLiteral("Paint"), true, QUuid::createUuid(), {tile}},
                           {QStringLiteral("Hidden"), false, QUuid::createUuid(), {}, {170, 187, 204, 0}}};
    source.image.activeLayer = 0;
    source.image.layers[0].opacity = 128;
    source.image.layers[0].userLocked = true;
    Kra::ImageMetadata metadata;
    metadata.resolution = {300, 150};
    metadata.documentInfo = QByteArrayLiteral("<document-info><about><title>Imported title</title></about></document-info>");
    metadata.iccProfile = QColorSpace(QColorSpace::SRgb).iccProfile();
    metadata.annotations = {{QStringLiteral("notes"), QStringLiteral("Project notes"), QByteArrayLiteral("note bytes")}};
    QCOMPARE(Kra::RasterCodec::updateImageMetadata(source, metadata).code, Kra::Status::Success);
    source.files.insert(QStringLiteral("notes/opaque.bin"), QByteArray::fromHex("ff0011"));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("import.kra"));
    QImage projection(source.image.size, QImage::Format_RGBA8888);
    projection.fill(Qt::transparent);
    projection.setColorSpace(QColorSpace::SRgb);
    QFile output(path);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, projection).code, Kra::Status::Success);
    output.close();
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QSignalSpy completed(document.data(), &KisDocument::completed);
    QVERIFY2(document->openPath(path, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    QCOMPARE(completed.size(), 1);
    const auto owner = document->image()->gpuImage();
    QVERIFY(owner);
    QCOMPARE(document->path(), path);
    QCOMPARE(document->image()->xRes() * 72, 300.0);
    QCOMPARE(document->image()->yRes() * 72, 150.0);
    QCOMPARE(document->image()->profile()->rawData(), metadata.iccProfile);
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Imported title"));
    QCOMPARE(document->image()->annotation(QStringLiteral("notes"))->annotation(), QByteArrayLiteral("note bytes"));
    QCOMPARE(document->image()->root()->at(0)->paintDevice()->colorSpace()->profile()->rawData(), metadata.iccProfile);
    QCOMPARE(document->image()->root()->at(0)->uuid(), source.image.layers[0].uuid);
    QCOMPARE(document->image()->root()->at(0)->opacity(), quint8(128));
    QVERIFY(document->image()->root()->at(0)->userLocked());
    Document publicDocument(document.data(), false);
    QCOMPARE(publicDocument.pixelData(5, 5, 1, 1), QByteArray::fromHex("33221180"));
    QVERIFY(!document->image()->root()->at(1)->visible());
    QCOMPARE(document->preActivatedNode(), document->image()->root()->at(0));
    QVERIFY(!document->isModified());
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
    const auto savedPath = directory.filePath(QStringLiteral("resaved.kra"));
    QVERIFY2(document->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    QFile input(savedPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto saved = Kra::RasterCodec::read(input);
    QCOMPARE(saved.code, Kra::Status::Success);
    QCOMPARE(saved.document.files.value(QStringLiteral("notes/opaque.bin")), QByteArray::fromHex("ff0011"));
    QCOMPARE(saved.document.image.layers[0].opacity, quint8(128));
    QVERIFY(saved.document.image.layers[0].userLocked);
    const auto savedMetadata = Kra::RasterCodec::readImageMetadata(saved.document.description, saved.document.files);
    QCOMPARE(savedMetadata.code, Kra::Status::Success);
    QCOMPARE(savedMetadata.metadata.iccProfile, metadata.iccProfile);
    const auto currentImage = document->image();
    const auto currentVersion = owner->operations().snapshot();
    const auto failedPath = directory.filePath(QStringLiteral("invalid.kra"));
    QFile invalid(failedPath);
    QVERIFY(invalid.open(QIODevice::WriteOnly));
    invalid.write("invalid archive");
    invalid.close();
    QSignalSpy cancelled(document.data(), &KisDocument::canceled);
    const auto completionsBeforeFailure = completed.size();
    QVERIFY(!document->openPath(failedPath, KisDocument::DontAddToRecent));
    QVERIFY(!document->errorMessage().isEmpty());
    QCOMPARE(cancelled.size(), 1);
    QCOMPARE(completed.size(), completionsBeforeFailure);
    QCOMPARE(document->image(), currentImage);
    QCOMPARE(owner->operations().snapshot(), currentVersion);
    QCOMPARE(document->path(), savedPath);
    QCOMPARE(document->localFilePath(), savedPath);
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Imported title"));
    QVERIFY(!document->isModified());
    for (const bool closeBeforeCompletion : {true, false}) {
        QScopedPointer<KisDocument> closingOwner(KisPart::instance()->createDocument());
        QPointer<KisDocument> closing = closingOwner.data();
        closing->setFileBatchMode(true);
        Gpu::PaintImageSP retained;
        connect(closing.data(), closeBeforeCompletion ? &KisDocument::sigLoadingFinished : &KisDocument::completed,
                this, [&] {
            retained = closing->image()->gpuImage();
            KisPart::instance()->removeDocument(closing.data(), false);
            delete closingOwner.take();
        });
        const bool opened = closing->openPath(path, KisDocument::DontAddToRecent);
        QVERIFY(opened);
        QVERIFY(!closing);
        QVERIFY(retained);
        QCOMPARE(retained->readRgbaPixels(retained->operations().activeLayer(), QRect(5, 5, 1, 1), 4).bytes,
                 QByteArray::fromHex("112233ff"));
    }
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuSaveAndExportPreserveHistoryAndFileIdentity()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("GPU file operations"));
    image->setResolution(300.0 / 72, 150.0 / 72);
    image->addAnnotation(new KisAnnotation(QStringLiteral("project-data"), QStringLiteral("Annotation description"), QByteArrayLiteral("preserved annotation")));
    document->setCurrentImage(image);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("保存契約"));
    document->setFileBatchMode(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto savedPath = directory.filePath(QStringLiteral("saved.kra"));
    const auto exportedPath = directory.filePath(QStringLiteral("exported.kra"));
    QSignalSpy saved(document.data(), &KisDocument::sigSavingFinished);
    QVERIFY2(document->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QCOMPARE(saved.size(), 1);
    QCOMPARE(document->path(), savedPath);
    QCOMPARE(document->mimeType(), KisDocument::nativeFormatMimeType());
    QVERIFY(!document->isModified());
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().savedHistoryIndex(), 1);
    QFile input(savedPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto savedBytes = input.readAll();
    QVERIFY(input.seek(0));
    const auto loaded = Kra::RasterCodec::read(input);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.size, QSize(64, 32));
    QCOMPARE(loaded.document.image.layers.size(), 1);
    QCOMPARE(loaded.document.image.layers.first().uuid, owner->operations().snapshot()->layers.first().uuid);
    QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4),
             QByteArray::fromHex("112233ff"));
    QDomDocument description;
    QVERIFY(description.setContent(loaded.document.description));
    const auto imageDescription = description.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    QCOMPARE(imageDescription.attribute(QStringLiteral("x-res")).toDouble(), 300.0);
    QCOMPARE(imageDescription.attribute(QStringLiteral("y-res")).toDouble(), 150.0);
    const auto folder = imageDescription.attribute(QStringLiteral("name"));
    QCOMPARE(loaded.document.files.value(folder + QStringLiteral("/annotations/icc")), image->profile()->rawData());
    QCOMPARE(loaded.document.files.value(folder + QStringLiteral("/annotations/project-data")), QByteArrayLiteral("preserved annotation"));
    QVERIFY(loaded.document.files.value(QStringLiteral("documentinfo.xml")).contains(QStringLiteral("保存契約").toUtf8()));
    input.close();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
    QVERIFY(document->isModified());
    const auto current = owner->operations().snapshot();
    Document publicDocument(document.data(), false);
    InfoObject settings;
    QVERIFY(publicDocument.exportImage(exportedPath, settings));
    QCOMPARE(document->path(), savedPath);
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), current);
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->operations().savedHistoryIndex(), 1);
    QCOMPARE(saved.size(), 1);
    QFile exported(exportedPath);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    const auto exportedValue = Kra::RasterCodec::read(exported);
    QVERIFY2(exportedValue.code == Kra::Status::Success, qPrintable(exportedValue.diagnostic));
    QCOMPARE(exportedValue.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4),
             QByteArray::fromHex("445566ff"));
    QVERIFY(!document->saveAs(directory.filePath(QStringLiteral("missing/failed.kra")), KisDocument::nativeFormatMimeType(), false));
    QVERIFY(!document->errorMessage().isEmpty());
    QVERIFY(!document->exportDocumentSync(savedPath, QByteArrayLiteral("image/png")));
    QVERIFY(!document->errorMessage().isEmpty());
    QCOMPARE(document->path(), savedPath);
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), current);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QCOMPARE(input.readAll(), savedBytes);
    QCOMPARE(saved.size(), 1);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuSaveNotificationLifetimes_data()
{
    QTest::addColumn<int>("reaction");
    QTest::newRow("later-edit") << 0;
    QTest::newRow("close-document") << 1;
    QTest::newRow("replace-image") << 2;
    QTest::newRow("close-on-completion") << 3;
    QTest::newRow("document-info-edit") << 4;
    QTest::newRow("background-edit") << 5;
}

void TestDocument::testGpuSaveNotificationLifetimes()
{
    QFETCH(int, reaction);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Save notification fixture")));
    document->setFileBatchMode(true);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Before save"));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("saved.kra"));
    const auto original = directory.filePath(QStringLiteral("original.kra"));
    document->setPath(original);
    QSignalSpy saved(document.get(), &KisDocument::sigSavingFinished);
    if (reaction == 3) {
        connect(document.get(), &KisDocument::completed, this, [&] {
            KisPart::instance()->removeDocument(document.get(), false);
            document.reset();
        });
    }
    Gpu::PaintImageSP replacement;
    connect(owner.data(), &Gpu::PaintImage::savedStateChanged, this, [&] {
        switch (reaction) {
        case 0:
            QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
            break;
        case 1:
            KisPart::instance()->removeDocument(document.get(), false);
            document.reset();
            break;
        case 2:
            replacement = Gpu::PaintImageSP::create(execution, QSize(32, 16));
            document->setCurrentImage(KisImage::fromGpuImage(replacement, QStringLiteral("Replacement image")));
            break;
        case 4:
            document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("After save"));
            document->setModified(true);
            break;
        case 5: {
            Document publicDocument(document.get(), false);
            QVERIFY(publicDocument.setBackgroundColor(QColor(17, 34, 51, 128)));
            break;
        }
        }
    });
    QVERIFY(document->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    QFile input(path);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(input);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4), QByteArray::fromHex("112233ff"));
    if (reaction == 0 || reaction == 4 || reaction == 5) {
        QCOMPARE(document->path(), path);
        QVERIFY(document->isModified());
        QCOMPARE(owner->operations().undoCount(), reaction == 0 ? 2 : 1);
        QCOMPARE(owner->operations().savedHistoryIndex(), 1);
        QCOMPARE(saved.size(), 1);
        QVERIFY(!document->isSaving());
        if (reaction == 4) {
            QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("After save"));
            QVERIFY(loaded.document.files.value(QStringLiteral("documentinfo.xml")).contains("Before save"));
            QVERIFY(!loaded.document.files.value(QStringLiteral("documentinfo.xml")).contains("After save"));
        }
        if (reaction == 5) {
            const std::array<quint8, 4> edited{17, 34, 51, 128};
            const std::array<quint8, 4> original{};
            QCOMPARE(owner->operations().snapshot()->projectionBackground, edited);
            QCOMPARE(loaded.document.image.projectionBackground, original);
        }
    } else if (reaction == 1 || reaction == 3) {
        QVERIFY(!document);
        QCOMPARE(saved.size(), 0);
        QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    } else {
        QCOMPARE(document->path(), original);
        QCOMPARE(document->image()->gpuImage(), replacement);
        QVERIFY(!document->isModified());
        QCOMPARE(saved.size(), 0);
        QVERIFY(!document->isSaving());
    }
    if (document) KisPart::instance()->removeDocument(document.get(), false);
}

void TestDocument::testGpuSnapshotKeepsIndependentPixelsAndDocumentMetadata()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    const auto hidden = owner->addLayer(QStringLiteral("Hidden layer"));
    QVERIFY(hidden);
    QCOMPARE(owner->writeRgbaPixels(hidden, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Gpu::Result::Success);
    QCOMPARE(owner->setLayerVisible(hidden, false), Gpu::Result::Success);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Snapshot source"));
    image->setResolution(300.0 / 72, 150.0 / 72);
    image->addAnnotation(new KisAnnotation(QStringLiteral("snapshot-data"), QStringLiteral("Snapshot annotation"), QByteArrayLiteral("annotation value")));
    image->root()->firstChild()->setUserLocked(true);
    image->root()->firstChild()->setColorLabelIndex(3);
    document->setCurrentImage(image);
    document->setPath(QStringLiteral("/tmp/snapshot-source.kra"));
    document->setLocalFilePath(document->path());
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Snapshot title"));
    const auto before = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    QScopedPointer<KisDocument> snapshot(document->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    auto copied = snapshot->image()->gpuImage();
    QVERIFY(copied);
    QVERIFY(copied != owner);
    QVERIFY(copied->handle() != owner->handle());
    QVERIFY(copied->operations().snapshot()->documentId() != before->documentId());
    QCOMPARE(copied->operations().undoCount(), 0);
    QCOMPARE(copied->operations().redoCount(), 0);
    QVERIFY(!snapshot->isModified());
    QCOMPARE(snapshot->path(), document->path());
    QCOMPARE(snapshot->localFilePath(), document->localFilePath());
    QCOMPARE(snapshot->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Snapshot title"));
    QCOMPARE(snapshot->image()->xRes(), image->xRes());
    QCOMPARE(snapshot->image()->yRes(), image->yRes());
    QCOMPARE(snapshot->image()->annotation(QStringLiteral("snapshot-data"))->annotation(), QByteArrayLiteral("annotation value"));
    QVERIFY(snapshot->image()->root()->firstChild()->userLocked());
    QCOMPARE(snapshot->image()->root()->firstChild()->colorLabelIndex(), 3);
    QCOMPARE(copied->operations().snapshot()->layers.size(), before->layers.size());
    for (int index = 0; index < before->layers.size(); ++index) {
        const auto &original = before->layers[index];
        const auto &cloned = copied->operations().snapshot()->layers[index];
        QCOMPARE(cloned.uuid, original.uuid);
        QCOMPARE(cloned.name, original.name);
        QCOMPARE(cloned.visible, original.visible);
    }
    QCOMPARE(copied->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    QCOMPARE(copied->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(copied->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().undoCount(), history);
    auto retained = snapshot->image()->root()->firstChild()->paintDevice();
    KisPart::instance()->removeDocument(document.data(), false);
    KisPart::instance()->removeDocument(snapshot.data(), false);
    snapshot.reset();
    copied.clear();
    QByteArray bytes(4, Qt::Uninitialized);
    retained->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 5, 5, 1, 1);
    QCOMPARE(bytes, QByteArray::fromHex("665544ff"));
}

void TestDocument::testThumbnail()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    d.refreshProjection();

    QImage thumb = d.thumbnail(10, 10);
    thumb.save("thumb.png");
    QVERIFY(thumb.width() == 10);
    QVERIFY(thumb.height() == 10);
    // Our thumbnail calculator in KisPaintDevice cannot make a filled 10x10 thumbnail from a 100x100 device,
    // it makes it 10x10 empty, then puts 8x8 pixels in there... Not a bug in the Node class
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            QVERIFY(thumb.pixelColor(i, j) == QColor(Qt::red));
        }
    }
    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateFillLayer()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 50, 50, KoColorSpaceRegistry::instance()->rgb16(), "test");
    kisdoc->setCurrentImage(image);
    Document d(kisdoc.data(), false);

    const QString pattern("pattern");
    const QString color("color");
    const QString filllayer = "filllayer";
    InfoObject info;
    Selection sel(image->globalSelection());

    FillLayer *f = d.createFillLayer("test1", pattern, info, sel);
    QVERIFY(f->generatorName() == pattern);
    QVERIFY(f->type() == filllayer);
    delete f;
    f = d.createFillLayer("test1", color, info, sel);
    QVERIFY(f->generatorName() == color);
    QVERIFY(f->type() == filllayer);

    info.setProperty(pattern, "Cross01.pat");
    QVERIFY(f->setGenerator(pattern, &info));
    QVERIFY(f->filterConfig()->property(pattern).toString() == "Cross01.pat");
    QVERIFY(f->generatorName() == pattern);
    QVERIFY(f->type() == filllayer);

    info.setProperty(color, QColor(Qt::red));
    QVERIFY(f->setGenerator(color, &info));
    QVariant v = f->filterConfig()->property(color);
    QColor c = v.value<QColor>();
    QVERIFY(c == QColor(Qt::red));
    QVERIFY(f->generatorName() == color);
    QVERIFY(f->type() == filllayer);

    bool r = f->setGenerator(QString("xxx"), &info);
    QVERIFY(!r);

    delete f;

    QVERIFY(d.createFillLayer("test1", "xxx", info, sel) == 0);

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateCloneLayer()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 50, 50, KoColorSpaceRegistry::instance()->rgb16(), "test");
    kisdoc->setCurrentImage(image);
    Document d(kisdoc.data(), false);

    const QString layerType = "clonelayer";
    const QString node1Name = "node1";
    const QString node2Name = "node2";

    Node *node1 = d.createNode(node1Name,"paintlayer");
    Node *node2 = d.createNode(node2Name,"paintlayer");
    CloneLayer *layer = d.createCloneLayer("test1", node1);
    Node* rootNode = d.rootNode();

    rootNode->addChildNode(node1,0);
    rootNode->addChildNode(node2,0);
    rootNode->addChildNode(layer,0);

    QVERIFY(layer->type() == layerType);

    Node *sourceNode1 = layer->sourceNode();

    QVERIFY(sourceNode1->name() == node1Name);

    layer->setSourceNode(node2);

    Node *sourceNode2 = layer->sourceNode();

    QVERIFY(sourceNode2->name() == node2Name);

    delete layer;
    delete node1;
    delete node2;
    delete sourceNode1;
    delete sourceNode2;
    delete rootNode;

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateTransparencyMask()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 50, 50, KoColorSpaceRegistry::instance()->rgb16(), "test");
    kisdoc->setCurrentImage(image);
    Document d(kisdoc.data(), false);

    const QString layerType = "transparencymask";

    Selection sel(image->globalSelection());

    sel.select(10,10,10,10,255);

    Node *node = d.createNode("node1","paintlayer");
    TransparencyMask *mask = d.createTransparencyMask("test1");
    Node* rootNode = d.rootNode();

    rootNode->addChildNode(node,0);
    node->addChildNode(mask,0);

    Selection* selResult = mask->selection();

    QVERIFY(mask->type() == layerType);
    QVERIFY(selResult->width() == sel.width() && selResult->height() == sel.height());

    delete selResult;
    delete mask;
    delete node;
    delete rootNode;

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateColorizeMask()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(new KisSurrogateUndoStore(),  10, 3, KoColorSpaceRegistry::instance()->rgb8(), "test");

    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    const QString layerType = "colorizemask";

    Node *node = d.createNode("node1","paintlayer");

    ColorizeMask *mask = d.createColorizeMask("test1");
    Node* rootNode = d.rootNode();

    QByteArray nodeData = QByteArray::fromBase64("AAAAAAAAAAAAAAAAEQYMBhEGDP8RBgz/EQYMAgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARBgz5EQYM/xEGDAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEQYMAhEGDAkRBgwCAAAAAAAAAAAAAAAA");

    node->setPixelData(nodeData,0,0,10,3);
    rootNode->addChildNode(node,0);

    d.waitForDone();
    d.refreshProjection();

    node->addChildNode(mask,0);

    qApp->processEvents();
    d.waitForDone();

    ManagedColor col1("RGBA","U8","");
    ManagedColor col2("RGBA","U8","");

    col1.setComponents({1.0, 0.0, 0.0, 1.0});
    col2.setComponents({0.0, 0.0, 1.0, 1.0});

    QVERIFY(mask->type() == layerType);

    mask->setEditKeyStrokes(true);
    QVERIFY(mask->editKeyStrokes() == true);

    mask->setUseEdgeDetection(true);
    QVERIFY(mask->useEdgeDetection() == true);

    mask->setEdgeDetectionSize(4.0);
    QVERIFY(mask->edgeDetectionSize() == 4.0);

    mask->setCleanUpAmount(70.0);
    QVERIFY(mask->cleanUpAmount() == 70.0);

    mask->setLimitToDeviceBounds(true);
    QVERIFY(mask->limitToDeviceBounds() == true);

    mask->initializeKeyStrokeColors({&col1, &col2});

    QByteArray pdata1 = QByteArray::fromBase64("//8AAAAAAAAAAP8AAAAAAAAAAAAAAAAAAAAAAAAA");
    QByteArray pdata2 = QByteArray::fromBase64("AAAAAAAAAAD//wAAAAAAAAAAAP8AAAAAAAAAAAAA");

    mask->setKeyStrokePixelData(pdata1,&col1,0,0,10,3);
    mask->setKeyStrokePixelData(pdata2,&col2,0,0,10,3);

    QList<ManagedColor *> checkColors(mask->keyStrokesColors());

    QVERIFY(checkColors.size() == 2);
    QVERIFY(col1.toQString() == checkColors[0]->toQString());
    QVERIFY(col2.toQString() == checkColors[1]->toQString());

    QVERIFY(mask->keyStrokePixelData(&col1,0,0,10,3) == pdata1);
    QVERIFY(mask->keyStrokePixelData(&col2,0,0,10,3) == pdata2);

    delete checkColors[0];
    delete checkColors[1];

    mask->updateMask(true);
    mask->setEditKeyStrokes(false);
    mask->setShowOutput(true);

    d.waitForDone();
    d.refreshProjection();

    QByteArray pdata = d.pixelData(0,0,10,3);

    QVERIFY(d.pixelData(3,2,1,1).toBase64() == "/wAA/w==");
    QVERIFY(d.pixelData(6,2,1,1).toBase64() == "AAD9/w==");

    mask->removeKeyStroke(&col2);
    qApp->processEvents();
    d.waitForDone();

    checkColors = mask->keyStrokesColors();

    QVERIFY(checkColors.size() == 1);
    QVERIFY(col1.toQString() == checkColors[0]->toQString());

    delete checkColors[0];
    delete mask;
    delete node;
    delete rootNode;

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}



void TestDocument::testAnnotations()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QVERIFY(d.annotationTypes().isEmpty());

    QBuffer buf;
    buf.open(QBuffer::WriteOnly);
    QTextStream in(&buf);
    KisPortingUtils::setUtf8OnStream(in);
    in << "AnnotationTest";
    buf.close();

    d.setAnnotation("test", "description", buf.data());

    QVERIFY(d.annotationTypes().size() == 1);
    QVERIFY(d.annotationTypes().contains("test"));
    QVERIFY(d.annotation("test").toHex() == buf.data().toHex());
    QVERIFY(d.annotationDescription("test") == "description");

    d.saveAs("roundtriptest.kra");

    d.removeAnnotation("test");
    QVERIFY(d.annotationTypes().isEmpty());

    kisdoc.take();
    layer.clear();
    image.clear();
    QVERIFY(d.close());
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    Krita *krita = Krita::instance();
    const bool previousBatchMode = krita->batchmode();
    const auto restoreBatchMode = qScopeGuard([&] { krita->setBatchmode(previousBatchMode); });
    krita->setBatchmode(true);
    Document *d2 = krita->openDocument("roundtriptest.kra");
    QVERIFY(d2);

    QVERIFY(d2->annotationTypes().size() == 1);
    QVERIFY(d2->annotationTypes().contains("test"));
    QVERIFY(d2->annotation("test").toHex() == buf.data().toHex());
    QVERIFY(d2->annotationDescription("test") == "description");

    d2->close();
}

void TestDocument::testNodeByName()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QVERIFY(d.nodeByName("test1")->name() == layer->name());
    QVERIFY(d.nodeByName("test2") == 0);
}

void TestDocument::testNodeByUniqueId()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QVERIFY(d.nodeByUniqueID(layer->uuid())->name() == layer->name());

    QUuid test(QUuid::createUuid());

    while (test == layer->uuid()) {
        test = QUuid::createUuid();
    }
    QVERIFY(d.nodeByUniqueID(test) == 0);
}

class KisTransformMaskAdapter;

KISTEST_MAIN(TestDocument)

