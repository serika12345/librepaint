/* SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "TestDocument.h"
#include <simpletest.h>

#include <KritaVersionWrapper.h>
#include <QColor>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QBuffer>
#include <QSignalSpy>
#include <QTextStream>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QDomDocument>
#include <QColorSpace>
#include <QScopeGuard>
#include <QDialog>
#include <QTimer>
#include <QSemaphore>
#include <QCheckBox>
#include <exception>
#include <future>
#include <chrono>
#include <cstring>
#include <limits>
#include <atomic>
#include <thread>

#include <Node.h>
#include <Krita.h>
#include <Document.h>
#include <InfoObject.h>
#include <KisImportExportManager.h>
#include <KisGpuExportRasterRows.h>
#include <QImageReader>
#include <metadata/KoDocumentInfo.h>
#include <metadata/kis_meta_data_store.h>
#include <metadata/kis_meta_data_entry.h>
#include <metadata/kis_meta_data_value.h>
#include <metadata/kis_meta_data_schema_registry.h>
#include <kra/KisKraRasterCodec.h>
#include <gpu/KisGpuPaintDocument.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <KisUniqueColorSet.h>
#include <KoStore.h>

#include <document/KisDocument.h>
#include <canvas/KisMirrorAxisConfig.h>
#include <kis_image.h>
#include <kis_image_animation_interface.h>
#include <kis_time_span.h>
#include <KisBusyWaitBroker.h>
#include <KisRunnableBasedStrokeStrategy.h>
#include <KisRunnableStrokeJobData.h>
#include <commands/kis_node_opacity_command.h>
#include <kis_image_config.h>
#include <kis_paint_device.h>
#include <kis_annotation.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <kis_fill_painter.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <application/ui/orchestration/KisPart.h>

#include <kis_transform_mask_params_factory_registry.h>
#include <kis_undo_stores.h>
#include <kis_crop_saved_extra_data.h>
#include <kis_properties_configuration.h>
#include <kundo2stack.h>
#include <testui.h>

#include <KisPortingUtils.h>

namespace {
class ExportProcessingJob final : public KisRunnableBasedStrokeStrategy
{
public:
    ExportProcessingJob() : KisRunnableBasedStrokeStrategy(QLatin1String("export-processing-job"))
    {
        enableJob(JOB_DOSTROKE);
        setClearsRedoOnStart(false);
    }
};

void initializeGpuDocument(KisDocument *document, QSize size, const QString &name)
{
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    if (!execution) throw std::runtime_error(diagnostic.toStdString());
    LibrePaint::Image::Gpu::RasterImage raster{size, {{QStringLiteral("Paint Layer 1"), true, {}, {}}}, 0};
    raster.colorProfile = KoColorSpaceRegistry::instance()->rgb8()->profile()->rawData();
    auto owner = LibrePaint::Image::Gpu::PaintImageSP::create(execution, raster);
    document->setCurrentImage(KisImage::fromGpuImage(owner, name));
}
}

void TestDocument::testColorHistoryReplacementAndCopyKeepNewestFirst_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("ordinary") << false;
    QTest::newRow("gpu") << true;
}

void TestDocument::testColorHistoryReplacementAndCopyKeepNewestFirst()
{
    QFETCH(bool, gpu);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoColor newest(QColor(192, 17, 64), cs), older(QColor(37, 163, 82), cs), other(QColor(25, 64, 211), cs);
    const QList<KoColor> expected{newest, older};
    const auto initialize = [&](KisDocument *document) {
        if (gpu) initializeGpuDocument(document, {4, 4}, QStringLiteral("Color history"));
        else {
            KisImageSP image = new KisImage(document->createUndoStore(), 4, 4, cs, QStringLiteral("Color history"));
            image->addNode(new KisPaintLayer(image, QStringLiteral("Paint"), 255));
            document->setCurrentImage(image);
            image->waitForDone();
        }
    };
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    initialize(source.data());
    source->setColorHistoryColors(expected);
    QCOMPARE(source->colorHistoryColors(), expected);
    QScopedPointer<KisDocument> snapshot(source->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    QCOMPARE(snapshot->colorHistoryColors(), expected);
    QScopedPointer<KisDocument> restored(KisPart::instance()->createDocument());
    initialize(restored.data());
    restored->setColorHistoryColors({other});
    restored->copyFromDocument(*source);
    QCOMPARE(restored->colorHistoryColors(), expected);
    restored->colorHistoryModel()->addColor(other);
    QCOMPARE(restored->colorHistoryColors(), (QList<KoColor>{other, newest, older}));
    QCOMPARE(source->colorHistoryColors(), expected);
    QCOMPARE(snapshot->colorHistoryColors(), expected);
    source.reset();
    QCOMPARE(restored->colorHistoryColors(), (QList<KoColor>{other, newest, older}));
    QCOMPARE(snapshot->colorHistoryColors(), expected);
}

void TestDocument::testNativeSavingKeepsColorHistoryOrderAndSelectedColor_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("gpuSession");
    QTest::newRow("ordinary") << false << false;
    QTest::newRow("gpu") << true << false;
    QTest::newRow("gpu-session") << true << true;
}

void TestDocument::testNativeSavingKeepsColorHistoryOrderAndSelectedColor()
{
    QFETCH(bool, gpu);
    QFETCH(bool, gpuSession);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", gpuSession ? "1" : "0");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const auto uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}"));
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        const LibrePaint::Image::Gpu::RasterImage raster{
            {4, 4}, {{QStringLiteral("Paint"), true, uuid}}};
        const auto owner = LibrePaint::Image::Gpu::PaintImageSP::create(execution, raster);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Color history")));
    } else {
        KisImageSP image = new KisImage(document->createUndoStore(), 4, 4, cs, QStringLiteral("Color history"));
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        layer->setUuid(uuid);
        image->addNode(layer);
        document->setCurrentImage(image);
        image->initialRefreshGraph();
        image->waitForDone();
    }
    const QColor newest(192, 17, 64), older(37, 163, 82);
    document->colorHistoryModel()->addColor(KoColor(older, cs));
    document->colorHistoryModel()->addColor(KoColor(newest, cs));
    const auto before = document->colorHistoryColors();
    document->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed color history"));
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    document->setModified(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto prefix = gpuSession ? QStringLiteral("color-history-session") :
        gpu ? QStringLiteral("color-history-gpu") : QStringLiteral("color-history-native");
    const auto path = QDir(destination).filePath(prefix + QStringLiteral(".kra"));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Color history save artifacts retained at" << destination;
        }
    });
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QCOMPARE(document->colorHistoryColors(), before);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QCOMPARE(bool(reopened->image()->gpuImage()), gpuSession);
    QCOMPARE(reopened->assistantsGlobalColor(), QColor(17, 63, 129, 201));
    const auto restored = reopened->colorHistoryColors();
    QCOMPARE(restored.size(), 2);
    // KRA's existing normalized color XML may use another channel depth.
    // Retain the exact selected colors and their priority when painting again.
    QCOMPARE(restored[0].toQColor(), newest);
    QCOMPARE(restored[1].toQColor(), older);
    auto *paint = dynamic_cast<KisPaintLayer *>(reopened->image()->root()->firstChild().data());
    QVERIFY(paint);
    auto selectedColor = restored[0];
    selectedColor.convertTo(paint->colorSpace());
    paint->paintDevice()->writeBytes(selectedColor.data(), 1, 1, 1, 1);
    QByteArray pixel(4, '\0');
    paint->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 1, 1, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("4011c0ff"));
    for (const bool clearHistory : {false, true}) {
        const QColor selected(25, 64, 211);
        const QList<KoColor> history = clearHistory ? QList<KoColor>{} : QList<KoColor>{KoColor(selected, cs)};
        const QColor assistants(67, 23, 149, 101);
        document->setColorHistoryColors(history);
        document->setAssistantsGlobalColor(assistants);
        document->setModified(true);
        const auto nextPath = QDir(destination).filePath(prefix +
            (clearHistory ? QStringLiteral("-cleared.kra") : QStringLiteral("-updated.kra")));
        QVERIFY2(document->saveAs(nextPath, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(document->errorMessage()));
        document->waitForSavingToComplete();
        QCOMPARE(document->colorHistoryColors(), history);
        QCOMPARE(document->assistantsGlobalColor(), assistants);
        reopened.reset(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(nextPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        QCOMPARE(bool(reopened->image()->gpuImage()), gpuSession);
        QCOMPARE(reopened->assistantsGlobalColor(), assistants);
        QCOMPARE(reopened->colorHistoryColors().size(), history.size());
        if (!clearHistory) QCOMPARE(reopened->colorHistoryColors().first().toQColor(), selected);
    }
}

void TestDocument::testExportImageWaitsForImageProcessingAndPreservesSource()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Busy export artifacts retained at" << directory.path();
        }
    });
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(document->createUndoStore(), 32, 16, cs, "Busy export");
    KisNodeSP layer = new KisPaintLayer(image, QStringLiteral("Queued pixels"), 255);
    image->addNode(layer);
    document->setCurrentImage(image);
    document->setFileBatchMode(true);
    const auto blue = QByteArray::fromHex("ff0000ff");
    const auto green = QByteArray::fromHex("00ff00ff");
    layer->paintDevice()->writeBytes(reinterpret_cast<const quint8*>(blue.constData()), 3, 3, 1, 1);
    document->undoStack()->push(new KisNodeOpacityCommand(layer, 127));
    layer->setDirty(image->bounds());
    image->waitForDone();
    Document api(document.data(), false);
    api.setFileName(directory.filePath(QStringLiteral("source.kra")));
    document->setModified(true);
    const auto name = api.fileName();
    const auto mime = document->mimeType();
    const auto history = document->undoStack()->index();
    const auto count = document->undoStack()->count();
    QSemaphore entered;
    std::atomic<bool> abort{false};
    const auto stroke = image->startStroke(new ExportProcessingJob);
    image->addJob(stroke, new KisRunnableStrokeJobData([&entered, &abort, layer, green]() mutable {
        entered.release();
        while (!abort.load() && !KisBusyWaitBroker::instance()->guiThreadIsWaitingForBetterWeather())
            std::this_thread::yield();
        if (abort.load()) return;
        layer->paintDevice()->writeBytes(reinterpret_cast<const quint8*>(green.constData()), 3, 3, 1, 1);
        layer->setDirty(QRect(3, 3, 1, 1));
    }));
    image->endStroke(stroke);
    const auto finishJob = qScopeGuard([&] {
        abort.store(true);
        image->waitForDone();
    });
    QVERIFY(entered.tryAcquire(1, 5000));
    QVERIFY(!image->isIdle());
    InfoObject settings;
    settings.setProperty("alpha", true);
    settings.setProperty("interlaced", false);
    settings.setProperty("compression", 3);
    settings.setProperty("saveSRGBProfile", true);
    settings.setProperty("forceSRGB", false);
    settings.setProperty("storeAuthor", false);
    settings.setProperty("storeMetaData", false);
    const auto path = directory.filePath(QStringLiteral("queued.png"));
    QVERIFY2(api.exportImage(path, settings), "A synchronous export must include completed queued image processing");
    const auto png = QImage(path).convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(png.size(), QSize(32, 16));
    QCOMPARE(png.pixelColor(3, 3), QColor(0, 255, 0, 127));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(png.constBits()), png.sizeInBytes()), api.pixelData(0, 0, 32, 16));
    QCOMPARE(png.colorSpace().iccProfile(), cs->profile()->rawData());
    QCOMPARE(document->image().data(), image.data());
    QCOMPARE(image->root()->firstChild(), layer);
    QCOMPARE(api.fileName(), name);
    QCOMPARE(document->mimeType(), mime);
    QVERIFY(document->isModified());
    QCOMPARE(document->undoStack()->index(), history);
    QCOMPARE(document->undoStack()->count(), count);
}

void TestDocument::testCanvasResizePreservesPixelsHistoryAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("cropLayers");
    QTest::addColumn<QRect>("requested");
    const QList<QPair<const char *, QRect>> regions{
        {"shrink", {8, 6, 32, 24}}, {"grow", {-5, -7, 80, 64}},
        {"same", {0, 0, 64, 48}}, {"outside", {100, 90, 16, 12}}};
    for (const bool gpu : {false, true}) for (const bool crop : {false, true}) for (const auto &region : regions) {
        const QByteArray name = (gpu ? QByteArrayLiteral("gpu-") : QByteArrayLiteral("ordinary-")) +
            (crop ? QByteArrayLiteral("crop-") : QByteArrayLiteral("canvas-")) + region.first;
        QTest::newRow(name.constData()) << gpu << crop << region.second;
    }
}

void TestDocument::testCanvasResizePreservesPixelsHistoryAndSaving()
try
{
    QFETCH(bool, gpu);
    QFETCH(bool, cropLayers);
    QFETCH(QRect, requested);
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const QSize originalSize(64, 48);
    const QRect sourceRegion(-8, -4, 80, 64), inspected(-160, -160, 320, 320);
    const QList<QPoint> positions{{7, -9}, {-11, 13}};
    const QList<QByteArray> defaults{QByteArray(4, '\0'), QByteArray::fromHex("33221100")};
    Gpu::RasterImage input;
    input.size = originalSize;
    input.layers = {{QStringLiteral("Visible pixels"), true, QUuid(QStringLiteral("{91929394-9596-9798-999a-9b9c9d9e9fa0}")), {}},
                    {QStringLiteral("Hidden locked pixels"), false, QUuid(QStringLiteral("{a1a2a3a4-a5a6-a7a8-a9aa-abacadaeafb0}")), {}}};
    QList<QByteArray> initialPixels;
    for (int i = 0; i < 2; ++i) {
        const auto &bgra = defaults[i];
        input.layers[i].position = positions[i];
        input.layers[i].defaultPixel = {quint8(bgra[2]), quint8(bgra[1]), quint8(bgra[0]), quint8(bgra[3])};
        input.layers[i].userLocked = i == 1;
        QByteArray pixels(sourceRegion.width() * sourceRegion.height() * 4, Qt::Uninitialized);
        for (int y = 0; y < sourceRegion.height(); ++y) for (int x = 0; x < sourceRegion.width(); ++x) {
            auto *pixel = pixels.data() + (y * sourceRegion.width() + x) * 4;
            pixel[0] = char((x * 3 + y + i * 17) & 255);
            pixel[1] = char((x + y * 7 + i * 29) & 255);
            pixel[2] = char((x * 11 + y * 5 + i * 31) & 255);
            pixel[3] = i ? 0 : char(255);
        }
        initialPixels.append(pixels);
    }
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    Gpu::PaintImageSP owner;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        owner = Gpu::PaintImageSP::create(execution, input);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Canvas resize")));
    } else {
        KisImageSP image = new KisImage(document->createUndoStore(), 64, 48, cs, QStringLiteral("Canvas resize"));
        for (int i = 0; i < 2; ++i) {
            const auto &properties = input.layers[i];
            KisPaintLayerSP layer = new KisPaintLayer(image, properties.name, 255);
            layer->setUuid(properties.uuid);
            layer->setX(positions[i].x()); layer->setY(positions[i].y());
            layer->setVisible(properties.visible); layer->setUserLocked(properties.userLocked);
            layer->paintDevice()->setDefaultPixel(KoColor(reinterpret_cast<const quint8*>(defaults[i].constData()), cs));
            image->addNode(layer, image->root(), i);
        }
        document->setCurrentImage(image);
    }
    Document api(document.data(), false);
    QList<NodeSP> nodes;
    QList<QByteArray> originals, expected;
    for (int i = 0; i < 2; ++i) {
        nodes.append(NodeSP(Node::createNode(document->image(), document->image()->root()->at(i))));
        const auto &pixels = initialPixels[i];
        QVERIFY(nodes[i]->setPixelData(pixels, sourceRegion.x(), sourceRegion.y(), sourceRegion.width(), sourceRegion.height()));
        originals.append(nodes[i]->pixelData(inspected.x(), inspected.y(), inspected.width(), inspected.height()));
        auto result = defaults[i].repeated(inspected.width() * inspected.height());
        for (int y = 0; y < inspected.height(); ++y) for (int x = 0; x < inspected.width(); ++x) {
            const QPoint source = inspected.topLeft() + QPoint(x, y) + requested.topLeft();
            if (!sourceRegion.contains(source) || (cropLayers && !requested.contains(source))) continue;
            const int offset = ((source.y() - sourceRegion.y()) * sourceRegion.width() + source.x() - sourceRegion.x()) * 4;
            std::memcpy(result.data() + (y * inspected.width() + x) * 4, pixels.constData() + offset, 4);
        }
        expected.append(result);
    }
    api.refreshProjection();
    document->setModified(false);
    const int history = document->undoStack()->index();
    const int steps = cropLayers || requested != QRect(QPoint(), originalSize) ? 1 : 0;
    QSignalSpy sizeChanged(document->image().data(), &KisImage::sigSizeChanged);
    if (cropLayers) api.crop(requested.x(), requested.y(), requested.width(), requested.height());
    else api.resizeImage(requested.x(), requested.y(), requested.width(), requested.height());
    QCOMPARE(document->image()->size(), requested.size());
    QCOMPARE(QSize(api.width(), api.height()), requested.size());
    QCOMPARE(document->undoStack()->index(), history + steps);
    if (steps) {
        QTRY_COMPARE(sizeChanged.size(), 1);
        QCOMPARE(sizeChanged.first()[0].toPointF(), QRectF(requested).center());
        QCOMPARE(sizeChanged.first()[1].toPointF(), QRectF(QPointF(), QSizeF(requested.size())).center());
        const auto *extra = dynamic_cast<const KisCropSavedExtraData*>(document->undoStack()->command(history)->extraData());
        QVERIFY(extra);
        QCOMPARE(extra->type(), cropLayers ? KisCropSavedExtraData::CROP_IMAGE : KisCropSavedExtraData::RESIZE_IMAGE);
        QCOMPARE(extra->cropRect(), requested);
    }
    for (int i = 0; i < 2; ++i) {
        QCOMPARE(nodes[i]->position(), positions[i] - requested.topLeft());
        QCOMPARE(nodes[i]->pixelData(inspected.x(), inspected.y(), inspected.width(), inspected.height()), expected[i]);
        QCOMPARE(nodes[i]->uniqueId(), input.layers[i].uuid);
        QCOMPARE(nodes[i]->locked(), i == 1);
    }
    if (steps) {
        document->undoStack()->undo();
        document->image()->waitForDone();
        QCOMPARE(document->image()->size(), originalSize);
        for (int i = 0; i < 2; ++i) {
            QCOMPARE(nodes[i]->position(), positions[i]);
            QCOMPARE(nodes[i]->pixelData(inspected.x(), inspected.y(), inspected.width(), inspected.height()), originals[i]);
        }
        document->undoStack()->redo();
        document->image()->waitForDone();
        QCOMPARE(document->image()->size(), requested.size());
        for (int i = 0; i < 2; ++i)
            QCOMPARE(nodes[i]->pixelData(inspected.x(), inspected.y(), inspected.width(), inspected.height()), expected[i]);
    }
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            output.setAutoRemove(false);
            qInfo() << "Canvas resize artifacts retained at" << output.path();
        }
    });
    const auto kra = output.filePath(QStringLiteral("resized.kra"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    KisPropertiesConfigurationSP kraSettings(new KisPropertiesConfiguration);
    kraSettings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    QVERIFY(document->exportDocumentSync(kra, KisDocument::nativeFormatMimeType(), kraSettings));
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY(reopened->openPath(kra, KisDocument::DontAddToRecent));
    QCOMPARE(reopened->image()->size(), requested.size());
    for (int i = 0; i < 2; ++i) {
        NodeSP node(Node::createNode(reopened->image(), reopened->image()->root()->at(i)));
        QCOMPARE(node->position(), positions[i] - requested.topLeft());
        QCOMPARE(node->pixelData(inspected.x(), inspected.y(), inspected.width(), inspected.height()), expected[i]);
    }
    InfoObject pngSettings;
    pngSettings.setProperty("alpha", true);
    pngSettings.setProperty("interlaced", false);
    pngSettings.setProperty("compression", 3);
    pngSettings.setProperty("saveSRGBProfile", true);
    pngSettings.setProperty("forceSRGB", false);
    pngSettings.setProperty("storeAuthor", false);
    pngSettings.setProperty("storeMetaData", false);
    const auto png = output.filePath(QStringLiteral("resized.png"));
    QVERIFY(api.exportImage(png, pngSettings));
    const auto projection = api.pixelData(0, 0, requested.width(), requested.height());
    const auto decoded = QImage(png).convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(decoded.size(), requested.size());
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(decoded.constBits()), decoded.sizeInBytes()), projection);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const auto prefix = QDir(results).filePath(QStringLiteral("canvas-resize-") + QString::fromLatin1(QTest::currentDataTag()));
        const auto write = [&](const QString &suffix, const QByteArray &bytes) {
            QFile file(prefix + suffix);
            return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
        };
        for (int i = 0; i < 2; ++i) QVERIFY(write(QStringLiteral(".layer-%1.bgra").arg(i), expected[i]));
        QVERIFY(write(QStringLiteral(".projection.bgra"), projection));
        QByteArray state;
        QDataStream stream(&state, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_15);
        stream << document->image()->size() << qint32(document->undoStack()->index());
        for (const auto &node : nodes) stream << node->position() << node->uniqueId() << node->locked();
        QVERIFY(write(QStringLiteral(".state"), state));
        QVERIFY(QFile::copy(kra, prefix + QStringLiteral(".kra")));
        QVERIFY(QFile::copy(png, prefix + QStringLiteral(".png")));
    }
}
catch (const std::exception &error)
{
    QFAIL(qPrintable(QStringLiteral("Public canvas resize failed: ") + QString::fromUtf8(error.what())));
}

void TestDocument::testGpuCanvasResizePreservesRejectedInputsAndNotificationLifetimes()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    for (const bool crop : {false, true}) for (const bool closeOnSize : {false, true}) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 48));
        const auto pixel = QByteArray::fromHex("112233ff");
        QCOMPARE(owner->writeRgbaPixels(1, {8, 8, 1, 1}, pixel), Gpu::Result::Success);
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Closing canvas resize"));
        document->setCurrentImage(image);
        QScopedPointer<Document> api(new Document(document.data(), false));
        const auto unchanged = owner->operations().preview();
        const auto requested = execution->requested(owner->handle());
        const auto resize = [&](Document *caller, int x, int y, int w, int h) {
            if (crop) caller->crop(x, y, w, h);
            else caller->resizeImage(x, y, w, h);
        };
        QVERIFY_EXCEPTION_THROWN(resize(api.data(), std::numeric_limits<int>::max(), 0, 1, 1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(resize(api.data(), 0, 0, std::numeric_limits<int>::min(), 1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(resize(api.data(), 0, 0, 0, 1), std::invalid_argument);
        QCOMPARE(owner->operations().preview(), unchanged);
        QCOMPARE(execution->requested(owner->handle()), requested);
        const auto close = [&] { api.reset(); document.reset(); };
        const auto connection = closeOnSize ? connect(image.data(), &KisImage::sigSizeChanged, this, close) :
                                             connect(owner.data(), &Gpu::PaintImage::changed, this, close);
        auto *caller = api.data();
        resize(caller, 8, 6, 32, 24);
        disconnect(connection);
        QVERIFY(!api);
        QVERIFY(!document);
        QCOMPARE(owner->operations().snapshot()->size, QSize(32, 24));
        QCOMPARE(owner->operations().undoCount(), 1);
        QCOMPARE(owner->readRgbaPixels(1, {0, 2, 1, 1}, 4).bytes, pixel);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot()->size, QSize(64, 48));
        QCOMPARE(owner->readRgbaPixels(1, {8, 8, 1, 1}, 4).bytes, pixel);
    }
}

void TestDocument::testOrdinaryCreationKeepsPngExportAndReopen()
{
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY(document->newImage(QStringLiteral("PNG editing"), 32, 16, cs,
                              KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    Document publicDocument(document.data(), false);
    NodeSP layer(Node::createNode(document->image(), document->image()->root()->firstChild()));
    const auto red = QByteArray::fromHex("0000ffff");
    QVERIFY(layer->setPixelData(red, 5, 5, 1, 1));
    publicDocument.refreshProjection();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("export.png"));
    InfoObject settings;
    QVERIFY2(publicDocument.exportImage(path, settings), qPrintable(document->errorMessage()));
    const QImage exported(path);
    QCOMPARE(exported.size(), QSize(32, 16));
    QCOMPARE(exported.pixelColor(5, 5), QColor(Qt::red));
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    Document publicReopened(reopened.data(), false);
    QCOMPARE(publicReopened.pixelData(5, 5, 1, 1), red);
}

void TestDocument::testGpuHierarchyKeepsPublicEditingHistoryAndNativeFiles_data()
{
    QTest::addColumn<bool>("nested");
    QTest::newRow("group") << false;
    QTest::newRow("nested") << true;
}

void TestDocument::testGpuHierarchyKeepsPublicEditingHistoryAndNativeFiles()
{
    QFETCH(bool, nested);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage input;
    input.size = {32, 16};
    input.projectionBackground = {41, 42, 43, 0};
    input.activeLayer = 1;
    input.layers = {
        {QStringLiteral("Backdrop"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}, {0, 0, 255, 255}},
        {QStringLiteral("Nested paint"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}, {17, 34, 51, 0}},
        {QStringLiteral("Group"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}}};
    input.layers[1].parentUuid = input.layers[2].uuid;
    input.layers[2].kind = Gpu::LayerKind::Group;
    input.layers[2].opacity = 128;
    if (nested) {
        input.layers.append({QStringLiteral("Outer"), true, QUuid(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}")), {}});
        input.layers[3].kind = Gpu::LayerKind::Group;
        input.layers[3].opacity = 128;
        input.layers[2].parentUuid = input.layers[3].uuid;
    }
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Grouped GPU document")));
    const auto image = document->image();
    QCOMPARE(image->root()->childCount(), quint32(2));
    const auto top = image->root()->lastChild();
    KisNodeSP group = nested ? top->firstChild() : top;
    QVERIFY(qobject_cast<const KisGroupLayer *>(group.data()));
    QCOMPARE(group->childCount(), quint32(1));
    const auto leaf = group->firstChild();
    QCOMPARE(leaf->uuid(), input.layers[1].uuid);
    QCOMPARE(document->preActivatedNode(), leaf);
    NodeSP publicGroup(Node::createNode(image, group));
    NodeSP publicLeaf(Node::createNode(image, leaf));
    QCOMPARE(publicGroup->type(), QStringLiteral("grouplayer"));
    QVERIFY(publicGroup->pixelData(0, 0, 1, 1).isEmpty());
    QVERIFY(!publicGroup->setPixelData(QByteArray::fromHex("0000ffff"), 0, 0, 1, 1));
    const auto groupDevice = qobject_cast<const KisGroupLayer *>(group.data())->original();
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(groupDevice->defaultPixel().data()), 4), QByteArray::fromHex("33221100"));
    QCOMPARE(qobject_cast<const KisGroupLayer *>(group.data())->defaultProjectionColor().toQColor(), QColor(0, 0, 0, 0));
    const auto background = image->defaultProjectionColor();
    QVERIFY_EXCEPTION_THROWN(qobject_cast<KisGroupLayer *>(group.data())->setDefaultProjectionColor(KoColor(QColor(Qt::red), image->colorSpace())), std::logic_error);
    QCOMPARE(image->defaultProjectionColor(), background);
    const auto sequence = groupDevice->sequenceNumber();
    const auto red = QByteArray::fromHex("0000ffff");
    QVERIFY(publicLeaf->setPixelData(red, 5, 5, 1, 1));
    QVERIFY(groupDevice->sequenceNumber() != sequence);
    QCOMPARE(publicGroup->projectionPixelData(5, 5, 1, 1), red);
    Document api(document.data(), false);
    QCOMPARE(api.pixelData(5, 5, 1, 1), nested ? QByteArray::fromHex("bf0040ff") : QByteArray::fromHex("7f0080ff"));
    QCOMPARE(owner->replaceRgbaPixels(2, {5, 5, 1, 1}, QByteArray::fromHex("00ff00ff")), Gpu::Result::Success);
    QCOMPARE(document->undoStack()->count(), 1);
    document->undoStack()->undo();
    QCOMPARE(publicLeaf->pixelData(5, 5, 1, 1), red);
    document->undoStack()->redo();
    QCOMPARE(publicLeaf->pixelData(5, 5, 1, 1), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(group->firstChild(), leaf);
    publicGroup->setOpacity(64);
    QCOMPARE(owner->operations().snapshot()->layers[2].opacity, quint8(64));
    QCOMPARE(publicGroup->thumbnail(8, 8).isNull(), false);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Public hierarchy artifacts retained at" << directory.path();
        }
    });
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const auto stem = QStringLiteral("public-hierarchy-%1").arg(nested ? QStringLiteral("nested") : QStringLiteral("group"));
    const auto kraPath = QDir(destination).filePath(stem + QStringLiteral(".kra"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    KisPropertiesConfigurationSP kraSettings(new KisPropertiesConfiguration);
    kraSettings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    QVERIFY2(document->saveAs(kraPath, KisDocument::nativeFormatMimeType(), false, kraSettings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    InfoObject configuration;
    configuration.setProperty("alpha", true);
    configuration.setProperty("interlaced", false);
    configuration.setProperty("compression", 3);
    configuration.setProperty("saveSRGBProfile", true);
    configuration.setProperty("forceSRGB", false);
    configuration.setProperty("storeAuthor", false);
    configuration.setProperty("storeMetaData", false);
    const auto pngPath = QDir(destination).filePath(stem + QStringLiteral(".png"));
    QVERIFY2(api.exportImage(pngPath, configuration), qPrintable(document->errorMessage()));
    const auto expected = api.pixelData(0, 0, 32, 16);
    QFile saved(kraPath);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    const auto decoded = Kra::RasterCodec::read(saved);
    QVERIFY2(decoded.code == Kra::Status::Success, qPrintable(decoded.diagnostic));
    auto reopenedOwner = Gpu::PaintImageSP::create(execution, decoded.document.image);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setCurrentImage(KisImage::fromGpuImage(reopenedOwner, QStringLiteral("Reopened hierarchy")));
    Document reopenedApi(reopened.data(), false);
    QCOMPARE(reopenedApi.pixelData(0, 0, 32, 16), expected);
    QCOMPARE(reopened->preActivatedNode()->uuid(), leaf->uuid());
    QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
    native->setFileBatchMode(true);
    QVERIFY2(native->openPath(kraPath, KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
    Document nativeApi(native.data(), false);
    QCOMPARE(nativeApi.pixelData(0, 0, 32, 16), expected);
    const auto nativeTop = native->image()->root()->lastChild();
    const auto nativeGroup = nested ? nativeTop->firstChild() : nativeTop;
    QVERIFY(qobject_cast<const KisGroupLayer *>(nativeGroup.data()));
    QCOMPARE(nativeGroup->firstChild()->uuid(), leaf->uuid());
    QVERIFY(publicGroup->remove());
    QCOMPARE(image->root()->childCount(), nested ? quint32(2) : quint32(1));
    if (nested) QCOMPARE(top->childCount(), quint32(0));
    document->undoStack()->undo();
    QCOMPARE(group->firstChild(), leaf);
    QCOMPARE(publicLeaf->pixelData(5, 5, 1, 1), QByteArray::fromHex("00ff00ff"));
}

void TestDocument::testOrdinaryKraKeepsLayerHierarchyThroughResaving()
{
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(source->createUndoStore(), 32, 16, cs, QStringLiteral("Grouped document"));
    KisNodeSP group = new KisGroupLayer(image, QStringLiteral("Group"), 255);
    KisNodeSP layer = new KisPaintLayer(image, QStringLiteral("Nested paint"), 255);
    image->addNode(group, image->root());
    image->addNode(layer, group);
    image->setResolution(300.0 / 72, 150.0 / 72);
    image->addAnnotation(new KisAnnotation(QStringLiteral("notes"), QStringLiteral("Project notes"), QByteArrayLiteral("note bytes")));
    const auto red = QByteArray::fromHex("0000ffff");
    layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(red.constData()), 5, 5, 1, 1);
    source->setCurrentImage(image);
    source->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Grouped title"));
    image->refreshGraphAsync();
    image->waitForDone();
    source->setFileBatchMode(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto path = directory.filePath(QStringLiteral("hierarchy.kra"));
    QVERIFY2(source->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(source->errorMessage()));
    source->waitForSavingToComplete();
    for (int pass = 0; pass < 2; ++pass) {
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        const auto reopenedGroup = reopened->image()->root()->firstChild();
        QVERIFY(qobject_cast<const KisGroupLayer *>(reopenedGroup.data()));
        QCOMPARE(reopenedGroup->name(), QStringLiteral("Group"));
        QCOMPARE(reopenedGroup->childCount(), quint32(1));
        QCOMPARE(reopenedGroup->firstChild()->name(), QStringLiteral("Nested paint"));
        QCOMPARE(reopenedGroup->uuid(), group->uuid());
        QCOMPARE(reopenedGroup->firstChild()->uuid(), layer->uuid());
        QCOMPARE(reopened->image()->xRes() * 72, 300.0);
        QCOMPARE(reopened->image()->yRes() * 72, 150.0);
        QCOMPARE(reopened->image()->profile()->rawData(), cs->profile()->rawData());
        QCOMPARE(reopened->image()->annotation(QStringLiteral("notes"))->annotation(), QByteArrayLiteral("note bytes"));
        QCOMPARE(reopened->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Grouped title"));
        Document publicReopened(reopened.data(), false);
        QCOMPARE(publicReopened.pixelData(5, 5, 1, 1), red);
        path = directory.filePath(QStringLiteral("resaved-%1.kra").arg(pass));
        QVERIFY2(reopened->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(reopened->errorMessage()));
        reopened->waitForSavingToComplete();
        const auto retainedImage = reopened->image();
        const auto retainedPath = reopened->path();
        const auto invalidPath = directory.filePath(QStringLiteral("invalid.kra"));
        QFile invalid(invalidPath);
        QVERIFY(invalid.open(QIODevice::WriteOnly));
        invalid.write("invalid archive");
        invalid.close();
        QSignalSpy completed(reopened.data(), &KisDocument::completed);
        QSignalSpy cancelled(reopened.data(), &KisDocument::canceled);
        QVERIFY(!reopened->openPath(invalidPath, KisDocument::DontAddToRecent));
        QCOMPARE(completed.size(), 0);
        QCOMPARE(cancelled.size(), 1);
        QCOMPARE(reopened->image(), retainedImage);
        QCOMPARE(reopened->path(), retainedPath);
    }
    for (const bool closeBeforeCompletion : {true, false}) {
        QScopedPointer<KisDocument> closingOwner(KisPart::instance()->createDocument());
        QPointer<KisDocument> closing = closingOwner.data();
        closing->setFileBatchMode(true);
        KisNodeSP retained;
        connect(closing.data(), closeBeforeCompletion ? &KisDocument::sigLoadingFinished : &KisDocument::completed,
                this, [&] {
            retained = closing->image()->root()->firstChild()->firstChild();
            delete closingOwner.take();
        });
        QVERIFY(closing->openPath(path, KisDocument::DontAddToRecent));
        QVERIFY(!closing);
        QVERIFY(retained);
        QCOMPARE(retained->name(), QStringLiteral("Nested paint"));
    }
}

void TestDocument::testOrdinaryCreationPreservesInitialDocumentState_data()
{
    QTest::addColumn<bool>("gpuSession");
    QTest::addColumn<int>("style");
    QTest::addColumn<int>("layerCount");
    for (const bool gpu : {false, true}) for (const auto style : {KisConfig::RASTER_LAYER, KisConfig::CANVAS_COLOR}) {
        for (const int layers : {1, 2}) {
            const auto name = QStringLiteral("%1-%2-%3-layers")
                .arg(gpu ? QStringLiteral("gpu") : QStringLiteral("ordinary"))
                .arg(style == KisConfig::RASTER_LAYER ? QStringLiteral("raster") : QStringLiteral("canvas")).arg(layers);
            QTest::newRow(qPrintable(name)) << gpu << int(style) << layers;
        }
    }
}

void TestDocument::testOrdinaryCreationPreservesInitialDocumentState()
{
    QFETCH(bool, gpuSession);
    QFETCH(int, style);
    QFETCH(int, layerCount);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", gpuSession ? "1" : "0");
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoColor color(QColor(17, 34, 51, 128), cs);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QVERIFY2(document->newImage(QStringLiteral("Ordinary document"), 32, 16, cs, color,
                                KisConfig::BackgroundStyle(style), layerCount, QStringLiteral("Kept description"), 2),
             qPrintable(document->errorMessage()));
    const auto image = KisImageSP(document->image());
    QCOMPARE(bool(image->gpuImage()), gpuSession);
    QCOMPARE(image->bounds().size(), QSize(32, 16));
    QCOMPARE(image->root()->childCount(), quint32(layerCount));
    QCOMPARE(document->undoStack()->count(), 0);
    QVERIFY(!document->isModified());
    QCOMPARE(image->xRes(), 2.0);
    QCOMPARE(image->yRes(), 2.0);
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Ordinary document"));
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("abstract")), QStringLiteral("Kept description"));
    QCOMPARE(image->colorSpace(), cs);
    const bool raster = style == int(KisConfig::RASTER_LAYER);
    QCOMPARE(image->root()->firstChild()->userLocked(), raster && layerCount > 1);
    QCOMPARE(image->root()->firstChild()->opacity(), raster ? quint8(128) : quint8(255));
    QCOMPARE(image->root()->firstChild()->name(), QStringLiteral("Background"));
    const bool autoPin = KisConfig(false).autoPinLayersToTimeline();
    QCOMPARE(image->root()->firstChild()->isPinnedToTimeline(), raster && autoPin);
    if (layerCount > 1) QCOMPARE(image->root()->lastChild()->isPinnedToTimeline(), autoPin);
    Document publicDocument(document.data(), false);
    publicDocument.refreshProjection();
    QCOMPARE(publicDocument.pixelData(3, 3, 1, 1), QByteArray::fromHex("33221180"));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    document->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("new.kra"));
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QCOMPARE(bool(reopened->image()->gpuImage()), gpuSession);
    Document publicReopened(reopened.data(), false);
    QCOMPARE(publicReopened.pixelData(3, 3, 1, 1), publicDocument.pixelData(3, 3, 1, 1));
    QCOMPARE(reopened->image()->defaultProjectionColor(), image->defaultProjectionColor());
    QCOMPARE(reopened->image()->root()->firstChild()->userLocked(), raster && layerCount > 1);
    QCOMPARE(reopened->image()->root()->firstChild()->opacity(), raster ? quint8(128) : quint8(255));
    QCOMPARE(reopened->image()->root()->firstChild()->isPinnedToTimeline(), raster && autoPin);
    if (layerCount > 1) QCOMPARE(reopened->image()->root()->lastChild()->isPinnedToTimeline(), autoPin);
    QCOMPARE(reopened->documentInfo()->aboutInfo(QStringLiteral("abstract")), QStringLiteral("Kept description"));
    const auto original = document->image();
    QVERIFY(!document->newImage(QStringLiteral("Invalid size"), 0, 16, cs, color,
                                KisConfig::BackgroundStyle(style), 1, {}, 1));
    QCOMPARE(document->image(), original);
    QVERIFY(!document->isModified());
}

void TestDocument::testGpuSessionNativeImportRetainsFormatDataAndRejectsUnsupportedEdits()
{
    namespace Kra = LibrePaint::ImportExport::Kra;
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) { directory.setAutoRemove(false); qInfo() << "GPU native admission artifacts at" << destination; }
    });
    const auto source = directory.filePath(QStringLiteral("source.kra"));
    Kra::Document value;
    value.image.size = {4, 4};
    value.image.layers = {{QStringLiteral("Paint"), true,
        QUuid(QStringLiteral("{11223344-5566-7788-99aa-bbccddeeff00}")), {}, {17, 34, 51, 128}}};
    value.files.insert(QStringLiteral("private/format-data"), QByteArrayLiteral("retained native format data"));
    QImage projection(4, 4, QImage::Format_RGBA8888);
    projection.fill(QColor(17, 34, 51, 128));
    QFile input(source);
    QVERIFY(input.open(QIODevice::ReadWrite));
    const auto encoded = Kra::RasterCodec::write(input, value, projection);
    QVERIFY2(encoded.code == Kra::Status::Success, qPrintable(encoded.diagnostic));
    QVERIFY(input.seek(0));
    const auto original = Kra::RasterCodec::read(input);
    QVERIFY2(original.code == Kra::Status::Success, qPrintable(original.diagnostic));
    input.close();
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    const QList<KoColor> retainedColors{KoColor(QColor(192, 17, 64), KoColorSpaceRegistry::instance()->rgb8())};
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    reference->setFileBatchMode(true);
    reference->setColorHistoryColors(retainedColors);
    QVERIFY2(reference->openPath(source, KisDocument::DontAddToRecent), qPrintable(reference->errorMessage()));
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setColorHistoryColors(retainedColors);
    QVERIFY2(document->openPath(source, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    QVERIFY(document->image()->gpuImage());
    QCOMPARE(document->mirrorAxisConfig(), reference->mirrorAxisConfig());
    QCOMPARE(document->image()->mirrorAxesCenter(), reference->image()->mirrorAxesCenter());
    QCOMPARE(document->image()->profile()->rawData(), reference->image()->profile()->rawData());
    QCOMPARE(document->colorHistoryColors(), reference->colorHistoryColors());
    QCOMPARE(document->colorHistoryColors(), retainedColors);
    Document api(document.data(), false);
    QCOMPARE(api.pixelData(0, 0, 1, 1), QByteArray::fromHex("33221180"));
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed GPU import"));
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    const auto saved = QDir(destination).filePath(QStringLiteral("gpu-session-import.kra"));
    document->setModified(true);
    QVERIFY2(document->saveAs(saved, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QFile result(saved);
    QVERIFY(result.open(QIODevice::ReadOnly));
    const auto decoded = Kra::RasterCodec::read(result);
    QVERIFY2(decoded.code == Kra::Status::Success, qPrintable(decoded.diagnostic));
    QCOMPARE(decoded.document.files.value(QStringLiteral("private/format-data")), value.files.value(QStringLiteral("private/format-data")));
    const auto image = document->image();
    const auto unchanged = image->gpuImage()->operations().preview();
    QBuffer malformed;
    malformed.setData(QByteArrayLiteral("invalid native archive"));
    QVERIFY(malformed.open(QIODevice::ReadOnly));
    auto invalidStatus = document->importGpuKra(malformed);
    QVERIFY(invalidStatus == KisImportExportErrorCode(ImportExportCodes::FileFormatIncorrect));
    QCOMPARE(document->image(), image);
    QCOMPARE(image->gpuImage()->operations().preview(), unchanged);
    for (const auto &feature : {QStringLiteral("layerstyle"), QStringLiteral("grid"),
                               QStringLiteral("Palettes"), QStringLiteral("resources"), QStringLiteral("animation")}) {
        auto unsupported = original.document;
        QDomDocument xml;
        QVERIFY(xml.setContent(unsupported.description));
        auto imageElement = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
        if (feature == QLatin1String("layerstyle"))
            imageElement.firstChildElement(QStringLiteral("layers")).firstChildElement().setAttribute(feature,
                QStringLiteral("{10203040-5060-7080-90a0-b0c0d0e0f000}"));
        else {
            auto container = xml.createElement(feature);
            if (feature != QLatin1String("grid")) container.appendChild(xml.createElement(QStringLiteral("resource")));
            imageElement.appendChild(container);
        }
        unsupported.description = xml.toByteArray();
        const auto badPath = directory.filePath(feature + QStringLiteral(".kra"));
        QFile bad(badPath);
        QVERIFY(bad.open(QIODevice::WriteOnly));
        const auto written = Kra::RasterCodec::write(bad, unsupported, projection);
        QVERIFY2(written.code == Kra::Status::Success, qPrintable(written.diagnostic));
        bad.close();
        QVERIFY(!document->openPath(badPath, KisDocument::DontAddToRecent));
        QVERIFY2(document->errorMessage().contains(feature), qPrintable(document->errorMessage()));
        QCOMPARE(document->image(), image);
        QCOMPARE(document->localFilePath(), saved);
        QCOMPARE(image->gpuImage()->operations().preview(), unchanged);
        QVERIFY(!document->isModified());
    }
    QSignalSpy timingImageChanges(document.data(), &KisDocument::sigCurrentImageChanged);
    for (const auto &field : {QStringLiteral("framerate"), QStringLiteral("range"), QStringLiteral("currentTime"),
                              QStringLiteral("sequenceInitialFrameNumber")}) {
        auto invalid = original.document;
        QDomDocument xml;
        QVERIFY(xml.setContent(invalid.description));
        auto timing = xml.createElement(QStringLiteral("animation"));
        auto value = xml.createElement(field);
        value.setAttribute(QStringLiteral("type"), field == QLatin1String("range") ? QStringLiteral("timerange") : QStringLiteral("value"));
        if (field == QLatin1String("range")) {
            value.setAttribute(QStringLiteral("from"), QStringLiteral("invalid"));
            value.setAttribute(QStringLiteral("to"), 100);
        } else value.setAttribute(QStringLiteral("value"), QStringLiteral("invalid"));
        if (field == QLatin1String("sequenceInitialFrameNumber")) {
            auto exports = xml.createElement(QStringLiteral("export-settings"));
            exports.appendChild(value);
            timing.appendChild(exports);
        } else timing.appendChild(value);
        xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).appendChild(timing);
        invalid.description = xml.toByteArray();
        QBuffer archive;
        QVERIFY(archive.open(QIODevice::ReadWrite));
        QCOMPARE(Kra::RasterCodec::write(archive, invalid, projection).code, Kra::Status::Success);
        QVERIFY(archive.seek(0));
        QVERIFY(document->importGpuKra(archive) == KisImportExportErrorCode(ImportExportCodes::FileFormatIncorrect));
        QVERIFY2(document->errorMessage().contains(field), qPrintable(document->errorMessage()));
        QCOMPARE(document->image(), image);
        QCOMPARE(document->localFilePath(), saved);
        QCOMPARE(image->gpuImage()->operations().preview(), unchanged);
        QVERIFY(!document->isModified());
        QVERIFY(timingImageChanges.isEmpty());
    }
    QScopedPointer<KisDocument> closing(KisPart::instance()->createDocument());
    closing->setFileBatchMode(true);
    bool metadataBeforeImage = false, closed = false;
    connect(closing->documentInfo(), &KoDocumentInfo::infoUpdated, this,
            [&](const QString &key, const QString &) {
        if (key != QLatin1String("title")) return;
        if (!closing->image() || !closing->image()->gpuImage()) { metadataBeforeImage = true; return; }
        QCOMPARE(closing->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Fixed GPU import"));
        closed = true;
        closing.reset();
    });
    auto *caller = closing.data();
    caller->openPath(saved, KisDocument::DontAddToRecent);
    QVERIFY2(!metadataBeforeImage, "Native metadata notifications must observe the adopted GPU image and file owner");
    QVERIFY(closed);
    QVERIFY(!closing);
}

void TestDocument::testGpuSessionOpensExistingStaticNativeDocuments_data()
{
    QTest::addColumn<QString>("settingsSource");
    for (const auto &mode : {QStringLiteral("default"), QStringLiteral("body-only"),
                            QStringLiteral("sidecar-priority"), QStringLiteral("partial-sidecar"),
                            QStringLiteral("export-path-fallback"), QStringLiteral("export-empty-fallback")})
        QTest::newRow(qPrintable(mode)) << mode;
}

void TestDocument::testGpuSessionOpensExistingStaticNativeDocuments()
{
    QFETCH(QString, settingsSource);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto retainFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Static native document artifacts at" << directory.path() << "and" << destination;
        }
    });
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    source->setFileBatchMode(true);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(source->newImage(QStringLiteral("Static native document"), 16, 8, cs, KoColor(QColor(255, 255, 255, 0), cs),
                             KisConfig::CANVAS_COLOR, 1, QStringLiteral("Retained static settings"), 1));
    KisNodeSP sourceLayer = KisImageSP(source->image())->root()->firstChild();
    sourceLayer->setUuid(QUuid(QStringLiteral("{5a080001-1122-3344-5566-778899aabbcc}")));
    sourceLayer->setPinnedToTimeline(false);
    source->setColorHistoryColors({});
    source->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
    source->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    source->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    source->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    QScopedPointer<Node> sourceNode(Node::createNode(source->image(), sourceLayer));
    QVERIFY(sourceNode->setPixelData(QByteArray::fromHex("33221180"), 3, 2, 1, 1));
    const auto native = directory.filePath(QStringLiteral("native.kra"));
    QVERIFY2(source->saveAs(native, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(source->errorMessage()));
    source->waitForSavingToComplete();
    QMap<QString, QByteArray> files;
    // Preserve the ordinary writer's entries; variants change only timing metadata.
    {
        QScopedPointer<KoStore> store(KoStore::createStore(native, KoStore::Read));
        QVERIFY(store && !store->bad());
        for (const auto &name : store->directoryList()) {
            if (name.endsWith(QLatin1Char('/'))) continue;
            QVERIFY(store->open(name));
            files.insert(name, store->read(store->size()));
            QVERIFY(store->close());
        }
    }
    QDomDocument body;
    QVERIFY(body.setContent(files.value(QStringLiteral("maindoc.xml"))));
    auto imageElement = body.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    QVERIFY(!imageElement.firstChildElement(QStringLiteral("Palettes")).isNull());
    QVERIFY(!imageElement.firstChildElement(QStringLiteral("resources")).isNull());
    const auto index = imageElement.attribute(QStringLiteral("name")) + QStringLiteral("/animation/index.xml");
    QVERIFY(files.contains(index));
    const auto setTiming = [](QDomElement root, int fps, int first, int last, int time) {
        root.firstChildElement(QStringLiteral("framerate")).setAttribute(QStringLiteral("value"), fps);
        auto range = root.firstChildElement(QStringLiteral("range"));
        range.setAttribute(QStringLiteral("from"), first);
        range.setAttribute(QStringLiteral("to"), last);
        root.firstChildElement(QStringLiteral("currentTime")).setAttribute(QStringLiteral("value"), time);
    };
    if (settingsSource != QLatin1String("default")) {
        setTiming(imageElement.firstChildElement(QStringLiteral("animation")), 12, 2, 42, 3);
        files[QStringLiteral("maindoc.xml")] = body.toByteArray();
        if (settingsSource == QLatin1String("body-only")) files.remove(index);
        else {
            QDomDocument sidecar;
            QVERIFY(sidecar.setContent(files.value(index)));
            setTiming(sidecar.documentElement(), 30, 5, 125, 17);
            auto exports = sidecar.documentElement().firstChildElement(QStringLiteral("export-settings"));
            exports.firstChildElement(QStringLiteral("sequenceFilePath")).setAttribute(QStringLiteral("value"), QStringLiteral("frames"));
            exports.firstChildElement(QStringLiteral("sequenceBaseName")).setAttribute(QStringLiteral("value"), QStringLiteral("pose"));
            exports.firstChildElement(QStringLiteral("sequenceInitialFrameNumber")).setAttribute(QStringLiteral("value"), 7);
            if (settingsSource == QLatin1String("partial-sidecar")) {
                auto root = sidecar.documentElement();
                root.removeChild(root.firstChildElement(QStringLiteral("range")));
                root.removeChild(root.firstChildElement(QStringLiteral("currentTime")));
            }
            if (settingsSource == QLatin1String("export-path-fallback") || settingsSource == QLatin1String("export-empty-fallback"))
                exports.removeChild(exports.firstChildElement(QStringLiteral("sequenceBaseName")));
            if (settingsSource == QLatin1String("export-empty-fallback"))
                exports.removeChild(exports.firstChildElement(QStringLiteral("sequenceFilePath")));
            files[index] = sidecar.toByteArray();
        }
    }
    const auto fixture = QDir(destination).filePath(QStringLiteral("static-native-") + settingsSource + QStringLiteral("-source.kra"));
    {
        QScopedPointer<KoStore> store(KoStore::createStore(fixture, KoStore::Write,
                                                        KisDocument::nativeFormatMimeType(), KoStore::Zip, false));
        QVERIFY(store && !store->bad());
        for (auto entry = files.cbegin(); entry != files.cend(); ++entry) {
            QVERIFY(store->open(entry.key()));
            QCOMPARE(store->write(entry.value()), qint64(entry.value().size()));
            QVERIFY(store->close());
        }
        QVERIFY(store->finalize());
    }
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    reference->setFileBatchMode(true);
    QVERIFY2(reference->openPath(fixture, KisDocument::DontAddToRecent), qPrintable(reference->errorMessage()));
    const auto matchesSettings = [&](KisDocument *document) {
        auto *actual = document->image()->animationInterface();
        auto *expected = reference->image()->animationInterface();
        QVERIFY(!actual->hasAnimation());
        QCOMPARE(actual->framerate(), expected->framerate());
        QCOMPARE(actual->documentPlaybackRange(), expected->documentPlaybackRange());
        QCOMPARE(actual->currentTime(), expected->currentTime());
        QCOMPARE(actual->currentUITime(), expected->currentUITime());
        QCOMPARE(actual->exportSequenceFilePath(), expected->exportSequenceFilePath());
        QCOMPARE(actual->exportSequenceBaseName(), expected->exportSequenceBaseName());
        QCOMPARE(actual->exportInitialFrameNumber(), expected->exportInitialFrameNumber());
    };
    const bool defaults = settingsSource == QLatin1String("default");
    const bool partialSidecar = settingsSource == QLatin1String("partial-sidecar");
    const bool fullSidecar = !defaults && !partialSidecar && settingsSource != QLatin1String("body-only");
    const bool sidecar = fullSidecar || partialSidecar;
    auto *expected = reference->image()->animationInterface();
    QCOMPARE(expected->framerate(), sidecar ? 30 : defaults ? 24 : 12);
    QCOMPARE(expected->currentUITime(), fullSidecar ? 17 : defaults ? 0 : 3);
    QCOMPARE(expected->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(fullSidecar ? 5 : defaults ? 0 : 2,
                                                                          fullSidecar ? 125 : defaults ? 100 : 42));
    if (settingsSource == QLatin1String("export-path-fallback")) QCOMPARE(expected->exportSequenceBaseName(), QStringLiteral("frames"));
    if (settingsSource == QLatin1String("export-empty-fallback")) QCOMPARE(expected->exportSequenceBaseName(), QString());
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY2(document->openPath(fixture, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    QVERIFY(document->image()->gpuImage());
    matchesSettings(document.data());
    Document api(document.data(), false), referenceApi(reference.data(), false);
    QCOMPARE(api.pixelData(0, 0, 16, 8), referenceApi.pixelData(0, 0, 16, 8));
    QCOMPARE(document->image()->profile()->rawData(), reference->image()->profile()->rawData());
    const auto saved = QDir(destination).filePath(QStringLiteral("static-native-") + settingsSource + QStringLiteral(".kra"));
    document->setModified(true);
    QVERIFY2(document->saveAs(saved, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(saved, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QVERIFY(reopened->image()->gpuImage());
    matchesSettings(reopened.data());
    Document reopenedApi(reopened.data(), false);
    QCOMPARE(reopenedApi.pixelData(0, 0, 16, 8), api.pixelData(0, 0, 16, 8));
    QScopedPointer<Node> edited(Node::createNode(reopened->image(), reopened->image()->root()->firstChild()));
    QVERIFY(edited->setPixelData(QByteArray::fromHex("a1b2c3ff"), 4, 5, 1, 1));
    QCOMPARE(reopenedApi.pixelData(4, 5, 1, 1), QByteArray::fromHex("a1b2c3ff"));
    const auto resaved = QDir(destination).filePath(QStringLiteral("static-native-") + settingsSource + QStringLiteral("-edited.kra"));
    QVERIFY2(reopened->saveAs(resaved, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(reopened->errorMessage()));
    reopened->waitForSavingToComplete();
    QScopedPointer<KisDocument> final(KisPart::instance()->createDocument());
    final->setFileBatchMode(true);
    QVERIFY2(final->openPath(resaved, KisDocument::DontAddToRecent), qPrintable(final->errorMessage()));
    matchesSettings(final.data());
    Document finalApi(final.data(), false);
    QCOMPARE(finalApi.pixelData(0, 0, 16, 8), reopenedApi.pixelData(0, 0, 16, 8));
}

void TestDocument::testBackgroundApiKeepsHistoryAndPinnedPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), {32, 16}, QStringLiteral("Background API"));
    const auto image = KisImageSP(document->image());
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("ff0000ff")), Gpu::Result::Success);
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(6, 6, 1, 1), QByteArray::fromHex("00ff00ff")), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto pinned = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(pinned), Gpu::Result::Success);
    Document publicDocument(document.data(), false);
    QVERIFY(!publicDocument.modified());
    const auto pending = owner->beginStroke(layer, Gpu::Brush{}, 17);
    QVERIFY(!pending.isNull());
    QCOMPARE(owner->appendSamples(pending, {{{8, 8}, 1, 0, 0, 0, 1}}), Gpu::Result::Success);
    QSignalSpy modified(document.data(), &KisDocument::modified);
    const QColor background(17, 34, 51, 128);
    try {
        QVERIFY(publicDocument.setBackgroundColor(background));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QCOMPARE(publicDocument.backgroundColor(), background);
    QCOMPARE(owner->operations().activeOperation(), pending);
    QCOMPARE(owner->operations().preview()->sampleCount, qsizetype(1));
    QCOMPARE(owner->cancelStroke(pending), Gpu::Result::Success);
    QCOMPARE(publicDocument.pixelData(0, 0, 1, 1), QByteArray::fromHex("33221180"));
    QCOMPARE(publicDocument.pixelData(-1, -1, 1, 1), QByteArray::fromHex("33221180"));
    QVERIFY(publicDocument.modified());
    QCOMPARE(modified.size(), 1);
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().redoCount(), 1);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("ff0000ff"));
    QCOMPARE(owner->execution().readRgbaPixels(owner->handle(), pinned, QRect(0, 0, 1, 1), 0, 4).bytes,
             QByteArray(4, '\0'));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(publicDocument.backgroundColor(), background);
    QCOMPARE(publicDocument.pixelData(6, 6, 1, 1), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(publicDocument.pixelData(6, 6, 1, 1), QByteArray::fromHex("33221180"));
    QVERIFY(publicDocument.modified());
    QCOMPARE(publicDocument.backgroundColor(), background);
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().redoCount(), 1);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(publicDocument.backgroundColor(), background);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("background-edited.kra"));
    document->setFileBatchMode(true);
    QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    QVERIFY(!publicDocument.modified());
    QCOMPARE(owner->operations().undoCount(), 2);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    Document publicReopened(reopened.data(), false);
    QCOMPARE(publicReopened.backgroundColor(), background);
    QCOMPARE(publicReopened.pixelData(0, 0, 1, 1), QByteArray::fromHex("33221180"));
}

void TestDocument::testBackgroundNotificationLifetimes_data()
{
    QTest::addColumn<bool>("replace");
    QTest::newRow("close") << false;
    QTest::newRow("replace") << true;
}

void TestDocument::testBackgroundNotificationLifetimes()
{
    QFETCH(bool, replace);
    QPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), {32, 16}, QStringLiteral("Background notification"));
    const auto owner = document->image()->gpuImage();
    LibrePaint::Image::Gpu::PaintImageSP replacement;
    const auto cleanup = qScopeGuard([&] { delete document.data(); });
    Document publicDocument(document.data(), false);
    QObject::connect(document.data(), &KisDocument::modified, &publicDocument, [&](bool modified) {
        if (!modified) return;
        if (replace) {
            replacement = owner->clone();
            document->setCurrentImage(KisImage::fromGpuImage(replacement, QStringLiteral("Background replacement")));
        } else {
            delete document.data();
        }
    });
    const QColor background(17, 34, 51, 128);
    try {
        QVERIFY(publicDocument.setBackgroundColor(background));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    if (replace) {
        QVERIFY(document);
        QCOMPARE(document->image()->gpuImage(), replacement);
        QVERIFY(!document->isModified());
    } else {
        QVERIFY(document.isNull());
    }
    const LibrePaint::Image::Gpu::RgbaPixel expected{17, 34, 51, 128};
    QCOMPARE(owner->operations().snapshot()->projectionBackground, expected);
    QCOMPARE(owner->readRgbaPixels(0, QRect(0, 0, 1, 1), 4).bytes, QByteArray::fromHex("11223380"));
}

void TestDocument::testCanvasBackgroundReferencePixels_data()
{
    QTest::addColumn<QColor>("background");
    QTest::addColumn<bool>("hiddenPixel");
    QTest::addColumn<QByteArray>("inside");
    QTest::addColumn<QByteArray>("outside");
    QTest::addColumn<int>("layerCount");
    QTest::addColumn<int>("pixelAlpha");
    QTest::newRow("opaque") << QColor(17, 34, 51, 255) << false << QByteArray::fromHex("332211ff") << QByteArray::fromHex("332211ff") << 1 << 0;
    QTest::newRow("partial") << QColor(17, 34, 51, 128) << false << QByteArray::fromHex("33221180") << QByteArray::fromHex("33221180") << 1 << 0;
    QTest::newRow("transparent") << QColor(255, 255, 255, 0) << false << QByteArray::fromHex("00000000") << QByteArray::fromHex("00000000") << 1 << 0;
    QTest::newRow("opaque-hidden-rgb") << QColor(17, 34, 51, 255) << true << QByteArray::fromHex("332211ff") << QByteArray::fromHex("332211ff") << 1 << 0;
    QTest::newRow("partial-hidden-rgb") << QColor(17, 34, 51, 128) << true << QByteArray::fromHex("33221180") << QByteArray::fromHex("33221180") << 1 << 0;
    QTest::newRow("transparent-hidden-rgb") << QColor(255, 255, 255, 0) << true << QByteArray::fromHex("66554400") << QByteArray::fromHex("00000000") << 1 << 0;
    QTest::newRow("transparent-two-layers") << QColor(255, 255, 255, 0) << false << QByteArray::fromHex("ffffff00") << QByteArray::fromHex("ffffff00") << 2 << 0;
    QTest::newRow("transparent-hidden-rgb-two-layers") << QColor(255, 255, 255, 0) << true << QByteArray::fromHex("ffffff00") << QByteArray::fromHex("ffffff00") << 2 << 0;
    QTest::newRow("opaque-colored-pixel") << QColor(17, 34, 51, 255) << true << QByteArray::fromHex("4d3c2bff") << QByteArray::fromHex("332211ff") << 1 << 128;
    QTest::newRow("partial-colored-pixel") << QColor(17, 34, 51, 128) << true << QByteArray::fromHex("554433c0") << QByteArray::fromHex("33221180") << 1 << 128;
    QTest::newRow("transparent-colored-pixel") << QColor(255, 255, 255, 0) << true << QByteArray::fromHex("66554480") << QByteArray::fromHex("00000000") << 1 << 128;
    QTest::newRow("transparent-black-two-layers") << QColor(0, 0, 0, 0) << false << QByteArray::fromHex("00000000") << QByteArray::fromHex("00000000") << 2 << 0;
    QTest::newRow("transparent-black-hidden-rgb-two-layers") << QColor(0, 0, 0, 0) << true << QByteArray::fromHex("00000000") << QByteArray::fromHex("00000000") << 2 << 0;
}

void TestDocument::testCanvasBackgroundReferencePixels()
{
    QFETCH(QColor, background);
    QFETCH(bool, hiddenPixel);
    QFETCH(QByteArray, inside);
    QFETCH(QByteArray, outside);
    QFETCH(int, layerCount);
    QFETCH(int, pixelAlpha);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const KoColor color(background, cs);
    KisImageSP image = new KisImage(nullptr, 32, 16, cs, QStringLiteral("Canvas background reference"));
    image->setDefaultProjectionColor(color);
    for (int i = 0; i < layerCount; ++i)
        image->addNode(new KisPaintLayer(image, QStringLiteral("Paint Layer %1").arg(i + 1), 255), image->root(), i);
    document->setCurrentImage(image);
    image->initialRefreshGraph();
    if (hiddenPixel) {
        auto pixel = QByteArray::fromHex("66554400");
        pixel[3] = char(pixelAlpha);
        image->root()->firstChild()->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 3, 3, 1, 1);
        image->root()->firstChild()->setDirty(QRect(3, 3, 1, 1));
    }
    image->waitForDone();
    QCOMPARE(image->defaultProjectionColor(), color);
    Document publicDocument(document.data(), false);
    QCOMPARE(publicDocument.pixelData(3, 3, 1, 1), inside);
    QCOMPARE(publicDocument.pixelData(-1, -1, 1, 1), outside);
    const auto referenceBounds = image->projection()->exactBounds();
    const auto referenceDefaultPixel = image->projection()->defaultPixel();
    const auto referenceThumbnail = publicDocument.thumbnail(32, 16);

    namespace Gpu = LibrePaint::Image::Gpu;
    Gpu::RasterImage input;
    input.size = {32, 16};
    for (int i = 0; i < layerCount; ++i) input.layers.append({QStringLiteral("Paint Layer %1").arg(i + 1), true, {}, {}});
    input.projectionBackground = {quint8(background.red()), quint8(background.green()),
                                   quint8(background.blue()), quint8(background.alpha())};
    if (hiddenPixel) {
        Gpu::RgbaTile tile{{0, 0}, QByteArray(Gpu::RgbaTile::ByteCount, '\0')};
        auto pixel = QByteArray::fromHex("44556600");
        pixel[3] = char(pixelAlpha);
        tile.rgba.replace((3 * 256 + 3) * 4, 4, pixel);
        input.layers[0].tiles = {tile};
    }
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    QScopedPointer<KisDocument> target(KisPart::instance()->createDocument());
    auto gpuImage = KisImage::fromGpuImage(owner, QStringLiteral("Canvas background GPU"));
    target->setCurrentImage(gpuImage);
    QCOMPARE(gpuImage->defaultProjectionColor(), color);
    QCOMPARE(gpuImage->rootLayer()->defaultProjectionColor(), color);
    QCOMPARE(gpuImage->projection()->defaultPixel(), referenceDefaultPixel);
    Document publicTarget(target.data(), false);
    QCOMPARE(publicTarget.pixelData(3, 3, 1, 1), inside);
    QCOMPARE(publicTarget.pixelData(-1, -1, 1, 1), outside);
    QCOMPARE(gpuImage->projection()->exactBounds(), referenceBounds);
    QCOMPARE(publicTarget.thumbnail(32, 16), referenceThumbnail);
}


void TestDocument::testGpuDocumentPreviewsTransferOnlyRequestedPixels_data()
{
    QTest::addColumn<QString>("fixture");
    QTest::addColumn<QSize>("sourceSize");
    QTest::addColumn<QSize>("requestedSize");
    QTest::newRow("bicubic-reduction") << QStringLiteral("gradient") << QSize(64, 32) << QSize(16, 8);
    QTest::newRow("bicubic-aspect") << QStringLiteral("gradient") << QSize(64, 32) << QSize(31, 17);
    QTest::newRow("bicubic-enlargement") << QStringLiteral("gradient") << QSize(64, 32) << QSize(96, 48);
    QTest::newRow("integer-pixel-art") << QStringLiteral("gradient") << QSize(64, 32) << QSize(128, 64);
    QTest::newRow("qt-smooth-reduction") << QStringLiteral("gradient") << QSize(513, 513) << QSize(1, 1);
    QTest::newRow("bicubic-minimum-scale") << QStringLiteral("gradient") << QSize(256, 256) << QSize(1, 1);
    QTest::newRow("qt-smooth-sparse-enlargement") << QStringLiteral("sparse") << QSize(2049, 2049) << QSize(8, 8);
    QTest::newRow("large-sparse-canvas") << QStringLiteral("sparse") << QSize(9000, 9000) << QSize(16, 16);
    QTest::newRow("large-transparent-background") << QStringLiteral("transparent-background") << QSize(9000, 9000) << QSize(16, 16);
    QTest::newRow("qt-smooth-one-pixel-axis") << QStringLiteral("gradient") << QSize(1, 513) << QSize(1, 1);
    QTest::newRow("qt-smooth-thin-canvas") << QStringLiteral("gradient") << QSize(1025, 2) << QSize(1, 1);
    QTest::newRow("sparse-content") << QStringLiteral("sparse") << QSize(64, 32) << QSize(16, 8);
    QTest::newRow("empty-content") << QStringLiteral("empty") << QSize(64, 32) << QSize(16, 8);
    QTest::newRow("implicit-background") << QStringLiteral("background") << QSize(33, 17) << QSize(16, 16);
}

void TestDocument::testGpuDocumentPreviewsTransferOnlyRequestedPixels()
{
    QFETCH(QString, fixture);
    QFETCH(QSize, sourceSize);
    QFETCH(QSize, requestedSize);
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig config(false);
    const auto checker1 = config.checkersColor1(), checker2 = config.checkersColor2();
    const auto restoreCheckers = qScopeGuard([&] { config.setCheckersColor1(checker1); config.setCheckersColor2(checker2); });
    config.setCheckersColor1(QColor(80, 96, 112));
    config.setCheckersColor2(QColor(160, 176, 192));
    KisImageConfig imageConfig(false);
    const int previousThreads = imageConfig.maxNumberOfThreads();
    const auto restoreThreads = qScopeGuard([&] { imageConfig.setMaxNumberOfThreads(previousThreads); });
    imageConfig.setMaxNumberOfThreads(1);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const QColor background = fixture == QLatin1String("background") ? QColor(17, 34, 51, 128) :
        fixture == QLatin1String("transparent-background") ? QColor(255, 255, 255, 0) : QColor(0, 0, 0, 0);
    KisImageSP reference = new KisImage(nullptr, sourceSize.width(), sourceSize.height(), cs, QStringLiteral("Preview reference"));
    reference->setDefaultProjectionColor(KoColor(background, cs));
    KisNodeSP layer = new KisPaintLayer(reference, QStringLiteral("Preview pixels"), 255);
    reference->addNode(layer);
    Gpu::RasterImage raster;
    raster.size = sourceSize;
    raster.projectionBackground = {quint8(background.red()), quint8(background.green()), quint8(background.blue()), quint8(background.alpha())};
    raster.layers = {{QStringLiteral("Preview pixels"), true, {}, {}}};
    const bool sparse = fixture == QLatin1String("sparse");
    if (fixture == QLatin1String("gradient") || sparse) {
        for (int ty = sparse ? -1 : 0; sparse ? ty <= 0 : ty * 256 < sourceSize.height(); ++ty) {
            for (int tx = sparse ? -1 : 0; sparse ? tx <= 0 : tx * 256 < sourceSize.width(); ++tx) {
                QByteArray rgba(Gpu::RgbaTile::ByteCount, '\0');
                for (int y = 0; y < 256; ++y) for (int x = 0; x < 256; ++x) {
                    const int px = tx * 256 + x, py = ty * 256 + y;
                    if ((!sparse && QRect(QPoint(), sourceSize).contains(px, py)) ||
                        (sparse && (QRect(17, 9, 5, 5).contains(px, py) || QPoint(px, py) == QPoint(-1, -1)))) {
                        auto *pixel = rgba.data() + (y * 256 + x) * 4;
                        pixel[0] = char((px * 17 + py * 3) & 255);
                        pixel[1] = char((px * 5 + py * 11) & 255);
                        pixel[2] = char((px * 13 + py * 7) & 255);
                        pixel[3] = char((px + py) % 4 == 0 ? 0 : (px + py) % 4 == 1 ? 73 : 255);
                    }
                }
                raster.layers.first().tiles.append({{tx, ty}, rgba});
                for (qsizetype i = 0; i < rgba.size(); i += 4) std::swap(rgba[i], rgba[i + 2]);
                layer->paintDevice()->writeBytes(reinterpret_cast<const quint8*>(rgba.constData()), QRect(tx * 256, ty * 256, 256, 256));
            }
        }
    }
    reference->initialRefreshGraph();
    reference->waitForDone();
    QScopedPointer<KisDocument> referenceDocument(KisPart::instance()->createDocument());
    referenceDocument->setCurrentImage(reference);
    Document expected(referenceDocument.data(), false);
    const auto thumbnail = expected.thumbnail(requestedSize.width(), requestedSize.height());
    QVERIFY(!thumbnail.isNull());
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device, 16 * Gpu::TileRenderer::TileBytes, 1, 4096);
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Bounded document preview")));
    Document actual(document.data(), false);
    const auto fixed = owner->operations().snapshot();
    const auto published = execution->published(owner->handle());
    const auto modified = document->isModified();
    QSignalSpy changes(owner.data(), &Gpu::PaintImage::changed);
    QImage result;
    try {
        result = actual.thumbnail(requestedSize.width(), requestedSize.height());
    } catch (const std::exception &error) {
        QFAIL(qPrintable(QStringLiteral("Document.thumbnail could not produce a bounded preview: ") + QString::fromUtf8(error.what())));
    }
    if (fixture == QLatin1String("background")) {
        QCOMPARE(result.pixelColor(0, 0), background);
        QCOMPARE(thumbnail.pixelColor(0, 0), background);
    }
    if (QTest::currentDataTag() == QLatin1String("large-sparse-canvas")) {
        // The raster sample is premultiplied RGBA(27,67,14,90).
        // Straight channels round integer quotients to the nearest value, ties up.
        QCOMPARE(result.pixelColor(5, 3), QColor(77, 190, 40, 90));
        QCOMPARE(thumbnail.pixelColor(5, 3), QColor(77, 190, 40, 90));
    }
    QImage converted, referenceConverted;
    QByteArray conversionProfileId;
    const bool convertsProfile = qreal(requestedSize.width()) / sourceSize.width() >= 1. / 256 &&
        qreal(requestedSize.height()) / sourceSize.height() >= 1. / 256 && fixture != QLatin1String("empty");
    if (convertsProfile) {
        const auto *linear = KoColorSpaceRegistry::instance()->profileByName(QStringLiteral("sRGB-elle-V2-g10.icc"));
        QVERIFY(linear);
        QVERIFY(linear->uniqueId() != cs->profile()->uniqueId());
        conversionProfileId = linear->uniqueId();
        referenceConverted = reference->convertToQImage(requestedSize, linear);
        converted = document->image()->convertToQImage(requestedSize, linear);
    }
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const auto name = QStringLiteral("document-preview-") + QString::fromLatin1(QTest::currentDataTag());
        QList<QPair<QString, QImage>> images{{name, result}, {name + QStringLiteral("-reference"), thumbnail}};
        if (convertsProfile) {
            images.append({name + QStringLiteral("-linear"), converted});
            images.append({name + QStringLiteral("-linear-reference"), referenceConverted});
        }
        for (const auto &entry : images) {
            const auto rgba = entry.second.convertToFormat(QImage::Format_RGBA8888);
            QFile pixels(QDir(results).filePath(entry.first + QStringLiteral(".rgba")));
            QVERIFY(pixels.open(QIODevice::WriteOnly));
            QCOMPARE(pixels.write(reinterpret_cast<const char*>(rgba.constBits()), rgba.sizeInBytes()), rgba.sizeInBytes());
            QVERIFY(entry.second.save(QDir(results).filePath(entry.first + QStringLiteral(".png"))));
            QFile state(QDir(results).filePath(entry.first + QStringLiteral(".state")));
            QVERIFY(state.open(QIODevice::WriteOnly));
            QDataStream stream(&state);
            stream.setVersion(QDataStream::Qt_5_15);
            stream << entry.second.size() << qint32(entry.second.format()) << entry.second.devicePixelRatio();
            stream << cs->profile()->uniqueId() << (entry.first.contains(QLatin1String("-linear")) ? conversionProfileId : cs->profile()->uniqueId());
            stream << qint32(imageConfig.maxNumberOfThreads());
            QCOMPARE(stream.status(), QDataStream::Ok);
        }
    }
    QCOMPARE(result, thumbnail);
    if (convertsProfile) QCOMPARE(converted, referenceConverted);
    QCOMPARE(document->generatePreview(requestedSize).toImage(), referenceDocument->generatePreview(requestedSize).toImage());
    QCOMPARE(owner->operations().snapshot(), fixed);
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(execution->requested(owner->handle()), fixed);
    QCOMPARE(execution->published(owner->handle()), published);
    QCOMPARE(document->isModified(), modified);
    QCOMPARE(changes.size(), 0);
}

void TestDocument::testSetColorSpace()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    QStringList profiles = Krita().profiles("GRAYA", "U16");
    d.setColorSpace("GRAYA", "U16", profiles.first());

    QVERIFY(layer->colorSpace()->colorModelId().id() == "GRAYA");
    QVERIFY(layer->colorSpace()->colorDepthId().id() == "U16");
    QVERIFY(layer->colorSpace()->profile()->name() == profiles.first());

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testSetColorProfile()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QStringList profiles = Krita().profiles("RGBA", "U8");
    Q_FOREACH(const QString &profileName, profiles) {
        const KoColorProfile *profile = KoColorSpaceRegistry::instance()->profileByName(profileName);

        // skip input-only profiles (e.g. for scanners)
        if (!profile->isSuitableForOutput()) continue;

        d.setColorProfile(profileName);
        QVERIFY(image->colorSpace()->profile()->name() == profileName);
    }
    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testPixelData()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    d.refreshProjection();

    QByteArray ba = d.pixelData(0, 0, 100, 100);
    QDataStream ds(ba);
    do {
        quint8 channelvalue;
        ds >> channelvalue;
        QVERIFY(channelvalue == 0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 255);
        ds >> channelvalue;
        QVERIFY(channelvalue == 255);
    } while (!ds.atEnd());

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testGpuImagePixelDataModifiedStateAndRetainedNode()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    std::weak_ptr<Gpu::Execution> lifetime = execution;
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto layerId = owner->operations().activeLayer();
    QCOMPARE(owner->writeRgbaPixels(layerId, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Public GPU document"));
    kisdoc->setCurrentImage(image);
    Document document(kisdoc.data(), false);
    QCOMPARE(document.width(), 64);
    QCOMPARE(document.height(), 32);
    QCOMPARE(document.pixelData(5, 5, 1, 1), QByteArray::fromHex("332211ff"));
    QVERIFY(!document.modified());
    QScopedPointer<Node> root(document.rootNode());
    const auto children = root->childNodes();
    QCOMPARE(children.size(), 1);
    NodeSP node(children.front());
    root.reset();
    QSignalSpy modified(kisdoc.data(), &KisDocument::modified);
    QVERIFY(node->setPixelData(QByteArray::fromHex("90a4bcff"), 5, 5, 1, 1));
    document.refreshProjection();
    QCOMPARE(document.pixelData(5, 5, 1, 1), QByteArray::fromHex("90a4bcff"));
    QVERIFY(!document.modified());
    QVERIFY(modified.isEmpty());
    QCOMPARE(owner->replaceRgbaPixels(layerId, QRect(5, 5, 1, 1), QByteArray::fromHex("223344ff")), Gpu::Result::Success);
    QVERIFY(document.modified());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!document.modified());
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(document.modified());
    QCOMPARE(modified.size(), 3);
    QCOMPARE(document.pixelData(5, 5, 1, 1), QByteArray::fromHex("443322ff"));
    owner.clear();
    image.clear();
    KisPart::instance()->removeDocument(kisdoc.data(), false);
    kisdoc.reset();
    execution.reset();
    device.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(node->pixelData(5, 5, 1, 1), QByteArray::fromHex("443322ff"));
    node.clear();
    QVERIFY(lifetime.expired());
}

void TestDocument::testSavedPngDensityUsesDocumentResolution_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QPointF>("resolution");
    QTest::addColumn<QSize>("density");
    for (bool gpu : {false, true}) {
        QTest::newRow(gpu ? "gpu-72dpi" : "ordinary-72dpi") << gpu << QPointF(72, 72) << QSize(2835, 2835);
        QTest::newRow(gpu ? "gpu-300x150dpi" : "ordinary-300x150dpi") << gpu << QPointF(300, 150) << QSize(11811, 5906);
    }
}

void TestDocument::testSavedPngDensityUsesDocumentResolution()
try {
    QFETCH(bool, gpu);
    QFETCH(QPointF, resolution);
    QFETCH(QSize, density);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const QUuid id(QStringLiteral("{d1000001-1122-3344-5566-778899aabbcc}"));
    KisImageSP image;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage raster;
        raster.size = {4, 2};
        raster.layers = {{QStringLiteral("Density layer"), true, id, {}}};
        raster.colorProfile = KoColorSpaceRegistry::instance()->rgb8()->profile()->rawData();
        image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, raster), QStringLiteral("Density image"));
    } else {
        image = new KisImage(document->createUndoStore(), 4, 2, KoColorSpaceRegistry::instance()->rgb8(), "Density image");
        auto layer = new KisPaintLayer(image, QStringLiteral("Density layer"), 255);
        layer->setUuid(id);
        image->addNode(layer);
    }
    image->setResolution(resolution.x() / 72, resolution.y() / 72);
    document->setCurrentImage(image);
    Document publicDocument(document.data(), false);
    QScopedPointer<Node> layer(Node::createNode(image, image->root()->firstChild()));
    QVERIFY(layer->setPixelData(QByteArray::fromHex("112233ff").repeated(8), 0, 0, 4, 2));
    publicDocument.refreshProjection();
    image->waitForDone();
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Fixed density document"));
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    document->setFileBatchMode(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Document PNG density artifacts retained at" << directory.path();
        }
    });
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto stem = QStringLiteral("document-density-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const auto savedPath = QDir(destination).filePath(stem + QStringLiteral(".kra"));
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    QVERIFY2(document->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false, configuration), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    InfoObject png;
    png.setProperty("alpha", true);
    png.setProperty("interlaced", false);
    png.setProperty("compression", 3);
    png.setProperty("saveSRGBProfile", true);
    png.setProperty("forceSRGB", false);
    png.setProperty("storeAuthor", false);
    png.setProperty("storeMetaData", false);
    const auto pngPath = QDir(destination).filePath(stem + QStringLiteral(".png"));
    QVERIFY(publicDocument.exportImage(pngPath, png));
    QFile file(savedPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto saved = Kra::RasterCodec::read(file);
    QVERIFY2(saved.code == Kra::Status::Success, qPrintable(saved.diagnostic));
    file.close();
    QScopedPointer<KoStore> store(KoStore::createStore(savedPath, KoStore::Read));
    QVERIFY(store && !store->bad());
    QList<QPair<QString, QImage>> images;
    for (const auto &name : {QStringLiteral("mergedimage.png"), QStringLiteral("preview.png")}) {
        QVERIFY(store->open(name));
        images.append({name, QImage::fromData(store->device()->readAll())});
        QVERIFY(store->close());
    }
    images.append({QStringLiteral("exported.png"), QImage(pngPath)});
    for (const auto &entry : images)
        qInfo() << entry.first << "document DPI" << resolution << "PNG density" << entry.second.dotsPerMeterX() << entry.second.dotsPerMeterY();
    for (const auto &entry : images) {
        QVERIFY2(!entry.second.isNull(), qPrintable(entry.first));
        QCOMPARE(entry.second.dotsPerMeterX(), density.width());
        QCOMPARE(entry.second.dotsPerMeterY(), density.height());
    }
    KisPart::instance()->removeDocument(document.data(), false);
}
catch (const std::exception &error) { QFAIL(error.what()); }

void TestDocument::testGpuSaveAndExportPreserveHistoryAndFileIdentity()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("GPU file operations"));
    image->setResolution(300.0 / 72, 150.0 / 72);
    image->addAnnotation(new KisAnnotation(QStringLiteral("project-data"), QStringLiteral("Annotation description"), QByteArrayLiteral("preserved annotation")));
    document->setCurrentImage(image);
    KisMirrorAxisConfig mirror;
    mirror.setAxisPosition(QPointF(21.25, 13.5));
    mirror.setMirrorHorizontal(true);
    mirror.setMirrorVertical(true);
    mirror.setLockHorizontal(true);
    mirror.setHideVerticalDecoration(true);
    mirror.setHandleSize(19.5f);
    mirror.setHorizontalHandlePosition(8.25f);
    mirror.setVerticalHandlePosition(-4.5f);
    document->setMirrorAxisConfig(mirror);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("保存契約"));
    document->setFileBatchMode(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto savedPath = directory.filePath(QStringLiteral("saved.kra"));
    const auto exportedPath = directory.filePath(QStringLiteral("exported.kra"));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "GPU document save artifacts retained at" << directory.path();
        }
    });
    QSignalSpy saved(document.data(), &KisDocument::sigSavingFinished);
    QVERIFY2(document->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QCOMPARE(saved.size(), 1);
    QCOMPARE(document->path(), savedPath);
    QCOMPARE(document->mimeType(), KisDocument::nativeFormatMimeType());
    QVERIFY(!document->isModified());
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().savedHistoryIndex(), 1);
    QFile input(savedPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto savedBytes = input.readAll();
    QVERIFY(input.seek(0));
    const auto loaded = Kra::RasterCodec::read(input);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.size, QSize(64, 32));
    QCOMPARE(loaded.document.image.layers.size(), 1);
    QCOMPARE(loaded.document.image.layers.first().uuid, owner->operations().snapshot()->layers.first().uuid);
    QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4),
             QByteArray::fromHex("112233ff"));
    QDomDocument description;
    QVERIFY(description.setContent(loaded.document.description));
    const auto imageDescription = description.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    QCOMPARE(imageDescription.attribute(QStringLiteral("x-res")).toDouble(), 300.0);
    QCOMPARE(imageDescription.attribute(QStringLiteral("y-res")).toDouble(), 150.0);
    QCOMPARE(imageDescription.attribute(QStringLiteral("profile")), image->profile()->name());
    KisMirrorAxisConfig loadedMirror;
    QVERIFY(loadedMirror.loadFromXml(imageDescription.firstChildElement(QStringLiteral("MirrorAxis"))));
    QVERIFY(loadedMirror == mirror);
    const auto folder = imageDescription.attribute(QStringLiteral("name"));
    QCOMPARE(folder, document->documentInfo()->aboutInfo(QStringLiteral("title")));
    QCOMPARE(loaded.document.files.value(folder + QStringLiteral("/annotations/icc")), image->profile()->rawData());
    QCOMPARE(loaded.document.files.value(folder + QStringLiteral("/annotations/project-data")), QByteArrayLiteral("preserved annotation"));
    QVERIFY(loaded.document.files.value(QStringLiteral("documentinfo.xml")).contains(QStringLiteral("保存契約").toUtf8()));
    input.close();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
    QVERIFY(document->isModified());
    const auto current = owner->operations().snapshot();
    Document publicDocument(document.data(), false);
    InfoObject settings;
    document->setMirrorAxisConfig(KisMirrorAxisConfig());
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Renamed export"));
    QVERIFY(publicDocument.exportImage(exportedPath, settings));
    QCOMPARE(document->path(), savedPath);
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), current);
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->operations().savedHistoryIndex(), 1);
    QCOMPARE(saved.size(), 1);
    QFile exported(exportedPath);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    const auto exportedValue = Kra::RasterCodec::read(exported);
    QVERIFY2(exportedValue.code == Kra::Status::Success, qPrintable(exportedValue.diagnostic));
    QDomDocument exportedDescription;
    QVERIFY(exportedDescription.setContent(exportedValue.document.description));
    QCOMPARE(exportedDescription.documentElement().firstChildElement(QStringLiteral("IMAGE"))
        .attribute(QStringLiteral("name")), QStringLiteral("Renamed export"));
    QVERIFY(!exportedValue.document.files.contains(folder + QStringLiteral("/annotations/icc")));
    QCOMPARE(exportedValue.document.files.value(QStringLiteral("Renamed export/annotations/icc")), image->profile()->rawData());
    QCOMPARE(exportedValue.document.files.value(QStringLiteral("Renamed export/annotations/project-data")), QByteArrayLiteral("preserved annotation"));
    QCOMPARE(exportedDescription.documentElement().firstChildElement(QStringLiteral("IMAGE"))
        .attribute(QStringLiteral("profile")), image->profile()->name());
    QVERIFY(exportedDescription.documentElement().firstChildElement(QStringLiteral("IMAGE"))
        .firstChildElement(QStringLiteral("MirrorAxis")).isNull());
    QCOMPARE(exportedValue.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4),
             QByteArray::fromHex("445566ff"));
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(exportedPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QCOMPARE(reopened->image()->objectName(), QStringLiteral("Renamed export"));
    QCOMPARE(reopened->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Renamed export"));
    QCOMPARE(reopened->image()->profile()->rawData(), image->profile()->rawData());
    Document publicReopened(reopened.data(), false);
    QCOMPARE(publicReopened.pixelData(5, 5, 1, 1), QByteArray::fromHex("665544ff"));
    KisPart::instance()->removeDocument(reopened.data(), false);
    QVERIFY(!document->saveAs(directory.filePath(QStringLiteral("missing/failed.kra")), KisDocument::nativeFormatMimeType(), false));
    QVERIFY(!document->errorMessage().isEmpty());
    QVERIFY(!document->exportDocumentSync(savedPath, QByteArrayLiteral("application/x-librepaint-unknown")));
    QVERIFY(!document->errorMessage().isEmpty());
    QCOMPARE(document->path(), savedPath);
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), current);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QCOMPARE(input.readAll(), savedBytes);
    QCOMPARE(saved.size(), 1);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuPngSaveAsPreservesHistoryAndAdoptsTheSavedFile()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(32, 16), QStringLiteral("PNG document"));
    document->setFileBatchMode(true);
    const auto owner = document->image()->gpuImage();
    const auto layer = owner->operations().activeLayer();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto originalPath = directory.filePath(QStringLiteral("original.kra"));
    const auto pngPath = directory.filePath(QStringLiteral("saved.png"));
    QVERIFY(document->saveAs(originalPath, KisDocument::nativeFormatMimeType(), false));
    QFile original(originalPath);
    QVERIFY(original.open(QIODevice::ReadOnly));
    const auto originalBytes = original.readAll();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    const auto version = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    QSignalSpy saved(document.data(), &KisDocument::sigSavingFinished);
    Document publicDocument(document.data(), false);
    QVERIFY2(publicDocument.saveAs(pngPath), qPrintable(document->errorMessage()));
    QCOMPARE(document->path(), pngPath);
    QCOMPARE(document->localFilePath(), pngPath);
    QCOMPARE(document->mimeType(), QByteArrayLiteral("image/png"));
    QVERIFY(!document->isModified());
    QCOMPARE(owner->operations().snapshot(), version);
    QCOMPARE(owner->operations().undoCount(), history);
    QCOMPARE(owner->operations().savedHistoryIndex(), history);
    QCOMPARE(saved.size(), 1);
    QCOMPARE(QImage(pngPath).pixelColor(5, 5), QColor(17, 34, 51));
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
    QVERIFY(document->isModified());
    QVERIFY2(publicDocument.save(), qPrintable(document->errorMessage()));
    QVERIFY(!document->isModified());
    QCOMPARE(saved.size(), 2);
    QCOMPARE(QImage(pngPath).pixelColor(5, 5), QColor(68, 85, 102));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(document->isModified());
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(!document->isModified());
    QVERIFY(!document->saveAs(directory.filePath(QStringLiteral("missing/failed.png")), QByteArrayLiteral("image/png"), false));
    QCOMPARE(document->path(), pngPath);
    QCOMPARE(document->mimeType(), QByteArrayLiteral("image/png"));
    QVERIFY(!document->isModified());
    QVERIFY(original.seek(0));
    QCOMPARE(original.readAll(), originalBytes);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(pngPath), qPrintable(reopened->errorMessage()));
    Document reopenedPublic(reopened.data(), false);
    QCOMPARE(reopenedPublic.pixelData(5, 5, 1, 1), QByteArray::fromHex("665544ff"));
    KisPart::instance()->removeDocument(reopened.data(), false);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuPngExportPreservesPixelsAndDocument_data()
{
    QTest::addColumn<bool>("alpha");
    QTest::addColumn<bool>("interlaced");
    QTest::addColumn<bool>("menuExport");
    QTest::addColumn<bool>("weighted");
    QTest::newRow("api-rgba") << true << false << false << false;
    QTest::newRow("api-rgba-interlaced") << true << true << false << false;
    QTest::newRow("api-white-background") << false << false << false << false;
    QTest::newRow("api-white-background-interlaced") << false << true << false << false;
    QTest::newRow("menu-rgba") << true << false << true << false;
    QTest::newRow("menu-rgba-interlaced") << true << true << true << false;
    QTest::newRow("menu-white-background") << false << false << true << false;
    QTest::newRow("menu-white-background-interlaced") << false << true << true << false;
    QTest::newRow("weighted-api-rgba") << true << false << false << true;
    QTest::newRow("weighted-menu-rgba") << true << false << true << true;
}

void TestDocument::testGpuPngExportPreservesPixelsAndDocument()
{
    QFETCH(bool, alpha);
    QFETCH(bool, interlaced);
    QFETCH(bool, menuExport);
    QFETCH(bool, weighted);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(32, 16), QStringLiteral("PNG export"));
    document->setFileBatchMode(true);
    KisImageSP image = document->image().toStrongRef();
    const auto owner = image->gpuImage();
    const auto layer = owner->operations().activeLayer();
    const QByteArray colors = QByteArray::fromHex("112233ff4455668077889900cb1d57ad");
    QByteArray rgba;
    for (int i = 0; i < 32 * 16; ++i) rgba.append(colors.mid((i % 4) * 4, 4));
    QCOMPARE(owner->replaceRgbaPixels(layer, image->bounds(), rgba), Gpu::Result::Success);
    if (weighted) {
        Gpu::Brush brush;
        brush.rgba = {221, 17, 44, 255};
        brush.diameter = 4;
        brush.basicSmoothing = true;
        brush.weightedSmoothing = Gpu::WeightedSmoothing{24, 48, 0.01, true};
        const QList<Gpu::InputSample> samples{{{4, 4}, 1, 0, 0, 0, 0, 0}, {{8, 8}, 0.8, 0, 0, 0, 8, 0.25},
                                            {{12, 4}, 0.6, 0, 0, 0, 16, 0.5}, {{16, 8}, 0.4, 0, 0, 0, 24, 0.75},
                                            {{20, 4}, 0.2, 0, 0, 0, 32, 1}, {{24, 8}, 0.1, 0, 0, 0, 40, 0}};
        QCOMPARE(owner->paintFigure(layer, brush, samples, Gpu::Shape{}, Gpu::FigureStyle{}, 42), Gpu::Result::Success);
        const auto painted = owner->readRgbaPixels(layer, image->bounds(), rgba.size());
        QCOMPARE(painted.code, Gpu::ExecutionCode::Success);
        QVERIFY(painted.bytes != rgba);
        rgba = painted.bytes;
    }
    image->setResolution(300.0 / 72, 150.0 / 72);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto savedPath = directory.filePath(QStringLiteral("saved.kra"));
    const auto pngPath = directory.filePath(QStringLiteral("export.png"));
    QVERIFY(document->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false));
    QFile savedFile(savedPath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    const auto savedBytes = savedFile.readAll();
    savedFile.close();
    QCOMPARE(owner->renameLayer(layer, QStringLiteral("Edited name")), Gpu::Result::Success);
    const auto version = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    const auto savedIndex = owner->operations().savedHistoryIndex();
    QSignalSpy saved(document.data(), &KisDocument::sigSavingFinished);
    Document publicDocument(document.data(), false);
    InfoObject settings;
    settings.setProperty(QStringLiteral("alpha"), alpha);
    settings.setProperty(QStringLiteral("interlaced"), interlaced);
    settings.setProperty(QStringLiteral("saveSRGBProfile"), true);
    KisPropertiesConfigurationSP menuSettings(new KisPropertiesConfiguration);
    menuSettings->setProperty(QStringLiteral("alpha"), alpha);
    menuSettings->setProperty(QStringLiteral("interlaced"), interlaced);
    menuSettings->setProperty(QStringLiteral("saveSRGBProfile"), true);
    const bool exportedSuccessfully = menuExport ?
        document->exportDocument(pngPath, QByteArrayLiteral("image/png"), false, false, menuSettings) :
        publicDocument.exportImage(pngPath, settings);
    QVERIFY2(exportedSuccessfully, qPrintable(document->errorMessage()));
    const QImage exported(pngPath);
    QCOMPARE(exported.size(), image->size());
    QCOMPARE(exported.dotsPerMeterX(), 11811);
    QCOMPARE(exported.dotsPerMeterY(), 5906);
    QCOMPARE(exported.colorSpace().iccProfile(), image->profile()->rawData());
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 32; ++x) {
            const auto *pixel = reinterpret_cast<const quint8 *>(rgba.constData() + (y * 32 + x) * 4);
            const auto blend = [pixel](int channel) {
                return (pixel[channel] * pixel[3] + 255 * (255 - pixel[3]) + 127) / 255;
            };
            const QColor expected = alpha ? QColor(pixel[0], pixel[1], pixel[2], pixel[3]) :
                                            QColor(blend(0), blend(1), blend(2));
            QCOMPARE(exported.pixelColor(x, y), expected);
        }
    }
    QFile png(pngPath);
    QVERIFY(png.open(QIODevice::ReadOnly));
    const auto pngBytes = png.readAll();
    QVERIFY(pngBytes.size() > 28);
    QCOMPARE(quint8(pngBytes[28]), quint8(interlaced ? 1 : 0));
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QSaveFile artifact(QDir(resultDirectory).filePath(QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".png")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pngBytes), pngBytes.size());
        QVERIFY(artifact.commit());
    }
    QVERIFY(!publicDocument.exportImage(directory.filePath(QStringLiteral("missing/failed.png")), settings));
    QVERIFY(!document->errorMessage().isEmpty());
    QCOMPARE(document->path(), savedPath);
    QCOMPARE(document->mimeType(), KisDocument::nativeFormatMimeType());
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), version);
    QCOMPARE(owner->operations().undoCount(), history);
    QCOMPARE(owner->operations().savedHistoryIndex(), savedIndex);
    QCOMPARE(saved.size(), menuExport ? 1 : 0);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    QCOMPARE(savedFile.readAll(), savedBytes);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuPngExportDialogLifetimes_data()
{
    QTest::addColumn<int>("reaction");
    QTest::addColumn<bool>("saving");
    for (const bool saving : {false, true}) {
        const auto prefix = saving ? QByteArrayLiteral("save-") : QByteArrayLiteral("export-");
        QTest::newRow(QByteArray(prefix + "cancel-preserves-existing-file").constData()) << 0 << saving;
        QTest::newRow(QByteArray(prefix + "later-edit-keeps-written-version").constData()) << 1 << saving;
        QTest::newRow(QByteArray(prefix + "close-document").constData()) << 2 << saving;
        QTest::newRow(QByteArray(prefix + "replace-image").constData()) << 3 << saving;
        QTest::newRow(QByteArray(prefix + "also-save-fixed-kra").constData()) << 4 << saving;
    }
}

void TestDocument::testGpuPngExportDialogLifetimes()
{
    QFETCH(int, reaction);
    QFETCH(bool, saving);
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(32, 16), QStringLiteral("PNG dialog"));
    const auto owner = document->image()->gpuImage();
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    if (reaction == 4) QVERIFY(owner->addLayer(QStringLiteral("Second layer")));
    const auto savingHistory = owner->operations().undoCount();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("existing.png"));
    const auto originalBytes = QByteArrayLiteral("existing file remains intact on cancellation");
    QFile existing(path);
    QVERIFY(existing.open(QIODevice::WriteOnly));
    QCOMPARE(existing.write(originalBytes), originalBytes.size());
    existing.close();
    bool dialogVisited = false;
    const bool previousAlsoAsKra = KisConfig(true).readEntry<bool>("AlsoSaveAsKra", false);
    const auto restoreAlsoAsKra = qScopeGuard([previousAlsoAsKra] {
        KisConfig(false).writeEntry<bool>("AlsoSaveAsKra", previousAlsoAsKra);
    });
    QTimer interaction;
    interaction.setInterval(1);
    connect(&interaction, &QTimer::timeout, this, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        interaction.stop();
        dialogVisited = true;
        if (reaction == 4) {
            auto checks = dialog->findChildren<QCheckBox *>();
            bool found = false;
            for (auto *check : checks) {
                if (check->text().contains(QStringLiteral("KRA"))) {
                    check->setChecked(true);
                    found = true;
                }
            }
            QVERIFY(found);
        }
        if (reaction == 0) {
            dialog->reject();
            return;
        }
        if (reaction == 1) {
            QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
        } else if (reaction == 2) {
            KisPart::instance()->removeDocument(document.data(), false);
            document.reset();
        } else if (reaction == 3) {
            initializeGpuDocument(document.data(), QSize(16, 16), QStringLiteral("Replacement"));
        }
        dialog->accept();
    });
    interaction.start();
    const bool success = saving ? document->saveAs(path, QByteArrayLiteral("image/png"), reaction == 4) :
        document->exportDocument(path, QByteArrayLiteral("image/png"), false, reaction == 4);
    QVERIFY(dialogVisited);
    QCOMPARE(success, reaction != 0);
    if (reaction == 0) {
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), originalBytes);
    } else {
        const QImage exported(path);
        QCOMPARE(exported.size(), QSize(32, 16));
        QCOMPARE(exported.pixelColor(5, 5), QColor(17, 34, 51, 255));
        if (reaction == 1)
            QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("445566ff"));
        if (reaction == 4) {
            QFile kra(path + QStringLiteral(".kra"));
            QVERIFY(kra.open(QIODevice::ReadOnly));
            const auto result = LibrePaint::ImportExport::Kra::RasterCodec::read(kra);
            QVERIFY2(result.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(result.diagnostic));
            QCOMPARE(result.document.image.layers.size(), 2);
            QCOMPARE(result.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4), QByteArray::fromHex("112233ff"));
        }
    }
    if (document) {
        if (saving && reaction != 0 && reaction != 3) {
            QCOMPARE(document->path(), path);
            QCOMPARE(document->mimeType(), QByteArrayLiteral("image/png"));
            QCOMPARE(document->isModified(), reaction == 1);
            QCOMPARE(owner->operations().savedHistoryIndex(), savingHistory);
        } else QVERIFY(document->path().isEmpty());
        KisPart::instance()->removeDocument(document.data(), false);
    }
}

void TestDocument::testGpuExportRowsRespectMemoryBudgetAndRetainTheirVersion()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(513, 513), QStringLiteral("Bounded export"));
    KisImageSP image = document->image().toStrongRef();
    const auto owner = image->gpuImage();
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("11223380")), Gpu::Result::Success);
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(261, 257, 1, 1), QByteArray::fromHex("445566aa")), Gpu::Result::Success);
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(512, 512, 1, 1), QByteArray::fromHex("778899ff")), Gpu::Result::Success);
    const auto pinned = owner->operations().snapshot();
    QVERIFY_EXCEPTION_THROWN(KisGpuExportRasterRows(image, 0), std::length_error);
    bool eventsProcessed = false;
    auto editResult = Gpu::Result::InvalidInput;
    QTimer edit;
    edit.setSingleShot(true);
    connect(&edit, &QTimer::timeout, this, [&] {
        eventsProcessed = true;
        editResult = owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("aabbccff"));
    });
    edit.start(0);
    auto source = std::make_unique<KisGpuExportRasterRows>(image, 16 * 1024 * 1024);
    edit.stop();
    QVERIFY(eventsProcessed);
    QCOMPARE(editResult, Gpu::Result::Success);
    QVERIFY(source->containsTransparency());
    const auto rows = source->rows();
    QCOMPARE(rows->colorSpace, image->colorSpace());
    QCOMPARE(rows->size, image->size());
    QVERIFY(owner->operations().snapshot() != pinned);
    QByteArray bytes;
    QVERIFY(rows->readRow(5, bytes).isOk());
    QCOMPARE(bytes.size(), 513 * 4);
    QCOMPARE(bytes.mid(5 * 4, 4), QByteArray::fromHex("33221180"));
    QVERIFY(rows->readRow(257, bytes).isOk());
    QCOMPARE(bytes.mid(261 * 4, 4), QByteArray::fromHex("665544aa"));
    QVERIFY(rows->readRow(512, bytes).isOk());
    QCOMPARE(bytes.mid(512 * 4, 4), QByteArray::fromHex("998877ff"));
    const auto retainedBytes = bytes;
    source.reset();
    QVERIFY(rows->readRow(512, bytes).isCancelled());
    QCOMPARE(bytes, retainedBytes);
    owner->close();
    QCOMPARE(retainedBytes.mid(512 * 4, 4), QByteArray::fromHex("998877ff"));
    KisPart::instance()->removeDocument(document.data(), false);
    // Metadata preparation dispatches events. A color conversion during that
    // wait must not pair the retained U16 pixels with the live U8 encoding.
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> changing(KisPart::instance()->createDocument());
    const auto *oldEncoding = KoColorSpaceRegistry::instance()->rgb16();
    QVERIFY(changing->newImage(QStringLiteral("Retained export encoding"), 32, 16,
        oldEncoding, KoColor(Qt::transparent, oldEncoding), KisConfig::CANVAS_COLOR, 1, {}, 1));
    KisImageSP changingImage = changing->image();
    const auto changingOwner = changingImage->gpuImage();
    const quint16 oldBgra[] = {0x1234, 0x5678, 0x9abc, 0xffff};
    changingImage->root()->firstChild()->paintDevice()->writeBytes(
        reinterpret_cast<const quint8 *>(oldBgra), QRect(5, 5, 1, 1));
    const auto retained = changingOwner->operations().snapshot();
    bool convertedDuringPreparation = false;
    QTimer conversion;
    conversion.setSingleShot(true);
    connect(&conversion, &QTimer::timeout, this, [&] {
        changingImage->convertImageColorSpace(KoColorSpaceRegistry::instance()->rgb8(),
            KoColorConversionTransformation::IntentPerceptual,
            KoColorConversionTransformation::HighQuality | KoColorConversionTransformation::NoOptimization);
        changingImage->waitForDone();
        convertedDuringPreparation = true;
    });
    conversion.start(0);
    KisGpuExportRasterRows fixedEncoding(changingImage, 16 * 1024 * 1024);
    conversion.stop();
    QVERIFY(convertedDuringPreparation);
    QCOMPARE(changingImage->colorSpace(), KoColorSpaceRegistry::instance()->rgb8());
    QCOMPARE(fixedEncoding.rows()->colorSpace, oldEncoding);
    QCOMPARE(retained->colorProfile, oldEncoding->profile()->rawData());
    QByteArray oldRow;
    QVERIFY(fixedEncoding.rows()->readRow(5, oldRow).isOk());
    QCOMPARE(oldRow.size(), 32 * 8);
    QCOMPARE(oldRow.mid(5 * 8, 8), QByteArray(reinterpret_cast<const char *>(oldBgra), 8));

}

void TestDocument::testGpuExportPreparationWaitsForAnOccupiedQueue()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(32, 16), QStringLiteral("Queued export"));
    KisImageSP image = document->image().toStrongRef();
    const auto owner = image->gpuImage();
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    QCOMPARE(owner->execution().beginReadbackFrame().code, Gpu::ExecutionCode::Success);
    bool released = false;
    QTimer release;
    release.setSingleShot(true);
    connect(&release, &QTimer::timeout, this, [&] {
        released = owner->execution().cancelFrame().code == Gpu::ExecutionCode::Success;
    });
    release.start(100);
    std::unique_ptr<KisGpuExportRasterRows> source;
    try {
        source = std::make_unique<KisGpuExportRasterRows>(image, 16 * 1024 * 1024);
    } catch (const std::exception &error) {
        release.stop();
        if (!released) owner->execution().cancelFrame();
        QFAIL(error.what());
    }
    QVERIFY(released);
    QVERIFY(source);
    QByteArray bytes;
    QVERIFY(source->rows()->readRow(5, bytes).isOk());
    QCOMPARE(bytes.mid(5 * 4, 4), QByteArray::fromHex("332211ff"));
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuExportRowsCancelPendingWorkerReads()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(513, 513), QStringLiteral("Cancelled row read"));
    const auto owner = document->image()->gpuImage();
    auto source = std::make_unique<KisGpuExportRasterRows>(document->image(), 16 * 1024 * 1024);
    const auto rows = source->rows();
    QCOMPARE(owner->execution().beginReadbackFrame().code, Gpu::ExecutionCode::Success);
    auto future = std::async(std::launch::async, [rows] {
        QByteArray bytes = QByteArrayLiteral("preserve caller bytes on cancellation");
        const auto result = rows->readRow(257, bytes);
        return std::make_pair(result.isCancelled(), bytes);
    });
    QTimer cancel;
    cancel.setSingleShot(true);
    connect(&cancel, &QTimer::timeout, this, [&] { source.reset(); });
    cancel.start(50);
    QTRY_VERIFY_WITH_TIMEOUT(future.wait_for(std::chrono::seconds(0)) == std::future_status::ready, 5000);
    cancel.stop();
    const auto result = future.get();
    QVERIFY(result.first);
    QCOMPARE(result.second, QByteArrayLiteral("preserve caller bytes on cancellation"));
    QCOMPARE(owner->execution().cancelFrame().code, Gpu::ExecutionCode::Success);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuPngExportRejectsExcessiveWorkingMemoryBeforeChangingFiles()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(int(Gpu::OperationDocument::CoordinateLimit), 257), QStringLiteral("Export memory limit"));
    document->setFileBatchMode(true);
    const auto owner = document->image()->gpuImage();
    QCOMPARE(owner->renameLayer(owner->operations().activeLayer(), QStringLiteral("Modified")), Gpu::Result::Success);
    const auto pinned = owner->operations().snapshot();
    const auto resident = owner->execution().residentBytes();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("existing.png"));
    QFile existing(path);
    QVERIFY(existing.open(QIODevice::WriteOnly));
    const auto originalBytes = QByteArrayLiteral("preserve existing file on resource rejection");
    QCOMPARE(existing.write(originalBytes), originalBytes.size());
    existing.close();
    QVERIFY(!document->exportDocumentSync(path, QByteArrayLiteral("image/png")));
    QVERIFY(!document->errorMessage().isEmpty());
    QVERIFY(existing.open(QIODevice::ReadOnly));
    QCOMPARE(existing.readAll(), originalBytes);
    QVERIFY(document->path().isEmpty());
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), pinned);
    QCOMPARE(owner->execution().residentBytes(), resident);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuPngExportStreamsLargeFixedDocuments()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(16384, 16384), QStringLiteral("Large streaming export"));
    document->setFileBatchMode(true);
    const auto owner = document->image()->gpuImage();
    QCOMPARE(owner->renameLayer(owner->operations().activeLayer(), QStringLiteral("Modified")), Gpu::Result::Success);
    const auto pinned = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    const auto resident = owner->execution().residentBytes();
    {
        KisGpuExportRasterRows source(document->image(), 48 * 1024 * 1024);
        QVERIFY(source.containsTransparency());
        QByteArray bytes;
        QVERIFY(source.rows()->readRow(0, bytes).isOk());
        QCOMPARE(bytes, QByteArray(16384 * 4, '\0'));
        QVERIFY(source.rows()->readRow(16383, bytes).isOk());
        QCOMPARE(bytes, QByteArray(16384 * 4, '\0'));
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("large.png"));
    QFile output(path);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QVERIFY(output.write("previous file") > 0);
    output.close();
    QVERIFY2(document->exportDocumentSync(path, QByteArrayLiteral("image/png")), qPrintable(document->errorMessage()));
    QImageReader reader(path, "PNG");
    QCOMPARE(reader.size(), QSize(16384, 16384));
    QVERIFY(output.open(QIODevice::ReadOnly));
    QVERIFY(output.seek(output.size() - 12));
    QCOMPARE(output.readAll(), QByteArray::fromHex("0000000049454e44ae426082"));
    QVERIFY(document->path().isEmpty());
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), pinned);
    QCOMPARE(owner->operations().undoCount(), history);
    QCOMPARE(owner->execution().residentBytes(), resident);
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestDocument::testGpuPngExportRequiresAtomicReplacementForExistingFiles()
{
#ifdef Q_OS_WIN
    QSKIP("Directory mode permissions are verified on POSIX filesystems");
#else
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), QSize(32, 16), QStringLiteral("Atomic PNG export"));
    document->setFileBatchMode(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("existing.png"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const auto originalBytes = QByteArrayLiteral("existing file remains intact when atomic replacement is unavailable");
    QCOMPARE(file.write(originalBytes), originalBytes.size());
    file.close();
    const auto previousPermissions = QFile::permissions(directory.path());
    const auto restorePermissions = qScopeGuard([&] { QFile::setPermissions(directory.path(), previousPermissions); });
    QVERIFY(QFile::setPermissions(directory.path(), QFile::ReadOwner | QFile::ExeOwner));
    QFile probe(directory.filePath(QStringLiteral("probe")));
    if (probe.open(QIODevice::WriteOnly)) QSKIP("This filesystem permits creation despite directory mode permissions");
    QVERIFY(!document->exportDocumentSync(path, QByteArrayLiteral("image/png")));
    QVERIFY(!document->errorMessage().isEmpty());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), originalBytes);
    KisPart::instance()->removeDocument(document.data(), false);
#endif
}

void TestDocument::testGpuSaveNotificationLifetimes_data()
{
    QTest::addColumn<int>("reaction");
    QTest::addColumn<bool>("png");
    for (const bool png : {false, true}) {
        const auto prefix = png ? QByteArrayLiteral("png-") : QByteArrayLiteral("kra-");
        QTest::newRow(QByteArray(prefix + "later-edit").constData()) << 0 << png;
        QTest::newRow(QByteArray(prefix + "close-document").constData()) << 1 << png;
        QTest::newRow(QByteArray(prefix + "replace-image").constData()) << 2 << png;
        QTest::newRow(QByteArray(prefix + "close-on-completion").constData()) << 3 << png;
        QTest::newRow(QByteArray(prefix + "document-info-edit").constData()) << 4 << png;
        QTest::newRow(QByteArray(prefix + "background-edit").constData()) << 5 << png;
    }
}

void TestDocument::testGpuSaveNotificationLifetimes()
{
    QFETCH(int, reaction);
    QFETCH(bool, png);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::RasterImage raster{QSize(64, 32), {{QStringLiteral("Paint Layer 1"), true, {}, {}}}, 0};
    raster.colorProfile = KoColorSpaceRegistry::instance()->rgb8()->profile()->rawData();
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Save notification fixture")));
    document->setFileBatchMode(true);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Before save"));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(png ? QStringLiteral("saved.png") : QStringLiteral("saved.kra"));
    const auto original = directory.filePath(QStringLiteral("original.kra"));
    document->setPath(original);
    QSignalSpy saved(document.get(), &KisDocument::sigSavingFinished);
    if (reaction == 3) {
        connect(document.get(), &KisDocument::completed, this, [&] {
            KisPart::instance()->removeDocument(document.get(), false);
            document.reset();
        });
    }
    Gpu::PaintImageSP replacement;
    connect(owner.data(), &Gpu::PaintImage::savedStateChanged, this, [&] {
        switch (reaction) {
        case 0:
            QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
            break;
        case 1:
            KisPart::instance()->removeDocument(document.get(), false);
            document.reset();
            break;
        case 2:
            replacement = Gpu::PaintImageSP::create(execution, QSize(32, 16));
            document->setCurrentImage(KisImage::fromGpuImage(replacement, QStringLiteral("Replacement image")));
            break;
        case 4:
            document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("After save"));
            document->setModified(true);
            break;
        case 5: {
            Document publicDocument(document.get(), false);
            QVERIFY(publicDocument.setBackgroundColor(QColor(17, 34, 51, 128)));
            break;
        }
        }
    });
    QVERIFY(document->saveAs(path, png ? QByteArrayLiteral("image/png") : KisDocument::nativeFormatMimeType(), false));
    Kra::Document loaded;
    if (png) {
        QCOMPARE(QImage(path).pixelColor(5, 5), QColor(17, 34, 51));
        QCOMPARE(QImage(path).pixelColor(0, 0).alpha(), 0);
    } else {
        QFile input(path);
        QVERIFY(input.open(QIODevice::ReadOnly));
        const auto result = Kra::RasterCodec::read(input);
        QVERIFY2(result.code == Kra::Status::Success, qPrintable(result.diagnostic));
        loaded = result.document;
        QCOMPARE(loaded.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4), QByteArray::fromHex("112233ff"));
    }
    if (reaction == 0 || reaction == 4 || reaction == 5) {
        QCOMPARE(document->path(), path);
        QVERIFY(document->isModified());
        QCOMPARE(owner->operations().undoCount(), reaction == 0 ? 2 : 1);
        QCOMPARE(owner->operations().savedHistoryIndex(), 1);
        QCOMPARE(saved.size(), 1);
        QVERIFY(!document->isSaving());
        if (reaction == 4) {
            QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("After save"));
            if (!png) {
                QVERIFY(loaded.files.value(QStringLiteral("documentinfo.xml")).contains("Before save"));
                QVERIFY(!loaded.files.value(QStringLiteral("documentinfo.xml")).contains("After save"));
            }
        }
        if (reaction == 5) {
            const LibrePaint::Image::Gpu::RgbaPixel edited{17, 34, 51, 128};
            const LibrePaint::Image::Gpu::RgbaPixel original{};
            QCOMPARE(owner->operations().snapshot()->projectionBackground, edited);
            if (!png) QCOMPARE(loaded.image.projectionBackground, original);
        }
    } else if (reaction == 1 || reaction == 3) {
        QVERIFY(!document);
        QCOMPARE(saved.size(), 0);
        QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    } else {
        QCOMPARE(document->path(), original);
        QCOMPARE(document->image()->gpuImage(), replacement);
        QVERIFY(!document->isModified());
        QCOMPARE(saved.size(), 0);
        QVERIFY(!document->isSaving());
    }
    if (document) KisPart::instance()->removeDocument(document.get(), false);
}

void TestDocument::testGpuSnapshotKeepsIndependentPixelsAndDocumentMetadata()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    const auto hidden = owner->addLayer(QStringLiteral("Hidden layer"));
    QVERIFY(hidden);
    QCOMPARE(owner->writeRgbaPixels(hidden, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Gpu::Result::Success);
    QCOMPARE(owner->setLayerVisible(hidden, false), Gpu::Result::Success);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Snapshot source"));
    auto *timing = image->animationInterface();
    timing->setFramerate(30);
    timing->setDocumentRange(KisTimeSpan::fromTimeToTime(5, 125));
    timing->setExportSequenceFilePath(QStringLiteral("frames"));
    timing->setExportSequenceBaseName(QStringLiteral("pose"));
    timing->setExportInitialFrameNumber(7);
    QVERIFY(timing->initializeStaticTime(17));
    image->setResolution(300.0 / 72, 150.0 / 72);
    image->addAnnotation(new KisAnnotation(QStringLiteral("snapshot-data"), QStringLiteral("Snapshot annotation"), QByteArrayLiteral("annotation value")));
    image->root()->firstChild()->setUserLocked(true);
    image->root()->firstChild()->setColorLabelIndex(3);
    document->setCurrentImage(image);
    document->setPath(QStringLiteral("/tmp/snapshot-source.kra"));
    document->setLocalFilePath(document->path());
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Snapshot title"));
    const auto before = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    QScopedPointer<KisDocument> snapshot(document->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    auto copied = snapshot->image()->gpuImage();
    QVERIFY(copied);
    QVERIFY(copied != owner);
    QVERIFY(copied->handle() != owner->handle());
    QVERIFY(copied->operations().snapshot()->documentId() != before->documentId());
    QCOMPARE(copied->operations().undoCount(), 0);
    QCOMPARE(copied->operations().redoCount(), 0);
    QVERIFY(!snapshot->isModified());
    QCOMPARE(snapshot->path(), document->path());
    QCOMPARE(snapshot->localFilePath(), document->localFilePath());
    QCOMPARE(snapshot->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Snapshot title"));
    QCOMPARE(snapshot->image()->xRes(), image->xRes());
    QCOMPARE(snapshot->image()->yRes(), image->yRes());
    QCOMPARE(snapshot->image()->annotation(QStringLiteral("snapshot-data"))->annotation(), QByteArrayLiteral("annotation value"));
    QVERIFY(snapshot->image()->root()->firstChild()->userLocked());
    QCOMPARE(snapshot->image()->root()->firstChild()->colorLabelIndex(), 3);
    auto *copiedTiming = snapshot->image()->animationInterface();
    QCOMPARE(copiedTiming->currentTime(), 17);
    QCOMPARE(copiedTiming->currentUITime(), 17);
    timing->setFramerate(48);
    timing->setDocumentRange(KisTimeSpan::fromTimeToTime(0, 48));
    timing->setExportSequenceFilePath(QStringLiteral("later"));
    timing->setExportSequenceBaseName(QStringLiteral("after"));
    timing->setExportInitialFrameNumber(9);
    QCOMPARE(copiedTiming->framerate(), 30);
    QCOMPARE(copiedTiming->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(5, 125));
    QCOMPARE(copiedTiming->exportSequenceFilePath(), QStringLiteral("frames"));
    QCOMPARE(copiedTiming->exportSequenceBaseName(), QStringLiteral("pose"));
    QCOMPARE(copiedTiming->exportInitialFrameNumber(), 7);
    QCOMPARE(copied->operations().snapshot()->layers.size(), before->layers.size());
    for (int index = 0; index < before->layers.size(); ++index) {
        const auto &original = before->layers[index];
        const auto &cloned = copied->operations().snapshot()->layers[index];
        QCOMPARE(cloned.uuid, original.uuid);
        QCOMPARE(cloned.name, original.name);
        QCOMPARE(cloned.visible, original.visible);
    }
    QCOMPARE(copied->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    QCOMPARE(copied->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(copied->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().undoCount(), history);
    auto retained = snapshot->image()->root()->firstChild()->paintDevice();
    KisPart::instance()->removeDocument(document.data(), false);
    KisPart::instance()->removeDocument(snapshot.data(), false);
    snapshot.reset();
    copied.clear();
    QByteArray bytes(4, Qt::Uninitialized);
    retained->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 5, 5, 1, 1);
    QCOMPARE(bytes, QByteArray::fromHex("665544ff"));
}

void TestDocument::testGpuDocumentCopyKeepsLayerImageMetadata_data()
{
    QTest::addColumn<QString>("operation");
    for (const auto &operation : {QStringLiteral("snapshot"), QStringLiteral("clone"), QStringLiteral("replace")})
        QTest::newRow(qPrintable(operation)) << operation;
}

void TestDocument::testGpuDocumentCopyKeepsLayerImageMetadata()
{
    QFETCH(QString, operation);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const QUuid group(QStringLiteral("{88223344-5566-7788-9900-aabbccddeeff}"));
    Gpu::RasterImage input;
    input.size = {8, 8};
    input.layers = {{QStringLiteral("Outside"), true, {}, {}},
                    {QStringLiteral("Inside"), true, {}, {}},
                    {QStringLiteral("Group"), true, group, {}}};
    input.layers[1].parentUuid = group;
    input.layers[2].kind = Gpu::LayerKind::Group;
    const auto owner = Gpu::PaintImageSP::create(execution, input);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Layer metadata source"));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(image);
    const QString uri = QStringLiteral("http://ns.adobe.com/tiff/1.0/");
    const auto schema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(uri);
    QVERIFY(schema);
    KisNodeList sourceNodes{image->root()};
    QMap<QString, QString> expected;
    for (qsizetype i = 0; i < sourceNodes.size(); ++i) {
        for (auto child = sourceNodes[i]->firstChild(); child; child = child->nextSibling()) sourceNodes.append(child);
        auto *layer = qobject_cast<KisLayer *>(sourceNodes[i].data());
        QVERIFY(layer);
        const QString value = QStringLiteral("Camera for ") + layer->name();
        QVERIFY(layer->metaData()->addEntry(KisMetaData::Entry(schema, QStringLiteral("Make"), KisMetaData::Value(QVariant(value)))));
        expected.insert(layer->name(), value);
    }
    const auto version = owner->operations().snapshot();
    QScopedPointer<KisDocument> copied;
    bool resized = false;
    bool metadataReadyOnResize = false;
    if (operation == QLatin1String("snapshot")) copied.reset(document->lockAndCreateSnapshot());
    else if (operation == QLatin1String("clone")) copied.reset(document->cloneWithImageReadLock());
    else {
        copied.reset(KisPart::instance()->createDocument());
        initializeGpuDocument(copied.data(), {4, 4}, QStringLiteral("Replaced metadata"));
        const auto target = copied->image().toStrongRef();
        connect(target.data(), &KisImage::sigSizeChanged, copied.data(), [&] {
            resized = true;
            auto *layer = qobject_cast<KisLayer *>(copied->image()->root()->firstChild().data());
            metadataReadyOnResize = layer && layer->metaData()->containsEntry(schema, QStringLiteral("Make"));
        });
        copied->copyFromDocument(*document);
    }
    QVERIFY(copied && copied->image()->gpuImage());
    if (operation == QLatin1String("replace")) {
        QVERIFY(resized);
        QVERIFY(metadataReadyOnResize);
    }
    KisNodeList copiedNodes{copied->image()->root()};
    for (qsizetype i = 0; i < copiedNodes.size(); ++i)
        for (auto child = copiedNodes[i]->firstChild(); child; child = child->nextSibling()) copiedNodes.append(child);
    QCOMPARE(copiedNodes.size(), sourceNodes.size());
    for (qsizetype i = 0; i < copiedNodes.size(); ++i) {
        auto *layer = qobject_cast<KisLayer *>(copiedNodes[i].data());
        QVERIFY(layer);
        QVERIFY2(layer->metaData()->containsEntry(schema, QStringLiteral("Make")), qPrintable(layer->name()));
        QCOMPARE(layer->metaData()->getValue(uri, QStringLiteral("Make")).asVariant().toString(), expected.value(layer->name()));
    }
    for (qsizetype i = 0; i < sourceNodes.size(); ++i) {
        auto *layer = qobject_cast<KisLayer *>(sourceNodes[i].data());
        layer->metaData()->removeEntry(schema, QStringLiteral("Make"));
        QVERIFY(layer->metaData()->addEntry(KisMetaData::Entry(schema, QStringLiteral("Make"), KisMetaData::Value(QVariant(QStringLiteral("Changed source"))))));
    }
    for (qsizetype i = 0; i < copiedNodes.size(); ++i) {
        auto *layer = qobject_cast<KisLayer *>(copiedNodes[i].data());
        QCOMPARE(layer->metaData()->getValue(uri, QStringLiteral("Make")).asVariant().toString(), expected.value(layer->name()));
    }
    QCOMPARE(owner->operations().snapshot(), version);
}

void TestDocument::testGpuRasterExportKeepsLayerImageMetadata_data()
{
    QTest::addColumn<QByteArray>("mime");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("menuExport");
    for (bool menu : {false, true}) {
        const QByteArray prefix = menu ? "menu-" : "api-";
        const QByteArray pngRow = prefix + "png-metadata";
        const QByteArray jpegRow = prefix + "jpeg-metadata";
        QTest::newRow(pngRow.constData()) << QByteArrayLiteral("image/png") << QStringLiteral("png") << menu;
        QTest::newRow(jpegRow.constData()) << QByteArrayLiteral("image/jpeg") << QStringLiteral("jpg") << menu;
    }
}

void TestDocument::testGpuRasterExportKeepsLayerImageMetadata()
{
    QFETCH(QByteArray, mime);
    QFETCH(QString, extension);
    QFETCH(bool, menuExport);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto retainFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Layer metadata export artifacts retained at" << directory.path();
        }
    });
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    initializeGpuDocument(document.data(), {16, 8}, QStringLiteral("Metadata export"));
    KisImageSP image = document->image().toStrongRef();
    const auto owner = image->gpuImage();
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), image->bounds(), QByteArray::fromHex("112233ff").repeated(128)),
             LibrePaint::Image::Gpu::Result::Success);
    KisImageSP referenceImage = new KisImage(reference->createUndoStore(), 16, 8, image->colorSpace(), QStringLiteral("Metadata reference"));
    KisPaintLayerSP referenceLayer = new KisPaintLayer(referenceImage, QStringLiteral("Metadata layer"), 255);
    referenceLayer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(QByteArray::fromHex("332211ff").repeated(128).constData()), 0, 0, 16, 8);
    referenceImage->addNode(referenceLayer);
    referenceImage->initialRefreshGraph();
    referenceImage->waitForDone();
    reference->setCurrentImage(referenceImage);
    document->setFileBatchMode(true);
    reference->setFileBatchMode(true);
    const QString uri = QStringLiteral("http://ns.adobe.com/tiff/1.0/");
    const auto schema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(uri);
    QVERIFY(schema);
    auto *sourceLayer = qobject_cast<KisLayer *>(image->root()->firstChild().data());
    QVERIFY(sourceLayer);
    for (auto *layer : {sourceLayer, static_cast<KisLayer *>(referenceLayer.data())})
        QVERIFY(layer->metaData()->addEntry(KisMetaData::Entry(schema, QStringLiteral("Make"), KisMetaData::Value(QVariant(QStringLiteral("Fixture camera"))))));
    for (auto *item : {document.data(), reference.data()}) {
        item->documentInfo()->setAboutInfo(QStringLiteral("date"), QStringLiteral("2001-02-03T04:05:06Z"));
        item->documentInfo()->setAboutInfo(QStringLiteral("creation-date"), QStringLiteral("2000-01-02T03:04:05Z"));
    }
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("alpha", true);
    settings->setProperty("indexed", false);
    settings->setProperty("interlaced", false);
    settings->setProperty("compression", 3);
    settings->setProperty("saveSRGBProfile", true);
    settings->setProperty("forceSRGB", false);
    settings->setProperty("storeMetaData", true);
    settings->setProperty("storeAuthor", false);
    settings->setProperty("exif", true);
    settings->setProperty("iptc", true);
    settings->setProperty("xmp", true);
    const auto expectedPath = directory.filePath(QStringLiteral("reference.") + extension);
    QVERIFY2(reference->exportDocumentSync(expectedPath, mime, settings), qPrintable(reference->errorMessage()));
    QFile expectedFile(expectedPath);
    QVERIFY(expectedFile.open(QIODevice::ReadOnly));
    const auto expected = expectedFile.readAll();
    referenceLayer->metaData()->removeEntry(schema, QStringLiteral("Make"));
    const auto withoutPath = directory.filePath(QStringLiteral("without-metadata.") + extension);
    QVERIFY(reference->exportDocumentSync(withoutPath, mime, settings));
    QFile withoutFile(withoutPath);
    QVERIFY(withoutFile.open(QIODevice::ReadOnly));
    QVERIFY2(expected != withoutFile.readAll(), "The CPU export must actually encode the populated layer metadata");
    const auto version = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    const auto path = directory.filePath(QStringLiteral("export.") + extension);
    Document api(document.data(), false);
    InfoObject publicSettings(settings);
    const bool success = menuExport ? document->exportDocument(path, mime, false, false, settings) : api.exportImage(path, publicSettings);
    QVERIFY2(success, qPrintable(document->errorMessage()));
    QFile output(path);
    QVERIFY(output.open(QIODevice::ReadOnly));
    const auto actual = output.readAll();
    QCOMPARE(actual, expected);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const QString prefix = QStringLiteral("layer-image-") + QString::fromLatin1(QTest::currentDataTag());
        for (const auto &entry : {qMakePair(QStringLiteral("-cpu."), expected), qMakePair(QStringLiteral("-gpu."), actual)}) {
            QSaveFile artifact(QDir(results).filePath(prefix + entry.first + extension));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(entry.second), entry.second.size());
            QVERIFY(artifact.commit());
        }
    }
    QCOMPARE(sourceLayer->metaData()->getValue(uri, QStringLiteral("Make")).asVariant().toString(), QStringLiteral("Fixture camera"));
    QCOMPARE(owner->operations().snapshot(), version);
    QCOMPARE(owner->operations().undoCount(), history);
}

void TestDocument::testThumbnail()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    d.refreshProjection();

    QImage thumb = d.thumbnail(10, 10);
    thumb.save("thumb.png");
    QVERIFY(thumb.width() == 10);
    QVERIFY(thumb.height() == 10);
    // Our thumbnail calculator in KisPaintDevice cannot make a filled 10x10 thumbnail from a 100x100 device,
    // it makes it 10x10 empty, then puts 8x8 pixels in there... Not a bug in the Node class
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            QVERIFY(thumb.pixelColor(i, j) == QColor(Qt::red));
        }
    }
    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateFillLayer()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 50, 50, KoColorSpaceRegistry::instance()->rgb16(), "test");
    kisdoc->setCurrentImage(image);
    Document d(kisdoc.data(), false);

    const QString pattern("pattern");
    const QString color("color");
    const QString filllayer = "filllayer";
    InfoObject info;
    Selection sel(image->globalSelection());

    FillLayer *f = d.createFillLayer("test1", pattern, info, sel);
    QVERIFY(f->generatorName() == pattern);
    QVERIFY(f->type() == filllayer);
    delete f;
    f = d.createFillLayer("test1", color, info, sel);
    QVERIFY(f->generatorName() == color);
    QVERIFY(f->type() == filllayer);

    info.setProperty(pattern, "Cross01.pat");
    QVERIFY(f->setGenerator(pattern, &info));
    QVERIFY(f->filterConfig()->property(pattern).toString() == "Cross01.pat");
    QVERIFY(f->generatorName() == pattern);
    QVERIFY(f->type() == filllayer);

    info.setProperty(color, QColor(Qt::red));
    QVERIFY(f->setGenerator(color, &info));
    QVariant v = f->filterConfig()->property(color);
    QColor c = v.value<QColor>();
    QVERIFY(c == QColor(Qt::red));
    QVERIFY(f->generatorName() == color);
    QVERIFY(f->type() == filllayer);

    bool r = f->setGenerator(QString("xxx"), &info);
    QVERIFY(!r);

    delete f;

    QVERIFY(d.createFillLayer("test1", "xxx", info, sel) == 0);

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateCloneLayer()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 50, 50, KoColorSpaceRegistry::instance()->rgb16(), "test");
    kisdoc->setCurrentImage(image);
    Document d(kisdoc.data(), false);

    const QString layerType = "clonelayer";
    const QString node1Name = "node1";
    const QString node2Name = "node2";

    Node *node1 = d.createNode(node1Name,"paintlayer");
    Node *node2 = d.createNode(node2Name,"paintlayer");
    CloneLayer *layer = d.createCloneLayer("test1", node1);
    Node* rootNode = d.rootNode();

    rootNode->addChildNode(node1,0);
    rootNode->addChildNode(node2,0);
    rootNode->addChildNode(layer,0);

    QVERIFY(layer->type() == layerType);

    Node *sourceNode1 = layer->sourceNode();

    QVERIFY(sourceNode1->name() == node1Name);

    layer->setSourceNode(node2);

    Node *sourceNode2 = layer->sourceNode();

    QVERIFY(sourceNode2->name() == node2Name);

    delete layer;
    delete node1;
    delete node2;
    delete sourceNode1;
    delete sourceNode2;
    delete rootNode;

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateTransparencyMask()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 50, 50, KoColorSpaceRegistry::instance()->rgb16(), "test");
    kisdoc->setCurrentImage(image);
    Document d(kisdoc.data(), false);

    const QString layerType = "transparencymask";

    Selection sel(image->globalSelection());

    sel.select(10,10,10,10,255);

    Node *node = d.createNode("node1","paintlayer");
    TransparencyMask *mask = d.createTransparencyMask("test1");
    Node* rootNode = d.rootNode();

    rootNode->addChildNode(node,0);
    node->addChildNode(mask,0);

    Selection* selResult = mask->selection();

    QVERIFY(mask->type() == layerType);
    QVERIFY(selResult->width() == sel.width() && selResult->height() == sel.height());

    delete selResult;
    delete mask;
    delete node;
    delete rootNode;

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}

void TestDocument::testCreateColorizeMask()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(new KisSurrogateUndoStore(),  10, 3, KoColorSpaceRegistry::instance()->rgb8(), "test");

    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);
    const QString layerType = "colorizemask";

    Node *node = d.createNode("node1","paintlayer");

    ColorizeMask *mask = d.createColorizeMask("test1");
    Node* rootNode = d.rootNode();

    QByteArray nodeData = QByteArray::fromBase64("AAAAAAAAAAAAAAAAEQYMBhEGDP8RBgz/EQYMAgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARBgz5EQYM/xEGDAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEQYMAhEGDAkRBgwCAAAAAAAAAAAAAAAA");

    node->setPixelData(nodeData,0,0,10,3);
    rootNode->addChildNode(node,0);

    d.waitForDone();
    d.refreshProjection();

    node->addChildNode(mask,0);

    qApp->processEvents();
    d.waitForDone();

    ManagedColor col1("RGBA","U8","");
    ManagedColor col2("RGBA","U8","");

    col1.setComponents({1.0, 0.0, 0.0, 1.0});
    col2.setComponents({0.0, 0.0, 1.0, 1.0});

    QVERIFY(mask->type() == layerType);

    mask->setEditKeyStrokes(true);
    QVERIFY(mask->editKeyStrokes() == true);

    mask->setUseEdgeDetection(true);
    QVERIFY(mask->useEdgeDetection() == true);

    mask->setEdgeDetectionSize(4.0);
    QVERIFY(mask->edgeDetectionSize() == 4.0);

    mask->setCleanUpAmount(70.0);
    QVERIFY(mask->cleanUpAmount() == 70.0);

    mask->setLimitToDeviceBounds(true);
    QVERIFY(mask->limitToDeviceBounds() == true);

    mask->initializeKeyStrokeColors({&col1, &col2});

    QByteArray pdata1 = QByteArray::fromBase64("//8AAAAAAAAAAP8AAAAAAAAAAAAAAAAAAAAAAAAA");
    QByteArray pdata2 = QByteArray::fromBase64("AAAAAAAAAAD//wAAAAAAAAAAAP8AAAAAAAAAAAAA");

    mask->setKeyStrokePixelData(pdata1,&col1,0,0,10,3);
    mask->setKeyStrokePixelData(pdata2,&col2,0,0,10,3);

    QList<ManagedColor *> checkColors(mask->keyStrokesColors());

    QVERIFY(checkColors.size() == 2);
    QVERIFY(col1.toQString() == checkColors[0]->toQString());
    QVERIFY(col2.toQString() == checkColors[1]->toQString());

    QVERIFY(mask->keyStrokePixelData(&col1,0,0,10,3) == pdata1);
    QVERIFY(mask->keyStrokePixelData(&col2,0,0,10,3) == pdata2);

    delete checkColors[0];
    delete checkColors[1];

    mask->updateMask(true);
    mask->setEditKeyStrokes(false);
    mask->setShowOutput(true);

    d.waitForDone();
    d.refreshProjection();

    QByteArray pdata = d.pixelData(0,0,10,3);

    QVERIFY(d.pixelData(3,2,1,1).toBase64() == "/wAA/w==");
    QVERIFY(d.pixelData(6,2,1,1).toBase64() == "AAD9/w==");

    mask->removeKeyStroke(&col2);
    qApp->processEvents();
    d.waitForDone();

    checkColors = mask->keyStrokesColors();

    QVERIFY(checkColors.size() == 1);
    QVERIFY(col1.toQString() == checkColors[0]->toQString());

    delete checkColors[0];
    delete mask;
    delete node;
    delete rootNode;

    KisPart::instance()->removeDocument(kisdoc.data(), false);
}



void TestDocument::testAnnotations()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QVERIFY(d.annotationTypes().isEmpty());

    QBuffer buf;
    buf.open(QBuffer::WriteOnly);
    QTextStream in(&buf);
    KisPortingUtils::setUtf8OnStream(in);
    in << "AnnotationTest";
    buf.close();

    d.setAnnotation("test", "description", buf.data());

    QVERIFY(d.annotationTypes().size() == 1);
    QVERIFY(d.annotationTypes().contains("test"));
    QVERIFY(d.annotation("test").toHex() == buf.data().toHex());
    QVERIFY(d.annotationDescription("test") == "description");

    d.saveAs("roundtriptest.kra");

    d.removeAnnotation("test");
    QVERIFY(d.annotationTypes().isEmpty());

    kisdoc.take();
    layer.clear();
    image.clear();
    QVERIFY(d.close());
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    Krita *krita = Krita::instance();
    const bool previousBatchMode = krita->batchmode();
    const auto restoreBatchMode = qScopeGuard([&] { krita->setBatchmode(previousBatchMode); });
    krita->setBatchmode(true);
    Document *d2 = krita->openDocument("roundtriptest.kra");
    QVERIFY(d2);

    QVERIFY(d2->annotationTypes().size() == 1);
    QVERIFY(d2->annotationTypes().contains("test"));
    QVERIFY(d2->annotation("test").toHex() == buf.data().toHex());
    QVERIFY(d2->annotationDescription("test") == "description");

    d2->close();
}

void TestDocument::testNodeByName()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QVERIFY(d.nodeByName("test1")->name() == layer->name());
    QVERIFY(d.nodeByName("test2") == 0);
}

void TestDocument::testNodeByUniqueId()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    image->addNode(layer);
    kisdoc->setCurrentImage(image);

    Document d(kisdoc.data(), false);

    QVERIFY(d.nodeByUniqueID(layer->uuid())->name() == layer->name());

    QUuid test(QUuid::createUuid());

    while (test == layer->uuid()) {
        test = QUuid::createUuid();
    }
    QVERIFY(d.nodeByUniqueID(test) == 0);
}

class KisTransformMaskAdapter;

KISTEST_MAIN(TestDocument)


void TestDocument::testGpuSessionOpensCpuAuthoredU16NativeDocument()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    auto *registry = KoColorSpaceRegistry::instance();
    const auto *cs = registry->rgb16();
    QVERIFY(cs && cs->profile());
    const QByteArray profile = cs->profile()->rawData();
    const quint16 background[] = {0x1234, 0x5678, 0x9abc, 0xffff};
    const quint16 authored[] = {0x0102, 0x0304, 0x7f80, 0xffff};
    const quint16 changed[] = {0x1111, 0x2222, 0x3333, 0xffff};
    const auto bytes = [](const quint16 *values, qsizetype count) {
        return QByteArray(reinterpret_cast<const char *>(values), count * qsizetype(sizeof(quint16)));
    };
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto authoredPath = directory.filePath(QStringLiteral("cpu-authored-u16.kra"));
    {
        // The CPU engine writes the reference file; the GPU session only reads it.
        qputenv("LIBREPAINT_GPU_ENGINE", "0");
        QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
        source->setFileBatchMode(true);
        QVERIFY2(source->newImage(QStringLiteral("CPU authored U16"), 16, 8, cs,
                                  KoColor(reinterpret_cast<const quint8 *>(background), cs),
                                  KisConfig::CANVAS_COLOR, 1, {}, 1), qPrintable(source->errorMessage()));
        KisNodeSP layer = KisImageSP(source->image())->root()->firstChild();
        QVERIFY(layer);
        QScopedPointer<Node> node(Node::createNode(source->image(), layer));
        QVERIFY(node->setPixelData(bytes(authored, 4), 3, 2, 1, 1));
        QVERIFY2(source->saveAs(authoredPath, KisDocument::nativeFormatMimeType(), false),
                 qPrintable(source->errorMessage()));
        source->waitForSavingToComplete();
    }
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY2(document->openPath(authoredPath, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    KisImageSP image = document->image();
    QVERIFY(image->gpuImage());
    QCOMPARE(image->colorSpace()->colorModelId().id(), QStringLiteral("RGBA"));
    QCOMPARE(image->colorSpace()->colorDepthId().id(), QStringLiteral("U16"));
    QCOMPARE(image->profile()->rawData(), profile);
    const auto opened = image->gpuImage()->operations().snapshot();
    QCOMPARE(opened->format, Gpu::PixelFormat::Rgba16);
    QCOMPARE(opened->colorProfile, profile);
    Document api(document.data(), false);
    QCOMPARE(api.pixelData(3, 2, 1, 1), bytes(authored, 4));
    {
        // The GPU document owner records one history entry, as the painting
        // tools do. A public Node pixel write is a raw write without history.
        const auto owner = image->gpuImage();
        const auto layer = owner->operations().activeLayer();
        const QByteArray changedRgba = Gpu::rgbaBytes({0x3333, 0x2222, 0x1111, 0xffff},
                                                      Gpu::PixelFormat::Rgba16);
        QCOMPARE(owner->replaceRgbaPixels(layer, QRect(3, 2, 1, 1), changedRgba), Gpu::Result::Success);
        QCOMPARE(api.pixelData(3, 2, 1, 1), bytes(changed, 4));
        document->undoStack()->undo();
        QCOMPARE(api.pixelData(3, 2, 1, 1), bytes(authored, 4));
        document->undoStack()->redo();
        QCOMPARE(api.pixelData(3, 2, 1, 1), bytes(changed, 4));
    }
    const auto savedPath = directory.filePath(QStringLiteral("edited-u16.kra"));
    QVERIFY2(document->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false),
             qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(savedPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QVERIFY(reopened->image()->gpuImage());
    QCOMPARE(reopened->image()->colorSpace()->colorModelId().id(), QStringLiteral("RGBA"));
    QCOMPARE(reopened->image()->colorSpace()->colorDepthId().id(), QStringLiteral("U16"));
    QCOMPARE(reopened->image()->profile()->rawData(), profile);
    Document reopenedApi(reopened.data(), false);
    QCOMPARE(reopenedApi.pixelData(3, 2, 1, 1), bytes(changed, 4));
    QScopedPointer<Node> edited(Node::createNode(reopened->image(), reopened->image()->root()->firstChild()));
    QVERIFY(edited->setPixelData(bytes(authored, 4), 3, 2, 1, 1));
    QCOMPARE(reopenedApi.pixelData(3, 2, 1, 1), bytes(authored, 4));
}

void TestDocument::testGpuU16PublicPixelsPreserveDocumentEncoding()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto cs = KoColorSpaceRegistry::instance()->rgb16();
    const quint16 background[] = {0x1234, 0x5678, 0x9abc, 0xffff};
    const KoColor color(reinterpret_cast<const quint8 *>(background), cs);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QVERIFY2(document->newImage(QStringLiteral("U16 public pixels"), 32, 16, cs, color,
                                KisConfig::CANVAS_COLOR, 1, {}, 1), qPrintable(document->errorMessage()));
    auto image = document->image();
    QVERIFY(image->gpuImage());
    QCOMPARE(image->colorSpace(), cs);
    QCOMPARE(image->gpuImage()->operations().snapshot()->format, Gpu::PixelFormat::Rgba16);
    QCOMPARE(image->projection()->defaultPixel(), color);
    QCOMPARE(image->defaultProjectionColor(), color);
    const auto preview = image->convertToQImage(image->size(), KoColorSpaceRegistry::instance()->rgb8()->profile());
    QCOMPARE(preview.pixelColor(4, 4), color.toQColor());
    Document publicDocument(document.data(), false);
    QScopedPointer<Node> root(publicDocument.rootNode());
    NodeSP node(root->childNodes().front());
    auto layer = image->root()->firstChild()->paintDevice();
    const quint16 input[] = {0x1357, 0x2468, 0xabcd, 0};
    const QByteArray bgra(reinterpret_cast<const char *>(input), sizeof(input));
    QVERIFY(node->setPixelData(bgra, -1, 2, 1, 1));
    QByteArray actual(8, Qt::Uninitialized);
    layer->readBytes(reinterpret_cast<quint8 *>(actual.data()), QRect(-1, 2, 1, 1));
    QCOMPARE(actual, bgra);
    QCOMPARE(node->pixelData(-1, 2, 1, 1), bgra);
    const auto owner = image->gpuImage();
    const auto id = owner->operations().activeLayer();
    const auto canonical = Gpu::rgbaBytes({0x1111, 0x2222, 0x3333, 0xffff}, Gpu::PixelFormat::Rgba16);
    QCOMPARE(owner->replaceRgbaPixels(id, QRect(-1, 2, 1, 1), canonical), Gpu::Result::Success);
    document->undoStack()->undo();
    layer->readBytes(reinterpret_cast<quint8 *>(actual.data()), QRect(-1, 2, 1, 1));
    QCOMPARE(actual, bgra);
    QCOMPARE(node->pixelData(-1, 2, 1, 1), bgra);
    document->undoStack()->redo();
    const quint16 redone[] = {0x3333, 0x2222, 0x1111, 0xffff};
    layer->readBytes(reinterpret_cast<quint8 *>(actual.data()), QRect(-1, 2, 1, 1));
    QCOMPARE(actual, QByteArray(reinterpret_cast<const char *>(redone), sizeof(redone)));
    KisPaintDeviceSP reference = new KisPaintDevice(cs);
    reference->writeBytes(reinterpret_cast<const quint8 *>(redone), QRect(-1, 2, 1, 1));
    for (const qreal oversample : {qreal(1), qreal(2)})
        QCOMPARE(layer->createThumbnail(16, 16, KisThumbnailBoundsMode::Precise, oversample),
                 reference->createThumbnail(16, 16, KisThumbnailBoundsMode::Precise, oversample));
    const auto fixed = owner->operations().snapshot();
    const auto fixedPreview = image->convertToQImage({48, 24}, KoColorSpaceRegistry::instance()->rgb8()->profile(), fixed, cs);
    const qsizetype minimumBudget = 8 * 1024 * 1024 + 32 * 8 * (2 * 16 + 16);
    QVERIFY_EXCEPTION_THROWN(KisGpuExportRasterRows(image, minimumBudget - 1), std::length_error);
    {
        KisGpuExportRasterRows rows(image, minimumBudget);
        QCOMPARE(owner->writeProjectionBackground({65535, 65535, 65535, 65535}), Gpu::Result::Success);
        QByteArray row;
        QVERIFY(rows.rows()->readRow(4, row).isOk());
        QCOMPARE(row, QByteArray(reinterpret_cast<const char *>(background), sizeof(background)).repeated(32));
        QCOMPARE(image->convertToQImage({48, 24}, KoColorSpaceRegistry::instance()->rgb8()->profile(), fixed, cs), fixedPreview);
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QScopedPointer<KisDocument> empty(KisPart::instance()->createDocument());
    empty->setFileBatchMode(true);
    QVERIFY(empty->newImage(QStringLiteral("Empty U16"), 32, 16, cs, KoColor(Qt::transparent, cs),
                            KisConfig::CANVAS_COLOR, 1, {}, 1));
    const auto path = directory.filePath(QStringLiteral("empty.kra"));
    QVERIFY2(empty->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(empty->errorMessage()));
    empty->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QCOMPARE(reopened->image()->colorSpace(), cs);

}

void TestDocument::testGpuImageColorConversionKeepsHistoryEncoding_data()
{
    QTest::addColumn<QByteArray>("sourceIcc");
    QTest::newRow("linear") << KoColorSpaceRegistry::instance()->rgb16()->profile()->rawData();
    QTest::newRow("nonlinear") << QColorSpace(QColorSpace::SRgb).iccProfile();
    QTest::newRow("different-primaries") << QColorSpace(QColorSpace::DisplayP3).iccProfile();
}

void TestDocument::testGpuImageColorConversionKeepsHistoryEncoding()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QFETCH(QByteArray, sourceIcc);
    auto *registry = KoColorSpaceRegistry::instance();
    const auto *sourceProfile = registry->createColorProfileFromData(
        QStringLiteral("RGBA"), QStringLiteral("U16"), sourceIcc);
    QVERIFY(sourceProfile);
    const auto *source = registry->colorSpace(QStringLiteral("RGBA"), QStringLiteral("U16"), sourceProfile);
    QVERIFY(source);
    const auto destination = KoColorSpaceRegistry::instance()->rgb8();
    const quint16 background[] = {0x1234, 0x5678, 0x9abc, 0xffff};
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QVERIFY(document->newImage(QStringLiteral("Color conversion"), 32, 16, source,
        KoColor(reinterpret_cast<const quint8 *>(background), source), KisConfig::RASTER_LAYER, 2, {}, 1));
    KisImageSP image = document->image();
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    QCOMPARE(owner->writeProjectionBackground({0x2468, 0x1357, 0xabcd, 0x9876}), Gpu::Result::Success);
    Document publicDocument(document.data(), false);
    QScopedPointer<Node> root(publicDocument.rootNode());
    NodeSP node(root->childNodes().back());
    const quint16 components[] = {0x1357, 0x2468, 0xabcd, 0x9876};
    const QByteArray input(reinterpret_cast<const char *>(components), sizeof(components));
    QVERIFY(node->setPixelData(input, -1, 2, 1, 1));
    QScopedPointer<Node> group(publicDocument.createNode(QStringLiteral("Group"), QStringLiteral("grouplayer")));
    QVERIFY(group);
    QVERIFY(root->addChildNode(group.data(), nullptr));
    const auto leafUuid = node->uniqueId();
    KisNodeSP nativeLeaf;
    for (auto child = image->root()->firstChild(); child; child = child->nextSibling())
        if (child->uuid() == leafUuid) nativeLeaf = child;
    QVERIFY(nativeLeaf);
    Gpu::LayerId leafId = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.uuid == leafUuid) leafId = layer.id;
    QVERIFY(leafId);
    QCOMPARE(owner->moveLayers({leafId}, group->uniqueId(), {}), Gpu::Result::Success);
    QCOMPARE(owner->setSelectionRect(QRect(2, 3, 7, 5)), Gpu::Result::Success);
    const auto fixed = owner->operations().snapshot();
    const auto retainedDevice = nativeLeaf->paintDevice();
    const auto oldBackground = image->defaultProjectionColor();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto livePath = directory.filePath(QStringLiteral("live.kra"));
    document->setFileBatchMode(true);
    QVERIFY2(document->saveAs(livePath, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    const int history = document->undoStack()->count();
    QByteArray expected(4, Qt::Uninitialized);
    const auto intent = KoColorConversionTransformation::IntentPerceptual;
    const auto flags = KoColorConversionTransformation::HighQuality | KoColorConversionTransformation::NoOptimization;
    source->convertPixelsTo(reinterpret_cast<const quint8 *>(input.constData()),
        reinterpret_cast<quint8 *>(expected.data()), destination, 1, intent, flags);
    QVERIFY(publicDocument.setColorSpace(destination->colorModelId().id(), destination->colorDepthId().id(), destination->profile()->name()));
    QCOMPARE(owner->operations().snapshot()->format, Gpu::PixelFormat::Rgba8);
    QCOMPARE(image->colorSpace(), destination);
    const auto converted = owner->operations().snapshot();
    QCOMPARE(converted->layers.size(), fixed->layers.size());
    QCOMPARE(converted->selection, fixed->selection);
    QCOMPARE(converted->selectedLayers, fixed->selectedLayers);
    QCOMPARE(converted->activeLayer, fixed->activeLayer);
    for (qsizetype i = 0; i < fixed->layers.size(); ++i) {
        QCOMPARE(converted->layers[i].id, fixed->layers[i].id);
        QCOMPARE(converted->layers[i].uuid, fixed->layers[i].uuid);
        QCOMPARE(converted->layers[i].parentUuid, fixed->layers[i].parentUuid);
        QCOMPARE(converted->layers[i].position, fixed->layers[i].position);
        if (fixed->layers[i].kind == Gpu::LayerKind::Paint) {
            const auto &rgba = fixed->layers[i].defaultPixel;
            const quint16 nativeDefault[] = {quint16(rgba[2]), quint16(rgba[1]), quint16(rgba[0]), quint16(rgba[3])};
            quint8 expectedDefault[4];
            QVERIFY(source->convertPixelsTo(reinterpret_cast<const quint8 *>(nativeDefault), expectedDefault, destination, 1, intent, flags));
            const Gpu::RgbaPixel expectedRgba{expectedDefault[2], expectedDefault[1], expectedDefault[0], expectedDefault[3]};
            QCOMPARE(converted->layers[i].defaultPixel, expectedRgba);
        }
    }
    auto expectedBackground = oldBackground;
    expectedBackground.convertTo(destination, intent, flags);
    QCOMPARE(image->defaultProjectionColor(), expectedBackground);
    QCOMPARE(converted->colorProfile, destination->profile()->rawData());
    QCOMPARE(fixed->colorProfile, source->profile()->rawData());
    QCOMPARE(retainedDevice->colorSpace(), destination);
    QCOMPARE(node->pixelData(-1, 2, 1, 1), expected);
    QCOMPARE(document->undoStack()->count(), history + 1);
    const QByteArray edited = QByteArray::fromHex("21436587");
    QVERIFY(node->setPixelData(edited, -1, 2, 1, 1));
    document->undoStack()->undo();
    QCOMPARE(image->colorSpace(), source);
    QCOMPARE(owner->operations().snapshot()->format, Gpu::PixelFormat::Rgba16);
    QCOMPARE(node->pixelData(-1, 2, 1, 1), input);
    QCOMPARE(fixed->format, Gpu::PixelFormat::Rgba16);
    document->undoStack()->redo();
    QCOMPARE(image->colorSpace(), destination);
    QCOMPARE(node->pixelData(-1, 2, 1, 1), edited);
    QVERIFY2(document->saveAs(livePath, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> liveReopened(KisPart::instance()->createDocument());
    liveReopened->setFileBatchMode(true);
    QVERIFY2(liveReopened->openPath(livePath, KisDocument::DontAddToRecent), qPrintable(liveReopened->errorMessage()));
    QCOMPARE(liveReopened->image()->colorSpace(), destination);
    Document publicLive(liveReopened.data(), false);
    QScopedPointer<Node> liveLeaf(publicLive.nodeByUniqueID(leafUuid));
    QVERIFY(liveLeaf);
    QCOMPARE(liveLeaf->uniqueId(), leafUuid);
    QCOMPARE(liveLeaf->pixelData(-1, 2, 1, 1), edited);
    const auto path = directory.filePath(QStringLiteral("old-version.kra"));
    const auto preview = image->convertToQImage({256, 256}, destination->profile(), fixed, source);
    LibrePaint::Document::Gpu::PaintDocument files(owner);
    LibrePaint::ImportExport::Kra::ImageMetadata metadata;
    metadata.iccProfile = destination->profile()->rawData();
    metadata.profileName = destination->profile()->name();
    QCOMPARE(files.setKraImageMetadata(metadata).code, LibrePaint::Document::Gpu::FileCode::Success);
    const auto saved = files.exportKra(path, fixed, preview);
    QCOMPARE(saved.code, LibrePaint::Document::Gpu::FileCode::Success);
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QCOMPARE(reopened->image()->colorSpace(), source);
    QCOMPARE(reopened->image()->gpuImage()->operations().snapshot()->colorProfile, fixed->colorProfile);
    Document publicReopened(reopened.data(), false);
    QScopedPointer<Node> reopenedLeaf(publicReopened.nodeByUniqueID(leafUuid));
    QVERIFY(reopenedLeaf);
    QCOMPARE(reopenedLeaf->pixelData(-1, 2, 1, 1), input);

    // Cancellation occurs before the first timer tick and preserves the redo branch.
    document->undoStack()->undo();
    const auto beforeCancel = owner->operations().snapshot();
    const int redo = owner->operations().redoCount();
    image->convertImageColorSpace(destination, intent, flags);
    QVERIFY(!image->isIdle());
    QVERIFY(!image->tryBarrierLock());
    image->requestStrokeCancellation();
    image->waitForDone();
    QCOMPARE(owner->operations().snapshot(), beforeCancel);
    QCOMPARE(owner->operations().redoCount(), redo);
    QVERIFY(image->isIdle());
}
