/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QCryptographicHash>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <gpu/KisGpuTileRenderer.h>

using namespace Krita::Image::Gpu;

class KisGpuTileRendererTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void roundBrushProducesRgbaPixelsOnTheGpu();
    void inputPartitionsAndPreviewProduceTheSamePixels();
    void unchangedTilesAreSharedAndBudgetsRejectAtomically();
    void erasingAndLayerVisibilityPreservePixelMeaning();
    void longDiagonalUsesOnlyIntersectingTiles();
    void shapesUndoAndRecoveryRestoreTheSamePixels();
    void versionsOutliveTheRendererAndRejectAnotherDeviceOwner();

private:
    RenderedSnapshot render(TileRenderer &renderer, const Snapshot &snapshot,
                            const RenderedSnapshot &base = {});
    QByteArray readTile(const RenderedSnapshot &version, LayerId layer, QPoint coordinate);
#if QT_CONFIG(vulkan)
    QVulkanInstance m_vulkan;
#endif
    std::unique_ptr<QRhi> m_rhi;
};

void KisGpuTileRendererTest::initTestCase()
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
    QVERIFY2(m_rhi != nullptr, "The native Metal/Vulkan device is required by this contract");
    QVERIFY(m_rhi->isFeatureSupported(QRhi::Compute));
    qInfo() << "GPU backend:" << m_rhi->backendName();
}

RenderedSnapshot KisGpuTileRendererTest::render(TileRenderer &renderer, const Snapshot &snapshot,
                                               const RenderedSnapshot &base)
{
    QRhiCommandBuffer *commands = nullptr;
    if (m_rhi->beginOffscreenFrame(&commands) != QRhi::FrameOpSuccess) return {};
    const auto result = renderer.record(*commands, snapshot, base);
    const auto submitted = m_rhi->endOffscreenFrame();
    if (result.code != RenderOutcome::Recorded) qWarning() << result.diagnostic;
    if (submitted != QRhi::FrameOpSuccess) return {};
    return result.version;
}

QByteArray KisGpuTileRendererTest::readTile(const RenderedSnapshot &version, LayerId layer, QPoint coordinate)
{
    for (const auto &tile : version->tiles(layer)) {
        if (tile.coordinate != coordinate) continue;
        QRhiCommandBuffer *commands = nullptr;
        if (m_rhi->beginOffscreenFrame(&commands) != QRhi::FrameOpSuccess) return {};
        QRhiReadbackResult readback;
        auto updates = m_rhi->nextResourceUpdateBatch();
        updates->readBackBuffer(tile.pixels, 0, TileRenderer::TileBytes, &readback);
        commands->resourceUpdate(updates);
        m_rhi->endOffscreenFrame();
        return readback.data;
    }
    return {};
}

void KisGpuTileRendererTest::roundBrushProducesRgbaPixelsOnTheGpu()
{
    OperationDocument document({512, 512});
    Brush brush;
    brush.rgba = {200, 30, 70, 255};
    brush.diameter = 16;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(*m_rhi);
    const auto preview = render(renderer, document.preview());
    QVERIFY(preview);
    const auto bytes = readTile(preview, document.activeLayer(), {0, 0});
    QCOMPARE(bytes.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    QCOMPARE(pixel(32, 32), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(0, 0), QByteArray(4, '\0'));
    QCOMPARE(preview->tiles(document.activeLayer()).size(), 1);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), preview), document.activeLayer(), {0, 0}), bytes);
}

void KisGpuTileRendererTest::inputPartitionsAndPreviewProduceTheSamePixels()
{
    QList<InputSample> samples;
    for (int i = 0; i < 37; ++i) samples.append({{20.125 + i * 7.25, 70.5 + (i % 5) * 2}, qreal(i % 8 + 1) / 8});
    OperationDocument single({512, 512}), chunks({512, 512});
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 29.25;
    brush.opacity = 177;
    const auto a = single.beginStroke(single.activeLayer(), brush, 123);
    const auto b = chunks.beginStroke(chunks.activeLayer(), brush, 123);
    QCOMPARE(single.appendSamples(a, samples), Result::Success);
    for (const auto &sample : samples) QCOMPARE(chunks.appendSamples(b, {sample}), Result::Success);
    TileRenderer renderer(*m_rhi);
    const auto left = render(renderer, single.preview());
    const auto right = render(renderer, chunks.preview());
    QVERIFY(left && right);
    for (const auto &tile : left->tiles(single.activeLayer())) {
        QCOMPARE(readTile(left, single.activeLayer(), tile.coordinate), readTile(right, chunks.activeLayer(), tile.coordinate));
    }
    // Algorithm 1: the fixed input above was verified on Metal and Vulkan.
    QCOMPARE(QCryptographicHash::hash(readTile(left, single.activeLayer(), {0, 0}), QCryptographicHash::Sha256).toHex(),
             QByteArray("bc2af72a97bb315f96c978a6dd39a838470400a441c26363bdc229b8747078cf"));
}

void KisGpuTileRendererTest::unchangedTilesAreSharedAndBudgetsRejectAtomically()
{
    OperationDocument document({1024, 1024});
    TileRenderer renderer(*m_rhi, 2 * TileRenderer::TileBytes);
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto first = render(renderer, document.snapshot());
    QVERIFY(first);
    const auto next = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(next, {{{800, 800}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(next), Result::Success);
    const auto second = render(renderer, document.snapshot(), first);
    QVERIFY(second);
    QCOMPARE(second->tiles(document.activeLayer()).first().pixels, first->tiles(document.activeLayer()).first().pixels);
    const auto before = readTile(first, document.activeLayer(), {0, 0});
    const auto third = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(third, {{{32, 32}, 1}}), Result::Success);
    QVERIFY(!render(renderer, document.preview(), second));
    QCOMPARE(readTile(first, document.activeLayer(), {0, 0}), before);
    QCOMPARE(renderer.residentBytes(), 2 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::erasingAndLayerVisibilityPreservePixelMeaning()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 16;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(*m_rhi);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    brush.erase = true;
    brush.opacity = 128;
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto erased = render(renderer, document.snapshot(), painted);
    QVERIFY(erased);
    QCOMPARE(readTile(erased, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff00007f"));
    QCOMPARE(document.setLayerVisible(document.activeLayer(), false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), erased);
    QVERIFY(hidden);
    QCOMPARE(hidden->tiles(document.activeLayer()).first().pixels, erased->tiles(document.activeLayer()).first().pixels);
    QCOMPARE(readTile(painted, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
}

void KisGpuTileRendererTest::longDiagonalUsesOnlyIntersectingTiles()
{
    OperationDocument document({4096, 4096});
    Brush brush;
    brush.diameter = 4;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{4, 4}, 1}, {{4092, 4092}, 1}}), Result::Success);
    TileRenderer renderer(*m_rhi, 64 * TileRenderer::TileBytes);
    const auto version = render(renderer, document.preview());
    QVERIFY2(version, "A sparse diagonal must fit without allocating its bounding rectangle");
    QVERIFY(version->tiles(document.activeLayer()).size() <= 64);
    QCOMPARE(readTile(version, document.activeLayer(), {0, 0}).mid((4 * TileRenderer::TileSize + 4) * 4, 4), QByteArray::fromHex("000000ff"));
}

void KisGpuTileRendererTest::shapesUndoAndRecoveryRestoreTheSamePixels()
{
    OperationDocument document({256, 256});
    Brush brush;
    brush.rgba = {17, 129, 241, 201};
    brush.diameter = 13.75;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{10, 20}, 0.75}, {{180, 140}, 0.5}}), Result::Success);
    TileRenderer renderer(*m_rhi);
    const auto freehand = render(renderer, document.preview());
    QVERIFY(freehand);
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Line, {60, 80}, {200, 140}, 0}), Result::Success);
    const auto line = render(renderer, document.preview(), freehand);
    QVERIFY(line);
    const auto linePixels = readTile(line, document.activeLayer(), {0, 0});
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Ellipse, {30, 40}, {210, 220}, 0.3}), Result::Success);
    const auto ellipse = render(renderer, document.preview(), line);
    QVERIFY(ellipse);
    const auto pixels = readTile(ellipse, document.activeLayer(), {0, 0});
    QVERIFY(pixels != linePixels);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), ellipse), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), ellipse);
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), document.activeLayer(), {0, 0}), pixels);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(readTile(render(renderer, restored.snapshot()), restored.activeLayer(), {0, 0}), pixels);
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(),
             QByteArray("9b38f73d4cc8a980990534b895dd2ef68b6d59f4156731881162ffc1574fd2ec"));
    const auto cancelled = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(cancelled, {{{128, 128}, 1}}), Result::Success);
    QVERIFY(render(renderer, document.preview(), ellipse));
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), ellipse), document.activeLayer(), {0, 0}), pixels);
}

void KisGpuTileRendererTest::versionsOutliveTheRendererAndRejectAnotherDeviceOwner()
{
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    RenderedSnapshot retained;
    {
        TileRenderer owner(*m_rhi);
        retained = render(owner, document.preview());
        QVERIFY(retained);
    }
    QCOMPARE(readTile(retained, document.activeLayer(), {0, 0}).size(), TileRenderer::TileBytes);
    TileRenderer replacement(*m_rhi);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    QCOMPARE(replacement.record(*commands, document.preview(), retained).code, RenderOutcome::InvalidVersion);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QVERIFY(render(replacement, document.preview()));
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuTileRendererTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuTileRendererTest.moc"
