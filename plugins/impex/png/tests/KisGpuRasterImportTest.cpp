/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testui.h>
#include <KisGpuPixelTolerance.h>
#include <KisPaintingEngineTestScope.h>
#include <KisGpuPaintDeviceTransfer_p.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_entry.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>
#include <KoColor.h>
#include <QFile>
#include <QTemporaryDir>
#include <memory>
#include <stdexcept>

namespace {
QByteArray pixels(KisPaintDeviceSP device, QRect area)
{
    QByteArray bytes(qsizetype(area.width()) * area.height() * device->pixelSize(), Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
    return bytes;
}

std::unique_ptr<KisDocument> document()
{
    std::unique_ptr<KisDocument> result(KisPart::instance()->createDocument());
    result->setFileBatchMode(true);
    result->setAutoSaveActive(false);
    return result;
}
}

class KisGpuRasterImportTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nativeFormatPixelsKeepEncodingAndFiles_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("jpeg-u8-icc-orientation") << false;
        QTest::newRow("png-u16-interlaced-alpha-rejection") << true;
    }
    void nativeFormatPixelsKeepEncodingAndFiles();
};

void KisGpuRasterImportTest::nativeFormatPixelsKeepEncodingAndFiles()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, boundary);
    const auto *space = KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(),
        boundary ? Integer16BitsColorDepthID.id() : Integer8BitsColorDepthID.id(),
        KoColorSpaceRegistry::instance()->p2020G10Profile());
    QVERIFY(space);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto sourcePath = directory.filePath(boundary ? QStringLiteral("source.png") : QStringLiteral("source.jpg"));
    {
        const TestUtil::PaintingEngineScope cpu(false);
        auto source = document();
        KisImageSP image = new KisImage(nullptr, 270, 259, space, QStringLiteral("Format pixels"));
        image->setResolution(300.0 / 72, 150.0 / 72);
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Source"), 255);
        layer->paintDevice()->fill(image->bounds(), KoColor(QColor(43, 157, 211), space));
        layer->paintDevice()->fill({252, 247, 18, 12}, KoColor(QColor(211, 37, 79, boundary ? 173 : 255), space));
        if (boundary) {
            const quint16 bgra[] = {591, 12345, 54321, 1};
            layer->paintDevice()->fill({255, 255, 3, 3}, KoColor(reinterpret_cast<const quint8 *>(bgra), space));
            layer->paintDevice()->fill({256, 256, 1, 1}, KoColor(Qt::transparent, space));
        }
        image->addNode(layer, image->root());
        image->initialRefreshGraph();
        image->waitForDone();
        source->setCurrentImage(image);
        KisPropertiesConfigurationSP options = new KisPropertiesConfiguration;
        options->setProperty("forceSRGB", false);
        options->setProperty("saveProfile", true);
        options->setProperty("saveSRGBProfile", true);
        options->setProperty("interlaced", boundary);
        options->setProperty("alpha", true);
        options->setProperty("tryToSaveAsIndexed", false);
        QVERIFY2(source->exportDocumentSync(sourcePath, boundary ? "image/png" : "image/jpeg", options), qPrintable(source->errorMessage()));
    }
    if (!boundary) {
        // A leading EXIF marker rotates the decoded RGB rows across both tile axes.
        QFile encoded(sourcePath);
        QVERIFY(encoded.open(QIODevice::ReadOnly));
        auto bytes = encoded.readAll();
        encoded.close();
        bytes.insert(2, QByteArray::fromHex("ffe1002245786966000049492a0008000000010012010300010000000600000000000000"));
        QVERIFY(encoded.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(encoded.write(bytes), bytes.size());
    }

    const TestUtil::PaintingEngineScope gpu(true);
    auto expected = document();
    {
        const TestUtil::PaintingEngineScope cpu(false);
        QVERIFY2(expected->openPath(sourcePath, KisDocument::DontAddToRecent), qPrintable(expected->errorMessage()));
    }
    expected->image()->waitForDone();
    auto actual = document();
    QVERIFY2(actual->openPath(sourcePath, KisDocument::DontAddToRecent), qPrintable(actual->errorMessage()));
    QVERIFY(actual->image()->gpuImage());
    QCOMPARE(actual->image()->size(), boundary ? QSize(270, 259) : QSize(259, 270));
    const auto referenceLayer = qobject_cast<KisLayer *>(expected->image()->root()->firstChild().data());
    QVERIFY(referenceLayer);
    const auto expectedPixels = pixels(referenceLayer->paintDevice(), expected->image()->bounds());
    const auto inspected = expected->image()->bounds().adjusted(-1, -1, 1, 1);
    const auto expectedEdges = pixels(referenceLayer->paintDevice(), inspected);
    const auto verifyImage = [&](KisImageSP image) {
        QCOMPARE(image->size(), expected->image()->size());
        QCOMPARE(image->colorSpace()->id(), space->id());
        QCOMPARE(image->colorSpace()->profile()->rawData(), space->profile()->rawData());
        QCOMPARE(image->xRes(), expected->image()->xRes());
        QCOMPARE(image->yRes(), expected->image()->yRes());
        const auto layer = qobject_cast<KisLayer *>(image->root()->firstChild().data());
        QVERIFY(layer);
        QVERIFY(!layer->nextSibling());
        QCOMPARE(layer->uuid(), referenceLayer->uuid());
        QCOMPARE(layer->name(), referenceLayer->name());
        QCOMPARE(QPoint(layer->x(), layer->y()), QPoint(referenceLayer->x(), referenceLayer->y()));
        QCOMPARE(layer->paintDevice()->extent(), referenceLayer->paintDevice()->extent());
        QCOMPARE(layer->opacity(), referenceLayer->opacity());
        auto keys = layer->metaData()->keys();
        auto expectedKeys = referenceLayer->metaData()->keys();
        keys.sort();
        expectedKeys.sort();
        QCOMPARE(keys, expectedKeys);
        for (const auto &entry : referenceLayer->metaData()->entries())
            QVERIFY(layer->metaData()->getEntry(entry.qualifiedName()) == entry);
        QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(layer->paintDevice(), image->bounds()), expectedPixels, boundary ? 2 : 1);
        QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(layer->paintDevice(), inspected), expectedEdges, boundary ? 2 : 1);
    };
    verifyImage(actual->image());

    if (boundary) {
        const auto row = expectedPixels.first(270 * space->pixelSize());
        const auto capacity = 4 * Gpu::RgbaTile::byteCount(Gpu::PixelFormat::Rgba16);
        QVERIFY_EXCEPTION_THROWN(KisGpuRasterImport(QSize(270, 259), space, capacity - 1), std::length_error);
        KisGpuRasterImport transfer({270, 259}, space, capacity);
        QVERIFY_EXCEPTION_THROWN(transfer.appendRow(QByteArrayView(row).first(row.size() - 1)), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(transfer.takeLayer(), std::invalid_argument);
        for (int y = 0; y < 259; ++y)
            transfer.appendRow(QByteArrayView(expectedPixels).sliced(y * row.size(), row.size()));
        const auto imported = transfer.takeLayer();
        QCOMPARE(imported.tiles.size(), 4);
        QVERIFY_EXCEPTION_THROWN(transfer.takeLayer(), std::invalid_argument);
    }

    const auto savedPath = directory.filePath(QStringLiteral("imported.kra"));
    QVERIFY2(actual->exportDocumentSync(savedPath, KisDocument::nativeFormatMimeType()), qPrintable(actual->errorMessage()));
    for (const bool useGpu : {true, false}) {
        const TestUtil::PaintingEngineScope engine(useGpu);
        auto reopened = document();
        QVERIFY2(reopened->openPath(savedPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        QCOMPARE(bool(reopened->image()->gpuImage()), useGpu);
        verifyImage(reopened->image());
    }
    auto owner = actual->image()->gpuImage();
    const auto id = owner->operations().snapshot()->layers.first().id;
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->setLayerOpacity(id, 197), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), before);
    verifyImage(actual->image());
}

KISTEST_MAIN(KisGpuRasterImportTest)
#include "KisGpuRasterImportTest.moc"
