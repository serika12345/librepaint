/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestNode.h"
#include "TestNodeMimeCopyInputs.h"

#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <InfoObject.h>
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
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <kundo2stack.h>
#include <nodes/kis_node_insertion_adapter.h>
#include <nodes/kis_node_manager.h>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPointer>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <memory>
#include <stdexcept>

namespace {
KisImageSP clipboardSource(bool gpu, bool wide, KisDocument *document, QPoint point = {3, 3}, bool extraRoot = false)
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
        if (extraRoot) input.layers.append({QStringLiteral("Top"), true,
            QUuid(QStringLiteral("{b1000003-1122-3344-5566-778899aabbcc}")), {}});
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
    if (extraRoot) {
        auto top = new KisPaintLayer(image, QStringLiteral("Top"), 255);
        top->setUuid(QUuid(QStringLiteral("{b1000003-1122-3344-5566-778899aabbcc}")));
        image->addNode(top);
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

void TestNode::testMergedClipboardSurvivesImageReplacement_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("ordinary") << false;
    QTest::newRow("gpu") << true;
}

void TestNode::testMergedClipboardSurvivesImageReplacement()
try {
    QFETCH(bool, gpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig(true).disableOpenGL();
    auto *clipboard = QApplication::clipboard();
    clipboard->clear();
    const auto clearClipboard = qScopeGuard([&] { clipboard->clear(); });
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    auto image = clipboardSource(gpu, false, document.get());
    document->setCurrentImage(image);
    image->waitForDone();
    KisImageSP replacement;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        replacement = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, QSize(12, 6)), QStringLiteral("Replacement"));
    } else {
        replacement = new KisImage(document->createUndoStore(), 12, 6, image->colorSpace(), "Replacement");
        replacement->addNode(new KisPaintLayer(replacement, QStringLiteral("Replacement"), 255));
    }
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
    image.clear();
    QObject receiver;
    bool replaced = false;
    QObject::connect(clipboard, &QClipboard::dataChanged, &receiver, [&] {
        if (replaced) return;
        replaced = true;
        lifetime->setCurrentImage(replacement);
    });
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(action);
    action->trigger();
    QTRY_VERIFY(replaced);
    QCOMPARE(lifetime->image().data(), replacement.data());
    const auto native = qobject_cast<const KisMimeData *>(KisClipboard::instance()->layersMimeData());
    QVERIFY(native);
    QCOMPARE(native->nodes().size(), 1);
    QScopedPointer<Node> copied(Node::createNode({}, native->nodes().first()));
    QCOMPARE(copied->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    Document current(lifetime.data(), false);
    current.refreshProjection();
    QCOMPARE(current.pixelData(0, 0, 12, 6), QByteArray(12 * 6 * 4, '\0'));
}
catch (const std::exception &error) { QFAIL(error.what()); }

void TestNode::testMergedClipboardPreservesProjectionAndSource_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<QString>("entry");
    for (const bool gpu : {false, true})
        for (const auto &scenario : {QStringLiteral("group"), QStringLiteral("opacity"), QStringLiteral("empty"),
                                    QStringLiteral("outside"), QStringLiteral("multiple-roots"), QStringLiteral("hidden-rgb")})
            for (const auto &entry : {QStringLiteral("normal"), QStringLiteral("fixed")})
                QTest::newRow(qPrintable((gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + entry + QLatin1Char('-') + scenario))
                    << gpu << scenario << entry;
    QTest::newRow("gpu-capacity") << true << QStringLiteral("capacity") << QStringLiteral("normal");
}

void TestNode::testMergedClipboardPreservesProjectionAndSource()
try {
    QFETCH(bool, gpu);
    QFETCH(QString, scenario);
    QFETCH(QString, entry);
    namespace Gpu = LibrePaint::Image::Gpu;
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
    auto *clipboard = QApplication::clipboard();
    clipboard->setText(QStringLiteral("Preserved clipboard"));
    const auto clearClipboard = qScopeGuard([&] { clipboard->clear(); });
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const QPoint point = scenario == QLatin1String("outside") ? QPoint(40, 20) : QPoint(3, 3);
    auto image = clipboardSource(gpu, false, document.get(), point, scenario == QLatin1String("multiple-roots"));
    document->setCurrentImage(image);
    document->setFileBatchMode(true);
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed merged clipboard"));
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    const auto group = image->root()->firstChild();
    QScopedPointer<Node> originalLeaf(Node::createNode(image, group->firstChild()));
    if (scenario == QLatin1String("empty") || scenario == QLatin1String("hidden-rgb"))
        QVERIFY(originalLeaf->setPixelData(scenario == QLatin1String("empty") ? QByteArray(4, '\0') :
                                          QByteArray::fromHex("ff221100"), point.x(), point.y(), 1, 1));
    if (scenario == QLatin1String("opacity")) {
        QScopedPointer<Node> originalGroup(Node::createNode(image, group));
        originalGroup->setOpacity(128);
    }
    image->waitForDone();
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
    const auto unlockAfterFailure = qScopeGuard([&] { while (image->locked()) image->unlock(); });
    auto *manager = window->viewManager()->nodeManager();
    manager->slotNonUiActivatedNode(group);
    manager->slotSetSelectedNodes({group});
    QTRY_COMPARE(manager->activeNode(), group);
    Document source(lifetime.data(), false);
    source.refreshProjection();
    const auto expected = source.pixelData(0, 0, 32, 16);
    const auto sourceLeafPixels = originalLeaf->pixelData(point.x(), point.y(), 1, 1);
    const auto history = lifetime->undoStack()->index();
    const auto modified = lifetime->isModified();
    const auto version = gpu ? image->gpuImage()->operations().snapshot() : Gpu::Snapshot();
    QList<Gpu::PaintImageSP> blockers;
    if (scenario == QLatin1String("capacity")) {
        QVERIFY(!KisClipboard::instance()->setGpuProjection(image, 1));
        QCOMPARE(clipboard->text(), QStringLiteral("Preserved clipboard"));
        QCOMPARE(image->gpuImage()->operations().snapshot(), version);
        for (;;) {
            try { blockers.append(Gpu::PaintImageSP::create(image->gpuImage()->execution(), QSize(1, 1))); }
            catch (const std::length_error &) { break; }
        }
        QVERIFY(!blockers.isEmpty());
    }
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(action);
    action->trigger();
    image->waitForDone();
    QCOMPARE(source.pixelData(0, 0, 32, 16), expected);
    QCOMPARE(originalLeaf->pixelData(point.x(), point.y(), 1, 1), sourceLeafPixels);
    QCOMPARE(lifetime->undoStack()->index(), history);
    QCOMPARE(lifetime->isModified(), modified);
    QVERIFY(!image->locked());
    if (gpu) QCOMPARE(image->gpuImage()->operations().snapshot(), version);
    if (scenario == QLatin1String("capacity")) {
        QCOMPARE(clipboard->text(), QStringLiteral("Preserved clipboard"));
        blockers.clear();
        action->trigger();
        image->waitForDone();
    }
    const auto mime = KisClipboard::instance()->layersMimeData();
    QVERIFY(mime);
    const auto native = qobject_cast<const KisMimeData *>(mime);
    QVERIFY(native);
    QCOMPARE(native->nodes().size(), 1);
    KisNodeSP copied = native->nodes().first();
    QVERIFY(copied->inherits("KisPaintLayer"));
    QCOMPARE(copied->childCount(), 0u);
    QCOMPARE(copied->name(), QStringLiteral("Projection"));
    QScopedPointer<Node> pixels(Node::createNode({}, copied));
    QCOMPARE(pixels->pixelData(0, 0, 32, 16), expected);
    QCOMPARE(pixels->pixelData(40, 20, 1, 1), QByteArray(4, '\0'));
    const auto bytes = mime->data("application/zip");
    QVERIFY(!bytes.isEmpty());
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = gpu && !results.isEmpty() ? results : output.path();
    QVERIFY(QDir().mkpath(destination));
    const auto stem = QStringLiteral("merged-clipboard-%1-%2").arg(entry, scenario);
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            output.setAutoRemove(false);
            qInfo() << "Merged clipboard artifacts retained at" << output.path();
        }
    });
    const auto display = KisMimeData::displayConfigForMimePastes();
    QVERIFY(display.profile);
    QCOMPARE(display.profile->rawData(), fixedProfile->rawData());
    QRect visible;
    for (int y = 0; y < 16; ++y) for (int x = 0; x < 32; ++x)
        if (quint8(expected[(y * 32 + x) * 4 + 3])) visible |= QRect(x, y, 1, 1);
    const auto converted = image->colorSpace()->convertToQImage(reinterpret_cast<const quint8 *>(expected.constData()),
        32, 16, display.profile, display.intent, display.conversionFlags);
    const auto expectedImage = visible.isEmpty() ? QImage() : converted.copy(visible);
    const auto actualImage = qvariant_cast<QImage>(mime->imageData());
    if (!actualImage.isNull()) QVERIFY(actualImage.save(QDir(destination).filePath(stem + QStringLiteral(".png"))));
    QCOMPARE(actualImage, expectedImage);
    QFile pixelFile(QDir(destination).filePath(stem + QStringLiteral(".pixels")));
    QVERIFY(pixelFile.open(QIODevice::WriteOnly));
    QCOMPARE(pixelFile.write(pixels->pixelData(0, 0, 32, 16)), qint64(expected.size()));
    pixelFile.close();
    QFile file(QDir(destination).filePath(stem + QStringLiteral("-clipboard.kra")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(bytes), qint64(bytes.size()));
    file.close();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(file.fileName(), KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QScopedPointer<Node> loaded(Node::createNode(reopened->image(), reopened->image()->root()->firstChild()));
    QCOMPARE(loaded->pixelData(0, 0, 32, 16), expected);
    if (entry == QLatin1String("fixed")) {
        auto *controller = dynamic_cast<KisShapeController *>(lifetime->shapeController());
        QVERIFY(controller);
        FixedMimeCopyInputs fixed(manager->nodeInsertionAdapter());
        QVERIFY(KisMimeData::insertMimeLayers(mime, image, controller, controller->dummyForNode(group),
            controller->dummyForNode(group->lastChild()), true, &fixed));
    } else {
        manager->pasteLayersFromClipboard();
    }
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    QCOMPARE(group->childCount(), 2u);
    const auto pasted = group->lastChild();
    QTRY_COMPARE(manager->activeNode(), pasted);
    QVERIFY(pasted->inherits("KisPaintLayer"));
    QCOMPARE(pasted->parent(), group);
    QScopedPointer<Node> pastedPixels(Node::createNode(image, pasted));
    QCOMPARE(pastedPixels->pixelData(0, 0, 32, 16), expected);
    if (entry == QLatin1String("fixed")) {
        KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
        configuration->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
        const auto path = QDir(destination).filePath(stem + QStringLiteral("-pasted.kra"));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false, configuration), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        QScopedPointer<KisDocument> saved(KisPart::instance()->createDocument());
        saved->setFileBatchMode(true);
        QVERIFY2(saved->openPath(path, KisDocument::DontAddToRecent), qPrintable(saved->errorMessage()));
        Document savedApi(saved.data(), false);
        QScopedPointer<Node> savedPixels(savedApi.nodeByUniqueID(pasted->uuid()));
        QVERIFY(savedPixels);
        QCOMPARE(savedPixels->pixelData(0, 0, 32, 16), expected);
        InfoObject png;
        png.setProperty("alpha", true);
        png.setProperty("interlaced", false);
        png.setProperty("compression", 3);
        png.setProperty("saveSRGBProfile", true);
        png.setProperty("forceSRGB", false);
        png.setProperty("storeAuthor", false);
        png.setProperty("storeMetaData", false);
        QVERIFY(source.exportImage(QDir(destination).filePath(stem + QStringLiteral("-pasted.png")), png));
        if (gpu) {
            QFile recovery(QDir(destination).filePath(stem + QStringLiteral(".recovery")));
            QVERIFY(recovery.open(QIODevice::WriteOnly));
            const auto data = image->gpuImage()->operations().recoveryData();
            QCOMPARE(recovery.write(data), qint64(data.size()));
        }
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), history);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(manager->activeNode(), pasted);
    QCOMPARE(pastedPixels->pixelData(0, 0, 32, 16), expected);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(source.pixelData(0, 0, 32, 16), expected);
    QVERIFY(originalLeaf->setPixelData(QByteArray::fromHex("ff0000ff"), point.x(), point.y(), 1, 1));
    QCOMPARE(pixels->pixelData(0, 0, 32, 16), expected);
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
    QTest::addColumn<bool>("empty");
    for (const bool gpu : {false, true}) for (const auto &format : {QStringLiteral("image"), QStringLiteral("kra")})
        for (const QPoint point : {QPoint(3, 3), QPoint(-3, -3), QPoint(40, 20)})
            QTest::newRow(qPrintable((gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + format +
                QStringLiteral("-%1-%2").arg(point.x()).arg(point.y()))) << gpu << format << point << false;
    for (const bool gpu : {false, true}) for (const auto &format : {QStringLiteral("image"), QStringLiteral("kra")})
        QTest::newRow(qPrintable((gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + format + QStringLiteral("-empty")))
            << gpu << format << QPoint(3, 3) << true;
}

void TestNode::testClipboardLayersProvideImageAndNativeData()
try {
    QFETCH(bool, gpu);
    QFETCH(QString, format);
    QFETCH(QPoint, point);
    QFETCH(bool, empty);
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
    if (empty) {
        QScopedPointer<Node> leaf(Node::createNode(image, image->root()->firstChild()->firstChild()));
        QVERIFY(leaf->setPixelData(QByteArray(4, '\0'), point.x(), point.y(), 1, 1));
    }
    image->waitForDone();
    QScopedPointer<QMimeData> mime(KisMimeData::mimeForLayersDeepCopy({image->root()->firstChild()}, image, true));
    QVERIFY(mime);
    source->setModified(false);
    source.reset();
    image.clear();
    const auto display = KisMimeData::displayConfigForMimePastes();
    QVERIFY(display.profile);
    QCOMPARE(display.profile->rawData(), fixedProfile->rawData());
    const auto green = empty ? QByteArray(4, '\0') : QByteArray::fromHex("00ff00ff");
    const auto expected = empty ? QImage() : KoColorSpaceRegistry::instance()->rgb8()->convertToQImage(
        reinterpret_cast<const quint8 *>(green.constData()), 1, 1, display.profile, display.intent, display.conversionFlags);
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = gpu && !results.isEmpty() ? results : output.path();
    QVERIFY(QDir().mkpath(destination));
    const auto stem = QStringLiteral("clipboard-%1-%2-%3%4").arg(format).arg(point.x()).arg(point.y())
        .arg(empty ? QStringLiteral("-empty") : QString());
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
        QCOMPARE(actual.size(), empty ? QSize(0, 0) : QSize(1, 1));
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
        const auto canvas = empty ? QSize(32, 16) : QRect(point, QSize(1, 1)).united(QRect(0, 0, 1, 1)).size();
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
