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
#include <application/ui/orchestration/KisPart.h>

#include <kis_transform_mask_params_factory_registry.h>
#include <kis_undo_stores.h>
#include <testui.h>

#include <KisPortingUtils.h>

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
        }
    });
    QVERIFY(document->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    QFile input(path);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(input);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4), QByteArray::fromHex("112233ff"));
    if (reaction == 0 || reaction == 4) {
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

    d.close();

    Krita *krita = Krita::instance();
    Document *d2 = krita->openDocument("roundtriptest.kra");

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

