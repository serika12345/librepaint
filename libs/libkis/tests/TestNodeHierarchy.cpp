/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestNode.h"

#include <simpletest.h>
#include <Document.h>
#include <GroupLayer.h>
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
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kundo2stack.h>
#include <nodes/kis_node_manager.h>
#include <QApplication>
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
