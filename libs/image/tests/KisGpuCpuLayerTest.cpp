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
#include <stdexcept>

namespace Gpu = LibrePaint::Image::Gpu;

namespace {
QByteArray pixels(KisPaintDeviceSP device, QRect area)
{
    QByteArray result(qsizetype(area.width()) * area.height() * device->pixelSize(), Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(result.data()), area);
    return result;
}

bool sameRaster(const Gpu::RasterLayer &left, const Gpu::RasterLayer &right)
{
    if (left.defaultPixel != right.defaultPixel || left.writtenAreas != right.writtenAreas ||
        left.tiles.size() != right.tiles.size() || left.name != right.name || left.uuid != right.uuid ||
        left.position != right.position || left.opacity != right.opacity) return false;
    for (int i = 0; i < left.tiles.size(); ++i)
        if (left.tiles[i].coordinate != right.tiles[i].coordinate || left.tiles[i].format != right.tiles[i].format ||
            left.tiles[i].rgba != right.tiles[i].rgba) return false;
    return true;
}
}

class KisGpuCpuLayerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void cpuRenderUsesIndependentVersionPixels_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8-composition") << false;
        QTest::newRow("u16-sparse-default-rejection") << true;
    }
    void cpuRenderUsesIndependentVersionPixels();
};

void KisGpuCpuLayerTest::cpuRenderUsesIndependentVersionPixels()
{
    QFETCH(bool, boundary);
    for (const auto feature : {Gpu::LayerFeature::Paint, Gpu::LayerFeature::Group,
            Gpu::LayerFeature::TransparencyMask, Gpu::LayerFeature::SelectionMask})
        QCOMPARE(Gpu::layerExecutionLocation(feature), Gpu::ExecutionLocation::Gpu);
    for (const auto feature : {Gpu::LayerFeature::Adjustment, Gpu::LayerFeature::Generator,
            Gpu::LayerFeature::Clone, Gpu::LayerFeature::File, Gpu::LayerFeature::Vector,
            Gpu::LayerFeature::Text, Gpu::LayerFeature::LayerStyle, Gpu::LayerFeature::ColorizeMask})
        QCOMPARE(Gpu::layerExecutionLocation(feature), Gpu::ExecutionLocation::Cpu);
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const qsizetype tileBytes = Gpu::RgbaTile::byteCount(format);
    const qsizetype capacity = 8 * tileBytes;
    const QSize size(270, 48);
    const QRect inspected = boundary ? QRect(-264, -16, 544, 80) : QRect(QPoint(), size);
    KisPaintDeviceSP native = new KisPaintDevice(space);
    if (boundary) {
        const quint16 bgra[] = {591, 12345, 54321, 23456};
        native->setDefaultPixel(KoColor(reinterpret_cast<const quint8 *>(bgra), space));
        native->fill({-258, -7, 5, 3}, KoColor(QColor(193, 37, 79, 211), space));
    }
    native->fill({252, 3, 13, 19}, KoColor(QColor(43, 157, 211, 173), space));
    Gpu::RasterImage source;
    source.size = size;
    source.format = format;
    source.colorProfile = space->profile()->rawData();
    Gpu::RasterLayer sourceLayer;
    sourceLayer.name = QStringLiteral("Source");
    source.layers = {sourceLayer};
    QVERIFY(kisGpuRasterLayerFromPaintDevice(native, format, capacity, source.layers.first()));
    // Two roots exercise the composed-input path after committed operations
    // have been replaced by their GPU pixel version.
    if (!boundary) {
        Gpu::RasterLayer upper;
        upper.name = QStringLiteral("Empty upper layer");
        source.layers.append(upper);
    }
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    auto owner = Gpu::PaintImageSP::create(execution, source);
    const auto initial = owner->operations().snapshot();
    const auto layerId = initial->layers.first().id;
    QCOMPARE(execution->beginReadbackFrame().code, Gpu::ExecutionCode::Success);
    const auto projected = execution->project(owner->handle(), QRect(QPoint(), size));
    QVERIFY2(projected.code == Gpu::RenderOutcome::Recorded, qPrintable(projected.diagnostic));
    QCOMPARE(execution->submitFrame().code, Gpu::ExecutionCode::Success);
    const auto original = execution->readRgbaPixels(owner->handle(), initial, inspected, boundary ? layerId : 0, capacity);
    QCOMPARE(original.code, Gpu::ExecutionCode::Success);
    QCOMPARE(owner->renameLayer(layerId, QStringLiteral("Source renamed")), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto fixed = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(fixed), Gpu::Result::Success);
    QVERIFY(fixed->layers.first().pixels);
    QVERIFY(fixed->layers.first().operations.isEmpty());
    KisGpuCpuLayerContext context{owner, fixed, boundary ? layerId : Gpu::LayerId(0)};
    Gpu::RasterLayer rendered;
    rendered.name = QStringLiteral("CPU result");
    rendered.uuid = QUuid::createUuid();
    rendered.opacity = 197;
    rendered.position = {7, -3};
    bool independent = false;
    auto compute = [&](KisPaintDeviceSP input) -> KisPaintDeviceSP {
        independent = input && input->defaultBounds()->bounds() == QRect(QPoint(), size) &&
            pixelStepDifference(pixels(input, inspected), kisGpuPixelTransfer(original.bytes, format, false), boundary ? 2 : 1).isEmpty();
        if (!independent) return {};
        input->fill({2, 3, 7, 5}, KoColor(QColor(211, 83, 17, 121), space));
        return input;
    };
    QVERIFY(kisGpuRenderCpuLayer(Gpu::LayerFeature::Clone, rendered, size, space, capacity, compute, context));
    QVERIFY(independent);
    QCOMPARE(owner->operations().snapshot(), fixed);
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!owner->operations().isModified());
    const auto sourcePixels = execution->readRgbaPixels(owner->handle(), fixed, inspected, boundary ? layerId : 0, capacity);
    QCOMPARE(sourcePixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuPixelTransfer(sourcePixels.bytes, format, false),
        kisGpuPixelTransfer(original.bytes, format, false), boundary ? 2 : 1);
    native->fill({2, 3, 7, 5}, KoColor(QColor(211, 83, 17, 121), space));
    auto result = source;
    result.layers = {rendered};
    auto uploaded = Gpu::PaintImageSP::create(execution, result);
    const auto read = uploaded->readRgbaPixels(uploaded->operations().snapshot()->layers.first().id, inspected, capacity);
    QCOMPARE(read.code, Gpu::ExecutionCode::Success);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuPixelTransfer(read.bytes, format, false), pixels(native, inspected), boundary ? 2 : 1);
    QCOMPARE(rendered.name, QStringLiteral("CPU result"));
    QCOMPARE(rendered.position, QPoint(7, -3));
    QCOMPARE(rendered.opacity, quint8(197));
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuPixelTransfer(Gpu::rgbaBytes(rendered.defaultPixel, format), format, false),
        kisGpuPixelTransfer(Gpu::rgbaBytes(source.layers.first().defaultPixel, format), format, false), boundary ? 2 : 1);

    if (boundary) {
        const auto accepted = rendered;
        QSignalSpy changed(owner.data(), &Gpu::PaintImage::changed);
        bool called = false;
        auto probe = [&](KisPaintDeviceSP input) -> KisPaintDeviceSP { called = true; return input; };
        QVERIFY(!kisGpuRenderCpuLayer(Gpu::LayerFeature::Clone, rendered, size, space, tileBytes - 1, probe, context));
        QVERIFY(!called);
        QVERIFY(sameRaster(rendered, accepted));
        QVERIFY(!kisGpuRenderCpuLayer(Gpu::LayerFeature::Generator, rendered, size, space, tileBytes,
            [&](KisPaintDeviceSP) -> KisPaintDeviceSP { return native; }));
        QVERIFY(sameRaster(rendered, accepted));
        QVERIFY(!kisGpuRenderCpuLayer(Gpu::LayerFeature::Clone, rendered, size, space, capacity,
            [](KisPaintDeviceSP input) -> KisPaintDeviceSP {
                input->clear();
                throw std::runtime_error("CPU render failed");
            }, context));
        QVERIFY(sameRaster(rendered, accepted));
        QCOMPARE(changed.size(), 0);
        QCOMPARE(owner->operations().snapshot(), fixed);
        QCOMPARE(owner->operations().redoCount(), 1);
        QVERIFY(!owner->operations().isModified());
        const auto unchanged = execution->readRgbaPixels(owner->handle(), fixed, inspected, layerId, capacity);
        QCOMPARE(unchanged.code, Gpu::ExecutionCode::Success);
        QCOMPARE_PIXELS16_WITHIN_STEP(kisGpuPixelTransfer(unchanged.bytes, format, false), kisGpuPixelTransfer(original.bytes, format, false));
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        const auto current = owner->operations().snapshot();
        called = false;
        QVERIFY(!kisGpuRenderCpuLayer(Gpu::LayerFeature::Clone, rendered, size, space, capacity, probe, context));
        QVERIFY(!called);
        QVERIFY(sameRaster(rendered, accepted));
        QCOMPARE(owner->operations().snapshot(), current);
    }
}

KISTEST_MAIN(KisGpuCpuLayerTest)
#include "KisGpuCpuLayerTest.moc"
