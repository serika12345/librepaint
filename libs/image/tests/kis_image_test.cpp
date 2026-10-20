/*
 *  SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_test.h"
#include <QApplication>
#include <QSignalSpy>
#include <QPointer>
#include <QSemaphore>
#include <atomic>
#include <thread>

#include <simpletest.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoCompositeOpRegistry.h>
#include <kis_psd_layer_style.h>
#include <kis_default_bounds.h>

#include "filter/kis_filter.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter_registry.h"

#include "kis_image.h"
#include "kis_projection_updates_filter.h"
#include "gpu/KisGpuPaintImage.h"
#include "gpu/KisGpuExecution.h"
#include "kis_paint_layer.h"
#include "kis_group_layer.h"
#include "kis_adjustment_layer.h"
#include "kis_selection.h"
#include <kis_debug.h>
#include <kis_layer_composition.h>
#include "kis_keyframe_channel.h"
#include "kis_selection_mask.h"
#include "kis_layer_utils.h"
#include "kis_annotation.h"
#include "KisProofingConfiguration.h"
#include <KisGlobalResourcesInterface.h>
#include "KisImageResolutionProxy.h"
#include <commands/kis_deselect_global_selection_command.h>
#include <commands/kis_reselect_global_selection_command.h>
#include <commands/kis_set_global_selection_command.h>

#include "kis_undo_stores.h"

#include <testimage.h>

#define IMAGE_WIDTH 128
#define IMAGE_HEIGHT 128

namespace {
struct PausedProjectionState {
    QSemaphore entered;
    QSemaphore resume;
    std::atomic<bool> destroyed{false};
};

class PausedProjectionFilter final : public KisProjectionUpdatesFilter
{
public:
    explicit PausedProjectionFilter(PausedProjectionState &state) : m_state(state) {}
    ~PausedProjectionFilter() override { m_state.destroyed = true; }
    bool filter(KisImage *, KisNode *, const QVector<QRect> &, KisProjectionUpdateFlags) override
    {
        return pause();
    }
    bool filterRefreshGraph(KisImage *, KisNode *, const QVector<QRect> &, const QRect &, KisProjectionUpdateFlags) override
    {
        return pause();
    }
private:
    bool pause()
    {
        auto &state = m_state;
        state.entered.release();
        state.resume.tryAcquire(1, 5000);
        return true;
    }
    PausedProjectionState &m_state;
};
}

void KisImageTest::testProjectionFilterSurvivesRemovalDuringARequest_data()
{
    QTest::addColumn<bool>("refreshGraph");
    QTest::newRow("dirty-request") << false;
    QTest::newRow("graph-refresh") << true;
}

void KisImageTest::testProjectionFilterSurvivesRemovalDuringARequest()
{
    QFETCH(bool, refreshGraph);
    PausedProjectionState state;
    KisImageSP image = new KisImage(nullptr, 32, 32, KoColorSpaceRegistry::instance()->rgb8(), QStringLiteral("Projection request"));
    KisProjectionUpdatesFilterSP filter(new PausedProjectionFilter(state));
    const auto cookie = image->addProjectionUpdatesFilter(filter);
    filter.clear();
    std::thread request([&] {
        if (refreshGraph) image->refreshGraphAsync(image->root(), image->bounds());
        else image->requestProjectionUpdate(image->rootLayer().data(), {image->bounds()}, {});
    });
    const bool entered = state.entered.tryAcquire(1, 5000);
    if (entered) image->removeProjectionUpdatesFilter(cookie).clear();
    const bool survivedRemoval = !state.destroyed.load();
    state.resume.release();
    request.join();
    QVERIFY(entered);
    QVERIFY2(survivedRemoval, "An executing projection filter must live until its request finishes");
    QVERIFY(state.destroyed.load());
    QVERIFY(!image->currentProjectionUpdatesFilter());
}

void KisImageTest::testGpuPaintChannelFlagsPreserveProtectedPixelsAndFrozenHistory()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto id = owner->operations().activeLayer();
    QCOMPARE(owner->writeRgbaPixels(id, QRect(32, 32, 1, 1), QByteArray::fromHex("0a28b480")), Gpu::Result::Success);
    const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Channel protection"));
    KisNodeSP node = image->root()->firstChild();
    auto *layer = qobject_cast<KisPaintLayer*>(node.data());
    QVERIFY(layer);
    // The public RGBA8 paint-layer flags use the existing BGRA channel order.
    QBitArray redOnly(4, false);
    redOnly.setBit(2);
    layer->setChannelLockFlags(redOnly);
    QCOMPARE(layer->channelLockFlags(), redOnly);
    QVERIFY(layer->alphaLocked());
    QCOMPARE(owner->operations().snapshot()->layers.first().channelWriteMask, quint8(1));
    Gpu::Brush brush;
    brush.rgba = {200, 70, 30, 255};
    brush.diameter = 8;
    const auto stroke = owner->beginStroke(id, brush, 7);
    QCOMPARE(owner->appendSamples(stroke, {{{32, 32}, 1}}), Gpu::Result::Success);
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    const auto painted = owner->operations().snapshot();
    layer->setAlphaLocked(false);
    QVERIFY(!layer->alphaLocked());
    QCOMPARE(owner->operations().snapshot()->layers.first().channelWriteMask, quint8(9));
    QCOMPARE(painted->layers.first().channelWriteMask, quint8(1));
    QCOMPARE(owner->readRgbaPixels(id, QRect(32, 32, 1, 1), 4).bytes, QByteArray::fromHex("c828b480"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(id, QRect(32, 32, 1, 1), 4).bytes, QByteArray::fromHex("c828b480"));
    const auto restored = Gpu::PaintImageSP::create(execution, QSize(1, 1));
    QCOMPARE(restored->restoreRecovery(owner->operations().recoveryData()), Gpu::Result::Success);
    const auto reopened = KisImage::fromGpuImage(restored, QStringLiteral("Recovered channel protection"));
    KisNodeSP restoredNode = reopened->root()->firstChild();
    auto *restoredLayer = qobject_cast<KisPaintLayer*>(restoredNode.data());
    auto redAndAlpha = redOnly;
    redAndAlpha.setBit(3);
    QCOMPARE(restoredLayer->channelLockFlags(), redAndAlpha);
    brush.rgba = {1, 2, 3, 255};
    const auto next = restored->beginStroke(restored->operations().activeLayer(), brush, 0);
    QCOMPARE(restored->appendSamples(next, {{{32, 32}, 1}}), Gpu::Result::Success);
    QCOMPARE(restored->commitStroke(next), Gpu::Result::Success);
    QCOMPARE(restored->readRgbaPixels(restored->operations().activeLayer(), QRect(32, 32, 1, 1), 4).bytes,
             QByteArray::fromHex("0128b4ff"));
}

void KisImageTest::testRetainedGpuLayerPixelsOutliveTheImageAndDiscardedHistory()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    std::weak_ptr<Gpu::Execution> executionLifetime = execution;
    auto owner = Gpu::PaintImageSP::create(execution, QSize(16, 16));
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Retained pixels"));
    const auto bottom = owner->operations().activeLayer();
    QVERIFY(owner->addLayer(QStringLiteral("Retained")));
    auto node = image->root()->lastChild();
    QPointer<KisNode> nodeLifetime(node.data());
    const auto first = QByteArray::fromHex("332211ff");
    node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(first.constData()), 5, 5, 1, 1);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->renameLayer(bottom, QStringLiteral("New branch")), Gpu::Result::Success);
    QCOMPARE(owner->operations().redoCount(), 0);
    QVERIFY(!node->parent());
    image.clear();
    owner.reset();
    execution.reset();
    device.reset();
    QVERIFY(!executionLifetime.expired());
    QByteArray actual(4, '\0');
    node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
    QCOMPARE(actual, first);
    const auto edited = QByteArray::fromHex("665544aa");
    node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(edited.constData()), 6, 6, 1, 1);
    node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
    QCOMPARE(actual, edited);
    node.clear();
    QVERIFY(!nodeLifetime);
    QVERIFY(executionLifetime.expired());
}

void KisImageTest::testGpuLayerReferencesSurviveHistoryAndReleaseDiscardedBranches()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(16, 16));
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Layer reference lifetime"));
    QPointer<KisImage> imageLifetime(image.data());
    const auto original = owner->operations().activeLayer();
    QVERIFY(owner->addLayer(QStringLiteral("Added"), 1, true));
    auto added = image->root()->lastChild();
    QPointer<KisNode> nodeLifetime(added.data());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!added->parent());
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(image->root()->lastChild(), added);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    added.clear();
    QVERIFY(nodeLifetime);
    QCOMPARE(owner->renameLayer(original, QStringLiteral("New branch")), Gpu::Result::Success);
    QCOMPARE(owner->operations().redoCount(), 0);
    QVERIFY(!nodeLifetime);
    image.clear();
    QVERIFY(!imageLifetime);
}

void KisImageTest::testGpuInsertionPreservesSupportedStateAndRejectsUnrepresentedState_data()
{
    QTest::addColumn<int>("property");
    QTest::newRow("blend") << 0;
    QTest::newRow("animation") << 2;
    QTest::newRow("layer-style") << 3;
    QTest::newRow("composition-channels") << 4;
    QTest::newRow("alpha-inheritance") << 5;
    QTest::newRow("temporary-layer") << 6;
    QTest::newRow("temporary-pixels") << 7;
}

void KisImageTest::testGpuInsertionPreservesSupportedStateAndRejectsUnrepresentedState()
{
    QFETCH(int, property);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Validated insertion"));
    KisImageSP source = new KisImage(nullptr, 32, 16, image->colorSpace(), QStringLiteral("Foreign input"));
    KisPaintLayerSP layer = new KisPaintLayer(source, QStringLiteral("Preserved input"), 255);
    auto pixels = layer->paintDevice();
    const auto bytes = QByteArray::fromHex("112233ff");
    pixels->writeBytes(reinterpret_cast<const quint8*>(bytes.constData()), 5, 7, 1, 1);
    if (property == 0) layer->setCompositeOpId(COMPOSITE_MULT);
    if (property == 2) layer->enableAnimation();
    if (property == 3) {
        auto style = KisPSDLayerStyleSP::create();
        style->dropShadow()->setEffectEnabled(true);
        layer->setLayerStyle(style->cloneWithResourcesSnapshot(KisGlobalResourcesInterface::instance(), {}));
    }
    if (property == 4) {
        QBitArray channels(4, true);
        channels.setBit(0, false);
        layer->setChannelFlags(channels);
    }
    if (property == 5) layer->disableAlphaChannel(true);
    if (property == 6) layer->setTemporary(true);
    if (property == 7) layer->setTemporaryTarget(new KisPaintDevice(layer->colorSpace()));
    const auto channels = layer->channelFlags();
    const auto before = owner->operations().snapshot();
    const auto serialized = owner->operations().recoveryData();
    QByteArray retained(4, Qt::Uninitialized);
    pixels->readBytes(reinterpret_cast<quint8*>(retained.data()), 5 + layer->x(), 7 + layer->y(), 1, 1);
    if (property == 4 || property == 5) {
        QVERIFY(image->insertGpuLayer(layer, 0));
        QCOMPARE(image->root()->childCount(), 2U);
        QCOMPARE(owner->operations().undoCount(), 1);
        QCOMPARE(layer->channelFlags(), channels);
        const quint8 expectedMask = property == 4 ? 11 : 7;
        QCOMPARE(owner->operations().snapshot()->layers.first().projectionChannelMask, expectedMask);
        QCOMPARE(owner->readRgbaPixels(owner->operations().snapshot()->layers.first().id, {5, 7, 1, 1}, 4).bytes,
                 QByteArray::fromHex("332211ff"));
        auto copied = image->cloneGpuNodeImage({layer});
        QVERIFY(copied);
        const auto copiedLayer = qobject_cast<const KisLayer*>(copied->root()->firstChild().data());
        QVERIFY(copiedLayer);
        QCOMPARE(copiedLayer->channelFlags(), channels);
        QCOMPARE(copied->gpuImage()->operations().snapshot()->layers.first().projectionChannelMask, expectedMask);
        QCOMPARE(copied->gpuImage()->readRgbaPixels(copied->gpuImage()->operations().activeLayer(), {5, 7, 1, 1}, 4).bytes,
                 QByteArray::fromHex("332211ff"));
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QVERIFY(!layer->parent());
        QCOMPARE(layer->channelFlags(), channels);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QCOMPARE(layer->channelFlags(), channels);
        QCOMPARE(owner->operations().snapshot()->layers.first().projectionChannelMask, expectedMask);
        return;
    }
    QVERIFY(!image->insertGpuLayer(layer, 0));
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().recoveryData(), serialized);
    QCOMPARE(image->root()->childCount(), 1U);
    QCOMPARE(layer->paintDevice(), pixels);
    QVERIFY(!layer->parent());
    if (property == 0) QCOMPARE(layer->compositeOpId(), QString(COMPOSITE_MULT));
    if (property == 2) QVERIFY(layer->isAnimated());
    if (property == 3) QVERIFY(layer->layerStyle()->dropShadow()->effectEnabled());
    QCOMPARE(layer->channelFlags(), channels);
    if (property == 6) QVERIFY(layer->temporary());
    if (property == 7) QVERIFY(layer->hasTemporaryTarget());
    QByteArray actual(4, Qt::Uninitialized);
    pixels->readBytes(reinterpret_cast<quint8*>(actual.data()), 5 + layer->x(), 7 + layer->y(), 1, 1);
    QCOMPARE(actual, retained);
}

void KisImageTest::testGpuInsertionKeepsRawPixelsFromWrappedInput()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Raw insertion"));
    KisImageSP source = new KisImage(nullptr, 32, 16, image->colorSpace(), QStringLiteral("Wrapped source"));
    KisPaintLayerSP layer = new KisPaintLayer(source, QStringLiteral("Fixed input"), 255);
    auto pixels = layer->paintDevice();
    pixels->setDefaultBounds(new KisWrapAroundBoundsWrapper(new KisDefaultBounds(source), QRect(0, 0, 32, 16)));
    QVERIFY(pixels->defaultBounds()->wrapAroundMode());
    const auto bytes = QByteArray::fromHex("112233ff");
    pixels->writeBytes(reinterpret_cast<const quint8*>(bytes.constData()), 5, 7, 1, 1);
    QVERIFY(image->insertGpuLayer(layer, 1));
    QByteArray actual(4, Qt::Uninitialized);
    pixels->readBytes(reinterpret_cast<quint8*>(actual.data()), 5, 7, 1, 1);
    QCOMPARE(actual, bytes);
    for (QPoint point : {QPoint(37, 7), QPoint(5, 23), QPoint(37, 23)}) {
        pixels->readBytes(reinterpret_cast<quint8*>(actual.data()), point.x(), point.y(), 1, 1);
        QCOMPARE(actual, QByteArray(4, '\0'));
    }
    QCOMPARE(owner->operations().undoCount(), 1);
}

void KisImageTest::testGpuImageNotificationLifetime_data()
{
    QTest::addColumn<int>("notification");
    QTest::newRow("layer-added") << 0;
    QTest::newRow("modified") << 1;
    QTest::newRow("projection-updated") << 2;
}

void KisImageTest::testGpuImageNotificationLifetime()
{
    QFETCH(int, notification);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(16, 16));
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Notification lifetime"));
    QPointer<KisImage> guard(image.data());
    bool retainedDuringDelivery = false;
    const auto release = [&] {
        image.clear();
        retainedDuringDelivery = bool(guard);
    };
    if (notification == 0) connect(image.data(), &KisImage::sigNodeAddedAsync, this, release);
    if (notification == 1) connect(image.data(), &KisImage::sigImageModified, this, release);
    if (notification == 2) connect(image.data(), &KisImage::sigImageUpdated, this, release);
    QVERIFY(owner->addLayer(QStringLiteral("Added")));
    QVERIFY(retainedDuringDelivery);
    QVERIFY(image.isNull());
    QVERIFY(!guard);
    QVERIFY(execution.requested(owner->handle()));
}

void KisImageTest::testGpuImagePublishesLayersProjectionAndSharedLifetime()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto handle = owner->handle();
    const auto bottomId = owner->operations().activeLayer();
    QCOMPARE(owner->writeRgbaPixels(bottomId, QRect(8, 12, 1, 1), QByteArray::fromHex("115bcbff")), Gpu::Result::Success);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("GPU image"));
    QCOMPARE(image->bounds(), QRect(0, 0, 64, 32));
    QCOMPARE(image->colorSpace(), KoColorSpaceRegistry::instance()->rgb8());
    QCOMPARE(image->gpuImage(), owner);
    QCOMPARE(image->root()->childCount(), 1U);
    KisNodeSP bottom = image->root()->firstChild();
    QCOMPARE(bottom->uuid(), owner->operations().snapshot()->layers.front().uuid);
    QByteArray pixel(4, Qt::Uninitialized);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 8, 12, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("cb5b11ff"));
    QSignalSpy nodeChanges(image.data(), &KisImage::sigNodeChanged);
    const auto stroke = owner->beginStroke(bottomId, Gpu::Brush{}, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{20, 20}, 1}}), Gpu::Result::Success);
    QVERIFY(!image->isIdle());
    QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
    QVERIFY(nodeChanges.isEmpty());
    QSignalSpy updates(image.data(), &KisImage::sigImageUpdated);
    const auto topId = owner->addLayer(QStringLiteral("Top"));
    QVERIFY(topId);
    QCOMPARE(image->root()->childCount(), 2U);
    QCOMPARE(image->root()->lastChild()->name(), QStringLiteral("Top"));
    QCOMPARE(owner->replaceRgbaPixels(topId, QRect(8, 12, 1, 1), QByteArray::fromHex("ed2713ff")), Gpu::Result::Success);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 8, 12, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("1327edff"));
    QCOMPARE(owner->setLayerVisible(topId, false), Gpu::Result::Success);
    QVERIFY(!image->root()->lastChild()->visible());
    image->refreshGraphAsync();
    image->waitForDone();
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 8, 12, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("cb5b11ff"));
    QCOMPARE(owner->renameLayer(bottomId, QStringLiteral("Bottom")), Gpu::Result::Success);
    QCOMPARE(image->root()->firstChild(), bottom);
    QCOMPARE(bottom->name(), QStringLiteral("Bottom"));
    QCOMPARE(owner->moveLayer(bottomId, 1), Gpu::Result::Success);
    QCOMPARE(image->root()->lastChild(), bottom);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(image->root()->firstChild(), bottom);
    QVERIFY(!updates.isEmpty());
    image->barrierLock();
    QVERIFY(image->locked());
    QVERIFY(image->tryBarrierLock());
    image->unlock();
    QVERIFY(image->locked());
    image->unlock();
    QVERIFY(image->isIdle());
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    QSignalSpy modifications(image.data(), &KisImage::sigImageModified);
    nodeChanges.clear();
    QCOMPARE(owner->replaceRgbaPixels(bottomId, QRect(5, 5, 1, 1), QByteArray::fromHex("010203ff")), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!owner->operations().isModified());
    QCOMPARE(modifications.size(), 2);
    QVERIFY(nodeChanges.isEmpty());
    owner.clear();
    image.clear();
    QVERIFY(execution.requested(handle));
    bottom->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 8, 12, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("cb5b11ff"));
    bottom.clear();
    QVERIFY(!execution.requested(handle));
}

void KisImageTest::layerTests()
{
    KisImageSP image = new KisImage(0, IMAGE_WIDTH, IMAGE_WIDTH, 0, "layer tests");
    QVERIFY(image->rootLayer() != 0);
    QVERIFY(image->rootLayer()->firstChild() == 0);

    KisLayerSP layer = new KisPaintLayer(image, "layer 1", OPACITY_OPAQUE_U8);
    image->addNode(layer);

    QVERIFY(image->rootLayer()->firstChild()->objectName() == layer->objectName());
}

void KisImageTest::benchmarkCreation()
{
    const QRect imageRect(0,0,3000,2000);
    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();

    QList<KisImageSP> images;
    QList<KisSurrogateUndoStore*> stores;


    QBENCHMARK {
        for (int i = 0; i < 10; i++) {
            stores << new KisSurrogateUndoStore();
        }

        for (int i = 0; i < 10; i++) {
            KisImageSP image = new KisImage(stores.takeLast(), imageRect.width(), imageRect.height(), cs, "test image");
            images << image;
        }
    }
}

#include <testutil.h>
#include "kis_stroke_strategy.h"
#include <functional>


class ForbiddenLodStrokeStrategy : public KisStrokeStrategy
{
public:
    ForbiddenLodStrokeStrategy(std::function<void()> lodCallback)
        : KisStrokeStrategy(QLatin1String("ForbiddenLodStrokeStrategy")),
          m_lodCallback(lodCallback)
    {
    }

    KisStrokeStrategy* createLodClone(int levelOfDetail) override {
        Q_UNUSED(levelOfDetail);
        m_lodCallback();
        return 0;
    }

private:
    std::function<void()> m_lodCallback;
};

void notifyVar(bool *value) {
    *value = true;
}

void testingSetOldDesiredLevelOfDetail(KisImageSP image, int lod)
{
    KisLodPreferences pref(lod);
    image->setLodPreferences(pref);
}

void testingSetOldLevelOfDetailBlocked(KisImageSP image, bool value)
{
    if (value) {
        testingSetOldDesiredLevelOfDetail(image, 0);
    } else {
        KisLodPreferences pref(KisLodPreferences::None, 0);
        image->setLodPreferences(pref);
    }
}

void KisImageTest::testBlockLevelOfDetail()
{
    TestUtil::MaskParent p;

    QCOMPARE(p.image->currentLevelOfDetail(), 0);

    testingSetOldDesiredLevelOfDetail(p.image, 1);
    p.image->waitForDone();

    QCOMPARE(p.image->currentLevelOfDetail(), 0);

    {
        bool lodCreated = false;
        KisStrokeId id = p.image->startStroke(
            new ForbiddenLodStrokeStrategy(
                std::bind(&notifyVar, &lodCreated)));
        p.image->endStroke(id);
        p.image->waitForDone();

        QVERIFY(lodCreated);
    }

    testingSetOldLevelOfDetailBlocked(p.image, true);

    {
        bool lodCreated = false;
        KisStrokeId id = p.image->startStroke(
            new ForbiddenLodStrokeStrategy(
                std::bind(&notifyVar, &lodCreated)));
        p.image->endStroke(id);
        p.image->waitForDone();

        QVERIFY(!lodCreated);
    }

    testingSetOldLevelOfDetailBlocked(p.image, false);
    testingSetOldDesiredLevelOfDetail(p.image, 1);

    {
        bool lodCreated = false;
        KisStrokeId id = p.image->startStroke(
            new ForbiddenLodStrokeStrategy(
                std::bind(&notifyVar, &lodCreated)));
        p.image->endStroke(id);
        p.image->waitForDone();

        QVERIFY(lodCreated);
    }
}

void KisImageTest::testConvertImageColorSpace()
{
    const KoColorSpace *cs8 = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 1000, 1000, cs8, "stest");

    KisPaintDeviceSP device1 = new KisPaintDevice(cs8);
    KisLayerSP paint1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8, device1);

    KisFilterSP filter = KisFilterRegistry::instance()->value("blur");
    Q_ASSERT(filter);
    KisFilterConfigurationSP configuration = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    Q_ASSERT(configuration);

    KisLayerSP blur1 = new KisAdjustmentLayer(image, "blur1", configuration->cloneWithResourcesSnapshot(), 0);

    image->addNode(paint1, image->root());
    image->addNode(blur1, image->root());

    image->initialRefreshGraph();

    const KoColorSpace *cs16 = KoColorSpaceRegistry::instance()->rgb16();
    image->convertImageColorSpace(cs16,
                                  KoColorConversionTransformation::internalRenderingIntent(),
                                  KoColorConversionTransformation::internalConversionFlags());
    image->waitForDone();

    QVERIFY(*cs16 == *image->colorSpace());
    QVERIFY(*cs16 == *image->root()->colorSpace());
    QVERIFY(*cs16 == *paint1->colorSpace());
    QVERIFY(*cs16 == *blur1->colorSpace());

    QVERIFY(!image->root()->compositeOp());
    QVERIFY(*cs16 == *paint1->compositeOp()->colorSpace());
    QVERIFY(*cs16 == *blur1->compositeOp()->colorSpace());

    image->refreshGraphAsync();
    image->waitForDone();
}

void KisImageTest::testAssignImageProfile()
{
    const KoColorSpace *rgb8 = KoColorSpaceRegistry::instance()->rgb8();
    const KoColorSpace *gray8 = KoColorSpaceRegistry::instance()->graya8();
    KisImageSP image = new KisImage(0, 1000, 1000, rgb8, "stest");

    KisPaintDeviceSP device1 = new KisPaintDevice(rgb8);
    KisLayerSP paint1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8, device1);

    KisPaintDeviceSP device2 = new KisPaintDevice(gray8);
    KisLayerSP paint2 = new KisPaintLayer(image, "paint2", OPACITY_OPAQUE_U8, device2);


    KisFilterSP filter = KisFilterRegistry::instance()->value("blur");
    Q_ASSERT(filter);
    KisFilterConfigurationSP configuration = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    Q_ASSERT(configuration);

    KisLayerSP blur1 = new KisAdjustmentLayer(image, "blur1", configuration->cloneWithResourcesSnapshot(), 0);

    image->addNode(paint1, image->root());
    image->addNode(paint2, image->root());
    image->addNode(blur1, image->root());

    QCOMPARE(*image->colorSpace(), *rgb8);
    QCOMPARE(*image->colorSpace()->profile(), *KoColorSpaceRegistry::instance()->p709SRGBProfile());

    QCOMPARE(*paint1->colorSpace(), *rgb8);
    QCOMPARE(*paint1->colorSpace()->profile(), *KoColorSpaceRegistry::instance()->p709SRGBProfile());

    QCOMPARE(*paint2->colorSpace(), *gray8);

    QCOMPARE(*blur1->colorSpace(), *rgb8);
    QCOMPARE(*blur1->colorSpace()->profile(), *KoColorSpaceRegistry::instance()->p709SRGBProfile());


    image->assignImageProfile(KoColorSpaceRegistry::instance()->p2020G10Profile());
    image->waitForDone();

    QVERIFY(*image->colorSpace() != *rgb8);
    QCOMPARE(*image->colorSpace()->profile(), *KoColorSpaceRegistry::instance()->p2020G10Profile());

    QVERIFY(*paint1->colorSpace() != *rgb8);
    QCOMPARE(*paint1->colorSpace()->profile(), *KoColorSpaceRegistry::instance()->p2020G10Profile());

    QCOMPARE(*paint2->colorSpace(), *gray8);

    QVERIFY(*blur1->colorSpace() != *rgb8);
    QCOMPARE(*blur1->colorSpace()->profile(), *KoColorSpaceRegistry::instance()->p2020G10Profile());
}

void KisImageTest::testGlobalSelection()
{
    const KoColorSpace *cs8 = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 1000, 1000, cs8, "stest");

    QCOMPARE(image->globalSelection(), KisSelectionSP(0));
    QCOMPARE(image->canReselectGlobalSelection(), false);
    QCOMPARE(image->root()->childCount(), 0U);

    KisSelectionSP selection1 = new KisSelection(new KisDefaultBounds(image), toQShared(new KisImageResolutionProxy(image)));
    KisSelectionSP selection2 = new KisSelection(new KisDefaultBounds(image), toQShared(new KisImageResolutionProxy(image)));
    KisSelectionSP selection3 = new KisSelection(new KisDefaultBounds(image), toQShared(new KisImageResolutionProxy(image)));

    image->undoAdapter()->addCommand(new KisSetGlobalSelectionCommand(image, selection1));
    QCOMPARE(image->globalSelection(), selection1);
    QCOMPARE(image->canReselectGlobalSelection(), false);
    QCOMPARE(image->root()->childCount(), 1U);

    image->undoAdapter()->addCommand(new KisSetGlobalSelectionCommand(image, selection2));
    QCOMPARE(image->globalSelection(), selection2);
    QCOMPARE(image->canReselectGlobalSelection(), false);
    QCOMPARE(image->root()->childCount(), 1U);

    image->undoAdapter()->addCommand(new KisDeselectGlobalSelectionCommand(image));
    QCOMPARE(image->globalSelection(), KisSelectionSP(0));
    QCOMPARE(image->canReselectGlobalSelection(), true);
    QCOMPARE(image->root()->childCount(), 0U);

    image->undoAdapter()->addCommand(new KisReselectGlobalSelectionCommand(image));
    QCOMPARE(image->globalSelection(), selection2);
    QCOMPARE(image->canReselectGlobalSelection(), false);
    QCOMPARE(image->root()->childCount(), 1U);

    // mixed deselecting/setting/reselecting

    image->undoAdapter()->addCommand(new KisDeselectGlobalSelectionCommand(image));
    QCOMPARE(image->globalSelection(), KisSelectionSP(0));
    QCOMPARE(image->canReselectGlobalSelection(), true);
    QCOMPARE(image->root()->childCount(), 0U);

    image->undoAdapter()->addCommand(new KisSetGlobalSelectionCommand(image, selection3));
    QCOMPARE(image->globalSelection(), selection3);
    QCOMPARE(image->canReselectGlobalSelection(), false);
    QCOMPARE(image->root()->childCount(), 1U);
}

void KisImageTest::testCloneImage()
{
    KisImageSP image = new KisImage(0, IMAGE_WIDTH, IMAGE_WIDTH, 0, "layer tests");
    QVERIFY(image->rootLayer() != 0);
    QVERIFY(image->rootLayer()->firstChild() == 0);

    KisAnnotationSP annotation = new KisAnnotation("mytype", "mydescription", QByteArray());
    image->addAnnotation(annotation);
    QVERIFY(image->annotation("mytype"));

    KisProofingConfigurationSP proofing = toQShared(new KisProofingConfiguration());
    image->setProofingConfiguration(proofing);
    QVERIFY(image->proofingConfiguration());

    const KoColor defaultColor(Qt::green, image->colorSpace());
    image->setDefaultProjectionColor(defaultColor);
    QCOMPARE(image->defaultProjectionColor(), defaultColor);

    KisLayerSP layer = new KisPaintLayer(image, "layer1", OPACITY_OPAQUE_U8);
    image->addNode(layer);
    KisLayerSP layer2 = new KisPaintLayer(image, "layer2", OPACITY_OPAQUE_U8);
    image->addNode(layer2);

    QVERIFY(layer->visible());
    QVERIFY(layer2->visible());

    QVERIFY(TestUtil::findNode(image->root(), "layer1"));
    QVERIFY(TestUtil::findNode(image->root(), "layer2"));

    QUuid uuid1 = layer->uuid();
    QUuid uuid2 = layer2->uuid();

    {
        KisImageSP newImage = image->clone();

        KisNodeSP newLayer1 = TestUtil::findNode(newImage->root(), "layer1");
        KisNodeSP newLayer2 = TestUtil::findNode(newImage->root(), "layer2");

        QVERIFY(newLayer1);
        QVERIFY(newLayer2);

        QVERIFY(newLayer1->uuid() != uuid1);
        QVERIFY(newLayer2->uuid() != uuid2);

        KisAnnotationSP newAnnotation = newImage->annotation("mytype");
        QVERIFY(newAnnotation);
        QVERIFY(newAnnotation != annotation);


        KisProofingConfigurationSP newProofing = newImage->proofingConfiguration();
        QVERIFY(newProofing);
        QVERIFY(newProofing != proofing);

        QCOMPARE(newImage->defaultProjectionColor(), defaultColor);
    }

    {
        KisImageSP newImage = image->clone(true);

        KisNodeSP newLayer1 = TestUtil::findNode(newImage->root(), "layer1");
        KisNodeSP newLayer2 = TestUtil::findNode(newImage->root(), "layer2");

        QVERIFY(newLayer1);
        QVERIFY(newLayer2);

        QVERIFY(newLayer1->uuid() == uuid1);
        QVERIFY(newLayer2->uuid() == uuid2);
    }
}

void KisImageTest::testLayerComposition()
{
    KisImageSP image = new KisImage(0, IMAGE_WIDTH, IMAGE_WIDTH, 0, "layer tests");
    QVERIFY(image->rootLayer() != 0);
    QVERIFY(image->rootLayer()->firstChild() == 0);

    KisLayerSP layer = new KisPaintLayer(image, "layer1", OPACITY_OPAQUE_U8);
    image->addNode(layer);
    KisLayerSP layer2 = new KisPaintLayer(image, "layer2", OPACITY_OPAQUE_U8);
    image->addNode(layer2);

    QVERIFY(layer->visible());
    QVERIFY(layer2->visible());

    KisLayerComposition comp(image, "comp 1");
    comp.store();

    layer2->setVisible(false);

    QVERIFY(layer->visible());
    QVERIFY(!layer2->visible());

    KisLayerComposition comp2(image, "comp 2");
    comp2.store();

    KisLayerCompositionSP comp3 = toQShared(new KisLayerComposition(image, "comp 3"));
    comp3->store();
    image->addComposition(comp3);

    comp.apply();

    QVERIFY(layer->visible());
    QVERIFY(layer2->visible());

    comp2.apply();

    QVERIFY(layer->visible());
    QVERIFY(!layer2->visible());

    comp.apply();

    QVERIFY(layer->visible());
    QVERIFY(layer2->visible());

    KisImageSP newImage = image->clone();

    KisNodeSP newLayer1 = TestUtil::findNode(newImage->root(), "layer1");
    KisNodeSP newLayer2 = TestUtil::findNode(newImage->root(), "layer2");

    QVERIFY(newLayer1);
    QVERIFY(newLayer2);

    QVERIFY(newLayer1->visible());
    QVERIFY(newLayer2->visible());

    KisLayerComposition newComp1(comp, newImage);
    newComp1.apply();
    QVERIFY(newLayer1->visible());
    QVERIFY(newLayer2->visible());

    KisLayerComposition newComp2(comp2, newImage);
    newComp2.apply();
    QVERIFY(newLayer1->visible());
    QVERIFY(!newLayer2->visible());

    newComp1.apply();
    QVERIFY(newLayer1->visible());
    QVERIFY(newLayer2->visible());

    QVERIFY(!newImage->compositions().isEmpty());
    KisLayerCompositionSP newComp3 = newImage->compositions().first();
    newComp3->apply();
    QVERIFY(newLayer1->visible());
    QVERIFY(!newLayer2->visible());
}

#include "kis_transparency_mask.h"
#include "kis_psd_layer_style.h"

struct FlattenTestImage
{
    FlattenTestImage()
        : refRect(0,0,512,512)
        , p(refRect)
    {

        image = p.image;
        undoStore = p.undoStore;
        layer1 = p.layer;

        layer5 = new KisPaintLayer(p.image, "paint5", 0.4 * OPACITY_OPAQUE_U8);
        layer5->disableAlphaChannel(true);

        layer2 = new KisPaintLayer(p.image, "paint2", OPACITY_OPAQUE_U8);
        tmask = new KisTransparencyMask(p.image, "tmask");

        // check channel flags
        // make addition composite op
        group1 = new KisGroupLayer(p.image, "group1", OPACITY_OPAQUE_U8);
        layer3 = new KisPaintLayer(p.image, "paint3", OPACITY_OPAQUE_U8);
        layer4 = new KisPaintLayer(p.image, "paint4", OPACITY_OPAQUE_U8);

        layer6 = new KisPaintLayer(p.image, "paint6", OPACITY_OPAQUE_U8);

        layer7 = new KisPaintLayer(p.image, "paint7", OPACITY_OPAQUE_U8);
        layer8 = new KisPaintLayer(p.image, "paint8", OPACITY_OPAQUE_U8);
        layer7->setCompositeOpId(COMPOSITE_ADD);
        layer8->setCompositeOpId(COMPOSITE_ADD);

        QRect rect1(100, 100, 100, 100);
        QRect rect2(150, 150, 150, 150);
        QRect tmaskRect(200,200,100,100);

        QRect rect3(400, 100, 100, 100);
        QRect rect4(500, 100, 100, 100);

        QRect rect5(50, 50, 100, 100);

        QRect rect6(50, 250, 100, 100);

        QRect rect7(50, 350, 50, 50);
        QRect rect8(50, 400, 50, 50);

        layer1->paintDevice()->fill(rect1, KoColor(Qt::red, p.image->colorSpace()));

        layer2->paintDevice()->fill(rect2, KoColor(Qt::green, p.image->colorSpace()));
        tmask->testingInitSelection(tmaskRect, layer2);

        layer3->paintDevice()->fill(rect3, KoColor(Qt::blue, p.image->colorSpace()));
        layer4->paintDevice()->fill(rect4, KoColor(Qt::yellow, p.image->colorSpace()));
        layer5->paintDevice()->fill(rect5, KoColor(Qt::green, p.image->colorSpace()));

        layer6->paintDevice()->fill(rect6, KoColor(Qt::cyan, p.image->colorSpace()));

        layer7->paintDevice()->fill(rect7, KoColor(Qt::red, p.image->colorSpace()));
        layer8->paintDevice()->fill(rect8, KoColor(Qt::green, p.image->colorSpace()));

        KisPSDLayerStyleSP style(new KisPSDLayerStyle());
        style->dropShadow()->setEffectEnabled(true);
        style->dropShadow()->setDistance(10.0);
        style->dropShadow()->setSpread(80.0);
        style->dropShadow()->setSize(10);
        style->dropShadow()->setNoise(0);
        style->dropShadow()->setKnocksOut(false);
        style->dropShadow()->setOpacity(80.0);
        layer2->setLayerStyle(style);

        layer2->setCompositeOpId(COMPOSITE_ADD);
        group1->setCompositeOpId(COMPOSITE_ADD);

        p.image->addNode(layer5);

        p.image->addNode(layer2);
        p.image->addNode(tmask, layer2);

        p.image->addNode(group1);
        p.image->addNode(layer3, group1);
        p.image->addNode(layer4, group1);

        p.image->addNode(layer6);

        p.image->addNode(layer7);
        p.image->addNode(layer8);

        p.image->initialRefreshGraph();

        // dbgKrita << ppVar(layer1->exactBounds());
        // dbgKrita << ppVar(layer5->exactBounds());
        // dbgKrita << ppVar(layer2->exactBounds());
        // dbgKrita << ppVar(group1->exactBounds());
        // dbgKrita << ppVar(layer3->exactBounds());
        // dbgKrita << ppVar(layer4->exactBounds());

        TestUtil::ReferenceImageChecker chk("flatten", "imagetest");
        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
    }

    QRect refRect;
    TestUtil::MaskParent p;

    KisImageSP image;
    KisSurrogateUndoStore *undoStore;
    KisPaintLayerSP layer1;

    KisPaintLayerSP layer2;
    KisTransparencyMaskSP tmask;

    KisGroupLayerSP group1;
    KisPaintLayerSP layer3;
    KisPaintLayerSP layer4;

    KisPaintLayerSP layer5;

    KisPaintLayerSP layer6;

    KisPaintLayerSP layer7;
    KisPaintLayerSP layer8;
};

template<class ContainerTest>
KisLayerSP flattenLayerHelper(ContainerTest &p, KisLayerSP layer, bool nothingHappens = false)
{
    QSignalSpy spy(p.image.data(), SIGNAL(sigNodeAddedAsync(KisNodeSP, KisNodeAdditionFlags)));

    //p.image->flattenLayer(layer);
    KisLayerUtils::flattenLayer(p.image, layer);
    p.image->waitForDone();

    if (nothingHappens) {
        Q_ASSERT(!spy.count());
        return layer;
    }

    Q_ASSERT(spy.count() == 1);
    QList<QVariant> arguments = spy.takeFirst();
    KisNodeSP newNode = arguments.first().value<KisNodeSP>();

    KisLayerSP newLayer = qobject_cast<KisLayer*>(newNode.data());
    return newLayer;
}

void KisImageTest::testFlattenLayer()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker chk("flatten", "imagetest");

    {
        QCOMPARE(p.layer2->compositeOpId(), COMPOSITE_ADD);

        KisLayerSP newLayer = flattenLayerHelper(p, p.layer2);

        //KisLayerSP newLayer = p.image->flattenLayer(p.layer2);
        //p.image->waitForDone();

        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer2_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
    }

    {
        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_ADD);

        KisLayerSP newLayer = flattenLayerHelper(p, p.group1);

        //KisLayerSP newLayer = p.image->flattenLayer(p.group1);
        //p.image->waitForDone();

        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "02_group1_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(newLayer->exactBounds(), QRect(400, 100, 200, 100));
    }

    {
        QCOMPARE(p.layer5->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.layer5->alphaChannelDisabled(), true);

        KisLayerSP newLayer = flattenLayerHelper(p, p.layer5, true);

        //KisLayerSP newLayer = p.image->flattenLayer(p.layer5);
        //p.image->waitForDone();

        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "03_layer5_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->exactBounds(), QRect(50, 50, 100, 100));

        QCOMPARE(newLayer->alphaChannelDisabled(), true);
    }
}

#include <kis_meta_data_merge_strategy_registry.h>

template<class ContainerTest>
KisLayerSP mergeHelper(ContainerTest &p, KisLayerSP layer)
{
    KisNodeSP parent = layer->parent();
    const int newIndex = parent->index(layer) - 1;

    p.image->mergeDown(layer, KisMetaData::MergeStrategyRegistry::instance()->get("Drop"));

    //KisLayerUtils::mergeDown(p.image, layer, KisMetaData::MergeStrategyRegistry::instance()->get("Drop"));
    p.image->waitForDone();

    KisLayerSP newLayer = qobject_cast<KisLayer*>(parent->at(newIndex).data());
    return newLayer;
}

void KisImageTest::testMergeDown()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");
    TestUtil::ReferenceImageChecker chk("mergedown_simple", "imagetest");


    {
        QCOMPARE(p.layer5->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.layer5->alphaChannelDisabled(), true);

        KisLayerSP newLayer = mergeHelper(p, p.layer5);

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer5_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->alphaChannelDisabled(), false);
    }

    {
        QCOMPARE(p.layer2->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.layer2->alphaChannelDisabled(), false);

        KisLayerSP newLayer = mergeHelper(p, p.layer2);

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "02_layer2_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->exactBounds(), QRect(100, 100, 213, 217));
        QCOMPARE(newLayer->alphaChannelDisabled(), false);
    }

    {
        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.group1->alphaChannelDisabled(), false);

        KisLayerSP newLayer = mergeHelper(p, p.group1);

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "03_group1_mergedown_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->exactBounds(), QRect(100, 100, 500, 217));
        QCOMPARE(newLayer->alphaChannelDisabled(), false);
    }
}

void KisImageTest::testMergeDownDestinationInheritsAlpha()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");
    TestUtil::ReferenceImageChecker chk("mergedown_dst_inheritsalpha", "imagetest");

    {
        QCOMPARE(p.layer2->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.layer2->alphaChannelDisabled(), false);

        KisLayerSP newLayer = mergeHelper(p, p.layer2);

        // WARN: this check is suspicious!
        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_proj_merged_layer2_over_layer5_IA"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer2_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->exactBounds(), QRect(50,50, 263, 267));
        QCOMPARE(newLayer->alphaChannelDisabled(), false);
    }
}

void KisImageTest::testMergeDownDestinationCustomCompositeOp()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");
    TestUtil::ReferenceImageChecker chk("mergedown_dst_customop", "imagetest");

    {
        QCOMPARE(p.layer6->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.layer6->alphaChannelDisabled(), false);

        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.group1->alphaChannelDisabled(), false);

        KisLayerSP newLayer = mergeHelper(p, p.layer6);

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer6_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->exactBounds(), QRect(50, 100, 550, 250));
        QCOMPARE(newLayer->alphaChannelDisabled(), false);
    }
}

void KisImageTest::testMergeDownDestinationSameCompositeOpLayerStyle()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");
    TestUtil::ReferenceImageChecker chk("mergedown_sameop_ls", "imagetest");

    {
        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.group1->alphaChannelDisabled(), false);

        QCOMPARE(p.layer2->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.layer2->alphaChannelDisabled(), false);

        KisLayerSP newLayer = mergeHelper(p, p.group1);

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_group1_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->exactBounds(), QRect(197, 100, 403, 217));
        QCOMPARE(newLayer->alphaChannelDisabled(), false);
    }
}

void KisImageTest::testMergeDownDestinationSameCompositeOp()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");
    TestUtil::ReferenceImageChecker chk("mergedown_sameop_fastpath", "imagetest");

    {
        QCOMPARE(p.layer8->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.layer8->alphaChannelDisabled(), false);

        QCOMPARE(p.layer7->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(p.layer7->alphaChannelDisabled(), false);

        KisLayerSP newLayer = mergeHelper(p, p.layer8);

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer8_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_ADD);
        QCOMPARE(newLayer->exactBounds(), QRect(50, 350, 50, 100));
        QCOMPARE(newLayer->alphaChannelDisabled(), false);
    }
}
#include "kis_image_animation_interface.h"
void KisImageTest::testMergeDownMultipleFrames()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");
    TestUtil::ReferenceImageChecker chk("mergedown_simple", "imagetest");

    QSet<int> initialFrames;
    {
        KisLayerSP l = p.layer5;
        l->enableAnimation();
        KisKeyframeChannel *channel = l->getKeyframeChannel(KisKeyframeChannel::Raster.id(), true);
        channel->addKeyframe(10);
        channel->addKeyframe(20);
        channel->addKeyframe(30);

        QCOMPARE(channel->keyframeCount(), 4);
        initialFrames = KisLayerUtils::fetchLayerFramesRecursive(l);
        QCOMPARE(initialFrames.size(), 4);
    }

    {
        QCOMPARE(p.layer5->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.layer5->alphaChannelDisabled(), true);

        KisLayerSP newLayer = mergeHelper(p, p.layer5);

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer5_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(newLayer->alphaChannelDisabled(), false);

        QVERIFY(newLayer->isAnimated());

        QSet<int> newFrames = KisLayerUtils::fetchLayerFramesRecursive(newLayer);
        QCOMPARE(newFrames, initialFrames);

        foreach (int frame, newFrames) {
            KisImageAnimationInterface *interface = p.image->animationInterface();
            int savedSwitchedTime = 0;
            interface->saveAndResetCurrentTime(frame, &savedSwitchedTime);
            QCOMPARE(newLayer->exactBounds(), QRect(100,100,100,100));
            interface->restoreCurrentTime(&savedSwitchedTime);
        }

        p.undoStore->undo();
        p.image->waitForDone();

         QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
    }
}

template<class ContainerTest>
KisNodeSP mergeMultipleHelper(ContainerTest &p, QList<KisNodeSP> selectedNodes, KisNodeSP putAfter)
{
    QSignalSpy spy(p.image.data(), SIGNAL(sigNodeAddedAsync(KisNodeSP, KisNodeAdditionFlags)));

    p.image->mergeMultipleLayers(selectedNodes, putAfter);
    //KisLayerUtils::mergeMultipleLayers(p.image, selectedNodes, putAfter);
    p.image->waitForDone();

    Q_ASSERT(spy.count() == 1);
    QList<QVariant> arguments = spy.takeFirst();
    KisNodeSP newNode = arguments.first().value<KisNodeSP>();
    return newNode;
}
void KisImageTest::testMergeMultiple()
{
    FlattenTestImage p;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");
    TestUtil::ReferenceImageChecker chk("mergemultiple", "imagetest");

    {
        QList<KisNodeSP> selectedNodes;

        selectedNodes << p.layer2
                      << p.group1
                      << p.layer6;

        {
            KisNodeSP newLayer = mergeMultipleHelper(p, selectedNodes, 0);

            //KisNodeSP newLayer = p.image->mergeMultipleLayers(selectedNodes, 0);
            //p.image->waitForDone();

            QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
            QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer8_layerproj"));

            QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
            QCOMPARE(newLayer->exactBounds(), QRect(50, 100, 550, 250));
        }
    }

    p.p.undoStore->undo();
    p.image->waitForDone();


    // Test reversed order, the result must be the same

    {
        QList<KisNodeSP> selectedNodes;

        selectedNodes << p.layer6
                      << p.group1
                      << p.layer2;

        {
            KisNodeSP newLayer = mergeMultipleHelper(p, selectedNodes, 0);

            //KisNodeSP newLayer = p.image->mergeMultipleLayers(selectedNodes, 0);
            //p.image->waitForDone();

            QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
            QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_layer8_layerproj"));

            QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
            QCOMPARE(newLayer->exactBounds(), QRect(50, 100, 550, 250));
        }
    }

}

void testMergeCrossColorSpaceImpl(bool useProjectionColorSpace, bool swapSpaces)
{
    TestUtil::MaskParent p;

    KisPaintLayerSP layer1;
    KisPaintLayerSP layer2;
    KisPaintLayerSP layer3;

    const KoColorSpace *cs2 = useProjectionColorSpace ?
        p.image->colorSpace() : KoColorSpaceRegistry::instance()->lab16();

    const KoColorSpace *cs3 = KoColorSpaceRegistry::instance()->rgb16();

    if (swapSpaces) {
        std::swap(cs2, cs3);
    }

    dbgKrita << "Testing testMergeCrossColorSpaceImpl:";
    dbgKrita << "    " << ppVar(cs2);
    dbgKrita << "    " << ppVar(cs3);

    layer1 = p.layer;
    layer2 = new KisPaintLayer(p.image, "paint2", OPACITY_OPAQUE_U8, cs2);
    layer3 = new KisPaintLayer(p.image, "paint3", OPACITY_OPAQUE_U8, cs3);

    QRect rect1(100, 100, 100, 100);
    QRect rect2(150, 150, 150, 150);
    QRect rect3(250, 250, 200, 200);

    layer1->paintDevice()->fill(rect1, KoColor(Qt::red, layer1->colorSpace()));
    layer2->paintDevice()->fill(rect2, KoColor(Qt::green, layer2->colorSpace()));
    layer3->paintDevice()->fill(rect3, KoColor(Qt::blue, layer3->colorSpace()));

    p.image->addNode(layer2);
    p.image->addNode(layer3);

    p.image->initialRefreshGraph();

    {
        KisLayerSP newLayer = mergeHelper(p, layer3);

        QCOMPARE(newLayer->colorSpace(), p.image->colorSpace());

        p.undoStore->undo();
        p.image->waitForDone();
    }

    {
        layer2->disableAlphaChannel(true);

        KisLayerSP newLayer = mergeHelper(p, layer3);

        QCOMPARE(newLayer->colorSpace(), p.image->colorSpace());

        p.undoStore->undo();
        p.image->waitForDone();
    }
}

void KisImageTest::testMergeCrossColorSpace()
{
    testMergeCrossColorSpaceImpl(true, false);
    testMergeCrossColorSpaceImpl(true, true);
    testMergeCrossColorSpaceImpl(false, false);
    testMergeCrossColorSpaceImpl(false, true);
}

void KisImageTest::testMergeSelectionMasks()
{
    TestUtil::MaskParent p;

    QRect rect1(100, 100, 100, 100);
    QRect rect2(150, 150, 150, 150);
    QRect rect3(50, 50, 100, 100);

    KisPaintLayerSP layer1 = p.layer;
    layer1->paintDevice()->fill(rect1, KoColor(Qt::red, layer1->colorSpace()));

    p.image->initialRefreshGraph();

    KisSelectionSP sel = new KisSelection(layer1->paintDevice()->defaultBounds(), toQShared(new KisImageResolutionProxy(p.image)));

    sel->pixelSelection()->select(rect2, MAX_SELECTED);
    KisSelectionMaskSP mask1 = new KisSelectionMask(p.image);
    mask1->initSelection(sel, layer1);
    p.image->addNode(mask1, layer1);

    QVERIFY(!layer1->selection());

    mask1->setActive(true);

    QCOMPARE(layer1->selection()->selectedExactRect(), QRect(150,150,150,150));

    sel->pixelSelection()->select(rect3, MAX_SELECTED);
    KisSelectionMaskSP mask2 = new KisSelectionMask(p.image);
    mask2->initSelection(sel, layer1);
    p.image->addNode(mask2, layer1);

    QCOMPARE(layer1->selection()->selectedExactRect(), QRect(150,150,150,150));

    mask2->setActive(true);

    QCOMPARE(layer1->selection()->selectedExactRect(), QRect(50,50,250,250));

    QList<KisNodeSP> selectedNodes;

    selectedNodes << mask2
                  << mask1;

    {
        KisNodeSP newLayer = mergeMultipleHelper(p, selectedNodes, 0);
        QCOMPARE(newLayer->parent(), KisNodeSP(layer1));
        QCOMPARE((int)layer1->childCount(), 1);
        QCOMPARE(layer1->selection()->selectedExactRect(), QRect(50,50,250,250));
    }
}

void KisImageTest::testFlattenImage()
{
    FlattenTestImage p;
    KisImageSP image = p.image;

    TestUtil::ReferenceImageChecker img("flatten", "imagetest");

    {
        KisLayerUtils::flattenImage(p.image, 0);
        p.image->waitForDone();
        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));

        p.undoStore->undo();
        p.image->waitForDone();

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
    }

    {
        KisLayerUtils::flattenImage(p.image, p.layer5); // flatten with active layer just under the root (not inside any group)
        p.image->waitForDone();
        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));

        p.undoStore->undo();
        p.image->waitForDone();

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
    }

    {
        KisLayerUtils::flattenImage(p.image, p.layer2); // flatten with active layer just under the root (not inside any group), but with a mask
        p.image->waitForDone();
        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));

        p.undoStore->undo();
        p.image->waitForDone();

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
    }

    {
        KisLayerUtils::flattenImage(p.image, p.layer3); // flatten with active layer inside of a group
        p.image->waitForDone();
        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));

        p.undoStore->undo();
        p.image->waitForDone();

        QVERIFY(img.checkDevice(p.image->projection(), p.image, "00_initial"));
    }
}

struct FlattenPassThroughTestImage
{
    FlattenPassThroughTestImage()
        : refRect(0,0,512,512)
        , p(refRect)
    {

        image = p.image;
        undoStore = p.undoStore;

        group1 = new KisGroupLayer(p.image, "group1", OPACITY_OPAQUE_U8);
        layer2 = new KisPaintLayer(p.image, "paint2", OPACITY_OPAQUE_U8);
        layer3 = new KisPaintLayer(p.image, "paint3", OPACITY_OPAQUE_U8);

        group4 = new KisGroupLayer(p.image, "group4", OPACITY_OPAQUE_U8);
        layer5 = new KisPaintLayer(p.image, "paint5", OPACITY_OPAQUE_U8);
        layer6 = new KisPaintLayer(p.image, "paint6", OPACITY_OPAQUE_U8);

        QRect rect2(100, 100, 100, 100);
        QRect rect3(150, 150, 100, 100);

        QRect rect5(200, 200, 100, 100);
        QRect rect6(250, 250, 100, 100);

        group1->setPassThroughMode(true);
        layer2->paintDevice()->fill(rect2, KoColor(Qt::red, p.image->colorSpace()));
        layer3->paintDevice()->fill(rect3, KoColor(Qt::green, p.image->colorSpace()));

        group4->setPassThroughMode(true);
        layer5->paintDevice()->fill(rect5, KoColor(Qt::blue, p.image->colorSpace()));
        layer6->paintDevice()->fill(rect6, KoColor(Qt::yellow, p.image->colorSpace()));


        p.image->addNode(group1);
        p.image->addNode(layer2, group1);
        p.image->addNode(layer3, group1);

        p.image->addNode(group4);
        p.image->addNode(layer5, group4);
        p.image->addNode(layer6, group4);

        p.image->initialRefreshGraph();

        TestUtil::ReferenceImageChecker chk("passthrough", "imagetest");
        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
    }

    QRect refRect;
    TestUtil::MaskParent p;

    KisImageSP image;
    KisSurrogateUndoStore *undoStore;

    KisGroupLayerSP group1;
    KisPaintLayerSP layer2;
    KisPaintLayerSP layer3;

    KisGroupLayerSP group4;
    KisPaintLayerSP layer5;
    KisPaintLayerSP layer6;
};

void KisImageTest::testFlattenPassThroughLayer()
{
    FlattenPassThroughTestImage p;

    TestUtil::ReferenceImageChecker chk("passthrough", "imagetest");

    {
        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.group1->passThroughMode(), true);

        KisLayerSP newLayer = flattenLayerHelper(p, p.group1);

        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(chk.checkDevice(newLayer->projection(), p.image, "01_group1_layerproj"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QVERIFY(newLayer->inherits("KisPaintLayer"));
    }
}

void KisImageTest::testMergeTwoPassThroughLayers()
{
    FlattenPassThroughTestImage p;

    TestUtil::ReferenceImageChecker chk("passthrough", "imagetest");

    {
        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.group1->passThroughMode(), true);

        KisLayerSP newLayer = mergeHelper(p, p.group4);

        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));

        QCOMPARE(newLayer->compositeOpId(), COMPOSITE_OVER);
        QVERIFY(newLayer->inherits("KisGroupLayer"));
    }
}

void KisImageTest::testMergePaintOverPassThroughLayer()
{
    FlattenPassThroughTestImage p;

    TestUtil::ReferenceImageChecker chk("passthrough", "imagetest");

    {
        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.group1->passThroughMode(), true);

        KisLayerSP newLayer = flattenLayerHelper(p, p.group4);
        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(newLayer->inherits("KisPaintLayer"));

        newLayer = mergeHelper(p, newLayer);
        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(newLayer->inherits("KisPaintLayer"));
    }
}

void KisImageTest::testMergePassThroughOverPaintLayer()
{
    FlattenPassThroughTestImage p;

    TestUtil::ReferenceImageChecker chk("passthrough", "imagetest");

    {
        QCOMPARE(p.group1->compositeOpId(), COMPOSITE_OVER);
        QCOMPARE(p.group1->passThroughMode(), true);

        KisLayerSP newLayer = flattenLayerHelper(p, p.group1);
        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(newLayer->inherits("KisPaintLayer"));

        newLayer = mergeHelper(p, p.group4);
        QVERIFY(chk.checkDevice(p.image->projection(), p.image, "00_initial"));
        QVERIFY(newLayer->inherits("KisPaintLayer"));
    }
}

#include "kis_paint_device_debug_utils.h"
#include "kis_algebra_2d.h"

void KisImageTest::testPaintOverlayMask()
{
    QRect refRect(0, 0, 512, 512);
    TestUtil::MaskParent p(refRect);

    QRect fillRect(50, 50, 412, 412);
    QRect selectionRect(200, 200, 100, 50);

    KisPaintLayerSP layer1 = p.layer;
    layer1->paintDevice()->fill(fillRect, KoColor(Qt::yellow, layer1->colorSpace()));

    KisSelectionMaskSP mask = new KisSelectionMask(p.image);
    KisSelectionSP selection = new KisSelection(new KisMaskDefaultBounds(layer1), toQShared(new KisImageResolutionProxy(p.image)));

    selection->pixelSelection()->select(selectionRect, 128);
    selection->pixelSelection()->select(KisAlgebra2D::blowRect(selectionRect,-0.3), 255);

    mask->setSelection(selection);

    //mask->setVisible(false);
    //mask->setActive(false);

    p.image->addNode(mask, layer1);

    // a simple layer to disable oblige-child mechanism
    KisPaintLayerSP layer2 = new KisPaintLayer(p.image, "layer2", OPACITY_OPAQUE_U8);
    p.image->addNode(layer2);

    p.image->initialRefreshGraph();

    KIS_DUMP_DEVICE_2(p.image->projection(), refRect, "00_initial", "dd");

    p.image->setOverlaySelectionMask(mask);
    p.image->waitForDone();

    KIS_DUMP_DEVICE_2(p.image->projection(), refRect, "01_activated_00_image", "dd");
    KIS_DUMP_DEVICE_2(p.image->root()->original(), refRect, "01_activated_01_root_original", "dd");
    KIS_DUMP_DEVICE_2(p.image->root()->projection(), refRect, "01_activated_02_root_projection", "dd");

    KisImageSP clonedImage = p.image->clone();
    clonedImage->waitForDone();
    KIS_DUMP_DEVICE_2(clonedImage->projection(), refRect, "02_cloned_when_activated_00_image", "dd");
    KIS_DUMP_DEVICE_2(clonedImage->root()->original(), refRect, "02_cloned_when_activated_01_root_original", "dd");
    KIS_DUMP_DEVICE_2(clonedImage->root()->projection(), refRect, "02_cloned_when_activated_02_root_projection", "dd");

    p.image->setOverlaySelectionMask(0);
    p.image->waitForDone();

    KIS_DUMP_DEVICE_2(p.image->projection(), refRect, "03_deactivated", "dd");
}

KISTEST_MAIN(KisImageTest)
