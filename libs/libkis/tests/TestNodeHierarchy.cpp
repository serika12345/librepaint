/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestNode.h"

#include <simpletest.h>
#include <Document.h>
#include <GroupLayer.h>
#include <InfoObject.h>
#include <Krita.h>
#include <Node.h>
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_config.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_image_config.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_properties_configuration.h>
#include <kundo2stack.h>
#include <metadata/KoDocumentInfo.h>
#include <nodes/kis_node_manager.h>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPointer>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <memory>
#include <exception>

void TestNode::testGroupCreationKeepsIndependentPixelsChildrenAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("entry");
    for (const bool gpu : {false, true}) {
        for (const auto &entry : {QStringLiteral("layer-panel"), QStringLiteral("layer-panel-root"),
                                 QStringLiteral("layer-panel-collapsed"), QStringLiteral("layer-panel-locked"),
                                 QStringLiteral("generic-api"), QStringLiteral("group-api")}) {
            const auto tag = (gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + entry;
            QTest::newRow(qPrintable(tag)) << gpu << entry;
        }
    }
}

void TestNode::testGroupCreationKeepsIndependentPixelsChildrenAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(QString, entry);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterLayer base{QStringLiteral("Base"), true, {}, {}};
        base.defaultPixel = {255, 0, 0, 255};
        Gpu::RasterLayer outer{QStringLiteral("Outer"), true, {}, {}};
        outer.kind = Gpu::LayerKind::Group;
        Gpu::RasterImage input;
        input.size = {32, 16};
        input.layers = {base, outer};
        input.activeLayer = 1;
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Group creation"));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        image = new KisImage(document->createUndoStore(), 32, 16, cs, "Group creation");
        KisNodeSP base = new KisPaintLayer(image, QStringLiteral("Base"), 255);
        base->paintDevice()->setDefaultPixel(KoColor(Qt::red, cs));
        image->addNode(base);
        image->addNode(new KisGroupLayer(image, QStringLiteral("Outer"), 255));
    }
    document->setCurrentImage(image);
    KisNodeSP outer = image->root()->lastChild();
    outer->setCollapsed(entry == QLatin1String("layer-panel-collapsed"));
    outer->setUserLocked(entry == QLatin1String("layer-panel-locked"));
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
    auto *manager = window->viewManager()->nodeManager();
    QTRY_VERIFY(lifetime->shapeForNode(outer));
    const KisNodeSP active = entry == QLatin1String("layer-panel-root") ? image->root()->firstChild() : outer;
    manager->slotNonUiActivatedNode(active);
    QTRY_COMPARE(manager->activeNode(), active);
    const KisNodeSP parent = entry == QLatin1String("layer-panel-root") ||
        entry == QLatin1String("layer-panel-collapsed") || entry == QLatin1String("layer-panel-locked") ? KisNodeSP(image->root()) : outer;
    const auto childCount = parent->childCount();
    const int position = parent == outer ? int(childCount) : parent->index(active) + 1;
    Document api(lifetime.data(), false);
    const auto history = lifetime->undoStack()->index();
    const auto transparent = QByteArray(4, '\0');
    const auto green = QByteArray::fromHex("00ff00ff");
    NodeSP group;
    KisNodeSP native;
    if (entry.startsWith(QLatin1String("layer-panel"))) {
        try {
            native = manager->createNode(QStringLiteral("KisGroupLayer"), true);
        } catch (const std::exception &error) {
            QFAIL(error.what());
        }
        QVERIFY(native);
        group.reset(Node::createNode(image, native));
    } else {
        group.reset(entry == QLatin1String("generic-api")
            ? api.createNode(QStringLiteral("Created"), QStringLiteral("grouplayer"))
            : api.createGroupLayer(QStringLiteral("Created")));
        QVERIFY(group);
        QCOMPARE(group->projectionPixelData(3, 3, 1, 1), transparent);
        QScopedPointer<Node> parent(Node::createNode(image, outer));
        QVERIFY(parent->addChildNode(group.data(), nullptr));
        native = outer->lastChild();
    }
    image->waitForDone();
    QVERIFY(group);
    QCOMPARE(group->type(), QStringLiteral("grouplayer"));
    QCOMPARE(native->parent(), parent);
    QCOMPARE(parent->childCount(), childCount + 1);
    QCOMPARE(parent->at(position), native);
    QCOMPARE(native->uuid(), group->uniqueId());
    QVERIFY(!qobject_cast<KisGroupLayer *>(native.data())->passThroughMode());
    QCOMPARE(group->projectionPixelData(3, 3, 1, 1), transparent);
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    QTRY_VERIFY(lifetime->shapeForNode(native));
    manager->slotNonUiActivatedNode(native);
    QTRY_COMPARE(manager->activeNode(), native);
    NodeSP child(api.createNode(QStringLiteral("Child pixels"), QStringLiteral("paintlayer")));
    QVERIFY(child);
    QVERIFY(child->setPixelData(green, 3, 3, 1, 1));
    QVERIFY(group->addChildNode(child.data(), nullptr));
    image->waitForDone();
    KisNodeSP childNative = native->lastChild();
    QVERIFY(childNative);
    QCOMPARE(childNative->uuid(), child->uniqueId());
    QCOMPARE(lifetime->undoStack()->index(), history + 2);
    QTRY_VERIFY(lifetime->shapeForNode(childNative));
    manager->slotNonUiActivatedNode(childNative);
    QTRY_COMPARE(manager->activeNode(), childNative);
    api.refreshProjection();
    QCOMPARE(group->projectionPixelData(3, 3, 1, 1), green);
    QCOMPARE(api.pixelData(3, 3, 1, 1), green);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(native->childCount(), 0U);
    QCOMPARE(group->projectionPixelData(3, 3, 1, 1), transparent);
    QCOMPARE(child->pixelData(3, 3, 1, 1), green);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(parent->childCount(), childCount);
    QVERIFY(!native->parent());
    QCOMPARE(group->projectionPixelData(3, 3, 1, 1), transparent);
    QTRY_VERIFY(manager->activeNode() != native && manager->activeNode() != childNative);
    QVERIFY(!manager->selectedNodes().contains(native));
    QVERIFY(!manager->selectedNodes().contains(childNative));
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(parent->at(position), native);
    QCOMPARE(native->childCount(), 0U);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(native->firstChild(), childNative);
    QTRY_COMPARE(manager->activeNode(), childNative);
    QCOMPARE(group->projectionPixelData(3, 3, 1, 1), green);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Group creation artifacts retained at" << directory.path();
        }
    });
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("created-group.kra"));
    QVERIFY(api.saveAs(path));
    api.waitForDone();
    QScopedPointer<Document> reopened(Krita::instance()->openDocument(path));
    QVERIFY(reopened);
    QScopedPointer<Node> loadedGroup(reopened->nodeByUniqueID(group->uniqueId()));
    QScopedPointer<Node> loadedChild(reopened->nodeByUniqueID(child->uniqueId()));
    QVERIFY(loadedGroup && loadedChild);
    QCOMPARE(loadedGroup->type(), QStringLiteral("grouplayer"));
    QCOMPARE(loadedGroup->projectionPixelData(3, 3, 1, 1), green);
    QScopedPointer<Node> loadedParent(loadedChild->parentNode());
    QVERIFY(loadedParent);
    QCOMPARE(loadedParent->uniqueId(), group->uniqueId());
    QVERIFY(reopened->close());
}

void TestNode::testGpuLayerSelectionKeepsProjectionNotificationsForPaint()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage input;
    input.size = {32, 16};
    input.layers = {{QStringLiteral("Bottom"), true, {}, {}}, {QStringLiteral("Top"), true, {}, {}}};
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Selection notifications"));
    const auto bottom = owner->operations().snapshot()->layers.first().id;
    const auto top = owner->operations().snapshot()->layers.last().id;
    QSignalSpy updates(image.data(), &KisImage::sigImageUpdated);
    QSignalSpy selections(image.data(), &KisImage::sigRequestNodeReselection);
    QCOMPARE(owner->selectLayer(top), Gpu::Result::Success);
    QCOMPARE(selections.size(), 1);
    QVERIFY(updates.isEmpty());
    QCOMPARE(owner->selectLayer(top), Gpu::Result::Success);
    QVERIFY(updates.isEmpty());
    const auto stroke = owner->beginStroke(top, {}, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{8, 8}, 1}}), Gpu::Result::Success);
    QVERIFY(!updates.isEmpty());
    updates.clear();
    QCOMPARE(owner->selectLayer(bottom), Gpu::Result::Success);
    QVERIFY(updates.isEmpty());
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    QVERIFY(!updates.isEmpty());
    updates.clear();
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!updates.isEmpty());
    updates.clear();
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(!updates.isEmpty());
}

void TestNode::testPaintLayerCreationInGroupsPreservesPlacementAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("operation");
    for (const bool gpu : {false, true}) {
        for (const auto &operation : {QStringLiteral("expanded"), QStringLiteral("child"),
                                      QStringLiteral("empty"), QStringLiteral("collapsed"),
                                      QStringLiteral("locked"), QStringLiteral("api-group"),
                                      QStringLiteral("api-root"), QStringLiteral("api-group-clone")}) {
            const auto tag = (gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + operation;
            QTest::newRow(qPrintable(tag)) << gpu << operation;
        }
    }
}

void TestNode::testPaintLayerCreationInGroupsPreservesPlacementAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(QString, operation);
    namespace Gpu = LibrePaint::Image::Gpu;
    const bool empty = operation == QLatin1String("empty");
    const bool apiCreation = operation.startsWith(QLatin1String("api-"));
    const auto sourceId = QUuid(QStringLiteral("{af150001-735b-47cf-93c5-7893d04916f0}"));
    const auto innerId = QUuid(QStringLiteral("{af150002-735b-47cf-93c5-7893d04916f0}"));
    const auto outerId = QUuid(QStringLiteral("{af150003-735b-47cf-93c5-7893d04916f0}"));
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterLayer base{QStringLiteral("Base"), true, {}, {}};
        Gpu::RasterLayer source{QStringLiteral("Source"), true, {}, {}};
        source.uuid = sourceId;
        source.parentUuid = innerId;
        Gpu::RasterLayer inner{QStringLiteral("Inner"), true, {}, {}};
        inner.kind = Gpu::LayerKind::Group;
        inner.uuid = innerId;
        inner.parentUuid = outerId;
        Gpu::RasterLayer outer{QStringLiteral("Outer"), true, {}, {}};
        outer.kind = Gpu::LayerKind::Group;
        outer.uuid = outerId;
        Gpu::RasterLayer top{QStringLiteral("Top"), true, {}, {}};
        Gpu::RasterImage input;
        input.size = {32, 16};
        input.layers = {base};
        if (!empty) input.layers.append(source);
        input.layers.append(inner);
        input.layers.append(outer);
        input.layers.append(top);
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Layer placement"));
    } else {
        image = new KisImage(document->createUndoStore(), 32, 16, KoColorSpaceRegistry::instance()->rgb8(), "Layer placement");
        image->addNode(new KisPaintLayer(image, QStringLiteral("Base"), 255));
        KisNodeSP outer = new KisGroupLayer(image, QStringLiteral("Outer"), 255);
        outer->setUuid(outerId);
        image->addNode(outer);
        KisNodeSP inner = new KisGroupLayer(image, QStringLiteral("Inner"), 255);
        inner->setUuid(innerId);
        image->addNode(inner, outer);
        if (!empty) {
            KisNodeSP source = new KisPaintLayer(image, QStringLiteral("Source"), 255);
            source->setUuid(sourceId);
            image->addNode(source, inner);
        }
        image->addNode(new KisPaintLayer(image, QStringLiteral("Top"), 255));
    }
    document->setCurrentImage(image);
    KisNodeSP outer = image->root()->at(1);
    KisNodeSP inner = outer->firstChild();
    KisNodeSP source = inner->firstChild();
    KisNodeSP top = image->root()->lastChild();
    QCOMPARE(outer->uuid(), outerId);
    QCOMPARE(inner->uuid(), innerId);
    if (empty) {
        const auto original = inner->original();
        QScopedPointer<Node> emptyGroup(Node::createNode(image, inner));
        QVERIFY(emptyGroup->remove());
        image->waitForDone();
        QVERIFY(!inner->parent());
        QCOMPARE(inner->original(), original);
        QCOMPARE(emptyGroup->projectionPixelData(3, 3, 1, 1), QByteArray(4, '\0'));
        document->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(inner->parent(), outer);
    }
    outer->setCollapsed(false);
    inner->setCollapsed(operation == QLatin1String("collapsed"));
    inner->setUserLocked(operation == QLatin1String("locked"));
    const auto red = QByteArray::fromHex("0000ffff");
    const auto yellow = QByteArray::fromHex("00ffffff");
    const auto green = QByteArray::fromHex("00ff00ff");
    if (source) {
        QScopedPointer<Node> sourceApi(Node::createNode(image, source));
        QVERIFY(sourceApi->setPixelData(red, 3, 3, 1, 1));
    }
    {
        QScopedPointer<Node> topApi(Node::createNode(image, top));
        QVERIFY(topApi->setPixelData(yellow, 3, 3, 1, 1));
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
    auto *manager = window->viewManager()->nodeManager();
    KisNodeSP active = operation == QLatin1String("child") ? source : inner;
    manager->slotNonUiActivatedNode(active);
    QTRY_COMPARE(manager->activeNode(), active);
    KisNodeSP parent = operation == QLatin1String("api-root") ? image->root()
        : operation == QLatin1String("collapsed") || operation == QLatin1String("locked") ? outer : inner;
    const auto position = parent->childCount();
    const auto history = lifetime->undoStack()->index();
    NodeSP inserted;
    KisNodeSP native;
    Document api(lifetime.data(), false);
    if (apiCreation) {
        if (operation == QLatin1String("api-group-clone")) {
            QScopedPointer<Node> sourceApi(Node::createNode(image, source));
            inserted.reset(sourceApi->clone());
        } else {
            inserted.reset(api.createNode(QStringLiteral("Inserted"), QStringLiteral("paintlayer")));
        }
        QVERIFY(inserted);
        QScopedPointer<Node> parentApi(Node::createNode(image, parent));
        QVERIFY(parentApi->addChildNode(inserted.data(), nullptr));
        native = parent->lastChild();
        QCOMPARE(native->uuid(), inserted->uniqueId());
        QTRY_VERIFY(lifetime->shapeForNode(native));
        manager->slotNonUiActivatedNode(native);
    } else {
        native = manager->createNode(QStringLiteral("KisPaintLayer"), true);
        QVERIFY(native);
        inserted.reset(Node::createNode(image, native));
    }
    image->waitForDone();
    QCOMPARE(native->parent(), parent);
    QCOMPARE(parent->index(native), int(position));
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    QTRY_COMPARE(manager->activeNode(), native);
    QTRY_VERIFY(manager->selectedNodes().contains(native));
    const auto identity = inserted->uniqueId();
    QVERIFY(inserted->setPixelData(green, 3, 3, 1, 1));
    api.refreshProjection();
    QScopedPointer<Node> parentApi(Node::createNode(image, parent));
    QCOMPARE(parentApi->projectionPixelData(3, 3, 1, 1), green);
    if (source) {
        QScopedPointer<Node> sourceApi(Node::createNode(image, source));
        QCOMPARE(sourceApi->pixelData(3, 3, 1, 1), red);
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(parent->childCount(), position);
    QVERIFY(!native->parent());
    QCOMPARE(inserted->pixelData(3, 3, 1, 1), green);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(parent->at(position), native);
    QCOMPARE(inserted->uniqueId(), identity);
    QTRY_VERIFY(lifetime->shapeForNode(native));
    manager->slotNonUiActivatedNode(native);
    QTRY_COMPARE(manager->selectedNodes(), KisNodeList{native});
    manager->removeNode();
    image->waitForDone();
    QCOMPARE(parent->childCount(), position);
    QCOMPARE(lifetime->undoStack()->index(), history + 2);
    QTRY_VERIFY(manager->activeNode() != native);
    QTRY_VERIFY(!manager->selectedNodes().contains(native));
    KisNodeSP selectedAfterRemoval = parent->lastChild() ? parent->lastChild() : parent;
    QTRY_COMPARE(manager->activeNode(), selectedAfterRemoval);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(parent->at(position), native);
    QTRY_COMPARE(manager->activeNode(), native);
    QCOMPARE(inserted->pixelData(3, 3, 1, 1), green);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Group editing artifacts retained at" << directory.path();
        }
    });
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("hierarchy.kra"));
    QVERIFY(api.saveAs(path));
    api.waitForDone();
    QScopedPointer<Document> reopened(Krita::instance()->openDocument(path));
    QVERIFY(reopened);
    QScopedPointer<Node> loaded(reopened->nodeByUniqueID(identity));
    QVERIFY(loaded);
    QCOMPARE(loaded->pixelData(3, 3, 1, 1), green);
    QScopedPointer<Node> loadedParent(loaded->parentNode());
    QVERIFY(loadedParent);
    QCOMPARE(loadedParent->name(), parentApi->name());
    const auto loadedSiblings = loadedParent->childNodes();
    const auto releaseSiblings = qScopeGuard([&] { qDeleteAll(loadedSiblings); });
    QCOMPARE(loadedSiblings.at(position)->uniqueId(), identity);
    loaded.reset();
    QVERIFY(reopened->close());
    if (operation == QLatin1String("expanded")) {
        KisNodeList roots;
        for (auto node = image->root()->firstChild(); node; node = node->nextSibling()) roots.append(node);
        manager->slotSetSelectedNodes(roots);
        QTRY_COMPARE(manager->selectedNodes(), roots);
        manager->removeNode();
        image->waitForDone();
        QCOMPARE(image->root()->childCount(), 1U);
        QVERIFY(image->root()->firstChild()->inherits("KisPaintLayer"));
        QVERIFY(!roots.contains(image->root()->firstChild()));
        QCOMPARE(lifetime->undoStack()->index(), history + 2);
        api.refreshProjection();
        QCOMPARE(api.pixelData(3, 3, 1, 1), QByteArray(4, '\0'));
        lifetime->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(image->root()->childCount(), 3U);
        QCOMPARE(image->root()->at(1), outer);
        QCOMPARE(inner->lastChild(), native);
        QCOMPARE(inserted->pixelData(3, 3, 1, 1), green);
        QTRY_COMPARE(manager->activeNode(), native);
    }
}

void TestNode::testRootGroupDuplicationKeepsSelectionAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("multiple");
    QTest::addColumn<bool>("activeGroup");
    for (const bool gpu : {false, true}) {
        const auto prefix = gpu ? QByteArrayLiteral("gpu-") : QByteArrayLiteral("ordinary-");
        QTest::newRow(QByteArray(prefix + "group").constData()) << gpu << false << true;
        QTest::newRow(QByteArray(prefix + "group-and-paint").constData()) << gpu << true << true;
        QTest::newRow(QByteArray(prefix + "group-and-paint-active-paint").constData()) << gpu << true << false;
    }
}

void TestNode::testRootGroupDuplicationKeepsSelectionAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, multiple);
    QFETCH(bool, activeGroup);
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const QUuid baseId(QStringLiteral("{00000001-1122-3344-5566-778899aabbcc}"));
    const QUuid leafId(QStringLiteral("{00000002-1122-3344-5566-778899aabbcc}"));
    const QUuid innerId(QStringLiteral("{00000003-1122-3344-5566-778899aabbcc}"));
    const QUuid outerId(QStringLiteral("{00000004-1122-3344-5566-778899aabbcc}"));
    const QUuid topId(QStringLiteral("{00000005-1122-3344-5566-778899aabbcc}"));
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterLayer base{QStringLiteral("Base"), true, baseId, {}};
        base.defaultPixel = {255, 0, 0, 255};
        Gpu::RasterLayer leaf{QStringLiteral("Leaf"), true, leafId, {}};
        leaf.parentUuid = innerId;
        QByteArray tile(Gpu::RgbaTile::ByteCount, '\0');
        tile.replace((3 * 256 + 3) * 4, 4, QByteArray::fromHex("00ff00ff"));
        leaf.tiles = {{{0, 0}, tile}};
        Gpu::RasterLayer inner{QStringLiteral("Inner"), true, innerId, {}};
        inner.kind = Gpu::LayerKind::Group;
        inner.parentUuid = outerId;
        Gpu::RasterLayer outer{QStringLiteral("Outer"), true, outerId, {}};
        outer.kind = Gpu::LayerKind::Group;
        Gpu::RasterLayer top{QStringLiteral("Top"), true, topId, {}};
        Gpu::RasterImage input;
        input.size = {32, 16};
        input.layers = {base, leaf, inner, outer, top};
        input.activeLayer = 3;
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Group duplication"));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        image = new KisImage(document->createUndoStore(), 32, 16, cs, "Group duplication");
        auto base = new KisPaintLayer(image, QStringLiteral("Base"), 255);
        base->setUuid(baseId);
        base->paintDevice()->setDefaultPixel(KoColor(Qt::red, cs));
        image->addNode(base);
        auto outer = new KisGroupLayer(image, QStringLiteral("Outer"), 255);
        outer->setUuid(outerId);
        image->addNode(outer);
        auto inner = new KisGroupLayer(image, QStringLiteral("Inner"), 255);
        inner->setUuid(innerId);
        image->addNode(inner, outer);
        auto leaf = new KisPaintLayer(image, QStringLiteral("Leaf"), 255);
        leaf->setUuid(leafId);
        const auto green = QByteArray::fromHex("00ff00ff");
        leaf->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(green.constData()), 3, 3, 1, 1);
        image->addNode(leaf, inner);
        auto top = new KisPaintLayer(image, QStringLiteral("Top"), 255);
        top->setUuid(topId);
        image->addNode(top);
    }
    document->setCurrentImage(image);
    document->setFileBatchMode(true);
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed root duplication"));
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    image->waitForDone();
    const auto outer = image->root()->at(1);
    const auto top = image->root()->at(2);
    KisNodeList originalRoots;
    for (auto node = image->root()->firstChild(); node; node = node->nextSibling()) originalRoots.append(node);
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
    QTRY_VERIFY(lifetime->shapeForNode(outer));
    QTRY_VERIFY(lifetime->shapeForNode(top));
    const KisNodeSP sourceActive = activeGroup ? outer : top;
    manager->slotNonUiActivatedNode(sourceActive);
    QTRY_COMPARE(manager->activeNode(), sourceActive);
    const KisNodeList selected = multiple ? KisNodeList{outer, top} : KisNodeList{outer};
    manager->slotSetSelectedNodes(selected);
    QTRY_COMPARE(manager->selectedNodes(), selected);
    const bool previousRename = KisImageConfig(true).renameDuplicatedLayers();
    const auto restoreRename = qScopeGuard([&] { KisImageConfig(false).setRenameDuplicatedLayers(previousRename); });
    { KisImageConfig configuration(false); configuration.setRenameDuplicatedLayers(false); }
    const int history = lifetime->undoStack()->index();
    manager->duplicateActiveNode();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), unsigned(originalRoots.size() + selected.size()));
    KisNodeList copies;
    KisNodeSP copiedGroup;
    KisNodeSP copiedPaint;
    for (auto node = image->root()->firstChild(); node; node = node->nextSibling()) {
        if (originalRoots.contains(node)) continue;
        copies.append(node);
        if (node->inherits("KisGroupLayer")) copiedGroup = node;
        else copiedPaint = node;
    }
    QCOMPARE(copies.size(), selected.size());
    QVERIFY(copiedGroup);
    QVERIFY(copiedGroup->uuid() != outer->uuid());
    QCOMPARE(copiedGroup->childCount(), 1U);
    QVERIFY(copiedGroup->firstChild()->uuid() != innerId);
    QVERIFY(copiedGroup->firstChild()->firstChild()->uuid() != leafId);
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    QTRY_COMPARE(manager->activeNode(), activeGroup ? copiedGroup : copiedPaint);
    const auto copiesSelected = [&] {
        const auto selected = manager->selectedNodes();
        if (selected.size() != copies.size()) return false;
        for (const auto &node : selected) if (!copies.contains(node)) return false;
        return true;
    };
    QTRY_VERIFY(copiesSelected());
    QScopedPointer<Node> copiedApi(Node::createNode(image, copiedGroup));
    QCOMPARE(copiedApi->projectionPixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            output.setAutoRemove(false);
            qInfo() << "Root duplication artifacts retained at" << output.path();
        }
    });
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = gpu && !results.isEmpty() ? results : output.path();
    QVERIFY(QDir().mkpath(destination));
    const auto stem = QStringLiteral("root-duplicate-%1%2").arg(multiple ? QStringLiteral("group-and-paint") : QStringLiteral("group"),
        activeGroup ? QString() : QStringLiteral("-active-paint"));
    const auto path = QDir(destination).filePath(stem + QStringLiteral(".kra"));
    const auto saveTime = QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate);
    KisPropertiesConfigurationSP saveConfiguration(new KisPropertiesConfiguration);
    saveConfiguration->setProperty("documentSaveTime", saveTime);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false, saveConfiguration), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY(reopened->openPath(path, KisDocument::DontAddToRecent));
    if (gpu) QCOMPARE(reopened->documentInfo()->aboutInfo("date"), saveTime.toString(Qt::ISODate));
    Document loaded(reopened.data(), false);
    QScopedPointer<Node> loadedGroup(loaded.nodeByUniqueID(copiedGroup->uuid()));
    QScopedPointer<Node> loadedLeaf(loaded.nodeByUniqueID(copiedGroup->firstChild()->firstChild()->uuid()));
    QVERIFY(loadedGroup && loadedLeaf);
    QCOMPARE(loadedGroup->projectionPixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(loadedLeaf->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    QScopedPointer<Node> loadedInner(loadedLeaf->parentNode());
    QVERIFY(loadedInner);
    QCOMPARE(loadedInner->uniqueId(), copiedGroup->firstChild()->uuid());
    QScopedPointer<Node> loadedOuter(loadedInner->parentNode());
    QVERIFY(loadedOuter);
    QCOMPARE(loadedOuter->uniqueId(), copiedGroup->uuid());
    for (const auto &node : copies) {
        QScopedPointer<Node> retained(loaded.nodeByUniqueID(node->uuid()));
        QVERIFY(retained);
    }
    Document api(lifetime.data(), false);
    const auto pixels = api.pixelData(0, 0, 32, 16);
    InfoObject pngConfiguration;
    pngConfiguration.setProperty("alpha", true);
    pngConfiguration.setProperty("interlaced", false);
    pngConfiguration.setProperty("compression", 3);
    pngConfiguration.setProperty("saveSRGBProfile", true);
    pngConfiguration.setProperty("forceSRGB", false);
    pngConfiguration.setProperty("storeAuthor", false);
    pngConfiguration.setProperty("storeMetaData", false);
    const auto pngPath = QDir(destination).filePath(stem + QStringLiteral(".png"));
    QVERIFY2(api.exportImage(pngPath, pngConfiguration), qPrintable(lifetime->errorMessage()));
    const auto png = QImage(pngPath).convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(png.size(), QSize(32, 16));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), pixels);
    if (gpu && !results.isEmpty()) {
        QFile recovery(QDir(destination).filePath(stem + QStringLiteral(".recovery")));
        QVERIFY(recovery.open(QIODevice::WriteOnly));
        const auto bytes = image->gpuImage()->operations().recoveryData();
        QCOMPARE(recovery.write(bytes), qint64(bytes.size()));
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), unsigned(originalRoots.size()));
    for (int i = 0; i < originalRoots.size(); ++i) QCOMPARE(image->root()->at(i), originalRoots[i]);
    lifetime->undoStack()->redo();
    image->waitForDone();
    for (const auto &node : copies) QCOMPARE(node->parent(), image->root());
    QTRY_COMPARE(manager->activeNode(), activeGroup ? copiedGroup : copiedPaint);
    QTRY_VERIFY(copiesSelected());
    manager->removeNode();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), unsigned(originalRoots.size()));
    QCOMPARE(lifetime->undoStack()->index(), history + 2);
    lifetime->undoStack()->undo();
    image->waitForDone();
    for (const auto &node : copies) QCOMPARE(node->parent(), image->root());
    QCOMPARE(copiedApi->projectionPixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
}

void TestNode::testHierarchyDuplicationPreservesPlacementPixelsHistoryAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<QString>("layout");
    for (const auto &scenario : {QStringLiteral("nested-paint"), QStringLiteral("nested-group"),
                                QStringLiteral("cross-parent"), QStringLiteral("ancestor-and-child"),
                                QStringLiteral("locked-parent"), QStringLiteral("locked-ancestor")}) {
        QTest::newRow(qPrintable(QStringLiteral("ordinary-") + scenario)) << false << scenario << QStringLiteral("child-first");
        for (const auto &layout : {QStringLiteral("child-first"), QStringLiteral("parent-first"), QStringLiteral("interleaved")})
            QTest::newRow(qPrintable(QStringLiteral("gpu-") + scenario + QLatin1Char('-') + layout)) << true << scenario << layout;
    }
}

void TestNode::testHierarchyDuplicationPreservesPlacementPixelsHistoryAndSaving()
{
    QFETCH(QString, scenario);
    QFETCH(bool, gpu);
    QFETCH(QString, layout);
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const QUuid baseId(QStringLiteral("{00000001-1122-3344-5566-778899aabbcc}"));
    const QUuid leafId(QStringLiteral("{00000002-1122-3344-5566-778899aabbcc}"));
    const QUuid innerId(QStringLiteral("{00000003-1122-3344-5566-778899aabbcc}"));
    const QUuid outerId(QStringLiteral("{00000004-1122-3344-5566-778899aabbcc}"));
    const QUuid topId(QStringLiteral("{00000005-1122-3344-5566-778899aabbcc}"));
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterLayer base{QStringLiteral("Base"), true, baseId, {}};
        base.defaultPixel = {255, 0, 0, 255};
        Gpu::RasterLayer leaf{QStringLiteral("Leaf"), true, leafId, {}};
        leaf.parentUuid = innerId;
        QByteArray tile(Gpu::RgbaTile::ByteCount, '\0');
        tile.replace((3 * 256 + 3) * 4, 4, QByteArray::fromHex("00ff00ff"));
        leaf.tiles = {{{0, 0}, tile}};
        Gpu::RasterLayer inner{QStringLiteral("Inner"), true, innerId, {}};
        inner.kind = Gpu::LayerKind::Group;
        inner.parentUuid = outerId;
        Gpu::RasterLayer outer{QStringLiteral("Outer"), true, outerId, {}};
        outer.kind = Gpu::LayerKind::Group;
        Gpu::RasterLayer top{QStringLiteral("Top"), true, topId, {}};
        Gpu::RasterImage input;
        input.size = {32, 16};
        input.layers = layout == QLatin1String("child-first") ? QList<Gpu::RasterLayer>{base, leaf, inner, outer, top}
            : layout == QLatin1String("parent-first") ? QList<Gpu::RasterLayer>{base, outer, inner, leaf, top}
            : QList<Gpu::RasterLayer>{base, outer, top, leaf, inner};
        input.activeLayer = layout == QLatin1String("child-first") ? 3 : 1;
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Group duplication"));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        image = new KisImage(document->createUndoStore(), 32, 16, cs, "Group duplication");
        auto base = new KisPaintLayer(image, QStringLiteral("Base"), 255);
        base->setUuid(baseId);
        base->paintDevice()->setDefaultPixel(KoColor(Qt::red, cs));
        image->addNode(base);
        auto outer = new KisGroupLayer(image, QStringLiteral("Outer"), 255);
        outer->setUuid(outerId);
        image->addNode(outer);
        auto inner = new KisGroupLayer(image, QStringLiteral("Inner"), 255);
        inner->setUuid(innerId);
        image->addNode(inner, outer);
        auto leaf = new KisPaintLayer(image, QStringLiteral("Leaf"), 255);
        leaf->setUuid(leafId);
        const auto green = QByteArray::fromHex("00ff00ff");
        leaf->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(green.constData()), 3, 3, 1, 1);
        image->addNode(leaf, inner);
        auto top = new KisPaintLayer(image, QStringLiteral("Top"), 255);
        top->setUuid(topId);
        image->addNode(top);
    }
    document->setCurrentImage(image);
    document->setFileBatchMode(true);
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed hierarchy duplication"));
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    image->waitForDone();
    KisNodeSP outer = image->root()->at(1);
    const auto top = image->root()->at(2);
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
    QTRY_VERIFY(lifetime->shapeForNode(outer));
    QTRY_VERIFY(lifetime->shapeForNode(top));

    KisNodeSP inner = outer->firstChild();
    const auto leaf = inner->firstChild();
    KisNodeList selected{leaf};
    KisNodeSP sourceActive = leaf;
    KisNodeSP destinationParent = inner;
    KisNodeSP anchor = leaf;
    QStringList copiedNames{QStringLiteral("Leaf")};
    if (scenario == QLatin1String("nested-group")) {
        selected = {inner}; sourceActive = inner; destinationParent = outer; anchor = inner;
        copiedNames = {QStringLiteral("Inner")};
    } else if (scenario == QLatin1String("cross-parent")) {
        selected = {leaf, top}; destinationParent = image->root(); anchor = top;
        copiedNames = {QStringLiteral("Leaf"), QStringLiteral("Top")};
    } else if (scenario == QLatin1String("ancestor-and-child")) {
        selected = {outer, leaf}; destinationParent = image->root(); anchor = outer;
        copiedNames = {QStringLiteral("Outer")};
    } else if (scenario == QLatin1String("locked-parent")) {
        inner->setUserLocked(true); destinationParent = outer; anchor = inner;
    } else if (scenario == QLatin1String("locked-ancestor")) {
        outer->setUserLocked(true); destinationParent = image->root(); anchor = outer;
    }
    manager->slotNonUiActivatedNode(sourceActive);
    QTRY_COMPARE(manager->activeNode(), sourceActive);
    manager->slotSetSelectedNodes(selected);
    QTRY_VERIFY([&] {
        const auto actual = manager->selectedNodes();
        if (actual.size() != selected.size()) return false;
        for (const auto &node : actual) if (!selected.contains(node)) return false;
        return true;
    }());
    const auto oldCount = destinationParent->childCount();
    const int insertion = destinationParent->index(anchor) + 1;
    const bool previousRename = KisImageConfig(true).renameDuplicatedLayers();
    const auto restoreRename = qScopeGuard([&] { KisImageConfig(false).setRenameDuplicatedLayers(previousRename); });
    { KisImageConfig configuration(false); configuration.setRenameDuplicatedLayers(false); }
    const auto history = lifetime->undoStack()->index();
    manager->duplicateActiveNode();
    image->waitForDone();
    QCOMPARE(destinationParent->childCount(), oldCount + unsigned(copiedNames.size()));
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    KisNodeList copies;
    for (int i = 0; i < copiedNames.size(); ++i) {
        const auto copy = destinationParent->at(insertion + i);
        QVERIFY(copy);
        QCOMPARE(copy->inherits("KisGroupLayer"), copiedNames[i] == QLatin1String("Inner") || copiedNames[i] == QLatin1String("Outer"));
        QVERIFY(!selected.contains(copy));
        QVERIFY(copy->uuid() != leafId && copy->uuid() != innerId && copy->uuid() != outerId && copy->uuid() != topId);
        copies.append(copy);
    }
    const auto copiesSelected = [&] {
        const auto selected = manager->selectedNodes();
        if (selected.size() != copies.size()) return false;
        for (const auto &node : selected) if (!copies.contains(node)) return false;
        return true;
    };
    QTRY_VERIFY(copiesSelected());
    QTRY_COMPARE(manager->activeNode(), copies.first());
    QScopedPointer<Node> copiedApi(Node::createNode(image, copies.first()));
    QCOMPARE(copiedApi->projectionPixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    KisNodeSP copiedLeaf = copies.first();
    while (copiedLeaf->firstChild()) copiedLeaf = copiedLeaf->firstChild();
    QScopedPointer<Node> copiedLeafApi(Node::createNode(image, copiedLeaf));
    QScopedPointer<Node> sourceLeafApi(Node::createNode(image, leaf));
    const auto blue = QByteArray::fromHex("ff0000ff");
    QVERIFY(copiedLeafApi->setPixelData(blue, 3, 3, 1, 1));
    Document api(lifetime.data(), false);
    api.refreshProjection();
    QCOMPARE(sourceLeafApi->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(copiedApi->projectionPixelData(3, 3, 1, 1), blue);
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            output.setAutoRemove(false);
            qInfo() << "Hierarchy duplication artifacts retained at" << output.path();
        }
    });
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = gpu && !results.isEmpty() ? results : output.path();
    QVERIFY(QDir().mkpath(destination));
    const auto stem = QStringLiteral("hierarchy-duplicate-%1-%2").arg(scenario, layout);
    const auto kraPath = QDir(destination).filePath(stem + QStringLiteral(".kra"));
    KisPropertiesConfigurationSP saveConfiguration(new KisPropertiesConfiguration);
    saveConfiguration->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    QVERIFY2(lifetime->saveAs(kraPath, KisDocument::nativeFormatMimeType(), false, saveConfiguration), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(kraPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    Document loaded(reopened.data(), false);
    QScopedPointer<Node> loadedLeaf(loaded.nodeByUniqueID(copiedLeaf->uuid()));
    QScopedPointer<Node> loadedSource(loaded.nodeByUniqueID(leafId));
    QVERIFY(loadedLeaf && loadedSource);
    QCOMPARE(loadedLeaf->pixelData(3, 3, 1, 1), blue);
    QCOMPARE(loadedSource->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    for (const auto &node : copies) {
        QScopedPointer<Node> loadedCopy(loaded.nodeByUniqueID(node->uuid()));
        QVERIFY(loadedCopy);
        QScopedPointer<Node> loadedParent(loadedCopy->parentNode());
        QVERIFY(loadedParent);
        if (destinationParent == image->root()) {
            QScopedPointer<Node> grandparent(loadedParent->parentNode());
            QVERIFY(!grandparent);
        } else {
            QCOMPARE(loadedParent->uniqueId(), destinationParent->uuid());
        }
    }
    const auto pixels = api.pixelData(0, 0, 32, 16);
    QCOMPARE(loaded.pixelData(0, 0, 32, 16), pixels);
    InfoObject pngConfiguration;
    pngConfiguration.setProperty("alpha", true);
    pngConfiguration.setProperty("interlaced", false);
    pngConfiguration.setProperty("compression", 3);
    pngConfiguration.setProperty("saveSRGBProfile", true);
    pngConfiguration.setProperty("forceSRGB", false);
    pngConfiguration.setProperty("storeAuthor", false);
    pngConfiguration.setProperty("storeMetaData", false);
    const auto pngPath = QDir(destination).filePath(stem + QStringLiteral(".png"));
    QVERIFY2(api.exportImage(pngPath, pngConfiguration), qPrintable(lifetime->errorMessage()));
    const auto png = QImage(pngPath).convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(png.size(), QSize(32, 16));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), pixels);
    if (gpu && !results.isEmpty()) {
        QFile recovery(QDir(destination).filePath(stem + QStringLiteral(".recovery")));
        QVERIFY(recovery.open(QIODevice::WriteOnly));
        const auto bytes = image->gpuImage()->operations().recoveryData();
        QCOMPARE(recovery.write(bytes), qint64(bytes.size()));
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(destinationParent->childCount(), oldCount);
    lifetime->undoStack()->redo();
    image->waitForDone();
    for (int i = 0; i < copies.size(); ++i) QCOMPARE(destinationParent->at(insertion + i), copies[i]);
    QTRY_COMPARE(manager->activeNode(), copies.first());
    QTRY_VERIFY(copiesSelected());
    QCOMPARE(copiedLeafApi->pixelData(3, 3, 1, 1), blue);
    QCOMPARE(sourceLeafApi->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    manager->removeNode();
    image->waitForDone();
    QCOMPARE(destinationParent->childCount(), oldCount);
    QCOMPARE(lifetime->undoStack()->index(), history + 2);
    lifetime->undoStack()->undo();
    image->waitForDone();
    for (int i = 0; i < copies.size(); ++i) QCOMPARE(destinationParent->at(insertion + i), copies[i]);
    QCOMPARE(copiedLeafApi->pixelData(3, 3, 1, 1), blue);
}
