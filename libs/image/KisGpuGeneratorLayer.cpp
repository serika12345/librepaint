/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuGeneratorLayer.h"
#include <generator/kis_generator.h>
#include <generator/kis_generator_registry.h>
#include <generator/kis_generator_layer.h>
#include <filter/kis_filter_configuration.h>
#include <KisGlobalResourcesInterface.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_processing_information.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_default_bounds.h>
#include <gpu/KisGpuPaintImage.h>
#include <KoColorSpace.h>
#include <KoColor.h>
#include <QDomDocument>
#include <cstring>
#include <stdexcept>

KisFilterConfigurationSP gpuGeneratorConfiguration(const LibrePaint::Image::Gpu::GeneratorDefinition &definition)
{
    const auto generator = KisGeneratorRegistry::instance()->value(definition.id);
    if (!generator) return {};
    auto configuration = generator->factoryConfiguration(KisGlobalResourcesInterface::instance());
    QDomDocument xml;
    if (!configuration || !xml.setContent(definition.xml)) return {};
    if (xml.documentElement().tagName() == QLatin1String("filterconfig")) configuration->fromLegacyXML(xml.documentElement());
    else configuration->fromXML(xml.documentElement());
    return configuration->cloneWithResourcesSnapshot();
}

bool renderGpuGeneratorLayer(LibrePaint::Image::Gpu::RasterLayer &layer, QSize size,
                            const KoColorSpace *space, qsizetype maximumBytes)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!layer.generator || !space || (space->pixelSize() != 4 && space->pixelSize() != 8)) return false;
    const auto &definition = *layer.generator;
    const auto configuration = gpuGeneratorConfiguration(definition);
    const auto generator = KisGeneratorRegistry::instance()->value(definition.id);
    if (!configuration || !generator || !definition.selection || definition.selection->isGpuGenerated()) return false;
    const auto format = space->pixelSize() == 8 ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const QRect bounds(QPoint(), size);
    const QRect area = generator->generatedRect(bounds, configuration).intersected(bounds);
    const int side = Gpu::RgbaTile::Size;
    const qint64 columns = (qint64(size.width()) + side - 1) / side;
    const qint64 rows = (qint64(size.height()) + side - 1) / side;
    if (columns * rows > maximumBytes / Gpu::RgbaTile::byteCount(format)) return false;
    KisPaintDeviceSP generated = new KisPaintDevice(space);
    generator->generate(KisProcessingInformation(generated, area.topLeft(), {}), area.size(), configuration);
    KisSelectionSP selection = new KisSelection();
    auto coverage = selection->pixelSelection();
    const quint8 defaultCoverage = definition.selection->defaultCoverage();
    coverage->setDefaultPixel(KoColor(&defaultCoverage, coverage->colorSpace()));
    for (const auto point : definition.selection->tileCoordinates()) {
        QByteArray bytes(side * side, Qt::Uninitialized);
        for (int y = 0; y < side; ++y)
            for (int x = 0; x < side; ++x)
                bytes[y * side + x] = char(definition.selection->coverage(point.x() * side + x, point.y() * side + y));
        coverage->writeBytes(reinterpret_cast<const quint8 *>(bytes.constData()), point.x() * side, point.y() * side, side, side);
    }
    KisPaintDeviceSP result = new KisPaintDevice(space);
    KisPainter::copyAreaOptimized(bounds.topLeft(), generated, result, bounds, selection);
    layer.tiles.clear();
    layer.defaultPixel = {};
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < columns; ++x) {
            QByteArray bytes(Gpu::RgbaTile::byteCount(format), Qt::Uninitialized);
            result->readBytes(reinterpret_cast<quint8 *>(bytes.data()), x * side, y * side, side, side);
            if (format == Gpu::PixelFormat::Rgba16) {
                auto *words = reinterpret_cast<quint16 *>(bytes.data());
                for (qsizetype i = 0; i < bytes.size() / 2; i += 4) std::swap(words[i], words[i + 2]);
            } else {
                for (qsizetype i = 0; i < bytes.size(); i += 4) std::swap(bytes[i], bytes[i + 2]);
            }
            layer.tiles.append({{x, y}, std::move(bytes), format});
        }
    }
    layer.generator->resolved = true;
    return true;
}

bool writeGpuGeneratorConfiguration(KisGeneratorLayer *layer, const KisFilterConfigurationSP &configuration)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto image = layer ? layer->image().toStrongRef() : KisImageSP{};
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP{};
    if (!owner || !layer->parent() || image->isSynchronizingGpuNodes()) return false;
    if (!configuration) throw std::invalid_argument("CPU generator requires a configuration");
    const auto version = owner->operations().generatorEditTarget() ? owner->operations().preview() : owner->operations().snapshot();
    for (const auto &value : version->layers) {
        if (value.uuid != layer->uuid() || !value.generator) continue;
        Gpu::RasterLayer rendered;
        rendered.name = value.name;
        rendered.uuid = value.uuid;
        rendered.generator = value.generator;
        rendered.generator->id = configuration->name();
        rendered.generator->version = configuration->version();
        rendered.generator->xml = configuration->toXML().toUtf8();
        if (rendered.generator->id == value.generator->id && rendered.generator->version == value.generator->version &&
            rendered.generator->xml == value.generator->xml) return true;
        if (!renderGpuGeneratorLayer(rendered, version->size, image->colorSpace(), owner->limits().rasterBytes))
            throw std::runtime_error("CPU generator rendering failed");
        if (owner->replaceGeneratedLayer(value.id, rendered) != Gpu::Result::Success)
            throw std::runtime_error("CPU generator upload failed");
        return true;
    }
    return false;
}
