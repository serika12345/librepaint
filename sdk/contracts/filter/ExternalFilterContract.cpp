/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QPluginLoader>
#include <QTest>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintDocument.h>
#include <filter/kis_filter.h>
#include <filter/kis_filter_registry.h>
#include <filter/kis_filter_configuration.h>
#include <KoColorSpaceRegistry.h>
#include <KoProgressBar.h>
#include <KoProgressUpdater.h>
#include <KoUpdater.h>
#include <KisGlobalResourcesInterface.h>
#include <kis_default_bounds.h>
#include <kis_paint_device.h>

namespace Gpu = Krita::Image::Gpu;
namespace Document = Krita::Document::Gpu;

class CanvasBounds final : public KisDefaultBounds
{
public:
    QRect bounds() const override { return QRect(0, 0, 64, 64); }
    QRect imageBorderRect() const override { return bounds(); }
};

class ExternalFilterContract : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void rebuiltFilterPreservesItsRegistrationAndEditsGpuPixels()
    {
        const auto path = qEnvironmentVariable("LIBREPAINT_EXTERNAL_FILTER");
        QVERIFY2(!path.isEmpty(), "Set LIBREPAINT_EXTERNAL_FILTER to the separately rebuilt filter");
        QPluginLoader plugin(path);
        const auto metadata = plugin.metaData().value(QStringLiteral("MetaData")).toObject();
        QCOMPARE(metadata.value(QStringLiteral("Id")).toString(), QStringLiteral("Round Corners Filter"));
        QVERIFY(metadata.value(QStringLiteral("X-KDE-ServiceTypes")).toArray().contains(QStringLiteral("Krita/Filter")));
        QVERIFY2(plugin.instance(), qPrintable(plugin.errorString()));
        const auto filter = KisFilterRegistry::instance()->value(QStringLiteral("roundcorners"));
        QVERIFY(filter);

        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        Gpu::Execution execution(device);
        const auto original = QByteArray::fromHex("316ac2ff").repeated(Gpu::RgbaTile::Size * Gpu::RgbaTile::Size);
        Gpu::RasterImage image;
        image.size = QSize(64, 64);
        Gpu::RasterLayer layer;
        layer.name = QStringLiteral("External filter");
        layer.tiles.append({QPoint(), original});
        image.layers.append(layer);
        Document::PaintDocument document(execution, image);
        const auto layerId = document.operations().activeLayer();
        const auto before = document.operations().snapshot();
        auto pixels = execution.readPixels(document.handle(), before, QRect(0, 0, 64, 64), {layerId}, false);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.tiles.size(), 1);
        QCOMPARE(pixels.tiles.first().rgba, original);

        QByteArray bgra = pixels.tiles.first().rgba;
        for (qsizetype i = 0; i < bgra.size(); i += 4) std::swap(bgra[i], bgra[i + 2]);
        KisPaintDeviceSP cpu = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
        cpu->setDefaultBounds(new CanvasBounds);
        cpu->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), 0, 0, Gpu::RgbaTile::Size, Gpu::RgbaTile::Size);
        auto configuration = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
        configuration->setProperty("radius", 16);
        KoProgressBar progress;
        KoProgressUpdater updater(&progress, KoProgressUpdater::Unthreaded);
        filter->processImpl(cpu, QRect(0, 0, 64, 64), configuration, updater.startSubtask());
        cpu->readBytes(reinterpret_cast<quint8 *>(bgra.data()), 0, 0, Gpu::RgbaTile::Size, Gpu::RgbaTile::Size);
        cpu.clear();
        for (qsizetype i = 0; i < bgra.size(); i += 4) std::swap(bgra[i], bgra[i + 2]);
        QCOMPARE(document.replaceTiles(layerId, {{QPoint(), bgra}}), Gpu::Result::Success);
        QCOMPARE(document.operations().undoCount(), 1);
        pixels = execution.readPixels(document.handle(), document.operations().snapshot(), QRect(0, 0, 64, 64), {layerId}, false);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.tiles.first().rgba, bgra);
        QCOMPARE(pixels.tiles.first().rgba.first(4), QByteArray::fromHex("316ac200"));
        QCOMPARE(pixels.tiles.first().rgba.mid((32 * Gpu::RgbaTile::Size + 32) * 4, 4), QByteArray::fromHex("316ac2ff"));
        QCOMPARE(pixels.tiles.first().rgba.mid((100 * Gpu::RgbaTile::Size + 100) * 4, 4), QByteArray::fromHex("316ac2ff"));
        QCOMPARE(document.undo(), Gpu::Result::Success);
        pixels = execution.readPixels(document.handle(), document.operations().snapshot(), QRect(0, 0, 64, 64), {layerId}, false);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.tiles.first().rgba, original);
        QCOMPARE(document.redo(), Gpu::Result::Success);
        pixels = execution.readPixels(document.handle(), document.operations().snapshot(), QRect(0, 0, 64, 64), {layerId}, false);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.tiles.first().rgba, bgra);
    }
};

QTEST_MAIN(ExternalFilterContract)
#include "ExternalFilterContract.moc"
