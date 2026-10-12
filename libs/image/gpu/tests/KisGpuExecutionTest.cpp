/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QWindow>
#include <rhi/qrhi.h>
#include <gpu/KisGpuExecution.h>

using namespace Krita::Image::Gpu;

class KisGpuExecutionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void latestRequestsRetainInputAndPublishAfterSubmission();
    void cancellationAndClosureInvalidateRecordedFrames();
    void replacingTheDeviceRebuildsAcceptedDocuments();
    void documentsShareTheTileBudgetAndKeepPublishedPixelsOnFailure();
    void presentationFramesSubmitThroughTheOwningWindow();
    void unconfiguredSurfacesAreRejectedBeforeNativeSubmission();
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
    qInfo() << "GPU backend:" << m_device->rhi().backendName();
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
    QCOMPARE(execution.requested(admitted.handle)->sampleCount, 240);
    QVERIFY(!execution.published(admitted.handle));
    const auto frame = execution.beginReadbackFrame();
    QCOMPARE(frame.code, ExecutionCode::Success);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Busy);
    const auto projection = execution.project(admitted.handle, {0, 0, 256, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QVERIFY(!execution.published(admitted.handle));
    QRhiReadbackResult pixels;
    auto updates = m_device->rhi().nextResourceUpdateBatch();
    updates->readBackBuffer(projection.version->tiles().first().pixels, 0, TileRenderer::TileBytes, &pixels);
    frame.commands->resourceUpdate(updates);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(admitted.handle), document.preview());
    QCOMPARE(execution.pendingDocuments(), 0);
    QCOMPARE(pixels.data.mid((32 * 256 + 247) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixels.data.mid((32 * 256 + 252) * 4, 4), QByteArray(4, '\0'));
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

void KisGpuExecutionTest::replacingTheDeviceRebuildsAcceptedDocuments()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    dot(document, {32, 32});
    const auto handle = execution.open(document.snapshot()).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    const auto old = execution.project(handle, {0, 0, 64, 64});
    QCOMPARE(old.code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    const auto generation = execution.generation();
    QString diagnostic;
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Busy);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    QCOMPARE(execution.generation(), generation + 1);
    QCOMPARE(execution.published(handle), document.snapshot());
    QCOMPARE(execution.pendingDocuments(), 1);
    const auto frame = execution.beginReadbackFrame();
    QCOMPARE(frame.code, ExecutionCode::Success);
    const auto rebuilt = execution.project(handle, {0, 0, 64, 64});
    QCOMPARE(rebuilt.code, RenderOutcome::Recorded);
    QVERIFY(rebuilt.version->tiles().first().pixels->rhi() != old.version->tiles().first().pixels->rhi());
    QRhiReadbackResult pixels;
    auto updates = replacement->rhi().nextResourceUpdateBatch();
    updates->readBackBuffer(rebuilt.version->tiles().first().pixels, 0, TileRenderer::TileBytes, &pixels);
    frame.commands->resourceUpdate(updates);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(pixels.data.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(execution.pendingDocuments(), 0);
}

void KisGpuExecutionTest::documentsShareTheTileBudgetAndKeepPublishedPixelsOnFailure()
{
    Execution execution(m_device, 2 * TileRenderer::TileBytes);
    OperationDocument a({64, 64}), b({64, 64});
    dot(a, {16, 16});
    dot(b, {32, 32});
    const auto ha = execution.open(a.snapshot()).handle;
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
    const auto frame = execution.beginReadbackFrame();
    QCOMPARE(frame.code, ExecutionCode::Success);
    const auto projection = execution.project(hb, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QRhiReadbackResult pixels;
    auto updates = m_device->rhi().nextResourceUpdateBatch();
    updates->readBackBuffer(projection.version->tiles().first().pixels, 0, TileRenderer::TileBytes, &pixels);
    frame.commands->resourceUpdate(updates);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(pixels.data.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixels.data.mid((48 * 256 + 48) * 4, 4), QByteArray::fromHex("000000ff"));
}

void KisGpuExecutionTest::presentationFramesSubmitThroughTheOwningWindow()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    const auto handle = execution.open(document.snapshot()).handle;
    QWindow window;
    if (m_device->rhi().backend() == QRhi::Metal) window.setSurfaceType(QSurface::MetalSurface);
    else {
        window.setSurfaceType(QSurface::VulkanSurface);
        window.setVulkanInstance(m_device->vulkanInstance());
    }
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    std::unique_ptr<QRhiSwapChain> surface(m_device->rhi().newSwapChain());
    surface->setWindow(&window);
    std::unique_ptr<QRhiRenderPassDescriptor> pass(surface->newCompatibleRenderPassDescriptor());
    surface->setRenderPassDescriptor(pass.get());
    QVERIFY(surface->createOrResize());
    const auto frame = execution.beginPresentationFrame(*surface);
    QCOMPARE(frame.code, ExecutionCode::Success);
    QVERIFY(frame.commands);
    QVERIFY(frame.target);
    const auto projection = execution.project(handle, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    frame.commands->beginPass(frame.target, Qt::white, {1, 0});
    frame.commands->endPass();
    QVERIFY(!execution.published(handle));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(handle), document.snapshot());
    QCOMPARE(execution.pendingDocuments(), 0);
    QString diagnostic;
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    QCOMPARE(execution.beginPresentationFrame(*surface).code, ExecutionCode::InvalidRequest);
}

void KisGpuExecutionTest::unconfiguredSurfacesAreRejectedBeforeNativeSubmission()
{
    Execution execution(m_device);
    std::unique_ptr<QRhiSwapChain> surface(m_device->rhi().newSwapChain());
    QCOMPARE(execution.beginPresentationFrame(*surface).code, ExecutionCode::SurfaceOutOfDate);
    QVERIFY(!m_device->rhi().isRecordingFrame());
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuExecutionTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuExecutionTest.moc"
