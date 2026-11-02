/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuColorFilterEditingTest.h"
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <KisGlobalResourcesInterface.h>
#include <KisGpuBlendMode.h>
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
#include <document/KisDocument.h>
#include <document/kis_filter_manager.h>
#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <filter/kis_filter_registry.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kra/KisKraRasterCodec.h>
#include <nodes/kis_node_manager.h>
#include <kis_config_widget.h>
#include <kis_image.h>
#include <kis_layer_utils.h>
#include <kis_paint_device.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <algorithm>
#include <memory>

namespace {
void exchangeRgbaBgra(QByteArray &bytes, int stride)
{
    const int channelBytes = stride / 4;
    for (qsizetype offset = 0; offset < bytes.size(); offset += stride)
        for (int b = 0; b < channelBytes; ++b) std::swap(bytes[offset + b], bytes[offset + 2 * channelBytes + b]);
}
}

void ColorFilterEditingTest::addData()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<bool>("selected");
    for (bool words : {false, true}) for (const auto *scenario : {
        "maximize", "minimize", "resettransparent", "overlay-normal", "overlay-tint", "overlay-multiply", "overlay-copy"}) {
        for (bool selected : {false, true}) {
            const QByteArray name = QByteArray(words ? "word-" : "byte-") + scenario + (selected ? "-selected" : "-whole");
            QTest::newRow(name) << words << QString::fromLatin1(scenario) << selected;
        }
    }
}

void ColorFilterEditingTest::run()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(QString, scenario);
    QFETCH(bool, selected);
    const QString id = scenario.startsWith(QLatin1String("overlay-")) ? QStringLiteral("fastcoloroverlay") : scenario;
    const auto filter = KisFilterRegistry::instance()->value(id);
    QVERIFY(filter);
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int stride = Gpu::pixelByteCount(format);
    const QRect area(0, 0, 32, 16);
    QByteArray initial(area.width() * area.height() * stride, '\0');
    const quint16 alpha16[]{0, 1, 127, 128, 129, 257, 32767, 65535};
    const quint16 alpha8[]{0, 1, 63, 127, 128, 254, 255, 255};
    for (int y = 0; y < area.height(); ++y) for (int x = 0; x < area.width(); ++x) {
        const auto scale = words ? 257 : 1;
        const quint16 a = words ? alpha16[x % 8] : alpha8[x % 8];
        const quint16 r = (19 + y * 11) * scale;
        const quint16 g = (x % 3 == 0 ? 19 + y * 11 : 91) * scale;
        const quint16 b = (x % 4 == 0 ? 19 + y * 11 : 219) * scale;
        initial.replace((y * area.width() + x) * stride, stride, Gpu::rgbaBytes({r, g, b, a}, format));
    }
    QByteArray nativeInitial = initial;
    exchangeRgbaBgra(nativeInitial, stride);
    KisPaintDeviceSP expectedDevice(new KisPaintDevice(cs));
    expectedDevice->writeBytes(reinterpret_cast<const quint8 *>(nativeInitial.constData()), area);
    QByteArray alpha(area.width() * area.height(), '\0');
    const quint8 coverages[]{0, 1, 64, 127, 128, 192, 254, 255};
    for (int i = 0; i < alpha.size(); ++i) alpha[i] = char(coverages[(i / 8 + i) % 8]);
    KisSelectionSP selection;
    if (selected) {
        selection = new KisSelection;
        selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), area);
    }
    auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    if (id == QLatin1String("fastcoloroverlay")) {
        config->setProperty("color", QColor(62, 140, 236, 193));
        config->setProperty("opacity", 63);
        const QString mode = scenario.endsWith(QLatin1String("tint")) ? COMPOSITE_TINT :
            scenario.endsWith(QLatin1String("multiply")) ? COMPOSITE_MULT :
            scenario.endsWith(QLatin1String("copy")) ? COMPOSITE_COPY : COMPOSITE_OVER;
        config->setProperty("compositeop", mode);
        config->setProperty("compositeopCustom", mode);
    }
    const auto expectedPixels = [&] {
        QByteArray bytes(initial.size(), '\0');
        expectedDevice->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    const auto nativeApply = [&] {
        const QRect processRect = id == QLatin1String("resettransparent") ? area : expectedDevice->exactBounds().intersected(area);
        filter->process(expectedDevice, expectedDevice, selection, processRect, config->cloneWithResourcesSnapshot());
    };
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = area.size(); raster.format = format; raster.colorProfile = cs->profile()->rawData();
    Gpu::RasterLayer imported;
    imported.name = QStringLiteral("Color filter editing"); imported.uuid = QUuid::createUuid();
    Gpu::RgbaTile tile; tile.format = format; tile.rgba = QByteArray(Gpu::RgbaTile::byteCount(format), '\0');
    for (int y = 0; y < area.height(); ++y)
        tile.rgba.replace(y * Gpu::RgbaTile::Size * stride, area.width() * stride, initial.mid(y * area.width() * stride, area.width() * stride));
    imported.tiles.append(tile); raster.layers.append(imported);
    auto owner = Gpu::PaintImageSP::create(execution, raster);
    QVERIFY(owner);
    if (selected) QCOMPARE(owner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, alpha, area)), Gpu::Result::Success);
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->readRgbaPixels(layer, area, initial.size()).bytes, initial);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Color filter editing"), cs));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get()); QVERIFY(view);
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
    window->viewManager()->nodeManager()->slotUiActivatedNode(view->image()->root()->firstChild());
    auto *manager = window->viewManager()->filterManager();
    auto *actions = window->viewManager()->actionManager();
    auto *action = actions->actionByName(QStringLiteral("krita_filter_") + id); QVERIFY(action);
    const auto count = owner->operations().undoCount();
    if (id == QLatin1String("fastcoloroverlay")) {
        action->trigger();
        QDialog *dialog = nullptr;
        QTRY_VERIFY((dialog = window->findChild<QDialog *>(QStringLiteral("gpuColorOverlayDialog"))));
        auto *widget = dialog->findChild<KisConfigWidget *>(); QVERIFY(widget);
        widget->setConfiguration(config);
        dialog->reject();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(owner->operations().undoCount(), count);
        QCOMPARE(owner->readRgbaPixels(layer, area, initial.size()).bytes, initial);
        manager->showFilterDialog(id, config);
        QTRY_VERIFY((dialog = window->findChild<QDialog *>(QStringLiteral("gpuColorOverlayDialog"))));
        dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    } else action->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), count + 1);
    QVERIFY(!manager->isStrokeRunning());
    nativeApply();
    const auto compare = [&](const QString &stage, const QByteArray &actual) {
        const auto expected = expectedPixels();
        const QString root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!root.isEmpty()) {
            QDir().mkpath(root);
            for (const auto &entry : {qMakePair(QStringLiteral("native"), expected), qMakePair(QStringLiteral("gpu"), actual)}) {
                QFile file(root + QStringLiteral("/filter-") + QString::fromLatin1(QTest::currentDataTag()) + QLatin1Char('-') + stage + QLatin1Char('-') + entry.first + QStringLiteral(".rgba"));
                if (!file.open(QIODevice::WriteOnly) || file.write(entry.second) != entry.second.size()) return false;
            }
        }
        if (actual != expected) for (int i = 0; i < actual.size(); ++i) if (actual[i] != expected[i]) {
            qWarning() << stage << "pixel" << (i / stride) % area.width() << (i / stride) / area.width()
                       << "GPU" << actual.mid(i / stride * stride, stride).toHex() << "native" << expected.mid(i / stride * stride, stride).toHex(); break;
        }
        return actual == expected;
    };
    QVERIFY(compare(QStringLiteral("first"), owner->readRgbaPixels(layer, area, initial.size()).bytes));
    const auto first = expectedPixels();
    actions->actionByName(QStringLiteral("edit_undo"))->trigger();
    QCOMPARE(owner->readRgbaPixels(layer, area, initial.size()).bytes, initial);
    actions->actionByName(QStringLiteral("edit_redo"))->trigger();
    QCOMPARE(owner->readRgbaPixels(layer, area, initial.size()).bytes, first);
    // Accepted settings remain frozen when the caller edits its configuration.
    auto accepted = config->clone();
    config->setProperty("opacity", 1);
    config->setProperty("color", QColor(Qt::green));
    auto *again = actions->actionByName(QStringLiteral("filter_apply_again")); QVERIFY(again && again->isEnabled());
    again->trigger();
    config = accepted;
    nativeApply();
    QVERIFY(compare(QStringLiteral("again"), owner->readRgbaPixels(layer, area, initial.size()).bytes));
    const auto twice = expectedPixels();
    QTemporaryDir directory;
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("filters.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
    const auto decoded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(decoded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(decoded.diagnostic));
    auto reopened = Gpu::PaintImageSP::create(execution, decoded.document.image); QVERIFY(reopened);
    const auto reopenedLayer = reopened->operations().activeLayer();
    QCOMPARE(reopened->readRgbaPixels(reopenedLayer, area, initial.size()).bytes, twice);
    lifetime->setCurrentImage(KisImage::fromGpuImage(reopened, QStringLiteral("Reopened filters"), cs));
    QTRY_COMPARE(view->image().data(), lifetime->image().data());
    window->viewManager()->nodeManager()->slotUiActivatedNode(view->image()->root()->firstChild());
    // KRA retains the selection as well as the edited pixels.
    manager->apply(config);
    nativeApply();
    QVERIFY(compare(QStringLiteral("reedit"), reopened->readRgbaPixels(reopenedLayer, area, initial.size()).bytes));
    actions->actionByName(QStringLiteral("edit_undo"))->trigger();
    QCOMPARE(reopened->readRgbaPixels(reopenedLayer, area, initial.size()).bytes, twice);
    actions->actionByName(QStringLiteral("edit_redo"))->trigger();
    QCOMPARE(reopened->readRgbaPixels(reopenedLayer, area, initial.size()).bytes, expectedPixels());
}
