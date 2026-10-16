/* SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "TestNode.h"
#include <simpletest.h>
#include <QColor>
#include <QApplication>
#include <QIcon>
#include <QPointer>
#include <QScopeGuard>
#include <KLocalizedString>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/ui/workspace/KisView.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/orchestration/kis_action.h>
#include <nodes/kis_node_manager.h>
#include <kis_config.h>
#include <opengl/kis_opengl.h>
#include <actions/input/KisApplicationInputActions.h>
#include <input/ui/kis_input_profile_manager.h>
#include <KisResourceStorage.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisGlobalResourcesInterface.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_settings.h>
#include <memory>
#include <algorithm>
#include <QDataStream>
#include <kundo2stack.h>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>

#include <KritaVersionWrapper.h>
#include <Node.h>
#include <Document.h>
#include <Krita.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>

#include <kis_image.h>
#include <kis_fill_painter.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_transaction.h>
#include <commands/kis_node_opacity_command.h>
#include <kis_shape_layer.h>
#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>

#include <testui.h>

void TestNode::initTestCase()
{
    Q_INIT_RESOURCE(krita);
    QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
    QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
    KLocalizedString::setApplicationDomain("krita");
    KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
    KisInputProfileManager::instance()->setActions(createApplicationInputActions());
    const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
    auto bundle = KisResourceStorageSP::create(bundlePath);
    QVERIFY(bundle->valid());
    QVERIFY(KisResourceLocator::instance()->addStorage(bundlePath, bundle));
    auto preset = KisPaintOpRegistry::instance()->defaultPreset(
        KoID("roundmarker"), KisGlobalResourcesInterface::instance());
    QVERIFY(preset);
    preset->setName(QStringLiteral("Public node window fixture"));
    preset->setFilename(QStringLiteral("public-node-window-fixture.kpp"));
    QImage preview(16, 16, QImage::Format_ARGB32);
    preview.fill(Qt::black);
    preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets);
    QVERIFY(presets.addResource(preset));
}

void TestNode::testLayerPropertiesPreservePublicHistorySemantics_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("existing-image") << false;
    QTest::newRow("gpu-image") << true;
}

void TestNode::testLayerPropertiesPreservePublicHistorySemantics()
{
    QFETCH(bool, gpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage input;
        input.size = {64, 64};
        input.layers = {{QStringLiteral("Recorded edit"), true, {}, {}}, {QStringLiteral("Other"), true, {}, {}}};
        owner = Gpu::PaintImageSP::create(execution, input);
        image = KisImage::fromGpuImage(owner, QStringLiteral("Layer properties"));
    } else {
        image = new KisImage(document->createUndoStore(), 64, 64, KoColorSpaceRegistry::instance()->rgb8(), "Layer properties");
        image->addNode(new KisPaintLayer(image, QStringLiteral("Recorded edit"), 255));
        image->addNode(new KisPaintLayer(image, QStringLiteral("Other"), 255));
    }
    document->setCurrentImage(image);
    auto layer = image->root()->firstChild();
    NodeSP node(Node::createNode(image, layer));
    const auto before = QByteArray::fromHex("0000ffff");
    const auto after = QByteArray::fromHex("ff0000ff");
    QVERIFY(node->setPixelData(before, 8, 12, 1, 1));
    if (gpu) {
        QCOMPARE(owner->replaceRgbaPixels(1, QRect(8, 12, 1, 1), QByteArray::fromHex("0000ffff")), Gpu::Result::Success);
    } else {
        KisTransaction transaction(layer->paintDevice());
        QVERIFY(node->setPixelData(after, 8, 12, 1, 1));
        transaction.commit(image->undoAdapter());
    }
    image->waitForDone();
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(node->pixelData(8, 12, 1, 1), before);
    QCOMPARE(document->undoStack()->index(), 0);
    QCOMPARE(document->undoStack()->count(), 1);
    QVERIFY(!document->isModified());
    const auto *historyCommand = document->undoStack()->command(0);
    QSignalSpy modified(document.data(), &KisDocument::modified);
    node->setName(QStringLiteral("Renamed"));
    node->setOpacity(-1);
    QCOMPARE(node->opacity(), 0);
    node->setOpacity(300);
    QCOMPARE(node->opacity(), 255);
    node->setOpacity(128);
    node->setVisible(false);
    node->setLocked(true);
    layer->setPinnedToTimeline(true);
    Document publicDocument(document.data(), false);
    publicDocument.refreshProjection();
    QCOMPARE(publicDocument.pixelData(8, 12, 1, 1), QByteArray(4, '\0'));
    QCOMPARE(node->pixelData(8, 12, 1, 1), before);
    QCOMPARE(document->undoStack()->index(), 0);
    QCOMPARE(document->undoStack()->count(), 1);
    QVERIFY(!document->isModified());
    QVERIFY(modified.isEmpty());
    document->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(node->name(), QStringLiteral("Renamed"));
    QCOMPARE(node->opacity(), 128);
    QVERIFY(!node->visible());
    QVERIFY(node->locked());
    QVERIFY(layer->isPinnedToTimeline());
    QCOMPARE(node->pixelData(8, 12, 1, 1), after);
    QCOMPARE(document->undoStack()->index(), 1);
    QCOMPARE(document->undoStack()->count(), 1);
    QCOMPARE(document->undoStack()->command(0), historyCommand);
    node->setName(QString());
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(node->name(), QString());
    QCOMPARE(node->opacity(), 128);
    QVERIFY(!node->visible());
    QVERIFY(node->locked());
    QVERIFY(layer->isPinnedToTimeline());
    QCOMPARE(node->pixelData(8, 12, 1, 1), before);
    QCOMPARE(document->undoStack()->count(), 1);
    QVERIFY(!document->isModified());
    if (gpu) {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath(QStringLiteral("layer-properties.kra"));
        document->setFileBatchMode(true);
        QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        const auto loaded = reopened->image()->root()->firstChild();
        QCOMPARE(loaded->name(), QString());
        QCOMPARE(loaded->opacity(), quint8(128));
        QVERIFY(!loaded->visible());
        QVERIFY(loaded->userLocked());
        QVERIFY(loaded->isPinnedToTimeline());
    }
}

void TestNode::testGpuNodeNotificationKeepsReentrantPropertyWrites()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Reentrant layer properties"));
    document->setCurrentImage(image);
    auto layer = image->root()->firstChild();
    NodeSP node(Node::createNode(image, layer));
    QCOMPARE(owner->writeRgbaPixels(1, QRect(8, 12, 1, 1), QByteArray::fromHex("ff0000ff")), Gpu::Result::Success);
    bool reacted = false;
    QObject receiver;
    QObject::connect(layer.data(), &KisNode::sigNodeChangedInternal, &receiver, [&] {
        if (reacted || node->visible()) return;
        reacted = true;
        node->setOpacity(64);
    });
    QCOMPARE(owner->setLayerVisible(1, false), Gpu::Result::Success);
    QVERIFY(reacted);
    QVERIFY(!node->visible());
    QCOMPARE(node->opacity(), 64);
    QCOMPARE(owner->operations().snapshot()->layers.first().opacity, quint8(64));
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(node->visible());
    QCOMPARE(node->opacity(), 64);
    Document publicDocument(document.data(), false);
    QCOMPARE(publicDocument.pixelData(8, 12, 1, 1), QByteArray::fromHex("0000ff40"));
    QCOMPARE(node->pixelData(8, 12, 1, 1), QByteArray::fromHex("0000ffff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(!node->visible());
    QCOMPARE(node->opacity(), 64);
}

void TestNode::testDirectOpacityWritesPreserveRecordedPropertyUndo_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("existing-image") << false;
    QTest::newRow("gpu-image") << true;
}

void TestNode::testDirectOpacityWritesPreserveRecordedPropertyUndo()
{
    QFETCH(bool, gpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
        image = KisImage::fromGpuImage(owner, QStringLiteral("Opacity history"));
    } else {
        image = new KisImage(document->createUndoStore(), 64, 64, KoColorSpaceRegistry::instance()->rgb8(), "Opacity history");
        image->addNode(new KisPaintLayer(image, QStringLiteral("Layer"), 255));
    }
    document->setCurrentImage(image);
    auto layer = image->root()->firstChild();
    NodeSP node(Node::createNode(image, layer));
    QScopedPointer<KisNodeOpacityCommand> command;
    if (gpu) {
        QCOMPARE(owner->setLayerOpacity(1, 128), Gpu::Result::Success);
    } else {
        command.reset(new KisNodeOpacityCommand(layer, 128));
        command->redo();
    }
    image->waitForDone();
    QCOMPARE(node->opacity(), 128);
    node->setOpacity(64);
    QCOMPARE(node->opacity(), 64);
    if (gpu) QCOMPARE(owner->undo(), Gpu::Result::Success);
    else command->undo();
    image->waitForDone();
    QCOMPARE(node->opacity(), 255);
    if (gpu) QCOMPARE(owner->redo(), Gpu::Result::Success);
    else command->redo();
    image->waitForDone();
    QCOMPARE(node->opacity(), 128);
    if (gpu) QCOMPARE(owner->undo(), Gpu::Result::Success);
    else command->undo();
    node->setOpacity(32);
    if (gpu) QCOMPARE(owner->redo(), Gpu::Result::Success);
    else command->redo();
    if (gpu) QCOMPARE(owner->undo(), Gpu::Result::Success);
    else command->undo();
    image->waitForDone();
    QCOMPARE(node->opacity(), 255);
}

void TestNode::testSetColorSpace()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QStringList profiles = Krita().profiles("GRAYA", "U16");
    node->setColorSpace("GRAYA", "U16", profiles.first());
    QCOMPARE(layer->colorSpace()->colorModelId().id() , "GRAYA");
    QCOMPARE(layer->colorSpace()->colorDepthId().id() , "U16");
    QCOMPARE(layer->colorSpace()->profile()->name(), "Gray-D50-elle-V2-g10.icc");
}

void TestNode::testSetColorProfile()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QStringList profiles = Krita().profiles("RGBA", "U8");
    Q_FOREACH(const QString &profile, profiles) {
        node->setColorProfile(profile);
        QVERIFY(layer->colorSpace()->profile()->name() == profile);
    }
}

void TestNode::testPixelData()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QByteArray ba = node->pixelData(0, 0, 100, 100);
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

    QDataStream ds2(&ba, QIODevice::WriteOnly);
    for (int i = 0; i < 100 * 100; i++) {
        ds2 << 255;
        ds2 << 255;
        ds2 << 255;
        ds2 << 255;
    }

    node->setPixelData(ba, 0, 0, 100, 100);
    for (int i = 0; i < 100 ; i++) {
        for (int j = 0; j < 100 ; j++) {
            QColor pixel;
            layer->paintDevice()->pixel(i, j, &pixel);
            QVERIFY(pixel == QColor(Qt::black));
        }
    }
}

void TestNode::testProjectionPixelData()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QByteArray ba = node->projectionPixelData(0, 0, 100, 100);
    QDataStream ds(ba);
    for (int i = 0; i < 100 * 100; i++) {
        quint8 channelvalue;
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xA4);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xA0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xA0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xFF);
    }
}

void TestNode::testRawPixelWritesPreserveFormatHistoryAndFailures()
{
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(document->createUndoStore(), 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "Pixels", 255);
    image->addNode(layer);
    document->setCurrentImage(image);
    image->waitForDone();
    NodeSP node(Node::createNode(image, layer));
    const auto raw = QByteArray::fromHex("120a2700197fafe00000ffffff000080");
    const auto count = document->undoStack()->count();
    const auto index = document->undoStack()->index();
    const auto modified = document->isModified();
    QVERIFY(node->setPixelData(raw + QByteArrayLiteral("ignored suffix"), -1, -1, 2, 2));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    QCOMPARE(node->pixelData(20, 20, 2, 2), QByteArray(16, '\0'));
    QVERIFY(!node->setPixelData(raw.first(15), -1, -1, 2, 2));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    QVERIFY(node->setPixelData({}, 0, 0, 0, 0));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    QCOMPARE(document->undoStack()->count(), count);
    QCOMPARE(document->undoStack()->index(), index);
    QCOMPARE(document->isModified(), modified);
    NodeSP group(Node::createNode(image, image->rootLayer()));
    QVERIFY(!group->setPixelData(raw, 0, 0, 2, 2));
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestNode::testRawPixelWritesPreserveOtherLayerHistoryAndRedo()
{
    QScopedPointer<KisDocument> owner(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(owner->createUndoStore(), 128, 128, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP edited = new KisPaintLayer(image, "Recorded edit", 255);
    KisNodeSP imported = new KisPaintLayer(image, "External pixels", 255);
    image->addNode(edited);
    image->addNode(imported);
    owner->setCurrentImage(image);
    NodeSP editedNode(Node::createNode(image, edited));
    NodeSP importedNode(Node::createNode(image, imported));
    const auto before = QByteArray::fromHex("0000ffff");
    const auto after = QByteArray::fromHex("ff0000ff");
    const auto external = QByteArray::fromHex("120a2700");
    QVERIFY(editedNode->setPixelData(before, 8, 12, 1, 1));
    {
        KisTransaction transaction(edited->paintDevice());
        QVERIFY(editedNode->setPixelData(after, 8, 12, 1, 1));
        transaction.commit(image->undoAdapter());
    }
    image->waitForDone();
    QCOMPARE(owner->undoStack()->count(), 1);
    owner->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(editedNode->pixelData(8, 12, 1, 1), before);
    const auto modified = owner->isModified();
    QVERIFY(importedNode->setPixelData(external, -1, -1, 1, 1));
    QCOMPARE(owner->undoStack()->index(), 0);
    QCOMPARE(owner->undoStack()->count(), 1);
    QVERIFY(owner->undoStack()->canRedo());
    QCOMPARE(owner->isModified(), modified);
    QVERIFY(editedNode->setPixelData(external, 8, 12, 1, 1));
    QVERIFY(editedNode->setPixelData(external, 96, 96, 1, 1));
    QCOMPARE(editedNode->pixelData(8, 12, 1, 1), external);
    owner->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(editedNode->pixelData(8, 12, 1, 1), after);
    QCOMPARE(editedNode->pixelData(96, 96, 1, 1), external);
    QCOMPARE(importedNode->pixelData(-1, -1, 1, 1), external);
    owner->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(editedNode->pixelData(8, 12, 1, 1), external);
    QCOMPARE(editedNode->pixelData(96, 96, 1, 1), external);
    QCOMPARE(importedNode->pixelData(-1, -1, 1, 1), external);
    KisPart::instance()->removeDocument(owner.data(), false);
}

void TestNode::testGpuPixelDataPreservesFormatHistoryAndSharedLifetime()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto image = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto handle = image->handle();
    QSignalSpy changed(image.data(), &Gpu::PaintImage::changed);
    auto pixels = KisPaintDevice::fromGpuImage(image, image->operations().activeLayer());
    KisNodeSP layer = new KisPaintLayer(KisImageWSP(), QStringLiteral("GPU pixels"), 255, pixels);
    NodeSP node(Node::createNode(KisImageSP(), layer));
    const auto raw = QByteArray::fromHex("120a2700197fafe00000ffffff000080");
    QVERIFY(node->setPixelData(raw + QByteArrayLiteral("ignored suffix"), -1, -1, 2, 2));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    QCOMPARE(node->pixelData(20, 20, 2, 2), QByteArray(16, '\0'));
    QVERIFY(!node->setPixelData(raw.first(15), -1, -1, 2, 2));
    QVERIFY(node->setPixelData({}, 0, 0, 0, 0));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    node->setVisible(false);
    QCOMPARE(node->projectionPixelData(-1, -1, 2, 2), raw);
    QCOMPARE(image->operations().undoCount(), 0);
    QCOMPARE(image->operations().redoCount(), 0);
    QVERIFY(!image->operations().isModified());
    QVERIFY(changed.isEmpty());
    const auto native = execution.readRgbaPixels(handle, image->operations().snapshot(), {-1, -1, 2, 2}, 1, 16);
    QCOMPARE(native.code, Gpu::ExecutionCode::Success);
    QCOMPARE(native.bytes, QByteArray::fromHex("270a1200af7f19e0ff0000ff0000ff80"));
    const auto retained = node->pixelData(-1, -1, 2, 2);
    const auto after = QByteArray::fromHex("01020304");
    image.clear();
    pixels.clear();
    layer.clear();
    QVERIFY(execution.requested(handle));
    QVERIFY(node->setPixelData(after, 0, 0, 1, 1));
    QCOMPARE(node->pixelData(0, 0, 1, 1), after);
    QCOMPARE(retained, raw);
    node.clear();
    QVERIFY(!execution.requested(handle));
    QCOMPARE(execution.pendingDocuments(), qsizetype(0));
}

void TestNode::testPixelCopiesAndNodesOutliveTheImageOwner()
{
    KisImageSP image = new KisImage(nullptr, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "Pixels", 255);
    image->addNode(layer);
    NodeSP node(Node::createNode(image, layer));
    const auto before = QByteArray::fromHex("197fafe0");
    const auto after = QByteArray::fromHex("120a2700");
    QVERIFY(node->setPixelData(before, 8, 12, 1, 1));
    const auto retained = node->pixelData(8, 12, 1, 1);
    QVERIFY(node->setPixelData(after, 8, 12, 1, 1));
    QCOMPARE(retained, before);
    QCOMPARE(node->pixelData(8, 12, 1, 1), after);
    image->waitForDone();
    layer.clear();
    image.clear();
    QCOMPARE(node->pixelData(8, 12, 1, 1), after);
    QCOMPARE(retained, before);
}

void TestNode::testDetachedLayerCopiesPreserveIndependentPixels_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("duplicate");
    QTest::addColumn<bool>("removed");
    QTest::newRow("ordinary-clone") << false << false << false;
    QTest::newRow("ordinary-duplicate") << false << true << false;
    QTest::newRow("gpu-clone") << true << false << false;
    QTest::newRow("gpu-duplicate") << true << true << false;
    QTest::newRow("gpu-removed-clone") << true << false << true;
    QTest::newRow("gpu-removed-duplicate") << true << true << true;
}

void TestNode::testDetachedLayerCopiesPreserveIndependentPixels()
{
    QFETCH(bool, gpu);
    QFETCH(bool, duplicate);
    QFETCH(bool, removed);
    namespace Gpu = LibrePaint::Image::Gpu;
    Gpu::PaintImageSP owner;
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
        image = KisImage::fromGpuImage(owner, QStringLiteral("Layer copy"));
    } else {
        image = new KisImage(nullptr, 64, 32, KoColorSpaceRegistry::instance()->rgb8(), "Layer copy");
        image->addNode(new KisPaintLayer(image, QStringLiteral("Source"), 173));
    }
    auto layer = image->root()->firstChild();
    NodeSP source(Node::createNode(image, layer));
    source->setName(QStringLiteral("Source"));
    source->setOpacity(173);
    source->setVisible(false);
    source->setAlphaLocked(true);
    const auto before = QByteArray::fromHex("197fafe00000ffff120a2700ff000080");
    const auto copiedEdit = QByteArray::fromHex("010203ff");
    const auto sourceEdit = QByteArray::fromHex("ffeedd80");
    QVERIFY(source->setPixelData(before, -1, 3, 2, 2));
    if (removed) QCOMPARE(owner->removeLayer(owner->operations().activeLayer()), Gpu::Result::Success);
    const auto version = owner ? owner->operations().snapshot() : Gpu::Snapshot();
    const auto history = owner ? owner->operations().undoCount() : 0;
    const auto childCount = image->root()->childCount();
    NodeSP copy;
    try {
        copy.reset(duplicate ? source->duplicate() : source->clone());
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QVERIFY(copy);
    QVERIFY(!copy->parentNode());
    QCOMPARE(image->root()->childCount(), childCount);
    QVERIFY(copy->uniqueId() != source->uniqueId());
    QCOMPARE(copy->name(), source->name());
    QCOMPARE(copy->opacity(), 173);
    QVERIFY(!copy->visible());
    QVERIFY(copy->alphaLocked());
    QCOMPARE(copy->pixelData(-1, 3, 2, 2), before);
    if (owner) {
        QCOMPARE(owner->operations().snapshot(), version);
        QCOMPARE(owner->operations().undoCount(), history);
    }
    QVERIFY(copy->setPixelData(copiedEdit, -1, 3, 1, 1));
    QCOMPARE(source->pixelData(-1, 3, 2, 2), before);
    QVERIFY(source->setPixelData(sourceEdit, 0, 4, 1, 1));
    const auto expected = copiedEdit + before.mid(4);
    QCOMPARE(copy->pixelData(-1, 3, 2, 2), expected);
    copy->setName(QStringLiteral("Independent"));
    QCOMPARE(source->name(), QStringLiteral("Source"));
    source.clear();
    layer.clear();
    image.clear();
    owner.clear();
    QCOMPARE(copy->pixelData(-1, 3, 2, 2), expected);
    QVERIFY(copy->setPixelData(sourceEdit, -1, 3, 1, 1));
    QCOMPARE(copy->pixelData(-1, 3, 1, 1), sourceEdit);
}

void TestNode::testInsertedLayersPreserveIdentityPixelsAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<int>("kind");
    for (const bool gpu : {false, true}) {
        for (const int kind : {0, 1, 2}) {
            const QByteArray name = QByteArray(gpu ? "gpu-" : "ordinary-") +
                (kind == 0 ? "created" : kind == 1 ? "cloned-above-source" : "duplicated");
            QTest::newRow(name.constData()) << gpu << kind;
        }
    }
}

void TestNode::testInsertedLayersPreserveIdentityPixelsAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(int, kind);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage input;
        input.size = {32, 16};
        input.layers = {{QStringLiteral("Source"), true, {}, {}}, {QStringLiteral("Top"), true, {}, {}}};
        owner = Gpu::PaintImageSP::create(execution, input);
        image = KisImage::fromGpuImage(owner, QStringLiteral("Public layer insertion"));
    } else {
        image = new KisImage(document->createUndoStore(), 32, 16, KoColorSpaceRegistry::instance()->rgb8(), "Public layer insertion");
        image->addNode(new KisPaintLayer(image, QStringLiteral("Source"), 255));
        image->addNode(new KisPaintLayer(image, QStringLiteral("Top"), 255));
    }
    document->setCurrentImage(image);
    Document api(document.data(), false);
    NodeSP root(api.rootNode());
    NodeSP source(Node::createNode(image, image->root()->firstChild()));
    const auto before = QByteArray::fromHex("197fafe00000ffff120a2700ff000080");
    const auto after = QByteArray::fromHex("a1b2c3ff");
    QVERIFY(source->setPixelData(before, 5, 7, 2, 2));
    NodeSP child(kind == 0 ? api.createNode(QStringLiteral("Inserted"), QStringLiteral("paintlayer")) :
                 kind == 1 ? source->clone() : source->duplicate());
    QVERIFY(child);
    child->setName(QStringLiteral("Inserted"));
    if (kind == 0) QVERIFY(child->setPixelData(before, 5, 7, 2, 2));
    const auto identity = child->uniqueId();
    const auto history = document->undoStack()->count();
    try {
        QVERIFY(root->addChildNode(child.data(), kind == 1 ? source.data() : nullptr));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    const auto index = kind == 1 ? 1 : 2;
    QCOMPARE(image->root()->childCount(), 3U);
    const auto native = image->root()->at(index);
    QCOMPARE(native->uuid(), identity);
    QCOMPARE(child->uniqueId(), identity);
    QCOMPARE(child->pixelData(5, 7, 2, 2), before);
    QScopedPointer<Node> parent(child->parentNode());
    QVERIFY(parent);
    QCOMPARE(parent->uniqueId(), root->uniqueId());
    QCOMPARE(document->undoStack()->count(), history + 1);
    QTRY_VERIFY(document->isModified());
    QVERIFY(child->setPixelData(after, 5, 7, 1, 1));
    api.refreshProjection();
    QCOMPARE(api.pixelData(5, 7, 1, 1), after);
    QCOMPARE(source->pixelData(5, 7, 2, 2), before);
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 2U);
    QVERIFY(!native->parent());
    QCOMPARE(child->pixelData(5, 7, 1, 1), after);
    document->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(image->root()->at(index), native);
    api.refreshProjection();
    QCOMPARE(api.pixelData(5, 7, 1, 1), after);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestNode::testGpuLayerInsertionPreservesNativeIdentityAndRejectsInvalidInputs()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Native insertion"));
    KisNodeSP layer = new KisPaintLayer(image, QStringLiteral("Wide input"), 255);
    auto pixels = layer->paintDevice();
    const auto bytes = QByteArray::fromHex("112233ff").repeated(65 * 256);
    pixels->writeBytes(reinterpret_cast<const quint8*>(bytes.constData()), 0, 0, 65 * 256, 1);
    NodeSP root(Node::createNode(image, image->root()));
    NodeSP child(Node::createNode(image, layer));
    QVERIFY(root->addChildNode(child.data(), nullptr));
    QCOMPARE(image->root()->lastChild(), layer);
    QCOMPARE(layer->paintDevice(), pixels);
    QCOMPARE(child->pixelData(65 * 256 - 1, 0, 1, 1), QByteArray::fromHex("112233ff"));
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(image->root()->lastChild(), layer);
    QCOMPARE(layer->paintDevice(), pixels);

    Gpu::Limits limits;
    limits.rasterBytes = Gpu::RgbaTile::ByteCount;
    auto bounded = Gpu::PaintImageSP::create(execution, QSize(32, 16), limits);
    auto boundedImage = KisImage::fromGpuImage(bounded, QStringLiteral("Bounded insertion"));
    NodeSP boundedRoot(Node::createNode(boundedImage, boundedImage->root()));
    NodeSP copy(child->clone());
    const auto before = bounded->operations().recoveryData();
    QVERIFY(!boundedRoot->addChildNode(copy.data(), nullptr));
    QCOMPARE(bounded->operations().recoveryData(), before);
    QVERIFY(!copy->parentNode());
    QCOMPARE(copy->pixelData(0, 0, 1, 1), QByteArray::fromHex("112233ff"));
    KisNodeSP invalid = new KisPaintLayer(boundedImage, QString(70000, QLatin1Char('x')), 255);
    const auto originalDevice = invalid->paintDevice();
    try {
        QVERIFY(!boundedImage->insertGpuLayer(invalid, 0));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QCOMPARE(bounded->operations().recoveryData(), before);
    QCOMPARE(invalid->paintDevice(), originalDevice);
    QVERIFY(!invalid->parent());
}

void TestNode::testLayerRemovalPreservesPixelsAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("cloned");
    QTest::addColumn<bool>("viaParent");
    for (bool gpu : {false, true}) for (bool cloned : {false, true}) for (bool viaParent : {false, true}) {
        const QByteArray name = QByteArray(gpu ? "gpu-" : "ordinary-") +
            (cloned ? "cloned-" : "created-") + (viaParent ? "parent" : "self");
        QTest::newRow(name.constData()) << gpu << cloned << viaParent;
    }
}

void TestNode::testLayerRemovalPreservesPixelsAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, cloned);
    QFETCH(bool, viaParent);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, QSize(32, 16)), "Public removal");
    } else {
        image = new KisImage(document->createUndoStore(), 32, 16, KoColorSpaceRegistry::instance()->rgb8(), "Public removal");
        image->addNode(new KisPaintLayer(image, QStringLiteral("Source"), 255));
    }
    document->setCurrentImage(image);
    Document api(document.data(), false);
    NodeSP root(api.rootNode());
    NodeSP bottom(Node::createNode(image, image->root()->firstChild()));
    const auto original = QByteArray::fromHex("112233ff");
    const auto inserted = QByteArray::fromHex("445566ff");
    const auto detached = QByteArray::fromHex("778899ff");
    QVERIFY(bottom->setPixelData(original, 5, 7, 1, 1));
    NodeSP child(cloned ? bottom->clone() : api.createNode("Removable", "paintlayer"));
    QVERIFY(root->addChildNode(child.data(), nullptr));
    QVERIFY(child->setPixelData(inserted, 5, 7, 1, 1));
    const auto native = image->root()->lastChild();
    const auto history = document->undoStack()->count();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    document->setFileBatchMode(true);
    QVERIFY(document->saveAs(directory.filePath("removal.kra"), KisDocument::nativeFormatMimeType(), false));
    QTRY_VERIFY(!document->isModified());
    if (gpu) {
        const auto owner = image->gpuImage();
        const auto before = owner->operations().snapshot();
        const auto stroke = owner->beginStroke(before->layers.first().id, {}, 17);
        QVERIFY(!stroke.isNull());
        QVERIFY(!(viaParent ? root->removeChildNode(child.data()) : child->remove()));
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(document->undoStack()->count(), history);
        QCOMPARE(native->parent(), image->root());
        QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
    }
    try {
        QVERIFY(viaParent ? root->removeChildNode(child.data()) : child->remove());
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 1U);
    QVERIFY(!native->parent());
    QCOMPARE(document->undoStack()->count(), history + 1);
    QTRY_VERIFY(document->isModified());
    api.refreshProjection();
    QCOMPARE(api.pixelData(5, 7, 1, 1), original);
    QCOMPARE(child->pixelData(5, 7, 1, 1), inserted);
    QVERIFY(child->setPixelData(detached, 5, 7, 1, 1));
    api.refreshProjection();
    QCOMPARE(api.pixelData(5, 7, 1, 1), original);
    QVERIFY(!child->remove());
    QCOMPARE(document->undoStack()->count(), history + 1);
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->root()->lastChild(), native);
    api.refreshProjection();
    QCOMPARE(api.pixelData(5, 7, 1, 1), detached);
    QTRY_VERIFY(!document->isModified());
    document->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 1U);
    api.refreshProjection();
    QCOMPARE(api.pixelData(5, 7, 1, 1), original);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestNode::testLastLayerRemovalInWindowClearsSelectionAndRestoresHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("ordinary-document") << false;
    QTest::newRow("gpu-document") << true;
}

void TestNode::testLastLayerRemovalInWindowClearsSelectionAndRestoresHistory()
{
    QFETCH(bool, gpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    Gpu::PaintImageSP owner;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Public removal in window")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Public removal in window"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.get()));
    KisImageSP image = lifetime->image();
    QCOMPARE(image->root()->childCount(), 1U);
    const auto native = image->root()->firstChild();
    auto *manager = window->viewManager()->nodeManager();
    QTRY_COMPARE(manager->activeNode(), native);
    QScopedPointer<Node> api(Node::createNode(image, native));
    QVERIFY(api->remove());
    image->waitForDone();
    QTRY_COMPARE(image->root()->childCount(), 0U);
    QTRY_VERIFY(manager->activeNode() != native);
    QTRY_VERIFY(!manager->selectedNodes().contains(native));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(image->root()->firstChild(), native);
    QTRY_COMPARE(manager->activeNode(), native);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(image->root()->childCount(), 0U);
    QTRY_VERIFY(manager->activeNode() != native);
    QTRY_VERIFY(!manager->selectedNodes().contains(native));
}

void TestNode::testScriptedFigurePaintsRequestedLayerAndPreservesHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("figure");
    QTest::addColumn<QString>("strokeStyle");
    QTest::addColumn<QString>("rejection");
    QTest::addColumn<bool>("cloned");
    for (bool gpu : {false, true}) {
        for (const auto &figure : {QStringLiteral("line"), QStringLiteral("rectangle"),
                                  QStringLiteral("ellipse"), QStringLiteral("polygon"), QStringLiteral("polygon-pair")}) {
            for (const auto &style : {QStringLiteral("ForegroundColor"), QStringLiteral("BackgroundColor")}) {
                const auto name = (gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + figure + '-' + style;
                QTest::newRow(qPrintable(name)) << gpu << figure << style << QString() << false;
            }
        }
    }
    for (bool gpu : {false, true}) {
        QTest::newRow(gpu ? "gpu-line-None" : "ordinary-line-None")
            << gpu << QStringLiteral("line") << QStringLiteral("None") << QString() << false;
    }
    for (bool gpu : {false, true}) {
        for (const auto &figure : {QStringLiteral("line"), QStringLiteral("rectangle"),
                                  QStringLiteral("ellipse"), QStringLiteral("polygon"), QStringLiteral("polygon-pair")}) {
            for (const auto &style : {QStringLiteral("ForegroundColor"), QStringLiteral("BackgroundColor")}) {
                const auto name = (gpu ? QStringLiteral("gpu-clone-") : QStringLiteral("ordinary-clone-")) + figure + '-' + style;
                QTest::newRow(qPrintable(name)) << gpu << figure << style << QString() << true;
            }
        }
        QTest::newRow(gpu ? "gpu-clone-line-None" : "ordinary-clone-line-None")
            << gpu << QStringLiteral("line") << QStringLiteral("None") << QString() << true;
    }
    for (const auto &reason : {QStringLiteral("fill"), QStringLiteral("path"),
                               QStringLiteral("busy"), QStringLiteral("invalid-pressure"), QStringLiteral("none")}) {
        QTest::newRow(qPrintable(QStringLiteral("gpu-preserve-") + reason))
            << true << QStringLiteral("line") << QStringLiteral("ForegroundColor") << reason << false;
    }
}

void TestNode::testScriptedFigurePaintsRequestedLayerAndPreservesHistory()
{
    QFETCH(bool, gpu);
    QFETCH(QString, figure);
    QFETCH(QString, strokeStyle);
    QFETCH(QString, rejection);
    QFETCH(bool, cloned);
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    std::shared_ptr<Gpu::Execution> execution;
    Gpu::PaintImageSP owner;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        Gpu::RasterImage raster;
        raster.size = {64, 64};
        raster.layers.append({QStringLiteral("Requested layer"), true, QUuid::createUuid(), {}});
        raster.layers.append({QStringLiteral("Active layer"), true, QUuid::createUuid(), {}});
        raster.activeLayer = 1;
        owner = Gpu::PaintImageSP::create(execution, raster);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Public figure")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Public figure"), 64, 64, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 2, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    window->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(window.get()));
    KisImageSP image = lifetime->image();
    KisNodeSP target = image->root()->firstChild();
    const auto active = image->root()->lastChild();
    QVERIFY(target != active);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotNonUiActivatedNode(active);
    QTRY_COMPARE(manager->activeNode(), active);
    auto preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
    QVERIFY(preset);
    preset->settings()->setPaintOpSize(8);
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    resources->setBGColor(KoColor(QColor(221, 17, 44), cs));
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    const auto expected = strokeStyle == QLatin1String("BackgroundColor") ?
        QByteArray::fromHex("2c11ddff") : QByteArray::fromHex("f18111ff");
    const QPoint sample(32, figure == QLatin1String("line") || figure == QLatin1String("polygon-pair") ? 32 : 12);
    QScopedPointer<Node> api(Node::createNode(image, target));
    QScopedPointer<Node> activeApi(Node::createNode(image, active));
    if (cloned) {
        QScopedPointer<Node> copy(api->clone());
        QVERIFY(copy);
        QScopedPointer<Node> root(Node::createNode(image, image->root()));
        QVERIFY(root->addChildNode(copy.data(), nullptr));
        image->waitForDone();
        api.swap(copy);
        for (KisNodeSP candidate = image->root()->firstChild(); candidate; candidate = candidate->nextSibling()) {
            if (candidate->uuid() == api->uniqueId()) target = candidate;
        }
        QCOMPARE(target->uuid(), api->uniqueId());
        QTRY_COMPARE(manager->activeNode(), target);
        manager->slotNonUiActivatedNode(active);
        QTRY_COMPARE(manager->activeNode(), active);
    }
    const auto before = api->pixelData(0, 0, 64, 64);
    const auto activeBefore = activeApi->pixelData(0, 0, 64, 64);
    const int count = lifetime->undoStack()->count();
    const int index = lifetime->undoStack()->index();
    Gpu::OperationId pending;
    if (rejection == QLatin1String("busy")) {
        pending = owner->beginStroke(owner->operations().activeLayer(), Gpu::Brush{}, 0);
        QVERIFY(!pending.isNull());
        QCOMPARE(owner->appendSamples(pending, {{{4, 4}, 1}}), Gpu::Result::Success);
    }
    const auto cancelPending = qScopeGuard([&] {
        if (owner && !pending.isNull() && owner->operations().activeOperation() == pending)
            owner->cancelStroke(pending);
    });
    try {
        if (rejection == QLatin1String("fill")) api->paintRectangle({12, 12, 40, 40}, strokeStyle, QStringLiteral("ForegroundColor"));
        else if (rejection == QLatin1String("path")) {
            QPainterPath path;
            path.moveTo(12, 32);
            path.cubicTo(20, 12, 40, 52, 52, 32);
            api->paintPath(path, strokeStyle, QStringLiteral("None"));
        } else if (rejection == QLatin1String("none")) {
            api->paintRectangle({12, 12, 40, 40}, QStringLiteral("None"), QStringLiteral("None"));
        } else if (figure == QLatin1String("line")) {
            api->paintLine({12, 32}, {52, 32}, rejection == QLatin1String("invalid-pressure") ? -1 : 0.5, 1, strokeStyle);
        }
        else if (figure == QLatin1String("rectangle")) api->paintRectangle({12, 12, 40, 40}, strokeStyle, QStringLiteral("None"));
        else if (figure == QLatin1String("ellipse")) api->paintEllipse({12, 12, 40, 40}, strokeStyle, QStringLiteral("None"));
        else if (figure == QLatin1String("polygon-pair")) api->paintPolygon({{12, 32}, {52, 32}}, strokeStyle, QStringLiteral("None"));
        else api->paintPolygon({{12, 12}, {52, 12}, {32, 52}}, strokeStyle, QStringLiteral("None"));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    resources->setFGColor(KoColor(Qt::green, cs));
    resources->setBGColor(KoColor(Qt::yellow, cs));
    resources->setSize(30);
    if (pending.isNull()) image->waitForDone();
    if (!rejection.isEmpty()) {
        QCOMPARE(lifetime->undoStack()->count(), count);
        QCOMPARE(lifetime->undoStack()->index(), index);
        QCOMPARE(api->pixelData(0, 0, 64, 64), before);
        QCOMPARE(activeApi->pixelData(0, 0, 64, 64), activeBefore);
        QCOMPARE(manager->activeNode(), active);
        QCOMPARE(owner->operations().activeOperation(), pending);
        if (!pending.isNull()) QCOMPARE(owner->cancelStroke(pending), Gpu::Result::Success);
        return;
    }
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(lifetime->undoStack()->index(), index + 1);
    QCOMPARE(api->pixelData(sample.x(), sample.y(), 1, 1), expected);
    QCOMPARE(activeApi->pixelData(0, 0, 64, 64), activeBefore);
    QCOMPARE(manager->activeNode(), active);
    const auto painted = api->pixelData(0, 0, 64, 64);
    QVERIFY(painted != before);
    Document apiDocument(lifetime.data(), false);
    apiDocument.refreshProjection();
    QCOMPARE(apiDocument.pixelData(sample.x(), sample.y(), 1, 1), expected);
    if (gpu && figure == QLatin1String("line")) {
        const auto version = owner->operations().snapshot();
        const auto layer = std::find_if(version->layers.cbegin(), version->layers.cend(),
                                        [&](const auto &value) { return value.uuid == target->uuid(); });
        QVERIFY(layer != version->layers.cend());
        QVERIFY(!layer->operations.isEmpty());
        const auto operation = std::get<Gpu::PaintSnapshot>(layer->operations.last());
        QVERIFY(operation);
        const auto samples = operation->inputSamples();
        QCOMPARE(samples.size(), 2);
        QCOMPARE(samples.first().pressure, 0.5);
        QCOMPARE(samples.last().pressure, 1.0);
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(api->pixelData(0, 0, 64, 64), before);
    QCOMPARE(activeApi->pixelData(0, 0, 64, 64), activeBefore);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(api->pixelData(0, 0, 64, 64), painted);
    QCOMPARE(activeApi->pixelData(0, 0, 64, 64), activeBefore);
}

void TestNode::testLayerAndDocumentProjectionsRemainDistinct()
{
    QScopedPointer<KisDocument> owner(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(owner->createUndoStore(), 64, 64, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP lower = new KisPaintLayer(image, "Lower", 255);
    KisNodeSP upper = new KisPaintLayer(image, "Upper", 255);
    image->addNode(lower);
    image->addNode(upper);
    owner->setCurrentImage(image);
    NodeSP lowerNode(Node::createNode(image, lower));
    NodeSP upperNode(Node::createNode(image, upper));
    const auto red = QByteArray::fromHex("0000ffff").repeated(4);
    const auto blue = QByteArray::fromHex("120a2700ff0000ffff0000ffff0000ff");
    QVERIFY(lowerNode->setPixelData(red, 8, 12, 2, 2));
    QVERIFY(upperNode->setPixelData(blue, 8, 12, 2, 2));
    upperNode->setVisible(false);
    Document document(owner.data(), false);
    document.refreshProjection();
    QCOMPARE(document.pixelData(8, 12, 2, 2), red);
    QCOMPARE(upperNode->pixelData(8, 12, 2, 2), blue);
    QCOMPARE(upperNode->projectionPixelData(8, 12, 2, 2), blue);
    upperNode->setVisible(true);
    document.refreshProjection();
    QCOMPARE(document.pixelData(8, 12, 2, 2), red.first(4) + blue.mid(4));
    QCOMPARE(lowerNode->pixelData(8, 12, 2, 2), red);
    QCOMPARE(upperNode->pixelData(8, 12, 2, 2), blue);
    KisPart::instance()->removeDocument(owner.data(), false);
}

void TestNode::testThumbnail()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QImage thumb = node->thumbnail(10, 10);
    thumb.save("thumb.png");
    QVERIFY(thumb.width() == 10);
    QVERIFY(thumb.height() == 10);
    // Our thumbnail calculator in KisPaintDevice cannot make a filled 10x10 thumbnail from a 100x100 device,
    // it makes it 10x10 empty, then puts 8x8 pixels in there... Not a bug in the Node class
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            QVERIFY(thumb.pixelColor(i, j) == QColor(Qt::gray));
        }
    }
}

void TestNode::testGpuBoundsAndThumbnailMatchNativePixels_data()
{
    QTest::addColumn<QByteArray>("defaultPixel");
    QTest::newRow("transparent") << QByteArray::fromHex("00000000");
    QTest::newRow("implicit-color") << QByteArray::fromHex("316ac2c8");
}

void TestNode::testGpuBoundsAndThumbnailMatchNativePixels()
{
    QFETCH(QByteArray, defaultPixel);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::RasterLayer imported;
    imported.name = QStringLiteral("Pixels");
    for (int i = 0; i < 4; ++i) imported.defaultPixel[i] = quint8(defaultPixel[i]);
    auto owner = Gpu::PaintImageSP::create(execution, Gpu::RasterImage{{64, 32}, {imported}, 0});
    auto gpuImage = KisImage::fromGpuImage(owner, QStringLiteral("GPU thumbnail"));
    auto gpuLayer = gpuImage->root()->firstChild();
    NodeSP gpuNode(Node::createNode(gpuImage, gpuLayer));
    KisImageSP cpuImage = new KisImage(nullptr, 64, 32, KoColorSpaceRegistry::instance()->rgb8(), QStringLiteral("Native thumbnail"));
    KisNodeSP cpuLayer = new KisPaintLayer(cpuImage, QStringLiteral("Pixels"), 255);
    cpuImage->addNode(cpuLayer);
    auto bgraDefault = defaultPixel;
    std::swap(bgraDefault[0], bgraDefault[2]);
    cpuLayer->paintDevice()->setDefaultPixel(KoColor(reinterpret_cast<const quint8 *>(bgraDefault.constData()), cpuImage->colorSpace()));
    NodeSP cpuNode(Node::createNode(cpuImage, cpuLayer));
    const auto bytes = QByteArray::fromHex("12345600aa1122ff00000000000000001020308000000000");
    QVERIFY(cpuNode->setPixelData(bytes, -1, -1, 3, 2));
    QVERIFY(gpuNode->setPixelData(bytes, -1, -1, 3, 2));
    QCOMPARE(gpuNode->bounds(), cpuNode->bounds());
    QCOMPARE(gpuNode->thumbnail(12, 10), cpuNode->thumbnail(12, 10));
    QCOMPARE(gpuNode->position(), cpuNode->position());
    const auto sequence = gpuLayer->thumbnailSeqNo();
    QVERIFY(owner->addLayer(QStringLiteral("Other")));
    QCOMPARE(gpuLayer->thumbnailSeqNo(), sequence);
    QVERIFY(gpuNode->setPixelData(QByteArray::fromHex("552211ff"), 0, -1, 1, 1));
    QVERIFY(gpuLayer->thumbnailSeqNo() != sequence);
    QVERIFY(cpuNode->setPixelData(QByteArray::fromHex("552211ff"), 0, -1, 1, 1));
    QCOMPARE(gpuNode->bounds(), cpuNode->bounds());
    QCOMPARE(gpuNode->thumbnail(12, 10), cpuNode->thumbnail(12, 10));
}

void TestNode::testMergeDown()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");

    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    {
        KisFillPainter gc(layer->paintDevice());
        gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer->colorSpace()));
    }
    image->addNode(layer);

    KisNodeSP layer2 = new KisPaintLayer(image, "test2", 255);
    {
        KisFillPainter gc(layer2->paintDevice());
        gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer2->colorSpace()));
    }
    image->addNode(layer2);
    NodeSP n1 = NodeSP(Node::createNode(image, layer));
    Node *n2 = n1->mergeDown();
    delete n2;
}

void TestNode::testFindChildNodes()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());

    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP gLayer1 = new KisGroupLayer(image, "gLayer1", OPACITY_OPAQUE_U8);
    KisNodeSP pLayer1 = new KisPaintLayer(image, "pLayer1", 255);
    KisNodeSP pLayer2 = new KisPaintLayer(image, "pLayer2", 255);
    KisNodeSP vLayer1 = new KisShapeLayer(kisdoc->shapeController(), image, "vLayer1", OPACITY_OPAQUE_U8);
    KisNodeSP vLayer2 = new KisShapeLayer(kisdoc->shapeController(), image, "vLayer2", OPACITY_OPAQUE_U8);

    gLayer1->setColorLabelIndex(2);

    NodeSP rootNode = NodeSP(Node::createNode(image, image->rootLayer()));

    image->addNode(pLayer1);
    image->addNode(vLayer1);
    image->addNode(pLayer2, gLayer1, gLayer1->childCount());
    image->addNode(vLayer2, gLayer1, gLayer1->childCount());
    image->addNode(gLayer1);

    kisdoc->setCurrentImage(image);

    {
        QStringList test = {"pLayer1", "vLayer1", "gLayer1"};
        QList<Node *> nodeList = rootNode->findChildNodes();

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"pLayer1", "vLayer1", "gLayer1", "vLayer2", "pLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("", true);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"vLayer1"};
        QList<Node *> nodeList = rootNode->findChildNodes("vLayer1");

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"vLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("vLayer2", true);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"vLayer1", "vLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("vLayer", true, true);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"pLayer1", "pLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("", true, false, "paintlayer");

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"gLayer1"};
        QList<Node *> nodeList = rootNode->findChildNodes("", true, false, "", 2);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

}


KISTEST_MAIN(TestNode)

