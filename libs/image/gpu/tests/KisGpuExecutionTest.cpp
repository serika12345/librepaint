/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QWindow>
#include <limits>
#include <webgpu/wgpu.h>
#include <gpu/KisGpuExecution.h>
#include "../KisGpuReadbackBatch_p.h"

using namespace LibrePaint::Image::Gpu;

class KisGpuExecutionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void nativeValidationErrorsReportAnErrorAndReleaseTheOwner();
    void abandonedReadbackResultsRemainValidThroughNativeCleanup();

    void latestRequestsRetainInputAndPublishAfterSubmission();
    void cancellationAndClosureInvalidateRecordedFrames();
    void documentsShareTheTileBudgetAndKeepPublishedPixelsOnFailure();
    void viewportsComposeVisibleTilesFromCommittedLayers();
    void committedPixelsPreserveHistoryAndTrimByStorage();
    void commitsBeforeRenderingKeepEveryStroke();
private:
    DevicePtr m_device;
};

namespace {
void dot(OperationDocument &document, QPointF point)
{
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{point, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
}
}

void KisGpuExecutionTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
}

void KisGpuExecutionTest::nativeValidationErrorsReportAnErrorAndReleaseTheOwner()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const std::weak_ptr<Device> lifetime = device;
    // wgpu-native aborts on wait, mapping and submission errors, so inject a
    // validation error that it reports through the application handler.
    WGPUBufferDescriptor invalid{};
    invalid.size = 256;
    invalid.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_Storage;
    Detail::NativeBuffer buffer(wgpuDeviceCreateBuffer(device->nativeDevice(), &invalid));
    QVERIFY(!device->diagnostic().isEmpty());
    QVERIFY(!device->isLost());
    QVERIFY(!device->beginCommands());
    buffer.reset();
    device.reset();
    QVERIFY(lifetime.expired());
}

void KisGpuExecutionTest::viewportsComposeVisibleTilesFromCommittedLayers()
{
    OperationDocument document(QSize(9 * TileRenderer::TileSize, 64));
    for (int tile = 0; tile < 9; ++tile) dot(document, {qreal(tile * 256 + 8), 12});
    const auto hidden = document.addLayer(QStringLiteral("Hidden"));
    dot(document, {8, 12});
    QCOMPARE(document.setLayerVisible(hidden, false), Result::Success);
    Execution execution(m_device, 10 * TileRenderer::TileBytes);
    const auto handle = execution.open(document.snapshot()).handle;
    for (const int tile : {0, 8, 0}) {
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        const auto projected = execution.project(handle, {tile * 256 + 8, 12, 1, 1});
        QVERIFY2(projected.code == RenderOutcome::Recorded, qPrintable(projected.diagnostic));
        QCOMPARE(projected.version->tiles().size(), 1);
        QCOMPARE(projected.version->tiles()[0].coordinate, QPoint(tile, 0));
        QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
        QCOMPARE(execution.published(handle), document.snapshot());
        QVERIFY(execution.residentBytes() <= 10 * TileRenderer::TileBytes);
    }
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    const auto empty = execution.project(handle, {-10, -10, 1, 1});
    QCOMPARE(empty.code, RenderOutcome::Recorded);
    QVERIFY(empty.version->tiles().isEmpty());
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
}

void KisGpuExecutionTest::latestRequestsRetainInputAndPublishAfterSubmission()
{
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 1);
    OperationDocument document({256, 64});
    const auto admitted = execution.open(document.snapshot());
    QCOMPARE(admitted.code, ExecutionCode::Success);
    QVERIFY(admitted.handle != 0);
    OperationDocument extra({64, 64});
    QCOMPARE(execution.open(extra.snapshot()).code, ExecutionCode::CapacityExceeded);
    Brush brush;
    brush.diameter = 4;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    for (int i = 0; i < 240; ++i) {
        InputSample sample;
        sample.position = {qreal(8 + i), 32};
        sample.pressure = 1;
        sample.timestamp = i;
        QCOMPARE(document.appendSamples(stroke, {sample}), Result::Success);
        QCOMPARE(execution.request(admitted.handle, document.preview()).code, ExecutionCode::Success);
        QCOMPARE(execution.pendingDocuments(), 1);
    }

    QVERIFY(!execution.published(admitted.handle));
    const auto frame = execution.beginReadbackFrame();
    QCOMPARE(frame.code, ExecutionCode::Success);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Busy);
    const auto projection = execution.project(admitted.handle, {0, 0, 256, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QVERIFY(!execution.published(admitted.handle));
    const auto pixels = Detail::ReadbackBatch::create(*m_device, 1);
    QVERIFY(pixels);
    QVERIFY(pixels->recordBuffer(*m_device, frame.commands, 0, projection.version->tiles().first().pixels, TileRenderer::TileBytes));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(admitted.handle), document.preview());
    QCOMPARE(execution.pendingDocuments(), 0);
    QCOMPARE(pixels->results.front()->data.mid((32 * 256 + 247) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixels->results.front()->data.mid((32 * 256 + 252) * 4, 4), QByteArray(4, '\0'));
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(execution.request(admitted.handle, document.snapshot()).code, ExecutionCode::Success);
    QCOMPARE(execution.pendingDocuments(), 0);
}

void KisGpuExecutionTest::cancellationAndClosureInvalidateRecordedFrames()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    const auto handle = execution.open(document.snapshot()).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(handle, {0, 0, 64, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    const auto original = execution.published(handle);
    auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(execution.request(handle, document.preview()).code, ExecutionCode::Success);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(handle, {0, 0, 64, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::Success);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.published(handle), original);
    QCOMPARE(execution.pendingDocuments(), 0);

    dot(document, {24, 24});
    QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::Success);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(handle, {0, 0, 64, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(handle), original);
    QCOMPARE(execution.pendingDocuments(), 1);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(handle, {0, 0, 64, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::InvalidRequest);
    QVERIFY(!execution.published(handle));
    QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::InvalidRequest);
    const auto reopened = execution.open(document.snapshot());
    QCOMPARE(reopened.code, ExecutionCode::Success);
    QVERIFY(reopened.handle != handle);
}

void KisGpuExecutionTest::documentsShareTheTileBudgetAndKeepPublishedPixelsOnFailure()
{
    Execution execution(m_device, 2 * TileRenderer::TileBytes);
    auto a = std::make_unique<OperationDocument>(QSize(64, 64));
    OperationDocument b({64, 64});
    dot(*a, {16, 16});
    dot(b, {32, 32});
    const auto ha = execution.open(a->snapshot()).handle;
    const auto hb = execution.open(b.snapshot()).handle;
    for (const auto handle : {ha, hb}) {
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        QCOMPARE(execution.project(handle, {0, 0, 64, 64}).code, RenderOutcome::Recorded);
        QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    }
    QCOMPARE(execution.residentBytes(), 2 * TileRenderer::TileBytes);
    const auto original = execution.published(hb);
    dot(b, {48, 48});
    QCOMPARE(execution.request(hb, b.snapshot()).code, ExecutionCode::Success);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(hb, {0, 0, 64, 64}).code, RenderOutcome::CapacityExceeded);
    QCOMPARE(execution.project(ha, {0, 0, 64, 64}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(hb), original);
    QCOMPARE(execution.pendingDocuments(), 1);
    QCOMPARE(execution.residentBytes(), 2 * TileRenderer::TileBytes);
    QCOMPARE(execution.close(ha).code, ExecutionCode::Success);
    a.reset();
    const auto frame = execution.beginReadbackFrame();
    QCOMPARE(frame.code, ExecutionCode::Success);
    const auto projection = execution.project(hb, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto pixels = Detail::ReadbackBatch::create(*m_device, 1);
    QVERIFY(pixels);
    QVERIFY(pixels->recordBuffer(*m_device, frame.commands, 0, projection.version->tiles().first().pixels, TileRenderer::TileBytes));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(pixels->results.front()->data.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixels->results.front()->data.mid((48 * 256 + 48) * 4, 4), QByteArray::fromHex("000000ff"));
}

void KisGpuExecutionTest::abandonedReadbackResultsRemainValidThroughNativeCleanup()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    std::weak_ptr<Device> nativeOwner = device;
    std::weak_ptr<Detail::ReadbackBatch> resultOwner;
    QByteArray observed;
    int completions = 0;
    {
        Execution execution(device);
        OperationDocument document({64, 64});
        const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
        TileRenderer renderer(device);
        const auto commands = device->beginCommands();
        QVERIFY(commands);
        const auto rendered = renderer.record(commands, document.snapshot());
        QCOMPARE(rendered.code, RenderOutcome::Recorded);
        const auto projection = renderer.compose(commands, rendered.version, {0, 0, 64, 64});
        QCOMPARE(projection.code, RenderOutcome::Recorded);
        auto batch = Detail::ReadbackBatch::create(*device, 1);
        QVERIFY(batch);
        resultOwner = batch;
        QVERIFY(!Detail::ReadbackBatch::create(*device, 1));
        auto result = batch->results.front().get();
        result->completed = [result, &observed, &completions] {
            observed = result->data;
            ++completions;
        };
        QVERIFY(batch->recordBuffer(*device, commands, 0, projection.version->tiles().first().pixels, TileRenderer::TileBytes));
        batch.reset();
        QVERIFY(!resultOwner.expired());
        QVERIFY(device->submitCommands(false));
    }
    QVERIFY(resultOwner.expired());
    device.reset();
    QVERIFY(nativeOwner.expired());
    QVERIFY(resultOwner.expired());
    QCOMPARE(completions, 1);
    QCOMPARE(observed.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("000000ff"));
}

void KisGpuExecutionTest::committedPixelsPreserveHistoryAndTrimByStorage()
{
    Limits limits;
    limits.historyBytes = TileRenderer::TileBytes;
    OperationDocument document(QSize(64, 64), limits);
    Execution execution(m_device);
    const auto handle = execution.open(document.snapshot()).handle;
    const auto layer = document.activeLayer();
    for (const int position : {8, 24, 40}) {
        dot(document, {qreal(position), qreal(position)});
        QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::Success);
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        const auto projection = execution.project(handle, {0, 0, 64, 64});
        QVERIFY2(projection.code == RenderOutcome::Recorded, qPrintable(projection.diagnostic));
        QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
        QVERIFY(document.snapshot()->renderedPixels(layer));
    }
    // Property versions share all tiles and do not consume additional history bytes.
    for (int index = 0; index < 3; ++index) {
        QCOMPARE(document.renameLayer(layer, QString::number(index)), Result::Success);
        QVERIFY(document.snapshot()->layers.first().pixels);
        QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    }
    QCOMPARE(document.undoCount(), 4);
    for (int index = 0; index < 4; ++index) QCOMPARE(document.undo(), Result::Success);
    auto pixels = execution.readRgbaPixels(handle, document.snapshot(), {0, 0, 64, 64}, layer, 64 * 64 * 4);
    QCOMPARE(pixels.code, ExecutionCode::Success);
    QCOMPARE(pixels.bytes.mid((24 * 64 + 24) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixels.bytes.mid((40 * 64 + 40) * 4, 4), QByteArray(4, '\0'));
    for (int index = 0; index < 4; ++index) QCOMPARE(document.redo(), Result::Success);
    pixels = execution.readRgbaPixels(handle, document.snapshot(), {0, 0, 64, 64}, layer, 64 * 64 * 4);
    QCOMPARE(pixels.code, ExecutionCode::Success);
    QCOMPARE(pixels.bytes.mid((40 * 64 + 40) * 4, 4), QByteArray::fromHex("000000ff"));
}

void KisGpuExecutionTest::commitsBeforeRenderingKeepEveryStroke()
{
    OperationDocument document({512, 64});
    const auto layer = document.activeLayer();
    for (const int x : {8, 280, 400}) dot(document, {qreal(x), 24});
    Execution execution(m_device);
    const auto handle = execution.open(document.snapshot()).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    const auto projection = execution.project(handle, {0, 0, 1, 1});
    QVERIFY2(projection.code == RenderOutcome::Recorded, qPrintable(projection.diagnostic));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    dot(document, {480, 24});
    QVERIFY(document.snapshot()->layers.first().pixels);
    QCOMPARE(document.snapshot()->layers.first().operations.size(), 1);
    auto pixels = execution.readRgbaPixels(handle, document.snapshot(), {0, 0, 512, 64}, layer, 512 * 64 * 4);
    QCOMPARE(pixels.code, ExecutionCode::Success);
    for (const int x : {8, 280, 400, 480})
        QCOMPARE(pixels.bytes.mid((24 * 512 + x) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(document.undo(), Result::Success);
    pixels = execution.readRgbaPixels(handle, document.snapshot(), {0, 0, 512, 64}, layer, 512 * 64 * 4);
    QCOMPARE(pixels.code, ExecutionCode::Success);
    QCOMPARE(pixels.bytes.mid((24 * 512 + 400) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixels.bytes.mid((24 * 512 + 480) * 4, 4), QByteArray(4, '\0'));
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuExecutionTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuExecutionTest.moc"
