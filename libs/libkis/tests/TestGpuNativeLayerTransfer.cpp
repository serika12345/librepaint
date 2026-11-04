/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testui.h>
#include <KisGpuPixelTolerance.h>
#include <KisPaintingEngineTestScope.h>
#include <Node.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_layer_utils.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <memory>

namespace {
QByteArray pixels(KisPaintDeviceSP device, QRect area)
{
    QByteArray result(qsizetype(area.width()) * area.height() * device->pixelSize(), Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(result.data()), area);
    return result;
}

QByteArray defaultPixel(KisPaintDeviceSP device)
{
    const auto value = device->defaultPixel();
    return QByteArray(reinterpret_cast<const char *>(value.data()), device->pixelSize());
}
}

class TestGpuNativeLayerTransfer : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nativeLayersKeepPixelsHistoryAndFiles_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8") << false;
        QTest::newRow("u16-shifted-default-outside-capacity") << true;
    }
    void nativeLayersKeepPixelsHistoryAndFiles();
};

void TestGpuNativeLayerTransfer::nativeLayersKeepPixelsHistoryAndFiles()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, boundary);
    const TestUtil::PaintingEngineScope gpu(true);
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage input;
    input.size = {270, 48};
    input.format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    input.colorProfile = space->profile()->rawData();
    auto owner = Gpu::PaintImageSP::create(execution, input);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("CPU raster transfer"), space);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    document->setCurrentImage(image);
    KisNodeSP layer = new KisPaintLayer(image, QStringLiteral("CPU result"), 197);
    layer->setColorLabelIndex(3);
    layer->setPinnedToTimeline(true);
    auto native = layer->paintDevice();
    const QPoint position = boundary ? QPoint(11, -7) : QPoint();
    native->moveTo(position);
    KoColor background = native->defaultPixel();
    if (boundary) {
        const quint16 bgra[] = {591, 12345, 54321, 23456};
        background = KoColor(reinterpret_cast<const quint8 *>(bgra), space);
        native->setDefaultPixel(background);
    }
    native->fill({252, 3, 13, 19}, KoColor(QColor(43, 157, 211, 173), space));
    if (boundary) {
        native->fill({-263, -11, 9, 7}, KoColor(QColor(193, 37, 79, 211), space));
        native->fill({257, 9, 2, 2}, KoColor(Qt::transparent, space));
    }
    const QRect inspected(-272, -16, 560, 80);
    const auto expected = pixels(native, inspected);
    const auto expectedDefault = defaultPixel(native);
    const auto expectedExtent = native->extent();
    const auto uuid = layer->uuid();
    NodeSP root(Node::createNode(image, image->root()));
    NodeSP child(Node::createNode(image, layer));
    const auto before = owner->operations().snapshot();
    QVERIFY(root->addChildNode(child.data(), nullptr));
    QCOMPARE(image->root()->firstChild(), layer);
    QCOMPARE(layer->paintDevice(), native);
    QCOMPARE(QPoint(layer->x(), layer->y()), position);
    QCOMPARE(native->extent(), expectedExtent);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(defaultPixel(native), expectedDefault, boundary ? 2 : 1);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(child->pixelData(inspected.x(), inspected.y(), inspected.width(), inspected.height()), expected, boundary ? 2 : 1);
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!layer->parent());
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(image->root()->firstChild(), layer);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(native, inspected), expected, boundary ? 2 : 1);
    const auto expectedProjection = pixels(image->projection(), image->bounds());

    if (boundary) {
        Gpu::Limits limits;
        const qsizetype tileBytes = Gpu::RgbaTile::byteCount(input.format);
        limits.rasterBytes = tileBytes + tileBytes / 2;
        auto bounded = Gpu::PaintImageSP::create(execution, input, limits);
        auto boundedImage = KisImage::fromGpuImage(bounded, QStringLiteral("Bounded CPU transfer"), space);
        KisNodeSP probe = new KisPaintLayer(boundedImage, QStringLiteral("Redo retained"), 255);
        QVERIFY(boundedImage->insertGpuLayer(probe, 0));
        QCOMPARE(bounded->undo(), Gpu::Result::Success);
        KisNodeList candidates;
        for (int i = 0; i < 2; ++i) {
            KisNodeSP candidate = new KisPaintLayer(boundedImage, QStringLiteral("Candidate %1").arg(i), 255);
            candidate->paintDevice()->fill({1, 1, 3, 2}, KoColor(Qt::blue, space));
            candidates.append(candidate);
        }
        QCOMPARE(bounded->markSaved(bounded->operations().snapshot()), Gpu::Result::Success);
        const auto saved = bounded->operations().snapshot();
        const auto candidatePixels = pixels(candidates.first()->paintDevice(), {0, 0, 8, 8});
        QSignalSpy changed(bounded.data(), &Gpu::PaintImage::changed);
        QVERIFY(!boundedImage->insertGpuLayers(candidates, 0));
        QCOMPARE(changed.size(), 0);
        QCOMPARE(bounded->operations().snapshot(), saved);
        QCOMPARE(bounded->operations().redoCount(), 1);
        QVERIFY(!bounded->operations().isModified());
        QVERIFY(!candidates.first()->parent());
        QCOMPARE(pixels(candidates.first()->paintDevice(), {0, 0, 8, 8}), candidatePixels);
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("native-result.kra"));
    QVERIFY(document->exportDocumentSync(path, KisDocument::nativeFormatMimeType()));
    for (const bool useGpu : {true, false}) {
        const TestUtil::PaintingEngineScope engine(useGpu);
        std::unique_ptr<KisDocument> restored(KisPart::instance()->createDocument());
        restored->setFileBatchMode(true);
        restored->setAutoSaveActive(false);
        QVERIFY2(restored->openPath(path, KisDocument::DontAddToRecent), qPrintable(restored->errorMessage()));
        QCOMPARE(bool(restored->image()->gpuImage()), useGpu);
        auto reopened = KisLayerUtils::findNodeByUuid(restored->image()->root(), uuid);
        QVERIFY(reopened);
        QCOMPARE(reopened->name(), layer->name());
        QCOMPARE(reopened->opacity(), layer->opacity());
        QCOMPARE(reopened->colorLabelIndex(), layer->colorLabelIndex());
        QCOMPARE(reopened->isPinnedToTimeline(), layer->isPinnedToTimeline());
        QCOMPARE(QPoint(reopened->x(), reopened->y()), position);
        QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(defaultPixel(reopened->paintDevice()), expectedDefault, boundary ? 2 : 1);
        QCOMPARE(reopened->paintDevice()->extent(), expectedExtent);
        QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(reopened->paintDevice(), inspected), expected, boundary ? 2 : 1);
        restored->image()->waitForDone();
        QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(restored->image()->projection(), image->bounds()), expectedProjection, boundary ? 2 : 1);
    }
}

KISTEST_MAIN(TestGpuNativeLayerTransfer)
#include "TestGpuNativeLayerTransfer.moc"
