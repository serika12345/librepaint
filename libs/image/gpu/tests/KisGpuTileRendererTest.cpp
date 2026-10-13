/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QCryptographicHash>
#include <QThread>
#include <rhi/qrhi.h>
#include <gpu/KisGpuTileRenderer.h>
#include <gpu/KisGpuDevice.h>

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
    void visibleLayersComposeOnTheGpuAndReuseUnchangedTiles();
    void compositionPreservesOrderAlphaAndExistingVersionsAtCapacity();
    void paintingAndCompositionShareOneSubmission();
    void longDocumentRebuildUsesBoundedTileStorage();
    void retainedPixelsOwnTheNativeDeviceUntilTheirLastReader();
    void nativeDeviceCreationRequiresTheApplicationThread();
    void rasterCheckpointsPreservePixelsThroughPaintingUndoAndRecovery();
    void overwrittenTileHistoryRebuildsWithinTwoBuffers();
    void tileWritesPreservePaintingOnOtherCoordinates();
    void implicitLayerPixelsStaySparseThroughEditsAndProjection_data();
    void implicitLayerPixelsStaySparseThroughEditsAndProjection();
    void implicitLayersRespectCompositionAndCapacity();
    void partialRenderingMatchesFullTilesAndRejectsIncompleteComposition();

private:
    RenderedSnapshot render(TileRenderer &renderer, const Snapshot &snapshot,
                            const RenderedSnapshot &base = {});
    QByteArray readTile(const RenderedSnapshot &version, LayerId layer, QPoint coordinate);
    QByteArray readBuffer(QRhiBuffer *pixels);
    ProjectedSnapshot project(TileRenderer &renderer, const RenderedSnapshot &version,
                              QRect visible, const ProjectedSnapshot &base = {});
    DevicePtr m_device;
    QRhi *m_rhi = nullptr;
};

void KisGpuTileRendererTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
    m_rhi = &m_device->rhi();
    QVERIFY2(m_rhi != nullptr, "The native Metal/Vulkan device is required by this contract");
    QVERIFY(m_rhi->isFeatureSupported(QRhi::Compute));
    qInfo() << "GPU backend:" << m_rhi->backendName();
}

void KisGpuTileRendererTest::partialRenderingMatchesFullTilesAndRejectsIncompleteComposition()
{
    OperationDocument document(QSize(1024, 512));
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 29.25;
    const auto stroke = document.beginStroke(layer, brush, 123);
    QCOMPARE(document.appendSamples(stroke, {{{20, 70}, 0.5}, {{800, 300}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto hidden = document.addLayer(QStringLiteral("Hidden"));
    QCOMPARE(document.setLayerVisible(hidden, false), Result::Success);
    const RgbaTile offCanvas{{-1, -1}, QByteArray::fromHex("12345678").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(document.replaceTiles(hidden, {offCanvas}), Result::Success);
    TileRenderer fullRenderer(m_device);
    const auto full = render(fullRenderer, document.snapshot());
    QVERIFY(full);
    TileRenderer partialRenderer(m_device, 3 * TileRenderer::TileBytes);
    const QRect region(263, 130, 1, 1);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto partial = partialRenderer.recordRegion(*commands, document.snapshot(), region, {layer});
    QCOMPARE(partial.code, RenderOutcome::Recorded);
    const auto composed = partialRenderer.compose(*commands, partial.version, region);
    QCOMPARE(composed.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(*commands, partial.version, {0, 0, 1024, 512}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(partial.version->tiles(layer).size(), 1);
    QVERIFY(partial.version->tiles(hidden).isEmpty());
    QCOMPARE(partial.version->tiles(layer)[0].coordinate, QPoint(1, 0));
    QCOMPARE(readTile(partial.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto reused = partialRenderer.recordRegion(*commands, document.snapshot(), region, {layer}, partial.version);
    QCOMPARE(reused.version, partial.version);
    const auto shifted = partialRenderer.recordRegion(*commands, document.snapshot(), {280, 140, 1, 1}, {layer}, partial.version);
    QCOMPARE(shifted.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
    auto expanded = partialRenderer.recordRegion(*commands, document.snapshot(), {256, 0, 512, 256}, {layer}, partial.version);
    QCOMPARE(expanded.code, RenderOutcome::Recorded);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(expanded.version->tiles(layer).size(), 2);
    QCOMPARE(partialRenderer.residentBytes(), qsizetype(2 * TileRenderer::TileBytes));
    QCOMPARE(readTile(expanded.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    QCOMPARE(readTile(expanded.version, layer, {2, 0}), readTile(full, layer, {2, 0}));
    expanded.version.clear();
    const auto edit = document.beginStroke(layer, brush, 456);
    QCOMPARE(document.appendSamples(edit, {{{300, 140}, 0.75}}), Result::Success);
    QCOMPARE(document.commitStroke(edit), Result::Success);
    const auto editedFull = render(fullRenderer, document.snapshot(), full);
    QVERIFY(editedFull);
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    auto edited = partialRenderer.recordRegion(*commands, document.snapshot(), {256, 0, 512, 256}, {layer}, partial.version);
    QCOMPARE(edited.code, RenderOutcome::Recorded);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(readTile(edited.version, layer, {1, 0}), readTile(editedFull, layer, {1, 0}));
    QCOMPARE(readTile(edited.version, layer, {2, 0}), readTile(editedFull, layer, {2, 0}));
    QCOMPARE(readTile(partial.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    edited.version.clear();
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto raw = partialRenderer.recordRegion(*commands, document.snapshot(), {-1, -1, 1, 1}, {hidden}, partial.version);
    QCOMPARE(raw.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(*commands, raw.version, {0, 0, 1, 1}).code, RenderOutcome::InvalidVersion);
    const auto incomplete = partialRenderer.recordRegion(*commands, document.snapshot(), {0, 0, 1, 1}, {hidden});
    QCOMPARE(incomplete.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(*commands, incomplete.version, {0, 0, 1, 1}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(partialRenderer.recordRegion(*commands, document.snapshot(), region, {layer, layer}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(partialRenderer.recordRegion(*commands, document.snapshot(), region, {999}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(raw.version->tiles(hidden).size(), 1);
    QCOMPARE(readTile(raw.version, hidden, {-1, -1}), offCanvas.rgba);
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
        return readBuffer(tile.pixels);
    }
    return {};
}

QByteArray KisGpuTileRendererTest::readBuffer(QRhiBuffer *pixels)
{
    QRhiCommandBuffer *commands = nullptr;
    if (m_rhi->beginOffscreenFrame(&commands) != QRhi::FrameOpSuccess) return {};
    QRhiReadbackResult readback;
    auto updates = m_rhi->nextResourceUpdateBatch();
    updates->readBackBuffer(pixels, 0, TileRenderer::TileBytes, &readback);
    commands->resourceUpdate(updates);
    m_rhi->endOffscreenFrame();
    return readback.data;
}

ProjectedSnapshot KisGpuTileRendererTest::project(TileRenderer &renderer, const RenderedSnapshot &version,
                                                 QRect visible, const ProjectedSnapshot &base)
{
    QRhiCommandBuffer *commands = nullptr;
    if (m_rhi->beginOffscreenFrame(&commands) != QRhi::FrameOpSuccess) return {};
    const auto result = renderer.compose(*commands, version, visible, base);
    const auto submitted = m_rhi->endOffscreenFrame();
    if (result.code != RenderOutcome::Recorded) qWarning() << result.diagnostic;
    return submitted == QRhi::FrameOpSuccess ? result.version : ProjectedSnapshot{};
}

void KisGpuTileRendererTest::roundBrushProducesRgbaPixelsOnTheGpu()
{
    OperationDocument document({512, 512});
    Brush brush;
    brush.rgba = {200, 30, 70, 255};
    brush.diameter = 16;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device);
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
    TileRenderer renderer(m_device);
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
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
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
    TileRenderer renderer(m_device);
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
    TileRenderer renderer(m_device, 64 * TileRenderer::TileBytes);
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
    TileRenderer renderer(m_device);
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
        TileRenderer owner(m_device);
        retained = render(owner, document.preview());
        QVERIFY(retained);
    }
    QCOMPARE(readTile(retained, document.activeLayer(), {0, 0}).size(), TileRenderer::TileBytes);
    TileRenderer replacement(m_device);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    QCOMPARE(replacement.record(*commands, document.preview(), retained).code, RenderOutcome::InvalidVersion);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QVERIFY(render(replacement, document.preview()));
}

void KisGpuTileRendererTest::visibleLayersComposeOnTheGpuAndReuseUnchangedTiles()
{
    OperationDocument document({1024, 1024});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 16;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto top = document.addLayer(QStringLiteral("Blue"));
    brush.rgba = {0, 0, 255, 128};
    stroke = document.beginStroke(top, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto layers = render(renderer, document.snapshot());
    QVERIFY(layers);
    const auto composed = project(renderer, layers, {0, 0, 256, 256});
    QVERIFY(composed);
    QCOMPARE(composed->tiles().size(), 1);
    const auto pixels = readBuffer(composed->tiles().first().pixels);
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("7f0080ff"));
    const auto outside = project(renderer, layers, {512, 512, 256, 256});
    QVERIFY(outside);
    QVERIFY(outside->tiles().isEmpty());
    QVERIFY(document.addLayer(QStringLiteral("Empty")));
    const auto expanded = render(renderer, document.snapshot(), layers);
    QVERIFY(expanded);
    const auto reused = project(renderer, expanded, {0, 0, 256, 256}, composed);
    QVERIFY(reused);
    QCOMPARE(reused->tiles().first().pixels, composed->tiles().first().pixels);
    QCOMPARE(document.setLayerVisible(top, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), expanded);
    const auto red = project(renderer, hidden, {0, 0, 256, 256}, reused);
    QVERIFY(red);
    QCOMPARE(readBuffer(red->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), pixels);
}

void KisGpuTileRendererTest::compositionPreservesOrderAlphaAndExistingVersionsAtCapacity()
{
    OperationDocument document({64, 64});
    const QList<std::array<quint8, 4>> colors{{255, 0, 0, 255}, {0, 0, 255, 128}, {0, 255, 0, 64}, {255, 255, 255, 128}};
    LayerId green = 0;
    for (qsizetype i = 0; i < colors.size(); ++i) {
        const auto layer = i == 0 ? document.activeLayer() : document.addLayer(QString::number(i));
        if (i == 2) green = layer;
        Brush brush;
        brush.rgba = colors[i];
        brush.diameter = 16;
        const auto stroke = document.beginStroke(layer, brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    TileRenderer renderer(m_device, 6 * TileRenderer::TileBytes);
    const auto layers = render(renderer, document.snapshot());
    QVERIFY(layers);
    auto composed = project(renderer, layers, {0, 0, 64, 64});
    QVERIFY(composed);
    const auto original = readBuffer(composed->tiles().first().pixels);
    QCOMPARE(original.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("afa0b0ff"));
    QCOMPARE(renderer.residentBytes(), 5 * TileRenderer::TileBytes);
    QCOMPARE(document.setLayerVisible(green, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), layers);
    QVERIFY(hidden);
    QVERIFY(!project(renderer, hidden, {0, 0, 64, 64}, composed));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), original);
    composed.clear();
    const auto reduced = project(renderer, hidden, {0, 0, 64, 64});
    QVERIFY(reduced);
    QCOMPARE(readBuffer(reduced->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
}

void KisGpuTileRendererTest::paintingAndCompositionShareOneSubmission()
{
    OperationDocument document({64, 64});
    for (const auto &color : QList<std::array<quint8, 4>>{{255, 0, 0, 128}, {0, 0, 255, 128}}) {
        if (!document.snapshot()->layers.first().operations.isEmpty()) QVERIFY(document.addLayer(QStringLiteral("Top")));
        Brush brush;
        brush.rgba = color;
        brush.diameter = 16;
        const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    TileRenderer renderer(m_device);
    QRhiCommandBuffer *commands = nullptr;
    QCOMPARE(m_rhi->beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
    const auto layers = renderer.record(*commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(*commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QRhiReadbackResult readback;
    auto updates = m_rhi->nextResourceUpdateBatch();
    updates->readBackBuffer(projection.version->tiles().first().pixels, 0, TileRenderer::TileBytes, &readback);
    commands->resourceUpdate(updates);
    QCOMPARE(m_rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    QCOMPARE(readback.data.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("5500aac0"));
}

void KisGpuTileRendererTest::longDocumentRebuildUsesBoundedTileStorage()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.diameter = 8;
    brush.rgba = {71, 139, 211, 128};
    Snapshot initial;
    for (int i = 0; i < OperationDocument::HistoryLimit; ++i) {
        const auto stroke = document.beginStroke(document.activeLayer(), brush, i);
        QCOMPARE(document.appendSamples(stroke, {{{qreal(8 + i % 48), 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
        if (i == 0) initial = document.snapshot();
    }
    QCOMPARE(document.undoCount(), OperationDocument::HistoryLimit);
    TileRenderer reference(m_device);
    const auto first = render(reference, initial);
    QVERIFY(first);
    const auto firstPixels = readTile(first, document.activeLayer(), {0, 0});
    const auto expected = render(reference, document.snapshot(), first);
    QVERIFY(expected);
    QCOMPARE(readTile(first, document.activeLayer(), {0, 0}), firstPixels);
    const auto pixels = readTile(expected, document.activeLayer(), {0, 0});
    // Algorithm 1: retained image contract for the fixed 250-stroke sequence.
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(),
        QByteArray("9ddc5af102159e58474239292756c24b2ec8a1813a88d33181b19b2dab66bdf7"));
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    TileRenderer bounded(m_device, 2 * TileRenderer::TileBytes);
    const auto rebuilt = render(bounded, restored.snapshot());
    QVERIFY2(rebuilt, "A retained 250-operation document must rebuild with two writable tiles per coordinate");
    QCOMPARE(readTile(rebuilt, restored.activeLayer(), {0, 0}), pixels);
    QCOMPARE(bounded.residentBytes(), TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::retainedPixelsOwnTheNativeDeviceUntilTheirLastReader()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    std::weak_ptr<Device> lifetime = device;
    RenderedSnapshot retained;
    ProjectedSnapshot projected;
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    {
        TileRenderer renderer(device);
        QRhiCommandBuffer *commands = nullptr;
        QCOMPARE(device->rhi().beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
        const auto result = renderer.record(*commands, document.preview());
        QCOMPARE(result.code, RenderOutcome::Recorded);
        retained = result.version;
        const auto merged = renderer.compose(*commands, retained, {0, 0, 64, 64});
        QCOMPARE(merged.code, RenderOutcome::Recorded);
        projected = merged.version;
        QCOMPARE(device->rhi().endOffscreenFrame(), QRhi::FrameOpSuccess);
    }
    device.reset();
    QVERIFY(!lifetime.expired());
    retained.clear();
    QVERIFY(!lifetime.expired());
    {
        const auto reader = lifetime.lock();
        QRhiCommandBuffer *commands = nullptr;
        QCOMPARE(reader->rhi().beginOffscreenFrame(&commands), QRhi::FrameOpSuccess);
        QRhiReadbackResult pixels;
        auto updates = reader->rhi().nextResourceUpdateBatch();
        updates->readBackBuffer(projected->tiles().first().pixels, 0, TileRenderer::TileBytes, &pixels);
        commands->resourceUpdate(updates);
        QCOMPARE(reader->rhi().endOffscreenFrame(), QRhi::FrameOpSuccess);
        QCOMPARE(pixels.data.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("000000ff"));
    }
    projected.clear();
    QVERIFY(lifetime.expired());
}

void KisGpuTileRendererTest::nativeDeviceCreationRequiresTheApplicationThread()
{
    DevicePtr device;
    QString diagnostic;
    std::unique_ptr<QThread> worker(QThread::create([&] { device = Device::createNative(diagnostic); }));
    worker->start();
    QVERIFY(worker->wait(2000));
    QVERIFY(!device);
    QVERIFY(!diagnostic.isEmpty());
}

void KisGpuTileRendererTest::rasterCheckpointsPreservePixelsThroughPaintingUndoAndRecovery()
{
    RasterImage image;
    image.size = {510, 255};
    const QByteArray invisible = QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4);
    const QByteArray blue = QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4);
    const QByteArray outside = QByteArray::fromHex("00ff0040").repeated(RgbaTile::ByteCount / 4);
    image.layers = {{QStringLiteral("Imported"), true, {}, {{{0, 0}, invisible}, {{1, 0}, blue}, {{-1, 0}, outside}}}};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device, 8 * TileRenderer::TileBytes);
    const auto imported = render(renderer, document.snapshot());
    QVERIFY(imported);
    QCOMPARE(imported->tiles(layer).size(), 3);
    QCOMPARE(readTile(imported, layer, {0, 0}), invisible);
    QCOMPARE(readTile(imported, layer, {1, 0}), blue);
    QCOMPARE(readTile(imported, layer, {-1, 0}), outside);
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto painted = render(renderer, document.snapshot(), imported);
    QVERIFY(painted);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixels.first(4), QByteArray::fromHex("120a2700"));
    QCOMPARE(pixels.last(4), QByteArray::fromHex("120a2700"));
    QCOMPARE(readTile(imported, layer, {0, 0}), invisible);
    for (const auto &tile : painted->tiles(layer)) {
        if (tile.coordinate == QPoint(0, 0)) continue;
        for (const auto &old : imported->tiles(layer)) {
            if (old.coordinate == tile.coordinate) QCOMPARE(tile.pixels, old.pixels);
        }
    }
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), painted);
    QVERIFY(undone);
    QCOMPARE(readTile(undone, layer, {0, 0}), invisible);
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    TileRenderer rebuilt(m_device, 4 * TileRenderer::TileBytes);
    const auto restored = render(rebuilt, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
    QCOMPARE(readTile(restored, layer, {-1, 0}), outside);
}

void KisGpuTileRendererTest::overwrittenTileHistoryRebuildsWithinTwoBuffers()
{
    OperationDocument document({256, 256});
    const auto layer = document.activeLayer();
    Brush blue;
    blue.rgba = {0, 0, 255, 255};
    for (int i = 0; i < 250; ++i) {
        const auto color = i % 2 ? QByteArray::fromHex("00ff00ff") : QByteArray::fromHex("ff0000ff");
        QCOMPARE(document.replaceTiles(layer, {{{0, 0}, color.repeated(RgbaTile::ByteCount / 4)}}), Result::Success);
        const auto stroke = document.beginStroke(layer, blue, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    QCOMPARE(document.undoCount(), 250);
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    const auto pixels = readTile(original, layer, {0, 0});
    QCOMPARE(pixels.first(4), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    const auto invisible = QByteArray::fromHex("0a141e00").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, invisible}}), Result::Success);
    const auto replaced = render(renderer, document.snapshot(), original);
    QVERIFY(replaced);
    QCOMPARE(readTile(replaced, layer, {0, 0}), invisible);
    QCOMPARE(readTile(original, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::tileWritesPreservePaintingOnOtherCoordinates()
{
    OperationDocument document({512, 256});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}, {{400, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto green = QByteArray::fromHex("00ff00ff").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, green}}), Result::Success);
    brush.rgba = {0, 0, 255, 255};
    stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{264, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    QCOMPARE(readTile(rendered, layer, {0, 0}), green);
    const auto right = readTile(rendered, layer, {1, 0});
    QCOMPARE(right.mid((32 * 256 + 8) * 4, 4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(right.mid((32 * 256 + 44) * 4, 4), QByteArray::fromHex("ff0000ff"));
}

void KisGpuTileRendererTest::implicitLayerPixelsStaySparseThroughEditsAndProjection_data()
{
    QTest::addColumn<QByteArray>("defaultRgba");
    QTest::newRow("opaque-white") << QByteArray::fromHex("ffffffff");
    QTest::newRow("transparent-rgb") << QByteArray::fromHex("11171f00");
}

void KisGpuTileRendererTest::implicitLayerPixelsStaySparseThroughEditsAndProjection()
{
    QFETCH(QByteArray, defaultRgba);
    RasterImage image;
    image.size = {4096, 4096};
    image.layers = {{QStringLiteral("White"), true, {}, {}}};
    for (int i = 0; i < 4; ++i) image.layers[0].defaultPixel[i] = quint8(defaultRgba[i]);
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device, 6 * TileRenderer::TileBytes);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    QVERIFY(original->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
    const auto corner = project(renderer, original, {4000, 4000, 96, 96});
    QVERIFY(corner);
    QCOMPARE(corner->tiles().size(), 1);
    QCOMPARE(corner->tiles().first().coordinate, QPoint(15, 15));
    QCOMPARE(readBuffer(corner->tiles().first().pixels), defaultRgba.repeated(RgbaTile::ByteCount / 4));
    Brush brush;
    brush.rgba = {0, 0, 255, 255};
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto painted = render(renderer, document.snapshot(), original);
    QVERIFY(painted);
    QCOMPARE(painted->tiles(layer).size(), 1);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.first(4), defaultRgba);
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    const auto visible = project(renderer, painted, {0, 0, 512, 256});
    QVERIFY(visible);
    QCOMPARE(visible->tiles().size(), 2);
    QCOMPARE(visible->tiles()[1].pixels, corner->tiles().first().pixels);
    QCOMPARE(readBuffer(corner->tiles().first().pixels).first(4), defaultRgba);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), painted);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(2 * TileRenderer::TileBytes));
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.redo(), Result::Success);
    TileRenderer rebuilt(m_device, 2 * TileRenderer::TileBytes);
    const auto restored = render(rebuilt, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::implicitLayersRespectCompositionAndCapacity()
{
    RasterImage image;
    image.size = {512, 256};
    image.layers = {{QStringLiteral("Red"), true, {}, {}}, {QStringLiteral("Blue"), true, {}, {}}};
    image.layers[0].defaultPixel = {255, 0, 0, 255};
    image.layers[1].defaultPixel = {0, 0, 255, 128};
    OperationDocument document(image);
    TileRenderer renderer(m_device, 4 * TileRenderer::TileBytes);
    const auto both = render(renderer, document.snapshot());
    QVERIFY(both);
    const auto merged = project(renderer, both, {0, 0, 512, 256});
    QVERIFY(merged);
    QCOMPARE(merged->tiles().size(), 2);
    for (const auto &tile : merged->tiles()) QCOMPARE(readBuffer(tile.pixels).first(4), QByteArray::fromHex("7f0080ff"));
    QCOMPARE(document.setLayerVisible(2, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), both);
    QVERIFY(hidden);
    const auto red = project(renderer, hidden, {0, 0, 512, 256}, merged);
    QVERIFY(red);
    QCOMPARE(red->tiles().size(), 2);
    for (const auto &tile : red->tiles()) QCOMPARE(readBuffer(tile.pixels).first(4), QByteArray::fromHex("ff0000ff"));
    TileRenderer small(m_device, TileRenderer::TileBytes);
    QVERIFY(!render(small, document.snapshot()));
    QCOMPARE(small.residentBytes(), qsizetype(0));
    image.size = {4096, 4096};
    image.layers.removeLast();
    OperationDocument large(image);
    const auto sparse = render(small, large.snapshot());
    QVERIFY(sparse);
    QVERIFY(!project(small, sparse, {0, 0, 4096, 4096}));
    QCOMPARE(small.residentBytes(), qsizetype(TileRenderer::TileBytes));
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuTileRendererTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuTileRendererTest.moc"
