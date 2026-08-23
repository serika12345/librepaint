/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_group_layer.h>
#include <commands/kis_node_commands_adapter.h>
#include <kis_paint_layer.h>
#include <kis_selection_mask.h>
#include <simpletest.h>
#include <testutil.h>

class KisNodeCommandsAdapterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAddNodeIsUndoable();
    void testImageCanBeRebound();
    void testMoveActiveSelectionMaskDeactivatesPreviousMask();
};

void KisNodeCommandsAdapterTest::testAddNodeIsUndoable()
{
    TestUtil::MaskParent context;
    KisNodeCommandsAdapter adapter(context.image);
    KisPaintLayerSP addedLayer = new KisPaintLayer(
        context.image, "added", OPACITY_OPAQUE_U8);

    adapter.addNode(
        addedLayer,
        context.image->rootLayer(),
        context.image->rootLayer()->lastChild());

    QCOMPARE(
        context.image->rootLayer()->lastChild().data(),
        addedLayer.data());
    QCOMPARE(context.image->rootLayer()->childCount(), 2U);

    context.undoStore->undo();

    QVERIFY(!addedLayer->parent());
    QCOMPARE(context.image->rootLayer()->childCount(), 1U);
}

void KisNodeCommandsAdapterTest::testImageCanBeRebound()
{
    TestUtil::MaskParent firstContext;
    TestUtil::MaskParent secondContext;
    KisNodeCommandsAdapter adapter(firstContext.image);
    KisPaintLayerSP addedLayer = new KisPaintLayer(
        secondContext.image, "added", OPACITY_OPAQUE_U8);

    adapter.setImage(secondContext.image);
    adapter.addNode(
        addedLayer,
        secondContext.image->rootLayer(),
        secondContext.image->rootLayer()->lastChild());

    QCOMPARE(firstContext.image->rootLayer()->childCount(), 1U);
    QCOMPARE(
        secondContext.image->rootLayer()->lastChild().data(),
        addedLayer.data());
    QCOMPARE(secondContext.image->rootLayer()->childCount(), 2U);
}

void KisNodeCommandsAdapterTest::testMoveActiveSelectionMaskDeactivatesPreviousMask()
{
    TestUtil::MaskParent context;
    KisNodeCommandsAdapter adapter(context.image);
    KisPaintLayerSP destinationLayer = new KisPaintLayer(
        context.image, "destination", OPACITY_OPAQUE_U8);
    KisSelectionMaskSP movedMask = new KisSelectionMask(context.image, "moved");
    KisSelectionMaskSP previousMask = new KisSelectionMask(context.image, "previous");

    context.image->addNode(destinationLayer, context.image->rootLayer());
    context.image->addNode(movedMask, context.layer);
    context.image->addNode(previousMask, destinationLayer);
    movedMask->setActive(true);
    previousMask->setActive(true);

    adapter.moveNode(movedMask, destinationLayer, 0);

    QCOMPARE(movedMask->parent().data(), destinationLayer.data());
    QVERIFY(movedMask->active());
    QVERIFY(!previousMask->active());
    QCOMPARE(destinationLayer->selectionMask().data(), movedMask.data());
}

SIMPLE_TEST_MAIN(KisNodeCommandsAdapterTest)

#include "kis_node_commands_adapter_test.moc"
