/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_operation_batch_test.h"

#include <algorithm>

#include <simpletest.h>
#include <commands/kis_node_operation_batch.h>

#include <KoColor.h>
#include <KoColorSpace.h>

void KisNodeOperationBatchTest::init()
{
    p.reset(new TestUtil::MaskParent);

    QRect rect1(100, 100, 100, 100);
    QRect rect2(150, 150, 150, 150);

    layer1 = p->layer;
    layer1->paintDevice()->fill(rect1, KoColor(Qt::red, layer1->colorSpace()));

    layer2 = new KisPaintLayer(p->image, "paint2", OPACITY_OPAQUE_U8);
    layer2->paintDevice()->fill(rect2, KoColor(Qt::blue, layer2->colorSpace()));

    layer3 = new KisPaintLayer(p->image, "paint3", OPACITY_OPAQUE_U8);
    group4 = new KisGroupLayer(p->image, "group4", OPACITY_OPAQUE_U8);
    layer5 = new KisPaintLayer(p->image, "paint5", OPACITY_OPAQUE_U8);
    layer6 = new KisPaintLayer(p->image, "paint6", OPACITY_OPAQUE_U8);

    p->image->addNode(layer2);
    p->image->addNode(layer3);
    p->image->addNode(group4);
    p->image->addNode(layer5, group4);
    p->image->addNode(layer6);

    p->image->initialRefreshGraph();
}

void KisNodeOperationBatchTest::cleanup()
{
    p.reset();
    layer1.clear();
    layer2.clear();
}

void KisNodeOperationBatchTest::testMove(int delayBeforeEnd)
{
    TestUtil::ReferenceImageChecker chk("node_juggler", "move_test");
    chk.setMaxFailingPixels(0);

    KisNodeOperationBatch batch(kundo2_i18n("Move Layer"), p->image, 600);
    QVERIFY(chk.checkImage(p->image, "initial"));

    batch.moveNode(layer1, p->image->root(), layer2);
    QTest::qWait(100);
    QVERIFY(chk.checkImage(p->image, "initial"));

    if (delayBeforeEnd) {
        QTest::qWait(delayBeforeEnd);
        QVERIFY(chk.checkImage(p->image, "moved"));
    }

    batch.end();
    p->image->waitForDone();
    QVERIFY(chk.checkImage(p->image, "moved"));

    p->undoStore->undo();
    p->image->waitForDone();

    QVERIFY(chk.checkImage(p->image, "initial"));
}

void KisNodeOperationBatchTest::testApplyUndo()
{
    testMove(1000);
}

void KisNodeOperationBatchTest::testEndBeforeUpdate()
{
    testMove(0);
}

void KisNodeOperationBatchTest::testDuplicateImpl(bool externalParent, bool useMove)
{
    TestUtil::ReferenceImageChecker chk("node_juggler", "move_test");
    chk.setMaxFailingPixels(0);

    QStringList initialRef;
    initialRef << "paint1";
    initialRef << "paint2";
    initialRef << "paint3";
    initialRef << "group4";
    initialRef << "+paint5";
    initialRef << "paint6";

    QVERIFY(TestUtil::checkHierarchy(p->image->root(), initialRef));

    KisNodeList selectedNodes;
    selectedNodes << layer2;
    selectedNodes << layer3;
    selectedNodes << layer5;

    KisNodeOperationBatch batch(kundo2_i18n("Duplicate Layers"), p->image, 600);

    if (!externalParent) {
        batch.duplicateNode(selectedNodes, layer2);
    } else {
        if (useMove) {
            batch.moveNode(selectedNodes, p->image->root(), layer6, layer2);
        } else {
            batch.copyNode(selectedNodes, p->image->root(), layer6, layer2);
        }
    }

    QTest::qWait(1000);

    batch.end();
    p->image->waitForDone();

    QStringList ref;

    if (!externalParent) {
        ref << "paint1";
        ref << "paint2";
        ref << "paint3";
        ref << "group4";
        ref << "+paint5";
        ref << "+Copy of paint2";
        ref << "+Copy of paint3";
        ref << "+Copy of paint5";
        ref << "paint6";
    } else if (!useMove) {
        ref << "paint1";
        ref << "paint2";
        ref << "paint3";
        ref << "group4";
        ref << "+paint5";
        ref << "paint6";
        ref << "Copy of paint2";
        ref << "Copy of paint3";
        ref << "Copy of paint5";
    } else {
        ref << "paint1";
        ref << "group4";
        ref << "paint6";
        ref << "paint2";
        ref << "paint3";
        ref << "paint5";
    }

    QVERIFY(TestUtil::checkHierarchy(p->image->root(), ref));

    p->undoStore->undo();
    p->image->waitForDone();

    QVERIFY(TestUtil::checkHierarchy(p->image->root(), initialRef));
}

void KisNodeOperationBatchTest::testDuplicate()
{
    testDuplicateImpl(false, false);
}

void KisNodeOperationBatchTest::testCopyLayers()
{
    testDuplicateImpl(true, false);
}

void KisNodeOperationBatchTest::testMoveLayers()
{
    testDuplicateImpl(true, true);
}

void KisNodeOperationBatchTest::testActiveNodeIsRestoredByUndo()
{
    QList<QPair<KisNodeSP, KisNodeList>> reselectionRequests;
    connect(p->image.data(), &KisImage::sigRequestNodeReselection,
            this, [&reselectionRequests](KisNodeSP activeNode,
                                         const KisNodeList &selectedNodes) {
        reselectionRequests.append(qMakePair(activeNode, selectedNodes));
    });

    KisNodeOperationBatch batch(kundo2_i18n("Remove Layer"), p->image, 600);
    batch.removeNode(KisNodeList() << layer2, layer2);
    batch.end();
    p->image->waitForDone();

    p->undoStore->undo();
    p->image->waitForDone();

    QTRY_VERIFY_WITH_TIMEOUT(
        std::any_of(
            reselectionRequests.cbegin(), reselectionRequests.cend(),
            [this](const QPair<KisNodeSP, KisNodeList> &request) {
                return request.first.data() == layer2.data() &&
                       request.second == (KisNodeList() << layer2);
            }),
        1000);
}

void KisNodeOperationBatchTest::testCreateGroup()
{
    KisNodeOperationBatch batch(kundo2_i18n("Quick Group"), p->image, 600);
    KisNodeSP newGroup;
    KisNodeSP newLastChild;

    QVERIFY(batch.createGroup(KisNodeList() << layer2 << layer3,
                              layer2,
                              QStringLiteral("Grouped Layers"),
                              &newGroup,
                              &newLastChild));

    batch.end();
    p->image->waitForDone();

    QCOMPARE(newGroup->name(), QStringLiteral("Grouped Layers"));
    QCOMPARE(newGroup->parent(), p->image->root());
    QCOMPARE(layer2->parent(), newGroup);
    QCOMPARE(layer3->parent(), newGroup);
    QCOMPARE(newLastChild, KisNodeSP(layer3));

    p->undoStore->undo();
    p->image->waitForDone();

    QVERIFY(!newGroup->parent());
    QCOMPARE(layer2->parent(), p->image->root());
    QCOMPARE(layer3->parent(), p->image->root());
}

void KisNodeOperationBatchTest::testUngroup()
{
    KisNodeOperationBatch batch(kundo2_i18n("Quick Ungroup"), p->image, 600);
    KisNodeSP incompatibleNode;
    KisNodeSP destinationParent;

    QVERIFY(batch.ungroupNodes(KisNodeList() << group4,
                               group4,
                               &incompatibleNode,
                               &destinationParent));
    QVERIFY(!incompatibleNode);
    QCOMPARE(destinationParent, p->image->root());

    batch.end();
    p->image->waitForDone();

    QVERIFY(!group4->parent());
    QCOMPARE(layer5->parent(), p->image->root());

    p->undoStore->undo();
    p->image->waitForDone();

    QCOMPARE(group4->parent(), p->image->root());
    QCOMPARE(layer5->parent(), KisNodeSP(group4));
}

SIMPLE_TEST_MAIN(KisNodeOperationBatchTest)
