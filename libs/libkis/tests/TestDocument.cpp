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
#include <QCheckBox>
#include <exception>
#include <future>
#include <chrono>
#include <cstring>
#include <limits>

#include <Node.h>
#include <Krita.h>
#include <Document.h>
#include <InfoObject.h>
#include <KisImportExportManager.h>
#include <KisGpuExportRasterRows.h>
#include <QImageReader>
#include <metadata/KoDocumentInfo.h>
#include <kra/KisKraRasterCodec.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>

#include <document/KisDocument.h>
#include <canvas/KisMirrorAxisConfig.h>
#include <kis_image.h>
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
void initializeGpuDocument(KisDocument *document, QSize size, const QString &name)
{
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    if (!execution) throw std::runtime_error(diagnostic.toStdString());
    auto owner = LibrePaint::Image::Gpu::PaintImageSP::create(execution, size);
    document->setCurrentImage(KisImage::fromGpuImage(owner, name));
}
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
        for (int ty = -1; ty <= 0; ++ty) for (int tx = -1; tx <= 0; ++tx) {
            auto tile = defaults[i].repeated(256 * 256);
            const QRect bounds(tx * 256, ty * 256, 256, 256);
            const auto area = bounds.intersected(sourceRegion);
            for (int y = area.top(); y <= area.bottom(); ++y) {
                const int source = ((y - sourceRegion.y()) * sourceRegion.width() + area.x() - sourceRegion.x()) * 4;
                const int destination = ((y - bounds.y()) * 256 + area.x() - bounds.x()) * 4;
                std::memcpy(tile.data() + destination, pixels.constData() + source, size_t(area.width()) * 4);
            }
            for (qsizetype offset = 0; offset < tile.size(); offset += 4) std::swap(tile[offset], tile[offset + 2]);
            input.layers[i].tiles.append({{tx, ty}, tile});
        }
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
        if (!gpu) QVERIFY(nodes[i]->setPixelData(pixels, sourceRegion.x(), sourceRegion.y(), sourceRegion.width(), sourceRegion.height()));
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
        const auto unchanged = owner->operations().recoveryData();
        const auto requested = execution->requested(owner->handle());
        const auto resize = [&](Document *caller, int x, int y, int w, int h) {
            if (crop) caller->crop(x, y, w, h);
            else caller->resizeImage(x, y, w, h);
        };
        QVERIFY_EXCEPTION_THROWN(resize(api.data(), std::numeric_limits<int>::max(), 0, 1, 1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(resize(api.data(), 0, 0, std::numeric_limits<int>::min(), 1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(resize(api.data(), 0, 0, 0, 1), std::invalid_argument);
        QCOMPARE(owner->operations().recoveryData(), unchanged);
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
    QTest::addColumn<int>("style");
    QTest::addColumn<int>("layerCount");
    QTest::newRow("raster-one-layer") << int(KisConfig::RASTER_LAYER) << 1;
    QTest::newRow("raster-two-layers") << int(KisConfig::RASTER_LAYER) << 2;
    QTest::newRow("canvas-one-layer") << int(KisConfig::CANVAS_COLOR) << 1;
    QTest::newRow("canvas-two-layers") << int(KisConfig::CANVAS_COLOR) << 2;
}

void TestDocument::testOrdinaryCreationPreservesInitialDocumentState()
{
    QFETCH(int, style);
    QFETCH(int, layerCount);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoColor color(QColor(17, 34, 51, 128), cs);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QVERIFY2(document->newImage(QStringLiteral("Ordinary document"), 32, 16, cs, color,
                                KisConfig::BackgroundStyle(style), layerCount, QStringLiteral("Kept description"), 2),
             qPrintable(document->errorMessage()));
    const auto image = KisImageSP(document->image());
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
    const auto recovery = owner->operations().recoveryData();
    QCOMPARE(owner->restoreRecovery(recovery), Gpu::Result::Success);
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
    const std::array<quint8, 4> expected{17, 34, 51, 128};
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
    KisMirrorAxisConfig loadedMirror;
    QVERIFY(loadedMirror.loadFromXml(imageDescription.firstChildElement(QStringLiteral("MirrorAxis"))));
    QVERIFY(loadedMirror == mirror);
    const auto folder = imageDescription.attribute(QStringLiteral("name"));
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
    QVERIFY(exportedDescription.documentElement().firstChildElement(QStringLiteral("IMAGE"))
        .firstChildElement(QStringLiteral("MirrorAxis")).isNull());
    QCOMPARE(exportedValue.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4),
             QByteArray::fromHex("445566ff"));
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
    QCOMPARE(exported.dotsPerMeterY(), 5905);
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
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
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
            const std::array<quint8, 4> edited{17, 34, 51, 128};
            const std::array<quint8, 4> original{};
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

