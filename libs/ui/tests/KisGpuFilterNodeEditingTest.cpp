/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuFilterNodeEditingTest.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <KisGlobalResourcesInterface.h>
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
#include <dialogs/kis_dlg_adjustment_layer.h>
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
#include <kis_node_filter_interface.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_undo_stores.h>
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
    const QRect area(0, 0, 16, 12);
    QByteArray raw;
    for (int y = 0; y < area.height(); ++y) for (int x = 0; x < area.width(); ++x) {
        const quint16 scale = words ? 257 : 1;
        raw += Gpu::rgbaBytes({quint16((17 + y * 9) * scale), quint16(91 * scale),
            quint16((203 - x * 7) * scale), quint16((x % 4 ? 137 : 0) * scale)}, format);
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
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = area.size(); raster.format = format; raster.colorProfile = cs->profile()->rawData();
    Gpu::RasterLayer paint; paint.name = "Source"; paint.uuid = QUuid::createUuid();
    Gpu::RgbaTile tile; tile.format = format; tile.rgba = QByteArray(Gpu::RgbaTile::byteCount(format), '\0');
    for (int y = 0; y < area.height(); ++y)
        tile.rgba.replace(y * Gpu::RgbaTile::Size * stride, area.width() * stride, raw.mid(y * area.width() * stride, area.width() * stride));
    paint.tiles.append(tile); raster.layers.append(paint);
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
            auto *dialog = qobject_cast<KisDlgAdjustmentLayer *>(QApplication::activeModalWidget());
            if (!dialog) return;
            timer.stop(); visited = true;
            dialog->setInitialConfiguration(config, true);
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
    runDialog(true, [&] { create->trigger(); });
    QVERIFY(visited); QVERIFY(previewReady);
    QCOMPARE(owner->operations().undoCount(), beforeCount + 1);
    QCOMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto inserted = owner->operations().snapshot();
    const auto target = inserted->activeLayer;
    KisNodeSP node;
    for (const auto &layer : inserted->layers) if (layer.id == target) {
        for (auto candidate : QList<KisNodeSP>{view->image()->root()->firstChild(), view->image()->root()->lastChild(),
             view->image()->root()->firstChild()->firstChild()})
            if (candidate && candidate->uuid() == layer.uuid) node = candidate;
    }
    QVERIFY(node);
    QVERIFY(mask ? node->inherits("KisFilterMask") : node->inherits("KisAdjustmentLayer"));
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    QCOMPARE(owner->readRgbaPixels(1, area, raw.size()).bytes, raw);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), inserted);
    manager->nodeManager()->slotNonUiActivatedNode(node);
    const QString updatedId = id == QLatin1String("maximize") ? QStringLiteral("minimize") : QStringLiteral("maximize");
    config = KisFilterRegistry::instance()->value(updatedId)->defaultConfiguration(KisGlobalResourcesInterface::instance());
    const auto firstPixels = expected();
    runDialog(false, [&] {
        if (mask) KisNodeManager::PropertyDialogAccess::showMaskProperties(manager->nodeManager());
        else KisNodeManager::PropertyDialogAccess::showLayerProperties(manager->nodeManager());
    });
    QVERIFY(visited); QVERIFY(previewReady);
    QCOMPARE(owner->operations().snapshot(), inserted);
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, firstPixels);
    runDialog(true, [&] {
        if (mask) KisNodeManager::PropertyDialogAccess::showMaskProperties(manager->nodeManager());
        else KisNodeManager::PropertyDialogAccess::showLayerProperties(manager->nodeManager());
    });
    QVERIFY(visited); QVERIFY(previewReady);
    QCOMPARE(owner->operations().undoCount(), beforeCount + 2);
    dynamic_cast<KisNodeFilterInterface *>(nativeNode.data())->setFilter(config->cloneWithResourcesSnapshot());
    nativeNode->setDirty(); native->refreshGraphAsync(); native->waitForDone();
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, expected());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, firstPixels);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(0, area, raw.size()).bytes, expected());
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
}
