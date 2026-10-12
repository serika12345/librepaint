/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QTransform>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <gpu/KisGpuTilePresentation.h>
#include <gpu/KisGpuTileRenderer.h>
#include <limits>

using namespace Krita::Image::Gpu;
using namespace Krita::Canvas::Gpu;

class KisGpuTilePresentationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void panZoomAndTransparencyUseThePaintingQueue();
    void clippedTilesKeepTheirIndependentTransforms();
    void rejectedPresentationPreservesTheTarget();
    void projectionCannotCrossDevices();

private:
#if QT_CONFIG(vulkan)
    QVulkanInstance m_vulkan;
#endif
    std::unique_ptr<QRhi> m_rhi;
};

namespace {
struct Surface
{
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> target;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;
    bool ready = false;
    Surface(QRhi &rhi, QSize size)
        : texture(rhi.newTexture(QRhiTexture::RGBA8, size, 1,
              QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource))
    {
        if (!texture->create()) return;
        target.reset(rhi.newTextureRenderTarget({texture.get()}));
        pass.reset(target->newCompatibleRenderPassDescriptor());
        target->setRenderPassDescriptor(pass.get());
        ready = target->create();
    }
};

void dot(OperationDocument &document, QPointF point, std::array<quint8, 4> color)
{
    Brush brush;
    brush.rgba = color;
    brush.diameter = 4;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{point, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
}
}

void KisGpuTilePresentationTest::initTestCase()
{
#if QT_CONFIG(metal)
    QRhiMetalInitParams params;
    m_rhi.reset(QRhi::create(QRhi::Metal, &params));
#elif QT_CONFIG(vulkan)
    m_vulkan.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
    QVERIFY(m_vulkan.create());
    QRhiVulkanInitParams params;
    params.inst = &m_vulkan;
    m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));
#endif
    QVERIFY(m_rhi);
    qInfo() << "GPU backend:" << m_rhi->backendName();
}

void KisGpuTilePresentationTest::panZoomAndTransparencyUseThePaintingQueue()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 4;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{8, 12}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    brush.rgba = {0, 0, 255, 128};
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{20, 16}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);

    std::unique_ptr<QRhiTexture> texture(m_rhi->newTexture(QRhiTexture::RGBA8, {128, 128}, 1,
        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());
    std::unique_ptr<QRhiTextureRenderTarget> target(m_rhi->newTextureRenderTarget({texture.get()}));
    std::unique_ptr<QRhiRenderPassDescriptor> pass(target->newCompatibleRenderPassDescriptor());
    target->setRenderPassDescriptor(pass.get());
    QVERIFY(target->create());
    TileRenderer renderer(*m_rhi);
    TilePresentation presentation(*m_rhi);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto layers = renderer.record(*commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(*commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(*commands, projection.version, *target,
        QTransform::fromTranslate(8, 16).scale(2, 2), QColor(0, 0, 0, 0));
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    QRhiReadbackResult readback;
    auto updates = m_rhi->nextResourceUpdateBatch();
    updates->readBackTexture({texture.get()}, &readback);
    commands->resourceUpdate(updates);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    const auto pixel = [&](int x, int y) { return readback.data.mid((y * 128 + x) * 4, 4); };
    QCOMPARE(pixel(24, 40), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(48, 48), QByteArray::fromHex("00008080"));
    QCOMPARE(pixel(8, 12), QByteArray(4, '\0'));
    QCOMPARE(drawn.tileCount, 1);

    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto background = presentation.record(*commands, projection.version, *target,
        QTransform::fromTranslate(8, 16).scale(2, 2), QColor(0, 255, 0, 128));
    QVERIFY2(background.recorded, qPrintable(background.diagnostic));
    updates = m_rhi->nextResourceUpdateBatch();
    updates->readBackTexture({texture.get()}, &readback);
    commands->resourceUpdate(updates);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(pixel(48, 48), QByteArray::fromHex("004080c0"));
    QCOMPARE(pixel(8, 12), QByteArray::fromHex("00800080"));
}

void KisGpuTilePresentationTest::clippedTilesKeepTheirIndependentTransforms()
{
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    dot(document, {264, 12}, {0, 255, 0, 255});
    dot(document, {8, 30}, {0, 0, 255, 255});
    TileRenderer renderer(*m_rhi);
    TilePresentation presentation(*m_rhi);
    Surface surface(*m_rhi, {512, 64});
    QVERIFY(surface.ready);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto layers = renderer.record(*commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(*commands, layers.version, {4, 4, 272, 16});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(*commands, projection.version, *surface.target, {}, Qt::transparent);
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    QCOMPARE(drawn.tileCount, 2);
    QRhiReadbackResult readback;
    QRhiReadbackResult sourceReadback;
    auto updates = m_rhi->nextResourceUpdateBatch();
    updates->readBackBuffer(projection.version->tiles().first().pixels, 0, TileRenderer::TileBytes, &sourceReadback);
    updates->readBackTexture({surface.texture.get()}, &readback);
    commands->resourceUpdate(updates);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(sourceReadback.data.mid((12 * 256 + 8) * 4, 4), QByteArray::fromHex("ff0000ff"));
    auto pixel = [&](int x, int y) { return readback.data.mid((y * 512 + x) * 4, 4); };
    QCOMPARE(pixel(8, 12), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(264, 12), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(pixel(8, 30), QByteArray(4, '\0'));

    Surface rotated(*m_rhi, {64, 64});
    QVERIFY(rotated.ready);
    TilePresentation oneVisibleTile(*m_rhi, 1);
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto rotatedDraw = oneVisibleTile.record(*commands, projection.version, *rotated.target,
        QTransform(0, 1, -1, 0, 64, 0), Qt::transparent);
    QVERIFY2(rotatedDraw.recorded, qPrintable(rotatedDraw.diagnostic));
    QCOMPARE(rotatedDraw.tileCount, 1);
    updates = m_rhi->nextResourceUpdateBatch();
    updates->readBackTexture({rotated.texture.get()}, &readback);
    commands->resourceUpdate(updates);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(readback.data.mid((8 * 64 + 51) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(readback.data.mid((8 * 64 + 33) * 4, 4), QByteArray(4, '\0'));
}

void KisGpuTilePresentationTest::rejectedPresentationPreservesTheTarget()
{
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    dot(document, {264, 12}, {0, 255, 0, 255});
    TileRenderer renderer(*m_rhi);
    TilePresentation presentation(*m_rhi, 1);
    Surface surface(*m_rhi, {512, 64});
    QVERIFY(surface.ready);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto layers = renderer.record(*commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(*commands, layers.version, {0, 0, 512, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    commands->beginPass(surface.target.get(), QColor(7, 13, 29), {1, 0});
    commands->endPass();
    QVERIFY(!presentation.record(*commands, projection.version, *surface.target, {}, Qt::transparent).recorded);
    QVERIFY(!presentation.record(*commands, projection.version, *surface.target,
        QTransform::fromScale(0, 1), Qt::transparent).recorded);
    QVERIFY(!presentation.record(*commands, projection.version, *surface.target,
        QTransform::fromTranslate(std::numeric_limits<double>::quiet_NaN(), 0), Qt::transparent).recorded);
    QRhiReadbackResult readback;
    auto updates = m_rhi->nextResourceUpdateBatch();
    updates->readBackTexture({surface.texture.get()}, &readback);
    commands->resourceUpdate(updates);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(readback.data, QByteArray::fromHex("070d1dff").repeated(512 * 64));
}

void KisGpuTilePresentationTest::projectionCannotCrossDevices()
{
    OperationDocument document({64, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    TileRenderer renderer(*m_rhi);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto layers = renderer.record(*commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(*commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    std::unique_ptr<QRhi> other;
#if QT_CONFIG(metal)
    QRhiMetalInitParams params;
    other.reset(QRhi::create(QRhi::Metal, &params));
#elif QT_CONFIG(vulkan)
    QRhiVulkanInitParams params;
    params.inst = &m_vulkan;
    other.reset(QRhi::create(QRhi::Vulkan, &params));
#endif
    QVERIFY(other);
    TilePresentation presentation(*other);
    Surface surface(*other, {64, 64});
    QVERIFY(surface.ready);
    QCOMPARE(other->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    commands->beginPass(surface.target.get(), Qt::white, {1, 0});
    commands->endPass();
    const auto drawn = presentation.record(*commands, projection.version, *surface.target, {}, Qt::transparent);
    QVERIFY(!drawn.recorded);
    QVERIFY(!drawn.diagnostic.isEmpty());
    QRhiReadbackResult readback;
    auto updates = other->nextResourceUpdateBatch();
    updates->readBackTexture({surface.texture.get()}, &readback);
    commands->resourceUpdate(updates);
    QCOMPARE(other->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(readback.data, QByteArray::fromHex("ffffffff").repeated(64 * 64));
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuTilePresentationTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuTilePresentationTest.moc"
