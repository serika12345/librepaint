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
#include <gpu/KisGpuPaintImage.h>
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
        Gpu::PaintImage image(execution, QSize(64, 64));
        const auto layerId = image.operations().activeLayer();
        const QRect transferRegion(0, 0, Gpu::RgbaTile::Size, Gpu::RgbaTile::Size);
        QCOMPARE(image.writeRgbaPixels(layerId, transferRegion, original), Gpu::Result::Success);
        QCOMPARE(image.operations().undoCount(), 0);
        QVERIFY(!image.operations().isModified());
        const auto before = image.operations().snapshot();
        auto pixels = execution.readRgbaPixels(image.handle(), before, transferRegion, layerId, Gpu::RgbaTile::ByteCount);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.bytes.size(), Gpu::RgbaTile::ByteCount);
        QCOMPARE(pixels.bytes, original);

        QByteArray bgra = pixels.bytes;
        for (qsizetype i = 0; i < bgra.size(); i += 4) std::swap(bgra[i], bgra[i + 2]);
        KisPaintDeviceSP cpu = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
        cpu->setDefaultBounds(new CanvasBounds);
        cpu->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), 0, 0, Gpu::RgbaTile::Size, Gpu::RgbaTile::Size);
        auto configuration = filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
        configuration->setProperty("radius", 16);
        KoProgressBar progress;
        KoProgressUpdater updater(&progress, KoProgressUpdater::Unthreaded);
        filter->processImpl(cpu, QRect(0, 0, 64, 64), configuration, updater.startSubtask());
        QByteArray edited(64 * 64 * 4, Qt::Uninitialized);
        cpu->readBytes(reinterpret_cast<quint8 *>(edited.data()), 0, 0, 64, 64);
        cpu.clear();
        for (qsizetype i = 0; i < edited.size(); i += 4) std::swap(edited[i], edited[i + 2]);
        auto expected = original;
        for (int y = 0; y < 64; ++y) expected.replace(y * Gpu::RgbaTile::Size * 4, 64 * 4, edited.mid(y * 64 * 4, 64 * 4));
        QCOMPARE(image.replaceRgbaPixels(layerId, QRect(0, 0, 64, 64), edited), Gpu::Result::Success);
        QCOMPARE(image.operations().undoCount(), 1);
        pixels = execution.readRgbaPixels(image.handle(), image.operations().snapshot(), transferRegion, layerId, Gpu::RgbaTile::ByteCount);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.bytes, expected);
        QCOMPARE(pixels.bytes.first(4), QByteArray::fromHex("316ac200"));
        QCOMPARE(pixels.bytes.mid((32 * Gpu::RgbaTile::Size + 32) * 4, 4), QByteArray::fromHex("316ac2ff"));
        QCOMPARE(pixels.bytes.mid((100 * Gpu::RgbaTile::Size + 100) * 4, 4), QByteArray::fromHex("316ac2ff"));
        QCOMPARE(image.undo(), Gpu::Result::Success);
        pixels = execution.readRgbaPixels(image.handle(), image.operations().snapshot(), transferRegion, layerId, Gpu::RgbaTile::ByteCount);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.bytes, original);
        QCOMPARE(image.redo(), Gpu::Result::Success);
        pixels = execution.readRgbaPixels(image.handle(), image.operations().snapshot(), transferRegion, layerId, Gpu::RgbaTile::ByteCount);
        QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
        QCOMPARE(pixels.bytes, expected);
    }
};

QTEST_MAIN(ExternalFilterContract)
#include "ExternalFilterContract.moc"
