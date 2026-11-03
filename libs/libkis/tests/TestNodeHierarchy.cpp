/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestNode.h"
#include "TestNodeMimeCopyInputs.h"

#include <simpletest.h>
#include <Document.h>
#include <GroupLayer.h>
#include <InfoObject.h>
#include <Krita.h>
#include <Node.h>
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoProperties.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <flake/kis_shape_controller.h>
#include <kis_mimedata.h>
#include <nodes/kis_node_insertion_adapter.h>
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
#include <QImage>
#include <QMap>
#include <QPointer>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <algorithm>
#include <memory>
#include <exception>

void TestNode::testPublicSubtreeInsertionKeepsChildrenPixelsAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("passThrough");
    for (bool gpu : {false, true}) for (bool passThrough : {false, true}) {
        const QByteArray name = QByteArray(gpu ? "gpu-" : "ordinary-") + (passThrough ? "pass-through" : "isolated");
        QTest::newRow(name.constData()) << gpu << passThrough;
    }
}

void TestNode::testPublicSubtreeInsertionKeepsChildrenPixelsAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, passThrough);
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig(true).disableOpenGL();
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const QUuid groupUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage input;
        input.size = {4, 4};
        input.layers = {{QStringLiteral("First"), true, {}, {}},
                        {QStringLiteral("Second"), true, {}, {}},
                        {QStringLiteral("Group"), true, groupUuid, {}}};
        input.layers[0].parentUuid = input.layers[1].parentUuid = groupUuid;
        input.layers[2].kind = Gpu::LayerKind::Group;
        input.layers[2].passThrough = passThrough;
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Subtree insertion"));
    } else {
        image = new KisImage(document->createUndoStore(), 4, 4, KoColorSpaceRegistry::instance()->rgb8(), "Subtree insertion");
        KisGroupLayerSP group = new KisGroupLayer(image, QStringLiteral("Group"), 255);
        group->setUuid(groupUuid);
        group->setPassThroughMode(passThrough);
        image->addNode(group);
        image->addNode(new KisPaintLayer(image, QStringLiteral("First"), 255), group);
        image->addNode(new KisPaintLayer(image, QStringLiteral("Second"), 255), group);
    }
    document->setCurrentImage(image);
    Document api(document.data(), false);
    QScopedPointer<Node> original(api.nodeByUniqueID(groupUuid));
    QVERIFY(original);
    auto sourceChildren = original->childNodes();
    const auto releaseSource = qScopeGuard([&] { qDeleteAll(sourceChildren); });
    QCOMPARE(sourceChildren.size(), 2);
    const auto red = QByteArray::fromHex("0000ffff");
    const auto blue = QByteArray::fromHex("ff000080");
    const auto green = QByteArray::fromHex("00ff00ff");
    const auto yellow = QByteArray::fromHex("00ffffff");
    QVERIFY(sourceChildren[0]->setPixelData(red, 1, 1, 1, 1));
    QVERIFY(sourceChildren[1]->setPixelData(blue, 2, 2, 1, 1));
    image->waitForDone();
    QScopedPointer<Node> copied(original->clone());
    QVERIFY(copied);
    const auto identity = copied->uniqueId();
    QVERIFY(identity != original->uniqueId());
    QScopedPointer<Node> root(api.rootNode());
    const auto history = document->undoStack()->index();
    if (gpu) {
        const auto owner = image->gpuImage();
        const auto stroke = owner->beginStroke(owner->operations().snapshot()->layers.first().id, Gpu::Brush{}, 43);
        QVERIFY(!stroke.isNull());
        QCOMPARE(owner->appendSamples(stroke, {{{3, 3}}}), Gpu::Result::Success);
        const auto pending = owner->operations().preview();
        QVERIFY(!root->addChildNode(copied.data(), nullptr));
        QCOMPARE(owner->operations().preview(), pending);
        QCOMPARE(image->root()->childCount(), 1u);
        QCOMPARE(document->undoStack()->index(), history);
        QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
    }
    QVERIFY(root->addChildNode(copied.data(), nullptr));
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 2u);
    QCOMPARE(document->undoStack()->index(), history + 1);
    QCOMPARE(image->root()->lastChild()->uuid(), identity);
    const auto *group = dynamic_cast<GroupLayer *>(copied.data());
    QVERIFY(group);
    QCOMPARE(group->passThroughMode(), passThrough);
    auto children = copied->childNodes();
    const auto releaseChildren = qScopeGuard([&] { qDeleteAll(children); });
    QCOMPARE(children.size(), 2);
    QCOMPARE(children[0]->name(), QStringLiteral("First"));
    QCOMPARE(children[1]->name(), QStringLiteral("Second"));
    QCOMPARE(children[0]->pixelData(1, 1, 1, 1), red);
    QCOMPARE(children[1]->pixelData(2, 2, 1, 1), blue);
    QVERIFY(sourceChildren[0]->setPixelData(green, 1, 1, 1, 1));
    QCOMPARE(children[0]->pixelData(1, 1, 1, 1), red);
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(image->root()->childCount(), 1u);
    QCOMPARE(document->undoStack()->index(), history);
    QCOMPARE(sourceChildren[0]->pixelData(1, 1, 1, 1), green);
    QVERIFY(children[1]->setPixelData(yellow, 2, 2, 1, 1));
    document->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(image->root()->lastChild()->uuid(), identity);
    QCOMPARE(document->undoStack()->index(), history + 1);
    QCOMPARE(children[0]->pixelData(1, 1, 1, 1), red);
    QCOMPARE(children[1]->pixelData(2, 2, 1, 1), yellow);
}

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
    QTest::addColumn<QString>("entry");
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<QString>("layout");
    for (const auto &entry : {QStringLiteral("menu"), QStringLiteral("mime"), QStringLiteral("fixed-mime")}) {
        for (const auto &scenario : {QStringLiteral("nested-paint"), QStringLiteral("nested-group"),
                                    QStringLiteral("cross-parent"), QStringLiteral("ancestor-and-child"),
                                    QStringLiteral("locked-parent"), QStringLiteral("locked-ancestor"),
                                    QStringLiteral("independent-active"), QStringLiteral("wide-group")}) {
            if ((scenario == QLatin1String("independent-active") || scenario == QLatin1String("wide-group")) &&
                entry == QLatin1String("menu")) continue;
            if (entry != QLatin1String("fixed-mime") && scenario != QLatin1String("wide-group")) {
                QTest::newRow(qPrintable(QStringLiteral("ordinary-") + entry + QLatin1Char('-') + scenario))
                    << false << entry << scenario << QStringLiteral("child-first");
            }
            for (const auto &layout : {QStringLiteral("child-first"), QStringLiteral("parent-first"), QStringLiteral("interleaved")})
                QTest::newRow(qPrintable(QStringLiteral("gpu-") + entry + QLatin1Char('-') + scenario + QLatin1Char('-') + layout))
                    << true << entry << scenario << layout;
        }
    }
}

void TestNode::testHierarchyDuplicationPreservesPlacementPixelsHistoryAndSaving()
try {
    QFETCH(QString, scenario);
    QFETCH(QString, entry);
    QFETCH(bool, gpu);
    QFETCH(QString, layout);
    namespace Gpu = LibrePaint::Image::Gpu;
    // Fix the option before image workers acquire their thread-local configuration.
    const bool previousRename = KisImageConfig(true).renameDuplicatedLayers();
    const auto restoreRename = qScopeGuard([&] { KisImageConfig(false).setRenameDuplicatedLayers(previousRename); });
    { KisImageConfig configuration(false); configuration.setRenameDuplicatedLayers(false); }
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
        if (scenario == QLatin1String("wide-group")) {
            for (int i = 0; i < 16; ++i) {
                Gpu::RasterLayer extra{QStringLiteral("Extra %1").arg(i), true,
                    QUuid::createUuidV5(innerId, QByteArray::number(i)), {}};
                extra.parentUuid = innerId;
                input.layers.append(extra);
            }
        }
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
        copiedNames = entry != QLatin1String("menu") ? QStringList{QStringLiteral("Outer"), QStringLiteral("Leaf")}
                                                    : QStringList{QStringLiteral("Outer")};
    } else if (scenario == QLatin1String("locked-parent")) {
        inner->setUserLocked(true); destinationParent = outer; anchor = inner;
    } else if (scenario == QLatin1String("locked-ancestor")) {
        outer->setUserLocked(true); destinationParent = image->root(); anchor = outer;
    }
    if (scenario == QLatin1String("wide-group")) {
        selected = {outer}; sourceActive = outer; destinationParent = image->root(); anchor = outer;
        copiedNames = {QStringLiteral("Outer")};
        outer->setCollapsed(true);
        outer->setNodeProperty(QStringLiteral("fixture-copy-property"), 42);
    }
    if (scenario == QLatin1String("independent-active")) sourceActive = top;
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
    const auto history = lifetime->undoStack()->index();
    if (entry != QLatin1String("menu")) {
        auto *controller = dynamic_cast<KisShapeController *>(lifetime->shapeController());
        QVERIFY(controller);
        QScopedPointer<QMimeData> mime(KisMimeData::mimeForLayers(selected, image, false));
        QVERIFY(mime);
        FixedMimeCopyInputs fixed(manager->nodeInsertionAdapter());
        auto *insertionOwner = entry == QLatin1String("fixed-mime")
            ? static_cast<KisMimeData::NodeInsertionInterface *>(&fixed) : manager->nodeInsertionAdapter();
        QVERIFY(KisMimeData::insertMimeLayers(mime.data(), image, controller,
            controller->dummyForNode(destinationParent), controller->dummyForNode(anchor), true,
            insertionOwner));
    } else {
        manager->duplicateActiveNode();
    }
    image->waitForDone();
    QCOMPARE(destinationParent->childCount(), oldCount + unsigned(copiedNames.size()));
    QCOMPARE(lifetime->undoStack()->index(), history + 1);
    KisNodeList copies;
    for (int i = 0; i < copiedNames.size(); ++i) {
        const auto copy = destinationParent->at(insertion + i);
        QVERIFY(copy);
        QCOMPARE(copy->name(), copiedNames[i]);
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
    if (scenario == QLatin1String("wide-group")) {
        QCOMPARE(copies.first()->firstChild()->childCount(), 17u);
        QVERIFY(copies.first()->collapsed());
        QCOMPARE(copies.first()->nodeProperties().intProperty(QStringLiteral("fixture-copy-property")), 42);
    }
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
    if (scenario == QLatin1String("ancestor-and-child") && entry != QLatin1String("menu")) {
        QScopedPointer<Node> independent(Node::createNode(image, copies[1]));
        QCOMPARE(independent->pixelData(3, 3, 1, 1), QByteArray::fromHex("00ff00ff"));
    }
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
    const auto stem = (entry == QLatin1String("fixed-mime") ? QStringLiteral("hierarchy-mime-fixed-%1-%2")
        : entry == QLatin1String("mime") ? QStringLiteral("hierarchy-mime-copy-%1-%2")
                                         : QStringLiteral("hierarchy-duplicate-%1-%2")).arg(scenario, layout);
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
    lifetime->undoStack()->undo();
    QTRY_COMPARE(lifetime->undoStack()->index(), history);
    image->waitForDone();
    QCOMPARE(destinationParent->childCount(), oldCount);
    if (gpu) {
        QTRY_COMPARE(manager->activeNode(), sourceActive);
        QTRY_VERIFY([&] {
            const auto restored = manager->selectedNodes();
            if (restored.size() != selected.size()) return false;
            for (const auto &node : restored) if (!selected.contains(node)) return false;
            return true;
        }());
    }
    lifetime->undoStack()->redo();
    QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
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
catch (const std::exception &error) { QFAIL(error.what()); }

void TestNode::testHierarchyOrderingPreservesPlacementPixelsHistoryAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<int>("scenario");
    QTest::addColumn<QString>("layout");
    QTest::addColumn<QString>("entry");
    const QStringList cases{
        QStringLiteral("group-down"), QStringLiteral("enter-up"), QStringLiteral("enter-down"),
        QStringLiteral("collapsed-up"), QStringLiteral("collapsed-down"), QStringLiteral("leaf-up"),
        QStringLiteral("leaf-down"), QStringLiteral("inner-up"), QStringLiteral("inner-down"),
        QStringLiteral("ancestor-and-child"), QStringLiteral("cross-parent"), QStringLiteral("locked-source"),
        QStringLiteral("locked-destination"), QStringLiteral("boundary"), QStringLiteral("noncontiguous-boundary"),
        QStringLiteral("locked-parent")};
    for (int i = 0; i < cases.size(); ++i) {
        for (const auto &entry : {QStringLiteral("buttons"), QStringLiteral("direct")}) {
            if (entry == QLatin1String("direct") && i >= 13) continue;
            const auto name = (entry == QLatin1String("direct") ? QStringLiteral("direct-") : QString()) + cases[i];
            QTest::newRow(qPrintable(QStringLiteral("ordinary-") + name)) << false << i << QStringLiteral("child-first") << entry;
            for (const auto &layout : {QStringLiteral("child-first"), QStringLiteral("parent-first"), QStringLiteral("interleaved")})
                QTest::newRow(qPrintable(QStringLiteral("gpu-") + name + QLatin1Char('-') + layout)) << true << i << layout << entry;
        }
    }
}

void TestNode::testHierarchyOrderingPreservesPlacementPixelsHistoryAndSaving()
try {
    QFETCH(bool, gpu);
    QFETCH(int, scenario);
    QFETCH(QString, layout);
    QFETCH(QString, entry);
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
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, input), QStringLiteral("Hierarchy ordering"));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        image = new KisImage(document->createUndoStore(), 32, 16, cs, "Hierarchy ordering");
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
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed hierarchy ordering"));
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



    const KisNodeSP base = image->root()->at(0);
    KisNodeSP inner = outer->firstChild();
    const KisNodeSP leaf = inner->firstChild();
    const bool lower = scenario == 0 || scenario == 2 || scenario == 4 || scenario == 6 || scenario == 8 || scenario == 11 || scenario == 13;
    const bool changes = scenario < 13;
    const KisNodeSP source = (scenario == 0 || scenario == 11) ? outer :
        (scenario == 2 || scenario == 4 || scenario == 14) ? top :
        (scenario == 5 || scenario == 6 || scenario == 9 || scenario == 10 || scenario == 15) ? leaf :
        (scenario == 7 || scenario == 8) ? inner : base;
    if (scenario == 3 || scenario == 4) outer->setCollapsed(true);
    if (scenario == 11 || scenario == 12) outer->setUserLocked(true);
    if (scenario == 15) inner->setUserLocked(true);
    const KisNodeList selected = scenario == 9 ? KisNodeList{inner, leaf} :
        scenario == 10 ? KisNodeList{leaf, base} : scenario == 14 ? KisNodeList{top, base} : KisNodeList{source};
    const KisNodeList expectedSelection = scenario == 9 ? KisNodeList{inner} : selected;
    const KisNodeSP expectedActive = entry == QLatin1String("direct") && scenario == 9 ? inner : source;
    manager->slotNonUiActivatedNode(source);
    QTRY_COMPARE(manager->activeNode(), source);
    manager->slotSetSelectedNodes(selected);
    const auto selectedAsExpected = [&] {
        const auto actual = manager->selectedNodes();
        if (actual.size() != expectedSelection.size()) return false;
        for (const auto &node : actual) if (!expectedSelection.contains(node)) return false;
        if (gpu) {
            const auto version = image->gpuImage()->operations().snapshot();
            const auto active = manager->activeNode();
            if (!active || version->selectedLayers.size() != expectedSelection.size()) return false;
            for (const auto &layer : version->layers) {
                if (layer.id == version->activeLayer && layer.uuid != active->uuid()) return false;
                if (version->selectedLayers.contains(layer.id) &&
                    std::none_of(expectedSelection.cbegin(), expectedSelection.cend(),
                                 [&](const auto &node) { return node->uuid() == layer.uuid; })) return false;
            }
        }
        return true;
    };
    const auto retainedNodesAreAttached = [&] {
        for (const auto &node : KisNodeList{base, leaf, inner, outer, top}) {
            KisNodeSP root = node;
            while (root->parent()) root = root->parent();
            if (root != image->root()) return false;
        }
        return true;
    };
    const auto hierarchy = [](KisImageSP current) {
        QMap<QUuid, QList<QUuid>> result;
        KisNodeList pending{current->root()};
        for (int i = 0; i < pending.size(); ++i) {
            const auto node = pending[i];
            const auto key = i == 0 ? QUuid() : node->uuid();
            result.insert(key, {});
            for (auto child = node->firstChild(); child; child = child->nextSibling()) {
                result[key].append(child->uuid());
                pending.append(child);
            }
        }
        return result;
    };
    const auto before = hierarchy(image);
    auto after = before;
    if (scenario == 0 || scenario == 3 || scenario == 11 || scenario == 12) after[QUuid()] = {outerId, baseId, topId};
    else if (scenario == 1) { after[QUuid()] = {outerId, topId}; after[outerId] = {baseId, innerId}; }
    else if (scenario == 2) { after[QUuid()] = {baseId, outerId}; after[outerId] = {innerId, topId}; }
    else if (scenario == 4) after[QUuid()] = {baseId, topId, outerId};
    else if (scenario == 5 || scenario == 6) {
        after[innerId].clear();
        after[outerId] = scenario == 5 ? QList<QUuid>{innerId, leafId} : QList<QUuid>{leafId, innerId};
    } else if (scenario == 7 || scenario == 8 || scenario == 9) {
        after[outerId].clear();
        after[QUuid()] = scenario == 8 ? QList<QUuid>{baseId, innerId, outerId, topId} : QList<QUuid>{baseId, outerId, innerId, topId};
    } else if (scenario == 10) {
        after[QUuid()] = {outerId, topId}; after[innerId].clear(); after[outerId] = {innerId, baseId, leafId};
    }
    QScopedPointer<Node> leafApi(Node::createNode(image, leaf));
    QScopedPointer<Node> baseApi(Node::createNode(image, base));
    const auto leafPixels = leafApi->pixelData(0, 0, 32, 16);
    const auto basePixels = baseApi->pixelData(0, 0, 32, 16);
    const auto history = lifetime->undoStack()->index();
    if (entry == QLatin1String("direct")) {
        KisNodeSP destination = image->root(), anchor;
        if (scenario == 1 || scenario == 2 || scenario == 5 || scenario == 6 || scenario == 10) destination = outer;
        if (scenario == 2 || scenario == 5 || scenario == 10) anchor = inner;
        else if (scenario == 3 || scenario == 7 || scenario == 9 || scenario == 12) anchor = outer;
        else if (scenario == 4 || scenario == 8) anchor = base;
        manager->moveNodesDirect(selected, destination, anchor);
    } else if (lower) manager->lowerNode();
    else manager->raiseNode();
    image->waitForDone();
    QCOMPARE(hierarchy(image), after);
    QVERIFY(retainedNodesAreAttached());
    if (changes || gpu || scenario == 15) QCOMPARE(lifetime->undoStack()->index(), history + (changes ? 1 : 0));
    QTRY_VERIFY(selectedAsExpected());
    QTRY_COMPARE(manager->activeNode(), expectedActive);
    QCOMPARE(leafApi->pixelData(0, 0, 32, 16), leafPixels);
    QCOMPARE(baseApi->pixelData(0, 0, 32, 16), basePixels);
    Document api(lifetime.data(), false);
    const auto pixels = api.pixelData(0, 0, 32, 16);
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            output.setAutoRemove(false);
            qInfo() << "Hierarchy ordering artifacts retained at" << output.path();
        }
    });
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = gpu && !results.isEmpty() ? results : output.path();
    QVERIFY(QDir().mkpath(destination));
    const auto stem = (entry == QLatin1String("direct") ? QStringLiteral("hierarchy-direct-move-%1-%2") :
                                                        QStringLiteral("hierarchy-order-%1-%2")).arg(scenario).arg(layout);
    const auto kraPath = QDir(destination).filePath(stem + QStringLiteral(".kra"));
    KisPropertiesConfigurationSP saveConfiguration(new KisPropertiesConfiguration);
    saveConfiguration->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    QVERIFY2(lifetime->saveAs(kraPath, KisDocument::nativeFormatMimeType(), false, saveConfiguration), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(kraPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QCOMPARE(hierarchy(reopened->image()), after);
    Document loaded(reopened.data(), false);
    QCOMPARE(loaded.pixelData(0, 0, 32, 16), pixels);
    QScopedPointer<Node> loadedLeaf(loaded.nodeByUniqueID(leafId));
    QScopedPointer<Node> loadedBase(loaded.nodeByUniqueID(baseId));
    QVERIFY(loadedLeaf && loadedBase);
    QCOMPARE(loadedLeaf->pixelData(0, 0, 32, 16), leafPixels);
    QCOMPARE(loadedBase->pixelData(0, 0, 32, 16), basePixels);
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
    const auto blue = QByteArray::fromHex("ff0000ff");
    QVERIFY(loadedLeaf->setPixelData(blue, 4, 4, 1, 1));
    loaded.refreshProjection();
    QCOMPARE(loadedLeaf->pixelData(4, 4, 1, 1), blue);
    QCOMPARE(hierarchy(reopened->image()), after);
    if (changes) {
        lifetime->undoStack()->undo();
        QTRY_COMPARE(lifetime->undoStack()->index(), history);
        image->waitForDone();
        QApplication::sendPostedEvents();
        QCOMPARE(hierarchy(image), before);
        QVERIFY(retainedNodesAreAttached());
        QTRY_VERIFY(selectedAsExpected());
        QTRY_COMPARE(manager->activeNode(), source);
        lifetime->undoStack()->redo();
        QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
        image->waitForDone();
        QApplication::sendPostedEvents();
        QCOMPARE(hierarchy(image), after);
        QVERIFY(retainedNodesAreAttached());
        QCOMPARE(lifetime->undoStack()->index(), history + 1);
        QTRY_VERIFY(selectedAsExpected());
        QTRY_COMPARE(manager->activeNode(), expectedActive);
        QCOMPARE(leafApi->pixelData(0, 0, 32, 16), leafPixels);
        QCOMPARE(baseApi->pixelData(0, 0, 32, 16), basePixels);
        const auto topLevel = image->root()->at(after[QUuid()].indexOf(outerId));
        QCOMPARE(topLevel, KisNodeSP(outer));
    }
}
catch (const std::exception &error)
{
    QFAIL(error.what());
}
