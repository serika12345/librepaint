/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDisplayColor.h"
#include "KisGpuDocumentWindow.h"
#include "KisRgb8DisplayLookup.h"
#include "KisGpuTestDevice.h"
#include "KisGpuWindowTestEventLoop.h"
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoTestConfig.h>
#include <QApplication>
#include <QFile>
#include <QPointer>
#include <QStandardPaths>
#include <QtTest>
#include <random>

namespace {
using Renderer = Krita::Canvas::GpuRenderer;
using Store = KisGpuTileStore;
using Conversion = KoColorConversionTransformation;
const auto Intent = Conversion::IntentRelativeColorimetric;
const auto Flags = Conversion::ConversionFlags(Conversion::NoOptimization | Conversion::BlackpointCompensation | Conversion::CopyAlpha);
const KoColorSpace *displaySpace(const QString &name)
{
    QFile file(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/profiles/elles-icc-profiles/") + name);
    if (!file.open(QIODevice::ReadOnly)) return nullptr;
    auto *registry = KoColorSpaceRegistry::instance();
    const auto *profile = registry->createColorProfile(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), file.readAll());
    return profile ? registry->rgb8(profile) : nullptr;
}
}
class KisGpuDisplayColorTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void gpuPixelsMatchDisplayConversion_data() {
        QTest::addColumn<QString>("profileFile");
        for (const auto &file : {"ClayRGB-elle-V4-g22.icc", "Rec2020-elle-V2-g10.icc", "sRGB-elle-V4-g22.icc"})
            QTest::newRow(file) << QString::fromLatin1(file);
    }
    void gpuPixelsMatchDisplayConversion() {
        QFETCH(QString, profileFile);
        auto *registry = KoColorSpaceRegistry::instance();
        const auto *source = registry->rgb8(registry->p709SRGBProfile());
        const auto *destination = displaySpace(profileFile);
        QVERIFY(source && destination);
        const std::atomic_bool cancelled{false};
        const auto lookup = KisRgb8DisplayLookup::build(source, destination, Intent, Flags, cancelled);
        QCOMPARE(lookup.status, KisRgb8DisplayLookup::Status::Ready);
        QVERIFY(lookup.values);
        constexpr int PixelCount = 256 * 256;
        QByteArray bgra(PixelCount * 4, 0), expected(PixelCount * 4, 0);
        std::mt19937 random(718993);
        for (int i = 0; i < PixelCount; ++i) {
            for (int c = 0; c < 3; ++c) bgra[i * 4 + c] = char(random() & 255);
            bgra[i * 4 + 3] = char(i % 256);
        }
        std::unique_ptr<Conversion> converter(source->createColorConverter(destination, Intent, Flags));
        QVERIFY(converter && converter->isValid());
        converter->transform(reinterpret_cast<const quint8 *>(bgra.constData()),
            reinterpret_cast<quint8 *>(expected.data()), PixelCount);
        auto rgba = bgra;
        for (int i = 0; i < PixelCount; ++i) {
            std::swap(rgba[i * 4], rgba[i * 4 + 2]);
            std::swap(expected[i * 4], expected[i * 4 + 2]);
            if (!quint8(rgba.at(i * 4 + 3)))
                for (int c = 0; c < 4; ++c) expected[i * 4 + c] = 0;
        }
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 128 * Store::TileBytes);
        const QRect bounds(0, 0, 256, 256);
        const auto uploaded = store.upload(store.emptyVersion(), bounds, rgba);
        const auto image = store.textureSnapshot(uploaded.version, bounds);
        QVERIFY(waitForGpuWindow([&] { store.poll(); return image.completion.status() == Store::Status::Succeeded; }));
        WGPUTextureDescriptor descriptor{};
        descriptor.dimension = WGPUTextureDimension_2D;
        descriptor.size = {256, 256, 1};
        descriptor.format = WGPUTextureFormat_RGBA8Unorm;
        descriptor.mipLevelCount = descriptor.sampleCount = 1;
        descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
        const auto release = [](WGPUTextureImpl *texture) { wgpuTextureRelease(texture); };
        std::unique_ptr<WGPUTextureImpl, decltype(release)> target(wgpuDeviceCreateTexture(gpu.device, &descriptor), release);
        Renderer renderer(gpu.owner, descriptor.format);
        Renderer::View view;
        view.sampling = Renderer::Sampling::Nearest;
        view.backgroundRgba = 0;
        view.displayLookup = lookup.values;
        const auto before = store.statistics();
        const auto frame = renderer.render(image, target.get(), view);
        QCOMPARE(frame.error, Renderer::Error::None);
        QVERIFY(waitForGpuWindow([&] { renderer.poll(); return frame.status() == Store::Status::Succeeded; }));
        QCOMPARE(gpu.read(target.get(), bounds.size()), expected);
        QCOMPARE(renderer.statistics().displayLookupUploadBytes, quint64(Renderer::DisplayLookupBytes));
        QCOMPARE(store.statistics().pixelUploadBytes, before.pixelUploadBytes);
        QCOMPARE(store.statistics().pixelReadbackBytes, before.pixelReadbackBytes);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void profileRequestsCoalesceAndKeepTheDocumentUnchanged() {
        auto *registry = KoColorSpaceRegistry::instance();
        const auto *source = registry->rgb8(registry->p709SRGBProfile());
        const auto *clay = displaySpace(QStringLiteral("ClayRGB-elle-V4-g22.icc"));
        const auto *wide = displaySpace(QStringLiteral("Rec2020-elle-V2-g10.icc"));
        QVERIFY(source && clay && wide);
        KisGpuDocumentDevice documents;
        KisGpuApplicationState activity(documents);
        activity.applicationStateChanged(Qt::ApplicationActive);
        const auto id = documents.createDocument({});
        auto model = documents.document(id);
        model->addLayer(QStringLiteral("Paint"));
        KisGpuDocumentWindow window(documents, activity, id, {0, 0, 64, 64});
        window.resize(128, 160);
        window.show();
        auto *canvas = window.findChild<KisGpuCanvasSession *>();
        QVERIFY(canvas);
        auto *color = new KisGpuDisplayColor(window, source);
        QSignalSpy ready(color, &KisGpuDisplayColor::ready);
        QSignalSpy failed(color, &KisGpuDisplayColor::failed);
        int heartbeats = 0;
        QTimer heartbeat;
        connect(&heartbeat, &QTimer::timeout, this, [&] { ++heartbeats; });
        heartbeat.start(1);
        const auto original = model->layers().first().paint.pixels;
        QVERIFY(color->request(clay, Intent, Flags));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 20000);
        QVERIFY(heartbeats > 0);
        heartbeat.stop();
        QTRY_COMPARE_WITH_TIMEOUT(canvas->statistics().rendering.displayLookupUploadBytes, quint64(Renderer::DisplayLookupBytes), 20000);
        window.resize(160, 180);
        QVERIFY(color->request(wide, Intent, Flags));
        QVERIFY(color->request(clay, Intent, Flags));
        QVERIFY(color->request(source, Intent, Flags));
        QTRY_VERIFY_WITH_TIMEOUT(!color->busy(), 20000);
        QCOMPARE(ready.size(), 2);
        QTRY_COMPARE_WITH_TIMEOUT(canvas->statistics().rendering.residentDisplayLookupBytes, quint64(0), 20000);
        QCOMPARE(canvas->statistics().rendering.displayLookupUploadBytes, quint64(Renderer::DisplayLookupBytes));
        QVERIFY(!color->request(registry->rgb16(), Intent, Flags));
        QCOMPARE(ready.size(), 2);
        QCOMPARE(failed.size(), 0);
        QCOMPARE(model->layers().first().paint.pixels, original);
        QCOMPARE(documents.store(id)->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(documents.store(id)->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(documents.device()->errorCount(), 0);
    }
    void windowDestructionCancelsItsWorker() {
        auto *registry = KoColorSpaceRegistry::instance();
        const auto *source = registry->rgb8(registry->p709SRGBProfile());
        const auto *destination = displaySpace(QStringLiteral("ClayRGB-elle-V4-g22.icc"));
        QVERIFY(source && destination);
        KisGpuDocumentDevice documents;
        KisGpuApplicationState activity(documents);
        activity.applicationStateChanged(Qt::ApplicationActive);
        const auto id = documents.createDocument({});
        documents.document(id)->addLayer(QStringLiteral("Paint"));
        auto window = std::make_unique<KisGpuDocumentWindow>(documents, activity, id, QRect(0, 0, 64, 64));
        QPointer<KisGpuDisplayColor> color = new KisGpuDisplayColor(*window, source);
        QVERIFY(color->request(destination, Intent, Flags));
        window.reset();
        QVERIFY(color.isNull());
        QVERIFY(!documents.document(id));
        QVERIFY(!documents.device());
    }
};
int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("EXTRA_RESOURCE_DIRS", QByteArray(KRITA_RESOURCE_DIRS_FOR_TESTS));
    qputenv("KRITA_PLUGIN_PATH", QByteArray(KRITA_PLUGINS_DIR_FOR_TESTS));
    QApplication app(argc, argv);
    KisGpuDisplayColorTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuDisplayColorTest.moc"
