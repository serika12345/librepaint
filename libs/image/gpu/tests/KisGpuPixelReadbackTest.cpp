/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <rhi/qrhi.h>
#include <gpu/KisGpuExecution.h>

using namespace Krita::Image::Gpu;

class KisGpuPixelReadbackTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void pinnedPixelsKeepNewerRequestsAndPublishedState();
    void hiddenLayersAndVisibleProjectionRemainDistinct();
    void boundedReadsRejectAtomicallyAndRecover();
    void closedDocumentsAndDeviceGenerationsGuardReads();
    void cpuPixelsOutliveExecutionAndDevices();
private:
    DevicePtr m_device;
};

namespace {
void dot(OperationDocument &document, QPointF point, std::array<quint8, 4> rgba)
{
    Brush brush;
    brush.rgba = rgba;
    brush.diameter = 6;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{point, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
}

QByteArray pixel(const PixelReadbackOutcome &read, LayerId layer, QPoint point)
{
    for (const auto &tile : read.tiles) {
        const QPoint origin = tile.coordinate * TileRenderer::TileSize;
        if (tile.layer == layer && QRect(origin, QSize(TileRenderer::TileSize, TileRenderer::TileSize)).contains(point)) {
            const auto local = point - origin;
            return tile.rgba.mid((local.y() * TileRenderer::TileSize + local.x()) * 4, 4);
        }
    }
    return {};
}
}

void KisGpuPixelReadbackTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
    qInfo() << "GPU backend:" << m_device->rhi().backendName();
}

void KisGpuPixelReadbackTest::pinnedPixelsKeepNewerRequestsAndPublishedState()
{
    Execution execution(m_device);
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto pinned = document.snapshot();
    const auto layer = document.activeLayer();
    const auto handle = execution.open(pinned).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(handle, {0, 0, 512, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    dot(document, {264, 12}, {0, 0, 255, 128});
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);

    const auto old = execution.readPixels(handle, pinned, {0, 0, 512, 64}, {layer});
    QCOMPARE(old.code, ExecutionCode::Success);
    QCOMPARE(old.document, pinned);
    QCOMPARE(old.generation, execution.generation());
    QCOMPARE(old.region, QRect(0, 0, 512, 64));
    QCOMPARE(old.tiles.size(), 2);
    QCOMPARE(pixel(old, layer, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(old, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(execution.requested(handle), latest);
    QCOMPARE(execution.published(handle), pinned);
    QCOMPARE(execution.pendingDocuments(), 1);
    const auto current = execution.readPixels(handle, latest, {256, 0, 256, 64}, {layer});
    QCOMPARE(current.code, ExecutionCode::Success);
    QCOMPARE(current.tiles.size(), 2);
    QCOMPARE(pixel(current, layer, {264, 12}), QByteArray::fromHex("0000ff80"));
    QCOMPARE(pixel(current, 0, {264, 12}), QByteArray::fromHex("0000ff80"));
    QCOMPARE(execution.requested(handle), latest);
    QCOMPARE(execution.published(handle), pinned);
    QCOMPARE(document.markSaved(pinned), Result::Success);
    QVERIFY(document.isModified());
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
}

void KisGpuPixelReadbackTest::hiddenLayersAndVisibleProjectionRemainDistinct()
{
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 16, 2 * TileRenderer::TileBytes);
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto second = document.addLayer(QStringLiteral("Hidden blue"));
    dot(document, {8, 12}, {0, 0, 255, 128});
    QCOMPARE(document.setLayerVisible(second, false), Result::Success);
    const auto pinned = document.snapshot();
    const auto handle = execution.open(pinned).handle;
    const auto read = execution.readPixels(handle, pinned, {-10, -10, 100, 100}, {first, second});
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.region, QRect(0, 0, 64, 64));
    QCOMPARE(read.tiles.size(), 3);
    QCOMPARE(pixel(read, first, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(read, second, {8, 12}), QByteArray::fromHex("0000ff80"));
    QCOMPARE(pixel(read, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    const auto rawLayer = execution.readPixels(handle, pinned, {0, 0, 64, 64}, {second}, false);
    QCOMPARE(rawLayer.code, ExecutionCode::Success);
    QCOMPARE(rawLayer.tiles.size(), 1);
    QCOMPARE(pixel(rawLayer, second, {8, 12}), QByteArray::fromHex("0000ff80"));
    QVERIFY(pixel(rawLayer, 0, {8, 12}).isEmpty());
    QVERIFY(!execution.published(handle));
    QCOMPARE(execution.pendingDocuments(), 1);
    QCOMPARE(document.setLayerVisible(second, true), Result::Success);
    const auto visible = execution.readPixels(handle, document.snapshot(), {0, 0, 64, 64}, {first, second});
    QCOMPARE(visible.code, ExecutionCode::CapacityExceeded);
    QVERIFY(visible.tiles.isEmpty());
    const auto onlyProjection = execution.readPixels(handle, document.snapshot(), {0, 0, 64, 64});
    QCOMPARE(onlyProjection.code, ExecutionCode::Success);
    QCOMPARE(pixel(onlyProjection, 0, {8, 12}), QByteArray::fromHex("7f0080ff"));
}

void KisGpuPixelReadbackTest::boundedReadsRejectAtomicallyAndRecover()
{
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 16, TileRenderer::TileBytes);
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    dot(document, {264, 12}, {0, 255, 0, 255});
    const auto pinned = document.snapshot();
    const auto layer = document.activeLayer();
    const auto handle = execution.open(pinned).handle;
    const auto full = execution.readPixels(handle, pinned, {0, 0, 512, 64}, {layer});
    QCOMPARE(full.code, ExecutionCode::CapacityExceeded);
    QVERIFY(full.tiles.isEmpty());
    QVERIFY(!full.diagnostic.isEmpty());
    QVERIFY(!m_device->rhi().isRecordingFrame());
    QCOMPARE(execution.requested(handle), pinned);
    QVERIFY(!execution.published(handle));
    const auto partial = execution.readPixels(handle, pinned, {256, 0, 256, 64}, {layer});
    QCOMPARE(partial.code, ExecutionCode::Success);
    QCOMPARE(partial.tiles.size(), 2);
    QCOMPARE(pixel(partial, layer, {264, 12}), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}, {layer, layer}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}, {999}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, -1, 64}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}).code, ExecutionCode::Busy);
    QCOMPARE(execution.project(handle, {0, 0, 512, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(handle), pinned);
}

void KisGpuPixelReadbackTest::closedDocumentsAndDeviceGenerationsGuardReads()
{
    Execution execution(m_device);
    OperationDocument document({64, 64}), other({64, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto pinned = document.snapshot();
    const auto handle = execution.open(pinned).handle;
    QCOMPARE(execution.readPixels(handle, other.snapshot(), {0, 0, 64, 64}).code, ExecutionCode::InvalidRequest);
    const auto empty = execution.readPixels(handle, pinned, {128, 128, 64, 64});
    QCOMPARE(empty.code, ExecutionCode::Success);
    QVERIFY(empty.region.isEmpty());
    QVERIFY(empty.tiles.isEmpty());
    const auto before = execution.readPixels(handle, pinned, {0, 0, 64, 64});
    QCOMPARE(before.code, ExecutionCode::Success);
    QString diagnostic;
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    const auto rebuilt = execution.readPixels(handle, pinned, {0, 0, 64, 64});
    QCOMPARE(rebuilt.code, ExecutionCode::Success);
    QVERIFY(rebuilt.generation > before.generation);
    QCOMPARE(pixel(rebuilt, 0, {8, 12}), pixel(before, 0, {8, 12}));
    QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(pixel(before, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
}

void KisGpuPixelReadbackTest::cpuPixelsOutliveExecutionAndDevices()
{
    PixelReadbackOutcome cpu;
    std::weak_ptr<Device> owner;
    {
        QString diagnostic;
        auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        owner = device;
        Execution execution(device);
        OperationDocument document({64, 64});
        dot(document, {8, 12}, {255, 0, 0, 255});
        const auto handle = execution.open(document.snapshot()).handle;
        cpu = execution.readPixels(handle, document.snapshot(), {0, 0, 64, 64});
        QCOMPARE(cpu.code, ExecutionCode::Success);
        device.reset();
        QVERIFY(!owner.expired());
    }
    QVERIFY(owner.expired());
    QVERIFY(cpu.document);
    QCOMPARE(pixel(cpu, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuPixelReadbackTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPixelReadbackTest.moc"
