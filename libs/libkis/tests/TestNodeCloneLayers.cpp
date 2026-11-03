/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestNode.h"
#include <simpletest.h>
#include <Document.h>
#include <CloneLayer.h>
#include <Krita.h>
#include <KoColor.h>
#include <KoToolManager.h>
#include <KoPointerEvent.h>
#include <KoColorSpaceRegistry.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <document/KisDocument.h>
#include <nodes/kis_node_manager.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_clone_layer.h>
#include <kis_image.h>
#include <kis_config.h>
#include <kis_tool.h>
#include <canvas/kis_canvas2.h>
#include <canvas/kis_coordinates_converter.h>
#include <kundo2stack.h>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDialog>
#include <QPointer>
#include <QMouseEvent>
#include <QRadioButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <memory>

void TestNode::testCloneLayersKeepOrdinaryEditingHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("panel");
    for (bool words : {false, true}) for (bool panel : {false, true}) {
        const QByteArray name = QByteArray(words ? "rgba16-" : "rgba8-") + (panel ? "layer-panel" : "public-api");
        QTest::newRow(name.constData()) << words << panel;
    }
}

void TestNode::testCloneLayersKeepOrdinaryEditingHistoryAndKra()
{
    QFETCH(bool, words);
    QFETCH(bool, panel);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto environment = qScopeGuard([&] { qputenv("LIBREPAINT_GPU_ENGINE", previous); });
    KisConfig(true).disableOpenGL();
    const auto space = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY(document->newImage(QStringLiteral("Clone editing"), 32, 32, space,
        KoColor(QColor(Qt::transparent), space), KisConfig::CANVAS_COLOR, 1, {}, 1));
    const auto image = KisImageSP(document->image());
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto cleanup = qScopeGuard([&] {
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
    Document api(lifetime.data(), false);
    QScopedPointer<Node> root(api.rootNode());
    const auto sourceNode = image->root()->firstChild();
    QScopedPointer<Node> source(Node::createNode(image, sourceNode));
    const auto sourceId = source->uniqueId();
    source->setName(QStringLiteral("Source"));
    source->setVisible(false);
    const quint16 firstComponents[]{1, 257, 65534, 65535};
    const quint16 secondComponents[]{65534, 1, 257, 65535};
    const auto first = words ? QByteArray(reinterpret_cast<const char *>(firstComponents), sizeof(firstComponents)) : QByteArray::fromHex("0111feff");
    const auto second = words ? QByteArray(reinterpret_cast<const char *>(secondComponents), sizeof(secondComponents)) : QByteArray::fromHex("fe0111ff");
    QVERIFY(source->setPixelData(first, 3, 4, 1, 1));
    QScopedPointer<Node> clone;
    auto *manager = window->viewManager()->nodeManager();
    const auto before = lifetime->undoStack()->index();
    if (panel) {
        manager->slotNonUiActivatedNode(sourceNode);
        QTRY_COMPARE(manager->activeNode(), sourceNode);
        const auto created = manager->createNode(QStringLiteral("KisCloneLayer"), true);
        QVERIFY(created);
        clone.reset(Node::createNode(image, created));
    } else {
        clone.reset(api.createCloneLayer(QStringLiteral("Clone"), source.data()));
        QVERIFY(clone);
        QVERIFY(root->addChildNode(clone.data(), source.data()));
    }
    QCOMPARE(lifetime->undoStack()->index(), before + 1);
    const auto cloneId = clone->uniqueId();
    clone->move(2, -1);
    QCOMPARE(clone->position(), QPoint(2, -1));
    QCOMPARE(clone->projectionPixelData(5, 3, 1, 1), first);
    KisNodeSP moveNode;
    for (auto node = image->root()->firstChild(); node; node = node->nextSibling())
        if (node->uuid() == cloneId) moveNode = node;
    manager->slotNonUiActivatedNode(moveNode);
    const auto toolId = QStringLiteral("KritaTransform/KisToolMove");
    KoToolManager::instance()->switchToolRequested(toolId);
    auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId));
    QVERIFY(tool);
    for (QWidget *panel : tool->optionWidgets())
        if (auto *selected = panel->findChild<QRadioButton *>(QStringLiteral("radioSelectedLayer"))) selected->setChecked(true);
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    KoPointerEvent begin(&press, converter->imageToDocument(QPointF(5, 3)));
    QMouseEvent motion(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    KoPointerEvent update(&motion, converter->imageToDocument(QPointF(6, 3)));
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    KoPointerEvent end(&release, converter->imageToDocument(QPointF(6, 3)));
    tool->beginPrimaryAction(&begin);
    tool->continuePrimaryAction(&update);
    tool->endPrimaryAction(&end);
    tool->requestStrokeEnd();
    QCOMPARE(clone->position(), QPoint(3, -1));
    QCOMPARE(clone->projectionPixelData(6, 3, 1, 1), first);
    lifetime->undoStack()->undo();
    QCOMPARE(clone->position(), QPoint(2, -1));
    lifetime->undoStack()->redo();
    QCOMPARE(clone->projectionPixelData(6, 3, 1, 1), first);
    lifetime->undoStack()->undo();
    QVERIFY(!clone->setPixelData(second, 5, 3, 1, 1));
    QScopedPointer<Node> alternative(api.createNode(QStringLiteral("Alternative"), QStringLiteral("paintlayer")));
    QVERIFY(alternative->setPixelData(second, 3, 4, 1, 1));
    QVERIFY(root->addChildNode(alternative.data(), nullptr));
    alternative->setVisible(false);
    QCOMPARE(alternative->pixelData(3, 4, 1, 1), second);
    KisCloneLayer *nativeClone = nullptr;
    for (auto node = image->root()->firstChild(); node; node = node->nextSibling())
        if (node->uuid() == cloneId) nativeClone = qobject_cast<KisCloneLayer *>(node.data());
    QVERIFY(nativeClone);
    const auto history = lifetime->undoStack()->index();
    for (bool accept : {false, true}) {
        manager->slotNonUiActivatedNode(nativeClone);
        manager->changeCloneSource();
        QPointer<QDialog> dialog;
        for (auto *widget : QApplication::topLevelWidgets())
            if (QByteArray(widget->metaObject()->className()) == "KisDlgChangeCloneSource")
                dialog = qobject_cast<QDialog *>(widget);
        QVERIFY(dialog);
        auto *combo = dialog->findChild<QComboBox *>(QStringLiteral("cmbSourceLayer"));
        QVERIFY(combo);
        const auto target = combo->findText(QStringLiteral("Alternative"));
        QVERIFY(target >= 0);
        combo->setCurrentIndex(target);
        QCOMPARE(nativeClone->copyFrom()->uuid(), alternative->uniqueId());
        QCOMPARE(lifetime->undoStack()->index(), history);
        if (accept) dialog->accept(); else dialog->reject();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!dialog);
        QCOMPARE(lifetime->undoStack()->index(), history + int(accept));
        QCOMPARE(clone->projectionPixelData(5, 3, 1, 1), accept ? second : first);
    }
    lifetime->undoStack()->undo();
    QCOMPARE(clone->projectionPixelData(5, 3, 1, 1), first);
    lifetime->undoStack()->redo();
    QCOMPARE(clone->projectionPixelData(5, 3, 1, 1), second);
    auto *publicClone = qobject_cast<CloneLayer *>(clone.data());
    QVERIFY(publicClone);
    publicClone->setSourceNode(source.data());
    QCOMPARE(clone->projectionPixelData(5, 3, 1, 1), first);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = QDir(directory.path()).filePath(QStringLiteral("clone.kra"));
    QVERIFY(api.saveAs(path));
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<Document> cpu(Krita::instance()->openDocument(path));
    QVERIFY(cpu);
    QScopedPointer<Node> cpuClone(cpu->nodeByUniqueID(cloneId));
    QVERIFY(cpuClone);
    QCOMPARE(cpuClone->type(), QStringLiteral("clonelayer"));
    QCOMPARE(cpuClone->projectionPixelData(5, 3, 1, 1), first);
    QVERIFY(cpu->saveAs(path));
    cpu->setModified(false);
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<Document> reopened(Krita::instance()->openDocument(path));
    QVERIFY(reopened);
    QScopedPointer<Node> loadedSource(reopened->nodeByUniqueID(sourceId));
    QScopedPointer<Node> loadedClone(reopened->nodeByUniqueID(cloneId));
    QVERIFY(loadedSource && loadedClone);
    QCOMPARE(loadedClone->type(), QStringLiteral("clonelayer"));
    QCOMPARE(loadedClone->position(), QPoint(2, -1));
    QScopedPointer<Node> linked(qobject_cast<CloneLayer *>(loadedClone.data())->sourceNode());
    QVERIFY(linked);
    QCOMPARE(linked->uniqueId(), sourceId);
    QVERIFY(loadedSource->setPixelData(second, 3, 4, 1, 1));
    QCOMPARE(loadedClone->projectionPixelData(5, 3, 1, 1), second);
    QVERIFY(loadedSource->remove());
    QScopedPointer<Node> detached(reopened->nodeByUniqueID(cloneId));
    QVERIFY(detached);
    QCOMPARE(detached->type(), QStringLiteral("paintlayer"));
    QCOMPARE(detached->projectionPixelData(5, 3, 1, 1), second);
    QVERIFY(detached->setPixelData(first, 6, 4, 1, 1));
    const auto detachedPath = QDir(directory.path()).filePath(QStringLiteral("detached.kra"));
    QVERIFY(reopened->saveAs(detachedPath));
    QScopedPointer<Document> restored(Krita::instance()->openDocument(detachedPath));
    QVERIFY(restored);
    QScopedPointer<Node> paint(restored->nodeByUniqueID(cloneId));
    QVERIFY(paint);
    QCOMPARE(paint->type(), QStringLiteral("paintlayer"));
    QCOMPARE(paint->pixelData(5, 3, 1, 1), second);
    QCOMPARE(paint->pixelData(6, 4, 1, 1), first);
    restored->setModified(false);
    reopened->setModified(false);
}
