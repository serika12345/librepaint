/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentViewTest.h"
#include <KisGpuCpuFilter.h>
#include <kis_config_widget.h>
#include <kis_transaction.h>
#include <KConfigGroup>
#include <KSharedConfig>

void KisGpuDocumentViewTest::filterDialogPreviewsKeepOneHistory_data()
{
    QTest::addColumn<bool>("mask");
    QTest::newRow("gpu-rgba-hsv") << false;
    QTest::newRow("cpu-alpha-pixelize") << true;
}

void KisGpuDocumentViewTest::filterDialogPreviewsKeepOneHistory()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, mask);
    KConfigGroup group(KSharedConfig::openConfig(), "filterdialog");
    const bool oldPreview = group.readEntry("showPreview", true);
    const bool oldGlobalSelection = KisConfig(true).showGlobalSelection();
    const auto restoreSettings = qScopeGuard([&] {
        group.writeEntry("showPreview", oldPreview);
        group.config()->sync();
        KisConfig(false).setShowGlobalSelection(oldGlobalSelection);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    group.writeEntry("showPreview", true);
    KisConfig(false).setShowGlobalSelection(true);
    KisConfigNotifier::instance()->notifyConfigChanged();
    const QRect area(0, 0, 32, 16);
    const auto filterId = mask ? QStringLiteral("pixelize") : QStringLiteral("hsvadjustment");
    const auto filter = KisFilterRegistry::instance()->value(filterId);
    QVERIFY(filter);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = area.size();
    const auto layerUuid = QUuid::createUuid();
    raster.layers = {{QStringLiteral("Filter preview"), true, layerUuid, {}}};
    const auto owner = paintImageWithEncoding(execution, raster);
    if (mask) {
        QByteArray alpha(Gpu::SelectionMask::AlphaTileBytes, char(0));
        for (int y = 0; y < area.height(); ++y) for (int x = 0; x < area.width(); ++x)
            alpha[y * 256 + x] = char((x * 37 + y * 17) % 256);
        QCOMPARE(owner->setSelectionMask(Gpu::SelectionMask::fromAlphaTiles({{{0, 0}, alpha}})), Gpu::Result::Success);
    }
    const auto layerId = owner->operations().activeLayer();
    QByteArray pixels(area.width() * area.height() * 4, Qt::Uninitialized);
    for (int i = 0; i < pixels.size(); i += 4)
        pixels.replace(i, 4, QByteArray::fromHex("c81e46ff"));
    QCOMPARE(owner->writeRgbaPixels(layerId, area, pixels), Gpu::Result::Success);
    // Keep a Redo branch across preview cancellation and preview disablement.
    QCOMPARE(owner->invertColors(layerId), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Filter preview history")));
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
    const KisNodeSP node = mask ? KisNodeSP(view->image()->rootLayer()->selectionMask()) :
        KisLayerUtils::findNodeByUuid(view->image()->root(), layerUuid);
    QVERIFY(node);
    manager->nodeManager()->slotUiActivatedNode(node);
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    const int history = owner->operations().undoCount();
    const auto original = kisGpuFilterConfigurationDevice(view->image().data(), node);
    QVERIFY(original);
    const auto read = [&](KisPaintDeviceSP source) {
        QByteArray bytes(area.width() * area.height() * source->pixelSize(), Qt::Uninitialized);
        source->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        return bytes;
    };
    const auto initial = read(original);
    const auto actual = [&] {
        if (mask) return read(node->paintDevice());
        auto bytes = execution->readRgbaPixels(owner->handle(), owner->operations().preview(), area, layerId, pixels.size()).bytes;
        for (qsizetype i = 0; i < bytes.size(); i += 4) std::swap(bytes[i], bytes[i + 2]);
        return bytes;
    };
    const auto matches = [&](const QByteArray &expected) {
        const auto bytes = actual();
        if (!mask) return pixelStepDifference(bytes, expected, 1).isEmpty();
        if (bytes.size() != expected.size()) return false;
        for (qsizetype i = 0; i < bytes.size(); ++i) {
            const int a = quint8(bytes[i]), b = quint8(expected[i]);
            if ((a == 0) != (b == 0) || qAbs(a - b) > 1) return false;
        }
        return true;
    };
    auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    auto *action = manager->actionManager()->actionByName(QStringLiteral("krita_filter_") + filterId);
    QVERIFY(action && action->isEnabled());
    action->trigger();
    const auto dialogName = mask ? QStringLiteral("gpuFilterDialog") : QStringLiteral("gpuHsvAdjustmentDialog");
    auto *dialog = window->findChild<QDialog *>(dialogName);
    QVERIFY(dialog);
    auto *preview = dialog->findChild<QCheckBox *>(QStringLiteral("checkBoxPreview"));
    QVERIFY(preview && preview->isChecked());
    const auto configure = [&](int value) {
        config->setProperty("h", value);
        config->setProperty("type", 0);
        config->setProperty("compatibilityMode", false);
        config->setProperty("pixelWidth", value);
        config->setProperty("pixelHeight", 3);
        config->setProperty("keepAspect", false);
        auto *widget = dialog->findChild<KisConfigWidget *>();
        if (!widget) return QByteArray{};
        widget->setConfiguration(config);
        QMetaObject::invokeMethod(widget, "sigConfigurationUpdated", Qt::DirectConnection);
        KisPaintDeviceSP reference = mask ? KisPaintDeviceSP(new KisPixelSelection(original)) :
            KisPaintDeviceSP(new KisPaintDevice(*original));
        {
            KisTransaction transaction(reference);
            filter->process(reference, area, config->cloneWithResourcesSnapshot());
        }
        return read(reference);
    };
    const auto first = configure(mask ? 3 : 35);
    QVERIFY(!first.isEmpty() && first != initial);
    QTRY_VERIFY(matches(first));
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().undoCount(), history);
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!lifetime->isModified());
    const auto second = configure(mask ? 7 : 110);
    QVERIFY(!second.isEmpty() && second != first);
    QTRY_VERIFY(matches(second));
    QCOMPARE(owner->operations().snapshot(), before);
    preview->setChecked(false);
    QVERIFY(matches(initial));
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!owner->operations().hasActiveStroke());
    preview->setChecked(true);
    QTRY_VERIFY(matches(second));
    dialog->reject();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(matches(initial));
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().redoCount(), 1);
    QVERIFY(!lifetime->isModified());
    action->trigger();
    dialog = window->findChild<QDialog *>(dialogName);
    QVERIFY(dialog);
    preview = dialog->findChild<QCheckBox *>(QStringLiteral("checkBoxPreview"));
    QVERIFY(preview);
    preview->setChecked(false);
    const auto accepted = configure(mask ? 5 : 60);
    // Accept before the delayed preview update, with Preview disabled.
    dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    QVERIFY(matches(accepted));
    QCOMPARE(owner->operations().undoCount(), history + 1);
    QCOMPARE(owner->operations().redoCount(), 0);
    QVERIFY(lifetime->isModified());
    QVERIFY(!manager->filterManager()->isStrokeRunning());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(matches(initial));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(matches(accepted));
}
