/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestNode.h"

#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <application/kis_config.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <document/KisDocument.h>
#include <canvas/KisDisplayConfig.h>
#include <flake/kis_shape_controller.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_image_config.h>
#include <kis_mimedata.h>
#include <kis_clipboard.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kundo2stack.h>
#include <nodes/kis_node_insertion_adapter.h>
#include <nodes/kis_node_manager.h>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPointer>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <memory>
#include <stdexcept>

namespace {
KisImageSP clipboardSource(bool gpu, bool wide, KisDocument *document, QPoint point = {3, 3})
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const QUuid groupId(QStringLiteral("{b1000001-1122-3344-5566-778899aabbcc}"));
    const QUuid leafId(QStringLiteral("{b1000002-1122-3344-5566-778899aabbcc}"));
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        if (!execution) throw std::runtime_error(diagnostic.toStdString());
        Gpu::RasterLayer group{QStringLiteral("Group"), true, groupId, {}};
        group.kind = Gpu::LayerKind::Group;
        Gpu::RasterLayer leaf{QStringLiteral("Leaf"), true, leafId, {}};
        leaf.parentUuid = groupId;
        QByteArray tile(Gpu::RgbaTile::ByteCount, '\0');
        const QPoint coordinate(point.x() < 0 ? -1 : 0, point.y() < 0 ? -1 : 0);
        const QPoint local = point - coordinate * 256;
        tile.replace((local.y() * 256 + local.x()) * 4, 4, QByteArray::fromHex("00ff00ff"));
        leaf.tiles = {{coordinate, tile}};
        Gpu::RasterImage input;
        input.size = {32, 16};
        input.layers = {group, leaf};
        if (wide) for (int i = 0; i < 16; ++i) {
            Gpu::RasterLayer extra{QStringLiteral("Extra %1").arg(i), true,
                QUuid::createUuidV5(groupId, QByteArray::number(i)), {}};
            extra.parentUuid = groupId;
            input.layers.append(extra);
        }
        return KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Clipboard source"));
    }
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(document->createUndoStore(), 32, 16, cs, "Clipboard source");
    auto group = new KisGroupLayer(image, QStringLiteral("Group"), 255);
    group->setUuid(groupId);
    image->addNode(group);
    auto leaf = new KisPaintLayer(image, QStringLiteral("Leaf"), 255);
    leaf->setUuid(leafId);
    const auto green = QByteArray::fromHex("00ff00ff");
    leaf->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(green.constData()), point.x(), point.y(), 1, 1);
    image->addNode(leaf, group);
    if (wide) for (int i = 0; i < 16; ++i) {
        auto extra = new KisPaintLayer(image, QStringLiteral("Extra %1").arg(i), 255);
        extra->setUuid(QUuid::createUuidV5(groupId, QByteArray::number(i)));
        image->addNode(extra, group);
    }
    return image;
}

void addClipboardRows()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("scenario");
    for (const bool gpu : {false, true})
        for (const auto &scenario : {QStringLiteral("group"), QStringLiteral("wide"), QStringLiteral("overlap")})
            QTest::newRow(qPrintable((gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + scenario)) << gpu << scenario;
}
}

void TestNode::testClipboardLayersSurviveSourceEditsAndClosure_data()
{ addClipboardRows(); }

void TestNode::testClipboardLayersSurviveSourceEditsAndClosure()
try {
    QFETCH(bool, gpu);
    QFETCH(QString, scenario);
    const bool previousRename = KisImageConfig(true).renameDuplicatedLayers();
    const auto restoreRename = qScopeGuard([&] { KisImageConfig(false).setRenameDuplicatedLayers(previousRename); });
    { KisImageConfig configuration(false); configuration.setRenameDuplicatedLayers(false); }
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisDocument> source(KisPart::instance()->createDocument());
    auto sourceImage = clipboardSource(gpu, scenario == QLatin1String("wide"), source.get());
    source->setCurrentImage(sourceImage);
    sourceImage->waitForDone();
    KisNodeSP group = sourceImage->root()->firstChild();
    KisNodeSP leaf = group->firstChild();
    const KisNodeList selected = scenario == QLatin1String("overlap") ? KisNodeList{group, leaf} : KisNodeList{group};
    QScopedPointer<QMimeData> mime(KisMimeData::mimeForLayersDeepCopy(selected, sourceImage, true));
    QVERIFY(mime);
    {
        QScopedPointer<Node> api(Node::createNode(sourceImage, leaf));
        QVERIFY(api->setPixelData(QByteArray::fromHex("ff0000ff"), 3, 3, 1, 1));
    }
    KisImageWSP oldImage = sourceImage;
    source->setModified(false);
    source.reset();
    sourceImage.clear();
    QVERIFY(!oldImage.isValid());

    std::unique_ptr<KisDocument> destination(KisPart::instance()->createDocument());
    auto image = clipboardSource(gpu, false, destination.get());
    destination->setCurrentImage(image);
    image->waitForDone();
    const auto original = image->root()->firstChild();
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(destination.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(destination.release());
    const auto cleanup = qScopeGuard([&] {
        if (lifetime) {
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
    auto *manager = window->viewManager()->nodeManager();
    auto *controller = dynamic_cast<KisShapeController *>(lifetime->shapeController());
    QVERIFY(controller);
    QTRY_VERIFY(controller->dummyForNode(original));
    manager->slotNonUiActivatedNode(original);
    manager->slotSetSelectedNodes({original});
    const auto history = lifetime->undoStack()->index();
    QVERIFY(KisMimeData::insertMimeLayers(mime.data(), image, controller,
        controller->dummyForNode(image->root()), controller->dummyForNode(original), true, manager->nodeInsertionAdapter()));
    image->waitForDone();
    const unsigned rootCount = scenario == QLatin1String("overlap") ? 3u : 2u;
    QCOMPARE(image->root()->childCount(), rootCount);
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    const auto copied = image->root()->at(1);
    QCOMPARE(copied->name(), QStringLiteral("Group"));
    QCOMPARE(copied->childCount(), scenario == QLatin1String("wide") ? 17u : 1u);
    QScopedPointer<Node> api(Node::createNode(image, copied->firstChild()));
    QCOMPARE(api->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    QTRY_COMPARE(manager->activeNode(), copied);
    QTRY_COMPARE(manager->selectedNodes().size(), qsizetype(rootCount - 1));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 1u);
    QCOMPARE(lifetime->undoStack()->index(), history);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(image->root()->at(1), copied);
    QCOMPARE(api->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    if (scenario == QLatin1String("overlap")) {
        QVERIFY(api->setPixelData(QByteArray::fromHex("ff0000ff"), 3, 3, 1, 1));
        QScopedPointer<Node> independent(Node::createNode(image, image->root()->at(2)));
        QCOMPARE(independent->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    }
    bool copy = true;
    const auto repeated = KisMimeData::loadNodesFast(mime.data(), image, controller, copy);
    QCOMPARE(repeated.size(), qsizetype(rootCount - 1));
    QScopedPointer<Node> frozen(Node::createNode({}, repeated.first()->firstChild()));
    QCOMPARE(frozen->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
}
catch (const std::exception &error) { QFAIL(error.what()); }

void TestNode::testGpuClipboardRefusalKeepsCapturedLayers()
try {
    namespace Gpu = LibrePaint::Image::Gpu;
    std::unique_ptr<KisDocument> source(KisPart::instance()->createDocument());
    const auto image = clipboardSource(true, false, source.get());
    source->setCurrentImage(image);
    const auto owner = image->gpuImage();
    QList<Gpu::PaintImageSP> blockers;
    for (;;) {
        try { blockers.append(Gpu::PaintImageSP::create(owner->execution(), QSize(1, 1))); }
        catch (const std::length_error &) { break; }
    }
    QVERIFY(!blockers.isEmpty());
    const auto version = owner->operations().snapshot();
    const auto group = image->root()->firstChild();
    QScopedPointer<QMimeData> refused(KisMimeData::mimeForLayersDeepCopy({group}, image, true));
    QVERIFY(!refused);
    QCOMPARE(owner->operations().snapshot(), version);
    blockers.removeLast();
    QScopedPointer<QMimeData> captured(KisMimeData::mimeForLayersDeepCopy({group}, image, true));
    QVERIFY(captured);
    const auto native = qobject_cast<const KisMimeData *>(captured.data());
    QVERIFY(native);
    const auto frozen = native->nodes();
    QVERIFY(!captured->imageData().isValid());
    QVERIFY(captured->data("application/zip").isEmpty());
    QCOMPARE(native->nodes(), frozen);
    QCOMPARE(owner->operations().snapshot(), version);
    blockers.clear();
    QVERIFY(!qvariant_cast<QImage>(captured->imageData()).isNull());
    QVERIFY(!captured->data("application/zip").isEmpty());
    QCOMPARE(native->nodes(), frozen);
    QCOMPARE(owner->operations().snapshot(), version);
}
catch (const std::exception &error) { QFAIL(error.what()); }

void TestNode::testGpuCutWaitsForClipboardCapture()
try {
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig(true).disableOpenGL();
    auto *clipboard = QApplication::clipboard();
    clipboard->setText(QStringLiteral("Preserved clipboard"));
    const auto clearClipboard = qScopeGuard([&] { clipboard->clear(); });
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    auto image = clipboardSource(true, false, document.get());
    document->setCurrentImage(image);
    image->waitForDone();
    const auto group = image->root()->firstChild();
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto cleanup = qScopeGuard([&] {
        if (lifetime) {
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
    auto *manager = window->viewManager()->nodeManager();
    manager->slotNonUiActivatedNode(group);
    manager->slotSetSelectedNodes({group});
    const auto owner = image->gpuImage();
    const auto version = owner->operations().snapshot();
    const auto history = lifetime->undoStack()->index();
    QList<Gpu::PaintImageSP> blockers;
    for (;;) {
        try { blockers.append(Gpu::PaintImageSP::create(owner->execution(), QSize(1, 1))); }
        catch (const std::length_error &) { break; }
    }
    QVERIFY(!blockers.isEmpty());
    manager->cutLayersToClipboard();
    image->waitForDone();
    QCOMPARE(clipboard->text(), QStringLiteral("Preserved clipboard"));
    QCOMPARE(image->root()->childCount(), 1u);
    QCOMPARE(image->root()->firstChild(), group);
    QCOMPARE(owner->operations().snapshot(), version);
    QCOMPARE(lifetime->undoStack()->index(), history);
    blockers.clear();
    manager->cutLayersToClipboard();
    image->waitForDone();
    QVERIFY(KisClipboard::instance()->hasLayers());
    QCOMPARE(image->root()->childCount(), 1u);
    const auto replacement = image->root()->firstChild();
    QVERIFY(replacement != group);
    QVERIFY(replacement->inherits("KisPaintLayer"));
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    manager->pasteLayersFromClipboard();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 2u);
    QCOMPARE(lifetime->undoStack()->index(), history + 2);
    const auto copied = image->root()->lastChild();
    QVERIFY(copied != group);
    QScopedPointer<Node> leaf(Node::createNode(image, copied->firstChild()));
    QCOMPARE(leaf->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 1u);
    QCOMPARE(image->root()->firstChild(), replacement);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->root()->firstChild(), group);
    QCOMPARE(lifetime->undoStack()->index(), history);
}
catch (const std::exception &error) { QFAIL(error.what()); }

void TestNode::testClipboardLayersProvideImageAndNativeData_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QPoint>("point");
    for (const bool gpu : {false, true}) for (const auto &format : {QStringLiteral("image"), QStringLiteral("kra")})
        for (const QPoint point : {QPoint(3, 3), QPoint(-3, -3), QPoint(40, 20)})
            QTest::newRow(qPrintable((gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + format +
                QStringLiteral("-%1-%2").arg(point.x()).arg(point.y()))) << gpu << format << point;
}

void TestNode::testClipboardLayersProvideImageAndNativeData()
try {
    QFETCH(bool, gpu);
    QFETCH(QString, format);
    QFETCH(QPoint, point);
    const auto fixedProfile = KoColorSpaceRegistry::instance()->p709SRGBProfile();
    const auto previousProfile = KisConfig(true).monitorProfile(0);
    const auto previousSystemProfile = KisConfig(true).useSystemMonitorProfile();
    const auto previousOverride = KisConfig(true).readEntry<bool>("monitorProfile/OverrideX11", false);
    const auto restoreProfile = qScopeGuard([&] {
        KisConfig config(false);
        config.setMonitorProfile(0, previousProfile, previousOverride);
        config.setUseSystemMonitorProfile(previousSystemProfile);
    });
    {
        KisConfig config(false);
        config.setMonitorProfile(0, fixedProfile->name(), true);
        config.setUseSystemMonitorProfile(false);
    }
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisDocument> source(KisPart::instance()->createDocument());
    auto image = clipboardSource(gpu, false, source.get(), point);
    source->setCurrentImage(image);
    image->waitForDone();
    QScopedPointer<QMimeData> mime(KisMimeData::mimeForLayersDeepCopy({image->root()->firstChild()}, image, true));
    QVERIFY(mime);
    source->setModified(false);
    source.reset();
    image.clear();
    const auto display = KisMimeData::displayConfigForMimePastes();
    QVERIFY(display.profile);
    QCOMPARE(display.profile->rawData(), fixedProfile->rawData());
    const auto green = QByteArray::fromHex("00ff00ff");
    const auto expected = KoColorSpaceRegistry::instance()->rgb8()->convertToQImage(
        reinterpret_cast<const quint8 *>(green.constData()), 1, 1, display.profile, display.intent, display.conversionFlags);
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = gpu && !results.isEmpty() ? results : output.path();
    QVERIFY(QDir().mkpath(destination));
    const auto stem = QStringLiteral("clipboard-%1-%2-%3").arg(format).arg(point.x()).arg(point.y());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            output.setAutoRemove(false);
            qInfo() << "Clipboard format artifacts retained at" << output.path();
        }
    });
    if (format == QLatin1String("image")) {
        const auto actual = qvariant_cast<QImage>(mime->imageData());
        actual.save(QDir(destination).filePath(stem + QStringLiteral(".png")));
        expected.save(output.filePath(QStringLiteral("expected.png")));
        QCOMPARE(actual.size(), QSize(1, 1));
        QCOMPARE(actual, expected);
    } else {
        const auto data = mime->data("application/zip");
        QVERIFY(!data.isEmpty());
        QFile file(QDir(destination).filePath(stem + QStringLiteral(".kra")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), qint64(data.size()));
        file.close();
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(file.fileName(), KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        const auto canvas = QRect(point, QSize(1, 1)).united(QRect(0, 0, 1, 1)).size();
        QCOMPARE(reopened->image()->size(), canvas);
        QCOMPARE(reopened->image()->root()->childCount(), 1u);
        const auto group = reopened->image()->root()->firstChild();
        QCOMPARE(group->name(), QStringLiteral("Group"));
        QCOMPARE(group->childCount(), 1u);
        QScopedPointer<Node> leaf(Node::createNode(reopened->image(), group->firstChild()));
        QCOMPARE(leaf->name(), QStringLiteral("Leaf"));
        QCOMPARE(leaf->pixelData(point.x(), point.y(), 1, 1), green);
        Document api(reopened.data(), false);
        if (QRect(QPoint(), canvas).contains(point)) QCOMPARE(api.pixelData(point.x(), point.y(), 1, 1), green);
        QCOMPARE(api.pixelData(0, 0, 1, 1), QByteArray(4, '\0'));
    }
}
catch (const std::exception &error) { QFAIL(error.what()); }
