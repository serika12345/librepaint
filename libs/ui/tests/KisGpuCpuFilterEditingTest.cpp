/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentViewTest.h"
#include <KisGpuCpuFilter.h>
#include <KisGpuFilterConfiguration.h>
#include <kis_filter_mask.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <kis_transaction.h>
#include <kis_config_widget.h>
#include <KoChannelInfo.h>
#include <QtEndian>
#include <cstring>

void KisGpuDocumentViewTest::cpuFiltersKeepNativeEditing_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("filterId");
    QTest::newRow("u8-posterize") << false << QStringLiteral("posterize");
    QTest::newRow("u16-selected-pixelize") << true << QStringLiteral("pixelize");
}

void KisGpuDocumentViewTest::cpuFiltersKeepNativeEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(QString, filterId);
    const auto filter = KisFilterRegistry::instance()->value(filterId);
    QVERIFY(filter);
    QCOMPARE(Gpu::filterExecutionLocation(filterId), Gpu::ExecutionLocation::Cpu);
    QCOMPARE(Gpu::filterExecutionLocation(QStringLiteral("invert")), Gpu::ExecutionLocation::Gpu);
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int stride = Gpu::pixelByteCount(format);
    const QRect area(0, 0, 270, 18);
    QByteArray initial(area.width() * area.height() * stride, '\0');
    for (int y = 0; y < area.height(); ++y) for (int x = 0; x < area.width(); ++x) {
        const quint16 scale = words ? 257 : 1;
        const quint16 alpha[] = {0, quint16(words ? 1 : 63), quint16(127 * scale), quint16(255 * scale)};
        const Gpu::RgbaPixel pixel{quint16((x * 17 + 19) % 256 * scale),
            quint16((y * 31 + 63) % 256 * scale), quint16((x + y * 13) % 256 * scale), alpha[x % 4]};
        initial.replace((y * area.width() + x) * stride, stride, Gpu::rgbaBytes(pixel, format));
    }
    const auto nativeBytes = [&](QByteArray bytes) {
        for (qsizetype i = 0; i < bytes.size(); i += stride) {
            if (words) {
                quint16 bgra[4];
                for (int c = 0; c < 4; ++c)
                    bgra[c] = qFromLittleEndian<quint16>(bytes.constData() + i + (c == 0 ? 2 : c == 2 ? 0 : c) * 2);
                std::memcpy(bytes.data() + i, bgra, sizeof(bgra));
            } else std::swap(bytes[i], bytes[i + 2]);
        }
        return bytes;
    };
    KisImageSP reference = new KisImage(nullptr, area.width(), area.height(), cs, "CPU filter reference");
    KisPaintDeviceSP expected = new KisPaintDevice(cs);
    expected->setDefaultBounds(new KisDefaultBounds(reference));
    const auto nativeInitial = nativeBytes(initial);
    expected->writeBytes(reinterpret_cast<const quint8 *>(nativeInitial.constData()), area);
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = area.size();
    raster.format = format;
    raster.layers = {{QStringLiteral("CPU filter layer"), true, QUuid::createUuid(), {}}};
    const auto owner = paintImageWithEncoding(execution, raster);
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->writeRgbaPixels(layer, area, initial), Gpu::Result::Success);
    KisSelectionSP selection;
    if (words) {
        const QRect selected(249, 3, 13, 11);
        QByteArray alpha(selected.width() * selected.height(), '\0');
        for (int i = 0; i < alpha.size(); ++i) alpha[i] = char(i % 3 == 0 ? 0 : i % 3 == 1 ? 128 : 255);
        QCOMPARE(owner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(selected, alpha, area)), Gpu::Result::Success);
        selection = new KisSelection;
        selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), selected);
    }
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("CPU filter editing"), cs));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto close = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto node = view->image()->root()->firstChild();
    window->viewManager()->nodeManager()->slotUiActivatedNode(node);
    auto *actions = window->viewManager()->actionManager();
    auto *action = actions->actionByName(QStringLiteral("krita_filter_") + filterId);
    QVERIFY(action && action->isEnabled());
    auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    if (words) {
        config->setProperty("pixelWidth", 7);
        config->setProperty("pixelHeight", 5);
        QBitArray flags(cs->channelCount(), true);
        for (int i = 0; i < cs->channels().size(); ++i)
            if (cs->channels()[i]->displayPosition() == 1) flags.clearBit(i);
        config->setChannelFlags(flags);
    } else config->setProperty("steps", 5);
    const auto history = owner->operations().undoCount();
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    action->trigger();
    auto *dialog = window->findChild<QDialog *>(QStringLiteral("gpuFilterDialog"));
    QVERIFY(dialog);
    auto *widget = dialog->findChild<KisConfigWidget *>();
    QVERIFY(widget);
    widget->setConfiguration(config);
    dialog->reject();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(owner->operations().snapshot(), before);
    QVERIFY(!lifetime->isModified());
    KisFilterConfigurationSP accepted;
    if (words) {
        accepted = config->clone();
        window->viewManager()->filterManager()->apply(config);
    } else {
        action->trigger();
        dialog = window->findChild<QDialog *>(QStringLiteral("gpuFilterDialog"));
        QVERIFY(dialog);
        widget = dialog->findChild<KisConfigWidget *>();
        QVERIFY(widget);
        widget->setConfiguration(config);
        // The settings widget supplies the accepted product configuration.
        accepted = KisFilterConfigurationSP(dynamic_cast<KisFilterConfiguration *>(widget->configuration().data()));
        QVERIFY(accepted);
        dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    }
    QCOMPARE(owner->operations().undoCount(), history + 1);
    QCOMPARE(lifetime->undoStack()->command(history)->text().toString(),
        kundo2_i18nc("Filter as an effect", "Filter \"%1\"", filter->name()).toString());
    QVERIFY(lifetime->isModified());
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
    QVERIFY(view->image()->gpuImage() == owner);
    const auto nativeApply = [&] {
        auto affected = expected->exactBounds();
        if (filter->needsTransparentPixels(accepted, cs)) affected |= area;
        const auto output = filter->changedRect(affected, accepted, 0).intersected(area);
        KisTransaction transaction(expected);
        filter->process(expected, expected, selection, output, accepted->cloneWithResourcesSnapshot());
    };
    const auto compare = [&](const QByteArray &actual) {
        QByteArray bytes(initial.size(), Qt::Uninitialized);
        expected->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        const auto difference = pixelStepDifference(nativeBytes(actual), bytes, words ? 2 : 1);
        if (!difference.isEmpty()) qWarning().noquote() << difference;
        return difference.isEmpty();
    };
    nativeApply();
    const auto first = owner->readRgbaPixels(layer, area, initial.size()).bytes;
    QVERIFY(first != initial);
    QVERIFY(compare(first));
    actions->actionByName(QStringLiteral("edit_undo"))->trigger();
    QCOMPARE(owner->readRgbaPixels(layer, area, initial.size()).bytes, initial);
    actions->actionByName(QStringLiteral("edit_redo"))->trigger();
    QCOMPARE(owner->readRgbaPixels(layer, area, initial.size()).bytes, first);
    actions->actionByName(QStringLiteral("filter_apply_again"))->trigger();
    QCOMPARE(owner->operations().undoCount(), history + 2);
    nativeApply();
    QVERIFY(compare(owner->readRgbaPixels(layer, area, initial.size()).bytes));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("cpu-filter.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    const auto reopenedOwner = reopened->image()->gpuImage();
    QVERIFY(reopenedOwner);
    QCOMPARE(reopenedOwner->operations().snapshot()->format, format);
    QCOMPARE(reopened->image()->colorSpace()->profile()->rawData(), cs->profile()->rawData());
    QCOMPARE(reopened->image()->root()->firstChild()->name(), node->name());
    QCOMPARE(reopened->image()->root()->firstChild()->uuid(), node->uuid());
    QVERIFY(compare(reopenedOwner->readRgbaPixels(reopenedOwner->operations().activeLayer(), area, initial.size()).bytes));
    if (words) {
        const auto lockedVersion = owner->operations().snapshot();
        node->setUserLocked(true);
        const auto locked = owner->operations().snapshot();
        QVERIFY(!kisGpuApplyCpuFilter(view->image().data(), node, accepted));
        QCOMPARE(owner->operations().snapshot(), locked);
        QVERIFY(locked != lockedVersion);
    }
}

void KisGpuDocumentViewTest::cpuMaskFiltersKeepNativeEditing_data()
{
    QTest::addColumn<bool>("filterMask");
    QTest::newRow("selected-transparency-pixelize") << false;
    QTest::newRow("u16-filter-mask-and-global-invert") << true;
}

void KisGpuDocumentViewTest::cpuMaskFiltersKeepNativeEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, filterMask);
    const bool showGlobalSelection = KisConfig(true).showGlobalSelection();
    const auto restoreVisibility = qScopeGuard([&] {
        KisConfig(false).setShowGlobalSelection(showGlobalSelection);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    KisConfig(false).setShowGlobalSelection(true);
    KisConfigNotifier::instance()->notifyConfigChanged();
    const QString filterId = filterMask ? QStringLiteral("invert") : QStringLiteral("pixelize");
    const auto filter = KisFilterRegistry::instance()->value(filterId);
    QVERIFY(filter);
    QCOMPARE(Gpu::filterExecutionLocation(filterId, Gpu::FilterTarget::Alpha), Gpu::ExecutionLocation::Cpu);
    const QRect area(0, 0, 270, 18);
    const QRect compared(-2, -2, 274, 22);
    const auto *cs = filterMask ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const quint8 defaultAlpha = filterMask ? 0 : 255;
    Gpu::SelectionTileMap tiles;
    for (int tx = 0; tx < 2; ++tx) {
        QByteArray bytes(Gpu::SelectionMask::AlphaTileBytes, char(defaultAlpha));
        for (int y = 0; y < area.height(); ++y) for (int x = tx * 256; x < qMin((tx + 1) * 256, area.width()); ++x)
            bytes[y * 256 + x % 256] = char((x * 17 + y * 43) % 256);
        tiles.insert({tx, 0}, bytes);
    }
    const auto originalMask = Gpu::SelectionMask::fromAlphaTiles(tiles, defaultAlpha);
    QVERIFY(originalMask);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = area.size();
    raster.format = filterMask ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const auto paintUuid = QUuid::createUuid(), maskUuid = QUuid::createUuid();
    Gpu::RasterLayer paint{QStringLiteral("Mask parent"), true, paintUuid, {}};
    paint.defaultPixel = filterMask ? Gpu::RgbaPixel{11000, 27000, 54000, 65535} : Gpu::RgbaPixel{41, 103, 217, 255};
    raster.layers = {paint};
    if (filterMask) {
        Gpu::RasterLayer mask{QStringLiteral("Internal coverage"), true, maskUuid, {}};
        mask.kind = Gpu::LayerKind::FilterMask;
        mask.parentUuid = paintUuid;
        const auto nodeFilter = KisFilterRegistry::instance()->value(QStringLiteral("fastcoloroverlay"));
        QVERIFY(nodeFilter);
        const auto frozen = freezeGpuFilterConfiguration(nodeFilter->defaultConfiguration(KisGlobalResourcesInterface::instance()), cs);
        QVERIFY(frozen);
        mask.filter = *frozen;
        mask.filterSelection = originalMask;
        raster.layers.append(mask);
    } else {
        raster.layers[0].transparencyMask = originalMask;
        raster.layers[0].transparencyMaskUuid = maskUuid;
        raster.layers[0].transparencyMaskName = QStringLiteral("Coverage");
    }
    const auto owner = paintImageWithEncoding(execution, raster);
    const auto paintId = owner->operations().snapshot()->layers.first().id;
    QByteArray selected(13 * 12, '\0');
    for (int i = 0; i < selected.size(); ++i) selected[i] = char(i % 3 == 0 ? 0 : i % 3 == 1 ? 128 : 255);
    const QRect selectedRect(249, 2, 13, 12);
    const auto selectedMask = Gpu::SelectionMask::fromAlphaCoverage(selectedRect, selected, area);
    QCOMPARE(owner->setSelectionMask(selectedMask), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask filter editing"), cs));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto close = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    auto *manager = window->viewManager();
    manager->setCurrentView(view);
    KisNodeSP node = KisLayerUtils::findNodeByUuid(view->image()->root(), maskUuid);
    QVERIFY(node);
    manager->nodeManager()->slotUiActivatedNode(node);
    auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    config->setProperty("pixelWidth", 7);
    config->setProperty("pixelHeight", 5);
    const auto reference = kisGpuFilterConfigurationDevice(view->image().data(), node);
    QVERIFY(reference);
    const auto before = owner->operations().snapshot();
    const auto count = owner->operations().undoCount();
    const auto originalPaint = owner->readRgbaPixels(paintId, area, area.width() * area.height() * Gpu::pixelByteCount(raster.format)).bytes;
    const auto nativeRead = [&](KisPaintDeviceSP pixels) {
        QByteArray bytes(compared.width() * compared.height(), Qt::Uninitialized);
        pixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), compared);
        return bytes;
    };
    const auto alphaMatches = [](const QByteArray &actual, const QByteArray &expected) {
        if (actual.size() != expected.size()) return false;
        for (qsizetype i = 0; i < actual.size(); ++i) {
            const int a = quint8(actual[i]), b = quint8(expected[i]);
            if ((a == 0) != (b == 0) || qAbs(a - b) > 1) {
                qWarning() << "Alpha difference at" << i << "actual" << a << "expected" << b;
                return false;
            }
        }
        return true;
    };
    const auto originalBytes = nativeRead(reference);
    auto affected = reference->exactBounds();
    if (filter->needsTransparentPixels(config, reference->colorSpace())) affected |= area;
    const auto process = filter->changedRect(affected, config, 0).intersected(area);
    KisSelectionSP selection = new KisSelection;
    selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(selected.constData()), selectedRect);
    {
        KisTransaction transaction(reference);
        filter->process(reference, reference, selection, process, config->cloneWithResourcesSnapshot());
    }
    auto *action = manager->actionManager()->actionByName(QStringLiteral("krita_filter_") + filterId);
    QVERIFY(action && action->isEnabled());
    if (filterMask) action->trigger();
    else {
        action->trigger();
        auto *dialog = window->findChild<QDialog *>(QStringLiteral("gpuFilterDialog"));
        QVERIFY(dialog);
        auto *widget = dialog->findChild<KisConfigWidget *>();
        QVERIFY(widget);
        widget->setConfiguration(config);
        dialog->reject();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(owner->operations().snapshot(), before);
        manager->filterManager()->apply(config);
    }
    QCOMPARE(owner->operations().undoCount(), count + 1);
    QCOMPARE(lifetime->undoStack()->command(count)->text().toString(),
        kundo2_i18nc("Filter as an effect", "Filter \"%1\"", filter->name()).toString());
    QVERIFY(alphaMatches(nativeRead(node->paintDevice()), nativeRead(reference)));
    QVERIFY(nativeRead(node->paintDevice()) != originalBytes);
    QCOMPARE(owner->readRgbaPixels(paintId, area, originalPaint.size()).bytes, originalPaint);
    const auto applied = owner->operations().snapshot();
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(nativeRead(node->paintDevice()), originalBytes);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), applied);
    if (filterMask) {
        // Filtering the active global selection uses no self-selection merge.
        auto global = view->image()->rootLayer()->selectionMask();
        QVERIFY(global);
        manager->nodeManager()->slotUiActivatedNode(global);
        QCOMPARE(manager->activeNode(), KisNodeSP(global));
        QCOMPARE(view->image()->overlaySelectionMask(), global);
        const auto globalReference = kisGpuFilterConfigurationDevice(view->image().data(), global);
        QVERIFY(globalReference);
        auto invert = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
        auto bounds = globalReference->exactBounds();
        if (filter->needsTransparentPixels(invert, globalReference->colorSpace())) bounds |= area;
        {
            KisTransaction transaction(globalReference);
            filter->process(globalReference, bounds.intersected(area), invert->cloneWithResourcesSnapshot());
        }
        manager->filterManager()->apply(invert);
        QCOMPARE(owner->operations().undoCount(), count + 2);
        QVERIFY(alphaMatches(nativeRead(view->image()->globalSelection()->pixelSelection()), nativeRead(globalReference)));
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot(), applied);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        node->setUserLocked(true);
        const auto locked = owner->operations().snapshot();
        QVERIFY(!kisGpuApplyCpuFilter(view->image().data(), node, config));
        QCOMPARE(owner->operations().snapshot(), locked);
    }
    const auto savedSelection = nativeRead(view->image()->globalSelection()->pixelSelection());
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("mask-filter.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    const auto mask = KisLayerUtils::findNodeByUuid(reopened->image()->root(), maskUuid);
    QVERIFY(mask);
    QCOMPARE(mask->name(), node->name());
    QCOMPARE(mask->userLocked(), node->userLocked());
    QVERIFY(alphaMatches(nativeRead(mask->paintDevice()), nativeRead(reference)));
    QCOMPARE(mask->paintDevice()->defaultPixel(), node->paintDevice()->defaultPixel());
    QVERIFY(reopened->image()->globalSelection());
    QVERIFY(alphaMatches(nativeRead(reopened->image()->globalSelection()->pixelSelection()), savedSelection));
    QCOMPARE(owner->operations().historyFilterId(count + 1), filterId);
}
