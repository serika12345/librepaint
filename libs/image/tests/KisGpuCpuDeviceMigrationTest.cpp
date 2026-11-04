/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testimage.h>
#include <testing_timed_default_bounds.h>
#include <KisGpuPaintDeviceTransfer_p.h>
#include <KisGpuPixelTolerance.h>
#include <KisRegion.h>
#include <gpu/KisGpuExecution.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_pixel_selection.h>
#include <kis_raster_keyframe_channel.h>
#include <kis_transaction.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <QSignalSpy>
#include <QRegion>

namespace Gpu = LibrePaint::Image::Gpu;

namespace {
QByteArray pixels(KisPaintDeviceSP device, QRect area)
{
    QByteArray bytes(qsizetype(area.width()) * area.height() * device->pixelSize(), Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
    return bytes;
}
}

class KisGpuCpuDeviceMigrationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nativeDevicesKeepPixelsAndFrameStorage_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8-pixels-and-frames") << false;
        QTest::newRow("u16-and-alpha-sparse-default-rejection") << true;
    }
    void nativeDevicesKeepPixelsAndFrameStorage();
};

void KisGpuCpuDeviceMigrationTest::nativeDevicesKeepPixelsAndFrameStorage()
{
    QFETCH(bool, boundary);
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    constexpr qsizetype capacity = 8 * 1024 * 1024;
    const QSize size(270, 48);
    const QRect inspected(-280, -24, 576, 104);
    const QPoint position = boundary ? QPoint(7, -3) : QPoint();
    KisPaintDeviceSP native = new KisPaintDevice(space);
    native->moveTo(position);
    if (boundary) {
        const quint16 bgra[] = {591, 12345, 54321, 23456};
        native->setDefaultPixel(KoColor(reinterpret_cast<const quint8 *>(bgra), space));
        native->fill({-258, -7, 5, 3}, KoColor(QColor(193, 37, 79, 211), space));
    }
    native->fill({252, 3, 13, 19}, KoColor(QColor(43, 157, 211, 173), space));
    Gpu::RasterImage raster;
    raster.size = size;
    raster.format = format;
    raster.colorProfile = space->profile()->rawData();
    Gpu::RasterLayer layer;
    layer.name = QStringLiteral("Migration source");
    layer.position = position;
    layer.opacity = 137;
    layer.transparencyMask = Gpu::SelectionMask::uniform(91);
    QVERIFY(kisGpuRasterLayerFromPaintDevice(native, format, capacity, layer));
    raster.layers = {layer};
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    const auto id = owner->operations().snapshot()->layers.first().id;
    KisPaintDeviceSP view = KisPaintDevice::fromGpuImage(owner, id, QStringLiteral("Retained device"), space);
    KisPaintLayerSP node = new KisPaintLayer({}, QStringLiteral("Retained node"), 255, view);
    const auto retained = view;
    auto *timedBounds = new TestUtil::TestingTimedDefaultBounds(QRect(QPoint(), size));
    view->setDefaultBounds(timedBounds);
    view->setSupportsWraparoundMode(true);
    QCOMPARE(owner->renameLayer(id, QStringLiteral("Redo entry")), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    const auto original = pixels(view, inspected);
    const auto region = view->region();
    if (boundary) {
        const auto stale = view->prepareCpuMigration(capacity);
        QVERIFY(stale);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QVERIFY(!view->applyCpuMigration(stale));
        QCOMPARE(owner->undo(), Gpu::Result::Success);
    }
    QSignalSpy changed(owner.data(), &Gpu::PaintImage::changed);

    QVERIFY(!view->prepareCpuMigration(1));
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), original, boundary ? 2 : 1);
    const auto prepared = view->prepareCpuMigration(capacity);
    QVERIFY(prepared);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), original, boundary ? 2 : 1);
    QVERIFY(view->applyCpuMigration(prepared));
    QVERIFY(!view->applyCpuMigration(prepared));
    QCOMPARE(view.data(), retained.data());
    QCOMPARE(node->paintDevice().data(), retained.data());
    QCOMPARE(node->name(), QStringLiteral("Retained node"));
    QCOMPARE(view->offset(), position);
    QCOMPARE(view->objectName(), QStringLiteral("Retained device"));
    QCOMPARE(view->defaultBounds().data(), timedBounds);
    QVERIFY(view->supportsWraproundMode());
    QCOMPARE(view->colorSpace()->profile()->rawData(), space->profile()->rawData());
    QCOMPARE(view->region().toQRegion(), region.toQRegion());
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), original, boundary ? 2 : 1);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), pixels(native, inspected), boundary ? 2 : 1);

    const QRect edited(11, 13, 3, 5);
    KisTransaction transaction(view);
    view->fill(edited, KoColor(QColor(211, 83, 17, 121), space));
    const auto painted = pixels(view, inspected);
    QVERIFY(!pixelStepDifference(painted, original, boundary ? 2 : 1).isEmpty());
    auto *command = transaction.endAndTake();
    command->redo();
    command->undo();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), original, boundary ? 2 : 1);
    command->redo();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), painted, boundary ? 2 : 1);
    delete command;

    auto *channel = view->createKeyframeChannel(KisKeyframeChannel::Raster);
    QVERIFY(channel);
    KisPaintDeviceSP frame = new KisPaintDevice(space);
    channel->writeToDevice(0, frame);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(frame, inspected), painted, boundary ? 2 : 1);
    KUndo2Command importCommand;
    channel->importFrame(7, native, &importCommand);
    timedBounds->testingSetTime(7);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), pixels(native, inspected), boundary ? 2 : 1);
    timedBounds->testingSetTime(0);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(view, inspected), painted, boundary ? 2 : 1);

    if (boundary) {
        const QRect block(3, -5, 64, 64);
        KisPixelSelectionSP alphaNative = new KisPixelSelection;
        alphaNative->moveTo(block.topLeft());
        const quint8 white = 255;
        alphaNative->setDefaultPixel(KoColor(&white, alphaNative->colorSpace()));
        alphaNative->select(block, 255);
        alphaNative->select({8, 9, 11, 13}, 73);
        Gpu::SelectionTileMap tiles;
        for (int ty = -1; ty <= 0; ++ty) {
            QByteArray bytes(Gpu::SelectionMask::AlphaTileBytes, Qt::Uninitialized);
            alphaNative->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 0,
                ty * Gpu::SelectionMask::TileSize, Gpu::SelectionMask::TileSize, Gpu::SelectionMask::TileSize);
            tiles.insert({0, ty}, bytes);
        }
        const auto coverage = Gpu::SelectionMask::fromAlphaTiles(tiles, 255)->withAllocatedBlocks({block});
        QVERIFY(coverage);
        KisPixelSelectionSP alpha = new KisPixelSelection;
        alpha->setDefaultBounds(view->defaultBounds());
        alpha->bindGpuCoverage(owner, coverage, QUuid::createUuid(), block.topLeft());
        const auto alphaBefore = pixels(alpha, inspected);
        QVERIFY(!alpha->prepareCpuMigration(64 * 64 - 1));
        const auto alphaPrepared = alpha->prepareCpuMigration(64 * 64);
        QVERIFY(alphaPrepared);
        const auto stale = view->applyCpuMigration(alphaPrepared);
        QVERIFY(!stale);
        QVERIFY(alpha->applyCpuMigration(alphaPrepared));
        QCOMPARE(alpha->offset(), block.topLeft());
        QCOMPARE(alpha->defaultPixel().data()[0], quint8(255));
        QCOMPARE(alpha->region().toQRegion(), alphaNative->region().toQRegion());
        QVERIFY(pixelStepDifference(pixels(alpha, inspected), alphaBefore, 1, 1).isEmpty());
        QVERIFY(pixelStepDifference(pixels(alpha, inspected), pixels(alphaNative, inspected), 1, 1).isEmpty());
        alpha->select({11, 13, 3, 5}, 0);
        QVERIFY(!pixelStepDifference(pixels(alpha, inspected), alphaBefore, 1, 1).isEmpty());
    }

    QCOMPARE(changed.size(), 0);
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!owner->operations().isModified());
    const auto source = owner->readRgbaPixels(id, inspected, capacity);
    QCOMPARE(source.code, Gpu::ExecutionCode::Success);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuPixelTransfer(source.bytes, format, false), original, boundary ? 2 : 1);
}

KISTEST_MAIN(KisGpuCpuDeviceMigrationTest)
#include "KisGpuCpuDeviceMigrationTest.moc"
