/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_group_layer.h>
#include <commands/kis_node_commands_adapter.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_paint_layer.h>
#include <kis_selection_mask.h>
#include <simpletest.h>
#include <testutil.h>
#include <exception>

class KisNodeCommandsAdapterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAddNodeIsUndoable();
    void testImageCanBeRebound();
    void testMoveActiveSelectionMaskDeactivatesPreviousMask();
    void gpuLayerPanelPropertiesUseTheDocumentHistory();
    void consecutivePropertyChangesUndoTogether_data();
    void consecutivePropertyChangesUndoTogether();
};

void KisNodeCommandsAdapterTest::consecutivePropertyChangesUndoTogether_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("opacity");
    QTest::addColumn<bool>("returnToInitial");
    QTest::newRow("cpu-name") << false << false << false;
    QTest::newRow("gpu-name") << true << false << false;
    QTest::newRow("cpu-opacity") << false << true << false;
    QTest::newRow("gpu-opacity") << true << true << false;
    QTest::newRow("cpu-name-reset") << false << false << true;
    QTest::newRow("gpu-name-reset") << true << false << true;
    QTest::newRow("cpu-opacity-reset") << false << true << true;
    QTest::newRow("gpu-opacity-reset") << true << true << true;
}

void KisNodeCommandsAdapterTest::consecutivePropertyChangesUndoTogether()
{
    QFETCH(bool, gpu);
    QFETCH(bool, opacity);
    QFETCH(bool, returnToInitial);
    namespace Gpu = LibrePaint::Image::Gpu;
    TestUtil::MaskParent reference({0, 0, 64, 64});
    std::unique_ptr<Gpu::Execution> execution;
    KisImageSP image = reference.image;
    Gpu::PaintImageSP owner;
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_unique<Gpu::Execution>(device);
        owner = Gpu::PaintImageSP::create(*execution, QSize(64, 64));
        image = KisImage::fromGpuImage(owner, QStringLiteral("Layer adjustments"));
    }
    KisNodeCommandsAdapter adapter(image);
    const auto node = image->rootLayer()->firstChild();
    const auto initialName = node->name();
    for (int value : {128, 64}) {
        if (opacity) adapter.setOpacity(node, value);
        else adapter.setNodeName(node, QString::number(value));
        image->waitForDone();
    }
    if (returnToInitial) {
        if (opacity) adapter.setOpacity(node, 255);
        else adapter.setNodeName(node, initialName);
        image->waitForDone();
    }
    const bool hasCommand = !(opacity && returnToInitial);
    if (gpu) QCOMPARE(owner->undo(), hasCommand ? Gpu::Result::Success : Gpu::Result::NothingToUndo);
    else QCOMPARE(bool(reference.undoStore->presentCommand()), hasCommand);
    if (!gpu) reference.undoStore->undo();
    image->waitForDone();
    QCOMPARE(node->opacity(), quint8(255));
    QCOMPARE(node->name(), initialName);
    if (gpu) QCOMPARE(owner->redo(), hasCommand ? Gpu::Result::Success : Gpu::Result::NothingToRedo);
    else reference.undoStore->redo();
    image->waitForDone();
    QCOMPARE(node->opacity(), quint8(opacity && !returnToInitial ? 64 : 255));
    QCOMPARE(node->name(), opacity || returnToInitial ? initialName : QStringLiteral("64"));
}

void KisNodeCommandsAdapterTest::gpuLayerPanelPropertiesUseTheDocumentHistory()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Layer panel"));
    KisNodeCommandsAdapter adapter(image);
    const auto node = image->rootLayer()->firstChild();
    const auto id = owner->operations().activeLayer();
    const auto initialName = node->name();
    QCOMPARE(owner->writeRgbaPixels(id, {0, 0, 1, 1}, QByteArray::fromHex("ff0000ff")), Gpu::Result::Success);
    try {
        adapter.setNodeName(node, QStringLiteral("Renamed"));
        adapter.setOpacity(node, 128);
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QCOMPARE(node->name(), QStringLiteral("Renamed"));
    QCOMPARE(node->opacity(), quint8(128));
    QCOMPARE(owner->operations().undoCount(), 2);
    const auto projection = owner->readRgbaPixels(0, {0, 0, 1, 1}, 4);
    QCOMPARE(projection.code, Gpu::ExecutionCode::Success);
    QCOMPARE(projection.bytes, QByteArray::fromHex("ff000080"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(node->opacity(), quint8(255));
    QCOMPARE(node->name(), QStringLiteral("Renamed"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(node->name(), initialName);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(node->name(), QStringLiteral("Renamed"));
    QCOMPARE(node->opacity(), quint8(128));
    QCOMPARE(owner->readRgbaPixels(id, {0, 0, 1, 1}, 4).bytes, QByteArray::fromHex("ff0000ff"));
}

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
