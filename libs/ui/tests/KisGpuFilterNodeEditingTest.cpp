/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuFilterNodeEditingTest.h"
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QDir>
#include <kis_default_bounds.h>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeView>
#include <QAbstractItemModel>
#include <KisGlobalResourcesInterface.h>
#include <KisGpuFilterConfiguration.h>
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <kis_config_widget.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <document/KisDocument.h>
#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <filter/kis_filter_registry.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <nodes/kis_node_manager.h>
#include <kis_adjustment_layer.h>
#include <kis_filter_mask.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_image_config.h>
#include <kis_node_filter_interface.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_undo_stores.h>
#include <kis_tool_utils.h>
#include <kis_transparency_mask.h>
#include <kis_resources_snapshot.h>
#include <algorithm>
#include <memory>
#include <functional>

namespace {
void exchangeRgbaBgra(QByteArray &bytes, int stride)
{
    for (qsizetype offset = 0; offset < bytes.size(); offset += stride)
        for (int b = 0; b < stride / 4; ++b) std::swap(bytes[offset + b], bytes[offset + stride / 2 + b]);
}
}

void FilterNodeEditingTest::addData()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("mask");
    QTest::addColumn<QString>("id");
    for (bool words : {false, true}) for (bool mask : {false, true})
        for (const auto *id : {"invert", "hsvadjustment", "maximize", "minimize", "fastcoloroverlay"}) {
            if (words && (QByteArray(id) == "invert" || QByteArray(id) == "hsvadjustment")) continue;
            const QByteArray row = QByteArray(words ? "word-" : "byte-") + (mask ? "mask-" : "adjustment-") + id;
            QTest::newRow(row.constData()) << words << mask << QString::fromLatin1(id);
        }
}

void FilterNodeEditingTest::addConvolutionData()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("mask");
    QTest::addColumn<QString>("id");
    for (bool words : {false, true}) for (bool mask : {false, true})
        for (const auto *id : {"sharpen", "mean removal", "emboss laplascian", "emboss all directions",
                             "emboss horizontal and vertical", "emboss vertical only", "emboss horizontal only"}) {
            const QByteArray row = QByteArray(words ? "word-" : "byte-") + (mask ? "mask-" : "adjustment-") + id;
            QTest::newRow(row.constData()) << words << mask << QString::fromLatin1(id);
        }
}

void FilterNodeEditingTest::addConvolutionChainData()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("defaultWhite");
    QTest::addColumn<bool>("sourceOutside");
    for (bool words : {false, true}) for (bool white : {false, true}) for (bool outside : {false, true}) {
        const QByteArray row = QByteArray(words ? "word-" : "byte-") + (white ? "default255" : "default0") +
            (outside ? "-outside" : "-canvas");
        QTest::newRow(row) << words << white << outside;
    }
}

void FilterNodeEditingTest::runConvolutionChain()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(bool, defaultWhite);
    QFETCH(bool, sourceOutside);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    KisImageConfig imageConfig(false);
    const auto threads = imageConfig.maxNumberOfThreads();
    const auto restore = qScopeGuard([&] {
        imageConfig.setMaxNumberOfThreads(threads);
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    imageConfig.setMaxNumberOfThreads(1);
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int stride = Gpu::pixelByteCount(format);
    const QRect canvas(0, 0, 270, 18), checked(-9, -7, 288, 32);
    const auto written = sourceOutside ? QRect(-5, -3, 280, 24) : canvas;
    KisImageSP native = new KisImage(new KisSurrogateUndoStore, canvas.width(), canvas.height(), cs, "Convolution chain");
    KisGroupLayerSP group = new KisGroupLayer(native, "Filtered group", 255); native->addNode(group);
    KisPaintLayerSP paint = new KisPaintLayer(native, "Source", 255); native->addNode(paint, group);
    if (sourceOutside) paint->paintDevice()->moveTo(3, -2);
    QByteArray raw;
    const quint16 a8[]{0, 1, 63, 127, 128, 192, 254, 255};
    const quint16 a16[]{0, 1, 127, 128, 129, 257, 32767, 65535};
    for (int y = 0; y < written.height(); ++y) for (int x = 0; x < written.width(); ++x) {
        const auto scale = words ? 257 : 1;
        raw += Gpu::rgbaBytes({quint16((19 + (y * 7 + x * 3) % 220) * scale),
            quint16((31 + (y * 5 + x * 11) % 200) * scale), quint16((13 + (y * 13 + x * 7) % 230) * scale),
            words ? a16[(x + y) % 8] : a8[(x + y) % 8]}, format);
    }
    auto bgra = raw; exchangeRgbaBgra(bgra, stride);
    paint->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), written);
    const auto config = [&](const char *id) {
        const auto filter = KisFilterRegistry::instance()->value(QString::fromLatin1(id));
        return filter ? filter->defaultConfiguration(KisGlobalResourcesInterface::instance())->cloneWithResourcesSnapshot() : KisFilterConfigurationSP();
    };
    const auto selection = [&](int seed, bool white) {
        KisSelectionSP result = new KisSelection;
        const quint8 coverage = white ? 255 : 0;
        result->pixelSelection()->setDefaultPixel(KoColor(&coverage, result->pixelSelection()->colorSpace()));
        QByteArray alpha(canvas.width() * canvas.height(), '\0');
        const quint8 levels[]{0, 1, 64, 127, 128, 192, 254, 255};
        for (qsizetype i = 0; i < alpha.size(); ++i) alpha[i] = char(levels[(i * 3 + i / canvas.width() + seed) % 8]);
        result->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), canvas);
        return result;
    };
    const auto addMask = [&](KisLayerSP parent, const char *id, int seed) {
        KisFilterMaskSP result = new KisFilterMask(native, QString::fromLatin1(id));
        result->initSelection(selection(seed, defaultWhite), parent);
        result->setFilter(config(id)); native->addNode(result, parent);
        return result;
    };
    addMask(paint, "sharpen", 1);
    KisTransparencyMaskSP transparency = new KisTransparencyMask(native, "Between filters");
    transparency->setSelection(selection(2, true)); native->addNode(transparency, paint);
    addMask(paint, "emboss laplascian", 3);
    auto groupFilter = addMask(group, "mean removal", 4);
    KisAdjustmentLayerSP adjustment = new KisAdjustmentLayer(native, "Top adjustment",
        config("emboss horizontal and vertical"), selection(5, defaultWhite));
    native->addNode(adjustment, native->root(), group);
    native->refreshGraphAsync(); native->waitForDone();
    const auto expected = [&](QRect region) {
        QByteArray bytes(region.width() * region.height() * stride, '\0');
        native->projection()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) { directory.setAutoRemove(false); qInfo() << "Convolution chain artifacts" << directory.path(); }
    });
    std::unique_ptr<KisDocument> nativeDocument(KisPart::instance()->createDocument());
    nativeDocument->setCurrentImage(native); nativeDocument->setFileBatchMode(true);
    const auto inputPath = directory.filePath("native-chain.kra");
    QVERIFY(nativeDocument->saveAs(inputPath, KisDocument::nativeFormatMimeType(), false));
    nativeDocument->waitForSavingToComplete();
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    std::unique_ptr<KisDocument> imported(KisPart::instance()->createDocument()); imported->setFileBatchMode(true);
    QVERIFY2(imported->openPath(inputPath, KisDocument::DontAddToRecent), qPrintable(imported->errorMessage()));
    const auto owner = imported->image()->gpuImage(); QVERIFY(owner);
    const auto compare = [&](const Gpu::PaintImageSP &value, QRect region, const char *stage) {
        const auto reference = expected(region);
        const auto actual = value->readRgbaPixels(0, region, reference.size());
        if (actual.bytes == reference) return true;
        qWarning() << stage << actual.diagnostic << "region" << region;
        for (const auto &entry : {qMakePair(QStringLiteral("native"), reference), qMakePair(QStringLiteral("gpu"), actual.bytes)}) {
            QFile file(directory.filePath(QString::fromLatin1(stage) + QLatin1Char('-') + entry.first + QStringLiteral(".rgba")));
            if (file.open(QIODevice::WriteOnly)) file.write(entry.second);
        }
        const auto wide = value->readRgbaPixels(0, checked, checked.width() * checked.height() * stride).bytes;
        const auto retried = value->readRgbaPixels(0, region, reference.size()).bytes;
        for (const auto &entry : {qMakePair(QStringLiteral("wide-native"), expected(checked)),
                                 qMakePair(QStringLiteral("wide-gpu"), wide),
                                 qMakePair(QStringLiteral("retried-gpu"), retried)}) {
            QFile file(directory.filePath(QString::fromLatin1(stage) + QLatin1Char('-') + entry.first + QStringLiteral(".rgba")));
            if (file.open(QIODevice::WriteOnly)) file.write(entry.second);
        }
        qWarning() << "Projection after wider read matches initial" << (retried == actual.bytes);
        if (value == owner && imported) {
            const KisNodeList referenceNodes{paint, group, adjustment};
            KisNodeList gpuNodes{imported->image()->root()};
            for (qsizetype i = 0; i < gpuNodes.size(); ++i)
                for (auto child = gpuNodes[i]->firstChild(); child; child = child->nextSibling()) gpuNodes.append(child);
            for (qsizetype i = 0; i < referenceNodes.size(); ++i) {
                KisNodeSP gpuNode;
                for (const auto &candidate : gpuNodes) if (candidate->uuid() == referenceNodes[i]->uuid()) gpuNode = candidate;
                if (!gpuNode) continue;
                const auto planeRegion = region.adjusted(-3, -3, 3, 3);
                qWarning() << "Diagnostic plane" << i << referenceNodes[i]->name()
                    << "Native default bounds" << referenceNodes[i]->projection()->defaultBounds()->bounds()
                    << "region" << planeRegion;
                for (bool gpu : {false, true}) for (bool original : {false, true}) {
                    QByteArray bytes(planeRegion.width() * planeRegion.height() * stride, '\0');
                    const auto node = gpu ? gpuNode : referenceNodes[i];
                    const auto pixels = original ? node->original() : node->projection();
                    if (!pixels) continue;
                    pixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), planeRegion);
                    exchangeRgbaBgra(bytes, stride);
                    QFile file(directory.filePath(QString::fromLatin1(stage) + QStringLiteral("-plane-") +
                        QString::number(i) + (original ? QStringLiteral("-original") : QStringLiteral("-projection")) +
                        (gpu ? QStringLiteral("-gpu.rgba") : QStringLiteral("-native.rgba"))));
                    if (file.open(QIODevice::WriteOnly)) file.write(bytes);
                }
            }
        }
        return false;
    };
    const QRect seam(254, 0, 5, 18);
    QVERIFY(compare(owner, seam, "seam-first"));
    QVERIFY(compare(owner, checked, "offcanvas"));
    QVERIFY(compare(owner, seam, "seam-after-expanded-read"));
    const auto before = owner->readRgbaPixels(0, checked, checked.width() * checked.height() * stride).bytes;
    Gpu::LayerId target = 0;
    for (const auto &node : owner->operations().snapshot()->layers) if (node.uuid == groupFilter->uuid()) target = node.id;
    QVERIFY(target);
    const auto updated = config("emboss all directions");
    const auto definition = freezeGpuFilterConfiguration(updated, cs); QVERIFY(definition);
    QCOMPARE(owner->setFilterDefinition(target, *definition, "Changed group filter"), Gpu::Result::Success);
    groupFilter->setFilter(updated); native->refreshGraphAsync(); native->waitForDone();
    QVERIFY(compare(owner, checked, "changed"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(0, checked, before.size()).bytes, before);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(compare(owner, seam, "redo-seam"));
    const auto saved = directory.filePath("gpu-chain.kra");
    QVERIFY(imported->saveAs(saved, KisDocument::nativeFormatMimeType(), false)); imported->waitForSavingToComplete();
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument()); reopened->setFileBatchMode(true);
    QVERIFY(reopened->openPath(saved, KisDocument::DontAddToRecent));
    const auto reopenedOwner = reopened->image()->gpuImage(); QVERIFY(reopenedOwner);
    imported.reset();
    QVERIFY(compare(reopenedOwner, checked, "reopened"));
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    std::unique_ptr<KisDocument> nativeReader(KisPart::instance()->createDocument()); nativeReader->setFileBatchMode(true);
    QVERIFY(nativeReader->openPath(saved, KisDocument::DontAddToRecent)); nativeReader->image()->waitForDone();
    QByteArray actual(checked.width() * checked.height() * stride, '\0');
    nativeReader->image()->projection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), checked);
    exchangeRgbaBgra(actual, stride);
    QCOMPARE(actual, expected(checked));
}

void FilterNodeEditingTest::run()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(bool, mask);
    QFETCH(QString, id);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto filter = KisFilterRegistry::instance()->value(id); QVERIFY(filter);
    auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    if (id == QLatin1String("hsvadjustment")) {
        config->setProperty("h", 31); config->setProperty("s", 17); config->setProperty("v", -13);
    } else if (id == QLatin1String("fastcoloroverlay")) {
        config->setProperty("color", QColor(62, 140, 236, 193)); config->setProperty("opacity", 63);
    }
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int stride = Gpu::pixelByteCount(format);
    const bool convolution = id == QLatin1String("sharpen") || id == QLatin1String("mean removal") ||
        id.startsWith(QLatin1String("emboss "));
    const QRect area(QPoint(), convolution ? QSize(270, 12) : QSize(16, 12));
    QByteArray raw;
    for (int y = 0; y < area.height(); ++y) for (int x = 0; x < area.width(); ++x) {
        const quint16 scale = words ? 257 : 1;
        raw += Gpu::rgbaBytes({quint16((17 + y * 9) * scale), quint16(91 * scale),
            quint16((203 - (convolution ? x % 27 : x) * 7) * scale), quint16((x % 4 ? 137 : 0) * scale)}, format);
    }
    QByteArray nativeRaw = raw; exchangeRgbaBgra(nativeRaw, stride);
    const quint8 levels[]{0, 1, 64, 127, 128, 192, 254, 255};
    QByteArray alpha(area.width() * area.height(), '\0');
    for (qsizetype i = 0; i < alpha.size(); ++i) alpha[i] = char(levels[i % 8]);
    KisSelectionSP selected = new KisSelection;
    selected->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), area);
    KisImageSP native = new KisImage(new KisSurrogateUndoStore, area.width(), area.height(), cs, "Filter node reference");
    KisPaintLayerSP nativePaint = new KisPaintLayer(native, "Source", 255);
    nativePaint->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(nativeRaw.constData()), area);
    native->addNode(nativePaint, native->root());
    KisNodeSP nativeNode;
    if (mask) {
        KisFilterMaskSP node = new KisFilterMask(native, "Filter");
        node->initSelection(selected, nativePaint);
        node->setFilter(config->cloneWithResourcesSnapshot());
        native->addNode(node, nativePaint);
        nativeNode = node;
    } else {
        nativeNode = new KisAdjustmentLayer(native, "Filter", config->cloneWithResourcesSnapshot(), selected);
        native->addNode(nativeNode, native->root(), nativePaint);
    }
    native->refreshGraphAsync(); native->waitForDone();
    const auto expected = [&] {
        QByteArray bytes(raw.size(), '\0');
        native->projection()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    int pixelComparison = 0;
    const auto pixelsEqual = [&](const QByteArray &actual, const QByteArray &reference) {
        ++pixelComparison;
        if (actual == reference) return true;
        for (qsizetype i = 0; i < qMin(actual.size(), reference.size()); ++i) if (actual[i] != reference[i]) {
            const auto offset = i / stride * stride;
            qWarning() << "Filter node comparison" << pixelComparison << "pixel" << (i / stride) % area.width()
                << (i / stride) / area.width() << "GPU" << actual.mid(offset, stride).toHex()
                << "Native" << reference.mid(offset, stride).toHex();
            break;
        }
        qWarning() << "Native source bounds" << nativePaint->paintDevice()->defaultBounds()->bounds()
            << "allocation" << nativePaint->paintDevice()->extent() << "exact" << nativePaint->paintDevice()->exactBounds();
        const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!root.isEmpty()) {
            QDir().mkpath(root);
            const auto stem = QStringLiteral("/node-") + QString::fromLatin1(QTest::currentDataTag()) +
                QLatin1Char('-') + QString::number(pixelComparison);
            for (const auto &entry : {qMakePair(QStringLiteral("native"), reference), qMakePair(QStringLiteral("gpu"), actual)}) {
                QFile file(root + stem + QLatin1Char('-') + entry.first + QStringLiteral(".rgba"));
                if (file.open(QIODevice::WriteOnly)) file.write(entry.second);
            }
        }
        return false;
    };
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = area.size(); raster.format = format; raster.colorProfile = cs->profile()->rawData();
    Gpu::RasterLayer paint; paint.name = "Source"; paint.uuid = QUuid::createUuid();
    for (int x = 0; x < area.width(); x += Gpu::RgbaTile::Size) {
        Gpu::RgbaTile tile; tile.coordinate = {x / Gpu::RgbaTile::Size, 0}; tile.format = format;
        tile.rgba = QByteArray(Gpu::RgbaTile::byteCount(format), '\0');
        const int rowBytes = qMin(Gpu::RgbaTile::Size, area.width() - x) * stride;
        for (int y = 0; y < area.height(); ++y)
            tile.rgba.replace(y * Gpu::RgbaTile::Size * stride, rowBytes, raw.mid((y * area.width() + x) * stride, rowBytes));
        paint.tiles.append(tile);
    }
    raster.layers.append(paint);
    auto owner = Gpu::PaintImageSP::create(execution, raster); QVERIFY(owner);
    QCOMPARE(owner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, alpha, area)), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, "Filter node editing", cs));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get()); QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto close = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName("file_close")->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    auto *manager = window->viewManager();
    manager->setCurrentView(view);
    manager->nodeManager()->slotUiActivatedNode(view->image()->root()->firstChild());
    auto *create = manager->actionManager()->actionByName(mask ? "add_new_filter_mask" : "add_new_adjustment_layer");
    QVERIFY(create);
    bool visited = false;
    bool previewReady = false;
    const auto runDialog = [&](bool accept, const std::function<void()> &operation) {
        visited = false; previewReady = false;
        QTimer timer;
        timer.setInterval(10);
        QObject::connect(&timer, &QTimer::timeout, &timer, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != QLatin1String("gpuFilterNodeDialog")) return;
            timer.stop(); visited = true;
            auto *tree = dialog->findChild<QTreeView *>(QStringLiteral("filtersSelector"));
            const auto chosenFilter = KisFilterRegistry::instance()->value(config->name());
            if (!tree || !tree->model() || !chosenFilter) { dialog->reject(); return; }
            const auto *model = tree->model();
            std::function<QModelIndex(const QModelIndex &)> find = [&](const QModelIndex &parent) -> QModelIndex {
                for (int row = 0; row < model->rowCount(parent); ++row) {
                    const auto index = model->index(row, 0, parent);
                    if (index.data().toString() == chosenFilter->name()) return index;
                    const auto nested = find(index);
                    if (nested.isValid()) return nested;
                }
                return {};
            };
            const auto chosen = find({});
            if (!chosen.isValid()) { dialog->reject(); return; }
            tree->setCurrentIndex(chosen);
            Q_EMIT tree->activated(chosen);
            if (auto *widget = dialog->findChild<KisConfigWidget *>()) {
                widget->setConfiguration(config);
                Q_EMIT widget->sigConfigurationUpdated();
            }
            const auto *button = dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
            previewReady = button->isEnabled();
            if (accept && previewReady) dialog->accept();
            else dialog->reject();
        });
        timer.start(); operation();
    };
    const auto before = owner->operations().snapshot();
    const auto beforeCount = owner->operations().undoCount();
    runDialog(false, [&] { create->trigger(); });
    QVERIFY(visited); QVERIFY(previewReady);
    QCOMPARE(owner->operations().snapshot(), before);
    QVERIFY(!owner->operations().hasActiveStroke());
    QVERIFY(manager->activeNode());
    QCOMPARE(manager->activeNode()->uuid(), paint.uuid);
    runDialog(true, [&] { create->trigger(); });
    QVERIFY(visited); QVERIFY(previewReady);
    QCOMPARE(owner->operations().undoCount(), beforeCount + 1);
    QCOMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto inserted = owner->operations().snapshot();
    const auto target = inserted->activeLayer;
    KisNodeSP node;
    for (const auto &layer : inserted->layers) if (layer.id == target) {
        KisNodeList pending{view->image()->root()};
        for (qsizetype i = 0; i < pending.size(); ++i) {
            const auto candidate = pending[i];
            if (candidate->uuid() == layer.uuid) { node = candidate; break; }
            for (auto child = candidate->firstChild(); child; child = child->nextSibling()) pending.append(child);
        }
    }
    QVERIFY(node);
    QVERIFY(mask ? node->inherits("KisFilterMask") : node->inherits("KisAdjustmentLayer"));
    QCOMPARE(node->compositeOpId(), nativeNode->compositeOpId());
    QVERIFY(pixelsEqual(owner->readRgbaPixels(0, area, raw.size()).bytes, expected()));
    QCOMPARE(owner->readRgbaPixels(1, area, raw.size()).bytes, raw);
    const auto physicalPixels = [&](KisNodeSP projectedNode) {
        QByteArray bytes(raw.size(), '\0');
        projectedNode->projection()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    QVERIFY(pixelsEqual(physicalPixels(mask ? node->parent() : node), physicalPixels(mask ? KisNodeSP(nativePaint) : nativeNode)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), inserted);
    KisNodeList restored{view->image()->root()};
    const auto targetUuid = node->uuid();
    node.clear();
    for (qsizetype i = 0; i < restored.size(); ++i) {
        const auto candidate = restored[i];
        if (candidate->uuid() == targetUuid) { node = candidate; break; }
        for (auto child = candidate->firstChild(); child; child = child->nextSibling()) restored.append(child);
    }
    QVERIFY(node);
    manager->nodeManager()->slotNonUiActivatedNode(node);
    QCOMPARE(owner->operations().snapshot(), inserted);
    const QString updatedId = convolution ? (id == QLatin1String("sharpen") ? QStringLiteral("mean removal") : QStringLiteral("sharpen")) :
        id == QLatin1String("maximize") ? QStringLiteral("minimize") : QStringLiteral("maximize");
    config = KisFilterRegistry::instance()->value(updatedId)->defaultConfiguration(KisGlobalResourcesInterface::instance());
    const auto firstPixels = expected();
    runDialog(false, [&] {
        manager->nodeManager()->nodeProperties(node);
    });
    QVERIFY(visited); QVERIFY(previewReady);
    // Layer-panel selection restoration can replace the version container.
    // Cancellation preserves the content revision, history and final selection.
    const auto cancelled = owner->operations().snapshot();
    QCOMPARE(cancelled->documentId(), inserted->documentId());
    QCOMPARE(cancelled->revision(), inserted->revision());
    QCOMPARE(cancelled->historyRevision(), inserted->historyRevision());
    QCOMPARE(cancelled->activeLayer, inserted->activeLayer);
    QCOMPARE(cancelled->selectedLayers, inserted->selectedLayers);
    QCOMPARE(owner->operations().undoCount(), beforeCount + 1);
    QCOMPARE(owner->operations().redoCount(), 0);
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, firstPixels);
    QCOMPARE(owner->readRgbaPixels(1, area, raw.size()).bytes, raw);
    runDialog(true, [&] {
        manager->nodeManager()->nodeProperties(node);
    });
    QVERIFY(visited); QVERIFY(previewReady);
    QCOMPARE(owner->operations().undoCount(), beforeCount + 2);
    dynamic_cast<KisNodeFilterInterface *>(nativeNode.data())->setFilter(config->cloneWithResourcesSnapshot());
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QVERIFY(pixelsEqual(owner->readRgbaPixels(0, area, raw.size()).bytes, expected()));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, firstPixels);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(pixelsEqual(owner->readRgbaPixels(0, area, raw.size()).bytes, expected()));
    QTemporaryDir directory; QVERIFY(directory.isValid());
    {
        std::unique_ptr<KisDocument> nativeDocument(KisPart::instance()->createDocument());
        nativeDocument->setFileBatchMode(true);
        nativeDocument->setCurrentImage(native);
        const auto nativePath = directory.filePath("native-filters.kra");
        QVERIFY2(nativeDocument->saveAs(nativePath, KisDocument::nativeFormatMimeType(), false), qPrintable(nativeDocument->errorMessage()));
        nativeDocument->waitForSavingToComplete();
        std::unique_ptr<KisDocument> imported(KisPart::instance()->createDocument());
        imported->setFileBatchMode(true);
        QVERIFY2(imported->openPath(nativePath, KisDocument::DontAddToRecent), qPrintable(imported->errorMessage()));
        const auto importedOwner = imported->image()->gpuImage(); QVERIFY(importedOwner);
        QCOMPARE(importedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    }
    QCOMPARE(owner->clearSelection(), Gpu::Result::Success);
    manager->nodeManager()->slotNonUiActivatedNode(node);
    QVERIFY(view->image()->isGpuEditableMaskView(node));
    const auto beforeMaskEdit = owner->operations().snapshot();
    Gpu::Brush erase;
    erase.erase = true; erase.diameter = 4;
    const auto stroke = owner->beginTransparencyMaskStroke(target, erase, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendPoint(stroke, {{5, 5}, 1}), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), beforeMaskEdit);
    QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), beforeMaskEdit);
    const auto filteredPixels = expected();
    auto *resources = manager->canvasResourceProvider();
    resources->setFGColor(KoColor(Qt::black, cs));
    resources->setOpacity(1); resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = manager->actionManager()->actionByName("fill_selection_foreground_color");
    QVERIFY(fill); QVERIFY(fill->isEnabled());
    const auto maskHistory = owner->operations().undoCount();
    fill->trigger();
    QCOMPARE(owner->operations().undoCount(), maskHistory + 1);
    const auto nativeCoverage = mask ? qobject_cast<KisFilterMask *>(nativeNode.data())->selection() :
        qobject_cast<KisAdjustmentLayer *>(nativeNode.data())->internalSelection();
    nativeCoverage->pixelSelection()->clear(area);
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QVERIFY(pixelsEqual(owner->readRgbaPixels(0, area, raw.size()).bytes, expected()));
    QCOMPARE(owner->readRgbaPixels(1, area, raw.size()).bytes, raw);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, filteredPixels);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(pixelsEqual(owner->readRgbaPixels(0, area, raw.size()).bytes, expected()));
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath("filters.kra");
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    {
        qputenv("LIBREPAINT_GPU_ENGINE", "0");
        const auto resetEngine = qScopeGuard([&] { qputenv("LIBREPAINT_GPU_ENGINE", "1"); });
        std::unique_ptr<KisDocument> nativeReader(KisPart::instance()->createDocument());
        nativeReader->setFileBatchMode(true);
        QVERIFY2(nativeReader->openPath(path, KisDocument::DontAddToRecent), qPrintable(nativeReader->errorMessage()));
        nativeReader->image()->waitForDone();
        QByteArray actual(raw.size(), '\0');
        nativeReader->image()->projection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), area);
        exchangeRgbaBgra(actual, stride);
        QCOMPARE(actual, expected());
    }
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    auto reopenedOwner = reopened->image()->gpuImage(); QVERIFY(reopenedOwner);
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    Gpu::LayerId reopenedPaint = 0;
    for (const auto &layer : reopenedOwner->operations().snapshot()->layers)
        if (layer.uuid == paint.uuid) reopenedPaint = layer.id;
    QVERIFY(reopenedPaint);
    QCOMPARE(reopenedOwner->readRgbaPixels(reopenedPaint, area, raw.size()).bytes, raw);
    for (const auto &layer : reopenedOwner->operations().snapshot()->layers)
        if (Gpu::isFilterNode(layer.kind)) {
            QVERIFY(layer.filter); QCOMPARE(layer.filter->id, updatedId); QVERIFY(layer.filter->resolved);
            auto updated = *layer.filter;
            QCOMPARE(reopenedOwner->setFilterDefinition(layer.id, updated, "Reopened settings"), Gpu::Result::Success);
        }
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    const auto reopenedPixels = expected();
    Gpu::LayerId reopenedFilter = 0;
    for (const auto &layer : reopenedOwner->operations().snapshot()->layers)
        if (Gpu::isFilterNode(layer.kind)) reopenedFilter = layer.id;
    QVERIFY(reopenedFilter);
    QCOMPARE(reopenedOwner->setFilterSelection(reopenedFilter, Gpu::SelectionMask::uniform(255)), Gpu::Result::Success);
    nativeCoverage->pixelSelection()->clear();
    nativeCoverage->pixelSelection()->setDefaultPixel(KoColor(Qt::white, nativeCoverage->pixelSelection()->colorSpace()));
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    QCOMPARE(reopenedOwner->readRgbaPixels(reopenedPaint, area, raw.size()).bytes, raw);
    QCOMPARE(reopenedOwner->undo(), Gpu::Result::Success);
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, reopenedPixels);
    QCOMPARE(reopenedOwner->redo(), Gpu::Result::Success);
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    const auto beforeDirect = reopenedOwner->operations().snapshot();
    const auto directCount = reopenedOwner->operations().undoCount();
    KisNodeSP publicNode;
    QList<KisNodeSP> publicNodes{reopened->image()->root()};
    for (qsizetype i = 0; i < publicNodes.size(); ++i) {
        auto candidate = publicNodes[i];
        if (candidate->inherits(mask ? "KisFilterMask" : "KisAdjustmentLayer")) publicNode = candidate;
        for (auto child = candidate->firstChild(); child; child = child->nextSibling()) publicNodes.append(child);
    }
    QVERIFY(publicNode);
    auto *publicFilter = dynamic_cast<KisNodeFilterInterface *>(publicNode.data()); QVERIFY(publicFilter);
    const auto directConfiguration = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    publicFilter->setFilter(directConfiguration->cloneWithResourcesSnapshot());
    QCOMPARE(reopenedOwner->operations().undoCount(), directCount);
    for (const auto &layer : beforeDirect->layers) if (layer.id == reopenedFilter) QCOMPARE(layer.filter->id, updatedId);
    for (const auto &layer : reopenedOwner->operations().snapshot()->layers)
        if (layer.id == reopenedFilter) QCOMPARE(layer.filter->id, id);
    dynamic_cast<KisNodeFilterInterface *>(nativeNode.data())->setFilter(directConfiguration->cloneWithResourcesSnapshot());
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    QVERIFY(reopened->saveAs(directory.filePath("reopened-edited-filters.kra"), KisDocument::nativeFormatMimeType(), false));
    reopened->waitForSavingToComplete();
    if (convolution) {
        std::unique_ptr<KisDocument> finalReader(KisPart::instance()->createDocument());
        finalReader->setFileBatchMode(true);
        QVERIFY(finalReader->openPath(directory.filePath("reopened-edited-filters.kra"), KisDocument::DontAddToRecent));
        const auto finalOwner = finalReader->image()->gpuImage(); QVERIFY(finalOwner);
        QCOMPARE(finalOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
        for (const auto &value : finalOwner->operations().snapshot()->layers)
            if (Gpu::isFilterNode(value.kind)) { QVERIFY(value.filter); QCOMPARE(value.filter->id, id); }
    }
    // Clipboard/document insertion retains editable filters and GPU coverage
    // through the destination's history and KRA storage.
    KisNodeList forest;
    for (auto child = reopened->image()->root()->firstChild(); child; child = child->nextSibling())
        if (qobject_cast<KisLayer *>(child.data())) forest.append(child);
    if (mask) {
        const auto detachedMask = reopened->image()->cloneGpuNodes({publicNode});
        QCOMPARE(detachedMask.size(), 1);
        QVERIFY(!detachedMask.first()->parent());
        QVERIFY(detachedMask.first()->uuid() != publicNode->uuid());
        auto maskOwner = Gpu::PaintImageSP::create(execution, raster);
        auto maskView = KisImage::fromGpuImage(maskOwner, "Pasted mask", cs);
        QVERIFY(maskView->insertGpuLayers(detachedMask, 0, maskView->root()->firstChild()));
        QCOMPARE(maskOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
        const auto insertedMask = maskOwner->operations().snapshot();
        QCOMPARE(maskOwner->undo(), Gpu::Result::Success);
        QCOMPARE(maskOwner->readRgbaPixels(0, area, raw.size()).bytes, raw);
        QCOMPARE(maskOwner->redo(), Gpu::Result::Success);
        QCOMPARE(maskOwner->operations().snapshot(), insertedMask);
    }
    auto copied = reopened->image()->cloneGpuNodeImage(forest); QVERIFY(copied);
    QCOMPARE(copied->gpuImage()->readRgbaPixels(0, area, raw.size()).bytes, expected());
    auto detached = reopened->image()->cloneGpuNodes(forest); QVERIFY(!detached.isEmpty());
    Gpu::RasterImage blank;
    blank.size = area.size(); blank.format = format; blank.colorProfile = cs->profile()->rawData();
    auto pastedOwner = Gpu::PaintImageSP::create(execution, blank);
    auto pastedView = KisImage::fromGpuImage(pastedOwner, "Pasted filters", cs);
    QVERIFY(pastedView->insertGpuLayers(detached, 0));
    QCOMPARE(pastedOwner->operations().undoCount(), 1);
    QCOMPARE(pastedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    const auto pasted = pastedOwner->operations().snapshot();
    QCOMPARE(pastedOwner->undo(), Gpu::Result::Success);
    QVERIFY(pastedOwner->operations().snapshot()->layers.isEmpty());
    QCOMPARE(pastedOwner->redo(), Gpu::Result::Success);
    QCOMPARE(pastedOwner->operations().snapshot(), pasted);
    for (const auto &layer : pasted->layers) if (Gpu::isFilterNode(layer.kind)) {
        QVERIFY(layer.filter); QCOMPARE(layer.filter->id, id);
        QVERIFY(layer.filterSelection);
    }
    std::unique_ptr<KisDocument> pastedDocument(KisPart::instance()->createDocument());
    pastedDocument->setFileBatchMode(true); pastedDocument->setCurrentImage(pastedView);
    const auto pastedPath = directory.filePath("pasted-editable-filters.kra");
    QVERIFY(pastedDocument->saveAs(pastedPath, KisDocument::nativeFormatMimeType(), false));
    pastedDocument->waitForSavingToComplete();
    std::unique_ptr<KisDocument> pastedReader(KisPart::instance()->createDocument());
    pastedReader->setFileBatchMode(true);
    QVERIFY(pastedReader->openPath(pastedPath, KisDocument::DontAddToRecent));
    QCOMPARE(pastedReader->image()->gpuImage()->readRgbaPixels(0, area, raw.size()).bytes, expected());

    // Delete on the selected editable mask changes alpha, including fractional
    // document selection, while the source paint pixels remain canonical.
    Gpu::SelectionTileMap maskTiles;
    for (int x = 0; x < area.width(); x += Gpu::SelectionMask::TileSize) {
        QByteArray maskTile(Gpu::SelectionMask::AlphaTileBytes, char(255));
        const int rowBytes = qMin(Gpu::SelectionMask::TileSize, area.width() - x);
        for (int y = 0; y < area.height(); ++y)
            maskTile.replace(y * Gpu::SelectionMask::TileSize, rowBytes, alpha.mid(y * area.width() + x, rowBytes));
        maskTiles.insert({x / Gpu::SelectionMask::TileSize, 0}, maskTile);
    }
    const auto finiteMask = Gpu::SelectionMask::fromAlphaTiles(maskTiles, 255); QVERIFY(finiteMask);
    QCOMPARE(reopenedOwner->setFilterSelection(reopenedFilter, finiteMask), Gpu::Result::Success);
    nativeCoverage->pixelSelection()->clear();
    const quint8 white = 255;
    nativeCoverage->pixelSelection()->setDefaultPixel(KoColor(&white, nativeCoverage->pixelSelection()->colorSpace()));
    nativeCoverage->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), area);
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    QCOMPARE(reopenedOwner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, alpha, area)), Gpu::Result::Success);
    const auto beforeClear = reopenedOwner->operations().snapshot();
    const auto beforeClearPixels = expected();
    QVERIFY(KisToolUtils::clearImage(reopened->image(), {publicNode}, reopened->image()->globalSelection()));
    nativeCoverage->pixelSelection()->clearSelection(selected);
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QCOMPARE(reopenedOwner->operations().historyAction(reopenedOwner->operations().undoCount()), Gpu::HistoryAction::ClearPixels);
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    QCOMPARE(reopenedOwner->readRgbaPixels(reopenedPaint, area, raw.size()).bytes, raw);
    const auto cleared = reopenedOwner->operations().snapshot();
    QCOMPARE(reopenedOwner->undo(), Gpu::Result::Success);
    QCOMPARE(reopenedOwner->operations().snapshot(), beforeClear);
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, beforeClearPixels);
    QCOMPARE(reopenedOwner->redo(), Gpu::Result::Success);
    QCOMPARE(reopenedOwner->operations().snapshot(), cleared);
    QCOMPARE(reopenedOwner->clearSelection(), Gpu::Result::Success);
    QVERIFY(KisToolUtils::clearImage(reopened->image(), {publicNode}, {}));
    nativeCoverage->pixelSelection()->clear();
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    const auto clearedPath = directory.filePath("cleared-filter-mask.kra");
    QVERIFY(reopened->saveAs(clearedPath, KisDocument::nativeFormatMimeType(), false));
    reopened->waitForSavingToComplete();
    std::unique_ptr<KisDocument> clearedReader(KisPart::instance()->createDocument());
    clearedReader->setFileBatchMode(true);
    QVERIFY(clearedReader->openPath(clearedPath, KisDocument::DontAddToRecent));
    QCOMPARE(clearedReader->image()->gpuImage()->readRgbaPixels(0, area, raw.size()).bytes, expected());

    if (!mask) {
        // An adjustment's own filter selection and its transparency child are
        // independently editable even though they have one RGB owner layer.
        QCOMPARE(reopenedOwner->addTransparencyMask(reopenedFilter), Gpu::Result::Success);
        KisNodeSP child;
        for (auto candidate = publicNode->firstChild(); candidate; candidate = candidate->nextSibling())
            if (candidate->inherits("KisTransparencyMask")) child = candidate;
        QVERIFY(child);
        const auto captured = captureGpuFillResources(reopened->image(), child,
            manager->canvasResourceProvider()->resourceManager());
        QVERIFY2(captured.diagnostic.isEmpty(), qPrintable(captured.diagnostic));
        QVERIFY(captured.transparencyMask);
        QCOMPARE(captured.maskUuid, child->uuid());
        QCOMPARE(captured.layer, reopenedFilter);
        QCOMPARE(reopenedOwner->replaceTransparencyMask(reopenedFilter, finiteMask, child->uuid()), Gpu::Result::Success);
        KisSelectionSP childSelection = new KisSelection;
        childSelection->pixelSelection()->setDefaultPixel(KoColor(&white, childSelection->pixelSelection()->colorSpace()));
        childSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), area);
        KisTransparencyMaskSP nativeChild = new KisTransparencyMask(native, "Transparency");
        nativeChild->setSelection(childSelection);
        QVERIFY(native->addNode(nativeChild, nativeNode));
        nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
        QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
        QCOMPARE(reopenedOwner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, alpha, area)), Gpu::Result::Success);
        const auto beforeChild = reopenedOwner->operations().snapshot();
        Gpu::SelectionMaskSP retainedInternal;
        for (const auto &layer : beforeChild->layers)
            if (layer.id == reopenedFilter) retainedInternal = layer.filterSelection;
        QVERIFY(retainedInternal);
        QVERIFY(KisToolUtils::clearImage(reopened->image(), {child}, reopened->image()->globalSelection()));
        childSelection->pixelSelection()->clearSelection(selected);
        nativeChild->setDirty(); native->refreshGraphAsync(); native->waitForDone();
        QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
        for (const auto &layer : reopenedOwner->operations().snapshot()->layers)
            if (layer.id == reopenedFilter) QCOMPARE(layer.filterSelection, retainedInternal);
        QCOMPARE(reopenedOwner->undo(), Gpu::Result::Success);
        QCOMPARE(reopenedOwner->operations().snapshot(), beforeChild);
        QCOMPARE(reopenedOwner->redo(), Gpu::Result::Success);
        QCOMPARE(reopenedOwner->readRgbaPixels(0, area, raw.size()).bytes, expected());
        const auto pairedPath = directory.filePath("paired-adjustment-masks.kra");
        QVERIFY(reopened->saveAs(pairedPath, KisDocument::nativeFormatMimeType(), false));
        reopened->waitForSavingToComplete();
        std::unique_ptr<KisDocument> pairedReader(KisPart::instance()->createDocument());
        pairedReader->setFileBatchMode(true);
        QVERIFY(pairedReader->openPath(pairedPath, KisDocument::DontAddToRecent));
        QCOMPARE(pairedReader->image()->gpuImage()->readRgbaPixels(0, area, raw.size()).bytes, expected());
    }

}
