/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testimage.h>
#include <KisGpuCpuLayer.h>
#include <KisGpuPaintDeviceTransfer_p.h>
#include <KisGpuPixelTolerance.h>
#include <gpu/KisGpuExecution.h>
#include <kis_paint_device.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <QSignalSpy>

namespace Gpu = LibrePaint::Image::Gpu;

class KisGpuCpuLayerVersionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void definitionAndPixelsShareHistory_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8-clone-settings-and-pixels") << false;
        QTest::newRow("u16-style-pixels-only-cancel-and-reject") << true;
    }
    void definitionAndPixelsShareHistory();
};

void KisGpuCpuLayerVersionTest::definitionAndPixelsShareHistory()
{
    QFETCH(bool, boundary);
    const auto feature = boundary ? Gpu::LayerFeature::LayerStyle : Gpu::LayerFeature::Clone;
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const qsizetype capacity = 8 * Gpu::RgbaTile::byteCount(format);
    const QSize size(270, 48);
    const QRect area = boundary ? QRect(-8, -8, 288, 64) : QRect(QPoint(), size);
    KisPaintDeviceSP native = new KisPaintDevice(space);
    if (boundary) native->setDefaultPixel(KoColor(QColor(13, 37, 59, 83), space));
    native->fill({252, 3, 13, 19}, KoColor(QColor(43, 157, 211, 173), space));
    Gpu::RasterImage source;
    source.size = size;
    source.format = format;
    source.colorProfile = space->profile()->rawData();
    Gpu::RasterLayer layer;
    layer.uuid = QUuid::createUuid();
    layer.name = QStringLiteral("Native CPU layer");
    layer.position = {7, -3};
    layer.opacity = 197;
    layer.userLocked = true;
    layer.cpuDefinition = Gpu::CpuLayerDefinition{QStringLiteral("native-owner"), 1,
        QByteArrayLiteral("<settings value='before'/>")};
    layer.cpuDefinition->feature = feature;
    layer.cpuDefinition->selection = Gpu::SelectionMask::uniform(173);
    QVERIFY(kisGpuRenderCpuLayer(feature, layer, size, space, capacity,
        [&](KisPaintDeviceSP) { return native; }));
    QVERIFY(layer.cpuDefinition->resolved);
    source.layers = {layer};
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::Limits limits;
    limits.rasterBytes = capacity;
    const auto owner = Gpu::PaintImageSP::create(execution, source, limits);
    const auto id = owner->operations().snapshot()->layers.first().id;
    const auto read = [&](const Gpu::Snapshot &version) {
        return execution->readRgbaPixels(owner->handle(), version, area, id, capacity);
    };
    QCOMPARE(execution->beginReadbackFrame().code, Gpu::ExecutionCode::Success);
    const auto projected = execution->project(owner->handle(), QRect(QPoint(), size));
    QVERIFY2(projected.code == Gpu::RenderOutcome::Recorded, qPrintable(projected.diagnostic));
    QCOMPARE(execution->submitFrame().code, Gpu::ExecutionCode::Success);
    const auto original = read(owner->operations().snapshot());
    QCOMPARE(original.code, Gpu::ExecutionCode::Success);
    QCOMPARE(owner->renameLayer(id, QStringLiteral("Redo entry")), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    QVERIFY(before->layers.first().pixels);
    QVERIFY(before->layers.first().operations.isEmpty());
    const auto untouched = owner->beginCpuLayerEdit(id);
    QVERIFY(!untouched.isNull());
    QCOMPARE(owner->commitCpuLayerEdit(untouched, layer.name), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!owner->operations().isModified());

    auto edit = owner->beginCpuLayerEdit(id);
    QVERIFY(!edit.isNull());
    auto expected = owner->operations().preview();
    auto rendered = layer;
    if (!boundary) {
        rendered.cpuDefinition->xml = QByteArrayLiteral("<settings value='after'/>");
        rendered.cpuDefinition->selection = Gpu::SelectionMask::uniform(211);
    }
    rendered.cpuDefinition->resolved = false;
    QVERIFY(kisGpuRenderCpuLayer(feature, rendered, size, space, capacity,
        [&](KisPaintDeviceSP input) {
            if (input) input->fill({2, 3, 7, 5}, KoColor(QColor(211, 83, 17, 121), space));
            return input;
        }, {owner, expected, id}));
    QCOMPARE(owner->replaceCpuLayer(id, rendered, expected), Gpu::Result::Success);
    const auto preview = owner->operations().preview();
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!owner->operations().isModified());
    QVERIFY(preview->layers.first().cpuDefinition == rendered.cpuDefinition);
    QVERIFY(before->layers.first().cpuDefinition == layer.cpuDefinition);
    const auto shown = read(preview);
    QCOMPARE(shown.code, Gpu::ExecutionCode::Success);
    QVERIFY(shown.bytes != original.bytes);
    if (boundary) {
        QSignalSpy changed(owner.data(), &Gpu::PaintImage::changed);
        QCOMPARE(owner->replaceCpuLayer(id, rendered, expected), Gpu::Result::StaleVersion);
        auto invalid = rendered;
        invalid.cpuDefinition->resolved = false;
        QCOMPARE(owner->replaceCpuLayer(id, invalid, preview), Gpu::Result::InvalidInput);
        invalid = rendered;
        invalid.cpuDefinition->feature = Gpu::LayerFeature::File;
        QCOMPARE(owner->replaceCpuLayer(id, invalid, preview), Gpu::Result::InvalidInput);
        invalid = rendered;
        invalid.tiles.append(invalid.tiles.first());
        QCOMPARE(owner->replaceCpuLayer(id, invalid, preview), Gpu::Result::InvalidInput);
        invalid = rendered;
        while (invalid.tiles.size() <= owner->limits().rasterBytes / Gpu::RgbaTile::byteCount(format))
            invalid.tiles.append(rendered.tiles.first());
        QCOMPARE(owner->replaceCpuLayer(id, invalid, preview), Gpu::Result::CapacityExceeded);
        QCOMPARE(changed.size(), 0);
        QCOMPARE(owner->operations().preview(), preview);
        QCOMPARE(owner->cancelCpuLayerEdit(edit), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(owner->operations().redoCount(), 1);
        QVERIFY(!owner->operations().isModified());
        const auto cancelled = read(before);
        QCOMPARE(cancelled.code, Gpu::ExecutionCode::Success);
        QCOMPARE_PIXELS16_WITHIN_STEP(cancelled.bytes, original.bytes);
        edit = owner->beginCpuLayerEdit(id);
        expected = owner->operations().preview();
        QCOMPARE(owner->replaceCpuLayer(id, rendered, expected), Gpu::Result::Success);
    }
    const auto name = boundary ? layer.name : QStringLiteral("Edited CPU layer");
    QCOMPARE(owner->commitCpuLayerEdit(edit, name), Gpu::Result::Success);
    const auto after = owner->operations().snapshot();
    const auto &adopted = after->layers.first();
    QVERIFY(adopted.cpuDefinition == rendered.cpuDefinition);
    QCOMPARE(adopted.name, name);
    QCOMPARE(adopted.uuid, layer.uuid);
    QCOMPARE(adopted.position, layer.position);
    QCOMPARE(adopted.opacity, layer.opacity);
    QCOMPARE(adopted.userLocked, layer.userLocked);
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().redoCount(), 0);
    QVERIFY(owner->operations().isModified());
    native->fill({2, 3, 7, 5}, KoColor(QColor(211, 83, 17, 121), space));
    QByteArray nativeBytes(qsizetype(area.width()) * area.height() * space->pixelSize(), Qt::Uninitialized);
    native->readBytes(reinterpret_cast<quint8 *>(nativeBytes.data()), area);
    const auto adoptedPixels = read(after);
    QCOMPARE(adoptedPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuPixelTransfer(adoptedPixels.bytes, format, false), nativeBytes, boundary ? 2 : 1);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->operations().snapshot()->layers.first().cpuDefinition == layer.cpuDefinition);
    QVERIFY(!owner->operations().isModified());
    const auto undone = read(owner->operations().snapshot());
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(undone.bytes, original.bytes, boundary ? 2 : 1);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(owner->operations().snapshot()->layers.first().cpuDefinition == rendered.cpuDefinition);
    const auto redone = read(owner->operations().snapshot());
    QCOMPARE(redone.code, Gpu::ExecutionCode::Success);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(redone.bytes, adoptedPixels.bytes, boundary ? 2 : 1);
    const auto pinned = read(before);
    QCOMPARE(pinned.code, Gpu::ExecutionCode::Success);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pinned.bytes, original.bytes, boundary ? 2 : 1);
}

KISTEST_MAIN(KisGpuCpuLayerVersionTest)
#include "KisGpuCpuLayerVersionTest.moc"
