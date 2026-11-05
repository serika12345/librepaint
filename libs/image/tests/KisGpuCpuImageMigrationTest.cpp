/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testimage.h>
#include <KisGpuPaintDeviceTransfer_p.h>
#include <KisGpuPixelTolerance.h>
#include <gpu/KisGpuExecution.h>
#include <kis_image.h>
#include <kis_layer_utils.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_adjustment_layer.h>
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_raster_keyframe_channel.h>
#include <kis_image_animation_interface.h>
#include <kis_transaction.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <QSignalSpy>

namespace Gpu = LibrePaint::Image::Gpu;

namespace {
QByteArray pixels(KisPaintDeviceSP device, QRect area)
{
    QByteArray bytes(qsizetype(area.width()) * area.height() * device->pixelSize(), Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
    return bytes;
}
}

class KisGpuCpuImageMigrationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void imageKeepsNodesAndRunsNativeUpdates_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8-groups-masks-and-frames") << false;
        QTest::newRow("u16-sparse-default-internal-selection-rejection") << true;
    }
    void imageKeepsNodesAndRunsNativeUpdates();
};

void KisGpuCpuImageMigrationTest::imageKeepsNodesAndRunsNativeUpdates()
{
    QFETCH(bool, boundary);
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    constexpr qsizetype capacity = 32 * 1024 * 1024;
    const QSize size(270, 48);
    const QRect inspected(-8, -8, 288, 72);
    const QRect island(1000003, -5, 3, 7);
    KisPaintDeviceSP native = new KisPaintDevice(space);
    if (boundary) {
        const quint16 bgra[] = {591, 12345, 54321, 23456};
        native->moveTo({7, -3});
        native->setDefaultPixel(KoColor(reinterpret_cast<const quint8 *>(bgra), space));
        native->fill(island, KoColor(QColor(193, 37, 79, 211), space));
    }
    native->fill({252, 3, 13, 19}, KoColor(QColor(43, 157, 211, 173), space));
    Gpu::RasterImage raster;
    raster.size = size;
    raster.format = format;
    raster.colorProfile = space->profile()->rawData();
    if (boundary) raster.projectionBackground = {2827, 7453, 12079, 65535};
    Gpu::RasterLayer group;
    group.uuid = QUuid::createUuid();
    group.kind = Gpu::LayerKind::Group;
    group.name = QStringLiteral("Retained group");
    Gpu::RasterLayer layer;
    layer.uuid = QUuid::createUuid();
    layer.parentUuid = group.uuid;
    layer.name = QStringLiteral("Retained paint");
    layer.opacity = 173;
    layer.pinnedToTimeline = true;
    layer.position = native->offset();
    layer.transparencyMask = Gpu::SelectionMask::fromRectangle({0, 0, 270, 42}, QRect(QPoint(), size));
    layer.transparencyMaskName = QStringLiteral("Retained mask");
    QVERIFY(kisGpuRasterLayerFromPaintDevice(native, format, capacity, layer));
    raster.layers = {group, layer};
    if (boundary) {
        Gpu::RasterLayer adjustment;
        adjustment.kind = Gpu::LayerKind::Adjustment;
        adjustment.name = QStringLiteral("Internal selection");
        adjustment.visible = false;
        Gpu::FilterDefinition filter;
        filter.id = QStringLiteral("maximize");
        filter.xml = QByteArrayLiteral("<params version=\"1\"/>");
        filter.settings.kind = Gpu::FilterKind::MaximizeChannel;
        adjustment.filter = filter;
        adjustment.filterSelection = Gpu::SelectionMask::fromRectangle({2, 3, 17, 19}, QRect(QPoint(), size));
        raster.layers.append(adjustment);
    }
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Native migration"), space);
    image->setResolution(1.5, 2.5);
    QVERIFY(image->setGpuSelectionRect({1, 2, 263, 35}, boundary));
    const auto id = owner->operations().snapshot()->layers[1].id;
    QCOMPARE(owner->renameLayer(id, QStringLiteral("Redo entry")), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    const auto root = image->rootLayer();
    const auto groupNode = KisLayerUtils::findNodeByUuid(root, group.uuid);
    auto paintNode = KisLayerUtils::findNodeByUuid(root, layer.uuid);
    QVERIFY(groupNode);
    QVERIFY(paintNode);
    auto paint = paintNode->paintDevice();
    const auto maskNode = qobject_cast<KisTransparencyMask *>(paintNode->firstChild().data());
    QVERIFY(maskNode);
    const auto mask = maskNode->selection()->pixelSelection();
    const auto selection = image->globalSelection();
    const auto raw = pixels(paint, inspected);
    const auto far = boundary ? pixels(paint, island) : QByteArray();
    const auto maskPixels = pixels(mask, inspected);
    const auto selectionPixels = pixels(selection->pixelSelection(), inspected);
    const auto projection = pixels(image->projection(), image->bounds());
    const auto background = image->defaultProjectionColor();
    KisPixelSelectionSP internal;
    QByteArray internalPixels;
    if (boundary) {
        const auto adjustment = qobject_cast<KisAdjustmentLayer *>(
            KisLayerUtils::findNodeByUuid(root, before->layers.last().uuid).data());
        QVERIFY(adjustment);
        internal = adjustment->internalSelection()->pixelSelection();
        internalPixels = pixels(internal, inspected);
    }
    QSignalSpy gpuChanged(owner.data(), &Gpu::PaintImage::changed);
    QSignalSpy modified(image.data(), &KisImage::sigImageModified);
    QVERIFY(!image->prepareCpuMigration(capacity));
    image->barrierLock();
    image->barrierLock();
    qsizetype bytes = 0;
    auto prepared = image->prepareCpuMigration(capacity, &bytes);
    QVERIFY(prepared);
    QVERIFY(bytes > 0 && bytes < capacity);
    if (boundary) {
        QVERIFY(!image->prepareCpuMigration(bytes - 1));
        QCOMPARE(image->gpuImage(), owner);
        QVERIFY(paint->prepareCpuMigration(capacity));
        QVERIFY(mask->prepareCpuMigration(capacity));
        QCOMPARE_PIXELS16_WITHIN_STEP(pixels(paint, inspected), raw);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QVERIFY(!image->applyCpuMigration(prepared));
        QCOMPARE(image->gpuImage(), owner);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        prepared = image->prepareCpuMigration(capacity);
        QVERIFY(prepared);
        gpuChanged.clear();
        modified.clear();
    }
    QVERIFY(image->applyCpuMigration(prepared));
    QVERIFY(!image->applyCpuMigration(prepared));
    QVERIFY(image->locked());
    QVERIFY(!image->gpuImage());
    QVERIFY(!paint->prepareCpuMigration(capacity));
    QVERIFY(!mask->prepareCpuMigration(capacity));
    QVERIFY(!internal || !internal->prepareCpuMigration(capacity));
    QCOMPARE(image->rootLayer(), root);
    QCOMPARE(root->firstChild(), groupNode);
    QCOMPARE(groupNode->firstChild(), paintNode);
    QCOMPARE(paintNode->paintDevice(), paint);
    QCOMPARE(paintNode->name(), layer.name);
    QCOMPARE(paintNode->opacity(), layer.opacity);
    QCOMPARE(paintNode->isPinnedToTimeline(), true);
    QCOMPARE(paint->offset(), native->offset());
    QCOMPARE(paint->defaultPixel(), native->defaultPixel());
    QCOMPARE(image->colorSpace(), space);
    QCOMPARE(image->xRes(), 1.5);
    QCOMPARE(image->yRes(), 2.5);
    QCOMPARE(image->defaultProjectionColor(), background);
    QCOMPARE(image->globalSelection(), selection);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, inspected), raw, boundary ? 2 : 1);
    if (boundary) QCOMPARE_PIXELS16_WITHIN_STEP(pixels(paint, island), far);
    QCOMPARE(pixels(mask, inspected), maskPixels);
    QCOMPARE(pixels(selection->pixelSelection(), inspected), selectionPixels);
    if (internal) QCOMPARE(pixels(internal, inspected), internalPixels);
    QCOMPARE(gpuChanged.size(), 0);
    QCOMPARE(modified.size(), 0);
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!owner->operations().isModified());

    image->refreshGraphAsync();
    image->unlock();
    QVERIFY(image->locked());
    image->unlock();
    image->waitForDone();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(image->projection(), image->bounds()), projection, boundary ? 2 : 1);

    KisTransaction transaction(paint);
    paint->fill({252, 3, 13, 19}, KoColor(QColor(211, 83, 17, 121), space));
    const auto painted = pixels(paint, inspected);
    QVERIFY(!pixelStepDifference(painted, raw, boundary ? 2 : 1).isEmpty());
    QScopedPointer<KUndo2Command> command(transaction.endAndTake());
    command->redo();
    command->undo();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, inspected), raw, boundary ? 2 : 1);
    command->redo();
    image->refreshGraphAsync();
    image->waitForDone();
    QVERIFY(!pixelStepDifference(pixels(image->projection(), image->bounds()), projection, boundary ? 2 : 1).isEmpty());

    paintNode->enableAnimation();
    const auto channel = qobject_cast<KisRasterKeyframeChannel *>(paintNode->getKeyframeChannel(KisKeyframeChannel::Raster.id(), true));
    QVERIFY(channel);
    channel->importFrame(7, native, nullptr);
    image->animationInterface()->switchCurrentTimeAsync(7);
    image->waitForDone();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, inspected), pixels(native, inspected), boundary ? 2 : 1);
    image->animationInterface()->switchCurrentTimeAsync(0);
    image->waitForDone();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, inspected), painted, boundary ? 2 : 1);
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().redoCount(), 1);
    QCOMPARE(gpuChanged.size(), 0);
}

KISTEST_MAIN(KisGpuCpuImageMigrationTest)
#include "KisGpuCpuImageMigrationTest.moc"
