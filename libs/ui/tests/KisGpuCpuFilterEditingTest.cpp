/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentViewTest.h"
#include <KisGpuCpuFilter.h>
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
