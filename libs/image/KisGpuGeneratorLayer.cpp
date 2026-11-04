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
#include <stdexcept>

KisFilterConfigurationSP gpuGeneratorConfiguration(const LibrePaint::Image::Gpu::CpuLayerDefinition &definition)
{
    if (definition.feature != LibrePaint::Image::Gpu::LayerFeature::Generator) return {};
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
                            const KoColorSpace *space, qsizetype maximumBytes,
                            const KisGpuCpuLayerContext &context)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!layer.cpuDefinition || layer.cpuDefinition->feature != Gpu::LayerFeature::Generator ||
        !space || (space->pixelSize() != 4 && space->pixelSize() != 8)) return false;
    const auto &definition = *layer.cpuDefinition;
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
    if (!kisGpuRenderCpuLayer(Gpu::LayerFeature::Generator, layer, size, space, maximumBytes,
        [&](KisPaintDeviceSP) -> KisPaintDeviceSP {
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
            return result;
        }, context)) return false;
    return true;
}

bool writeGpuGeneratorConfiguration(KisGeneratorLayer *layer, const KisFilterConfigurationSP &configuration)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto image = layer ? layer->image().toStrongRef() : KisImageSP{};
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP{};
    if (!owner || !layer->parent() || image->isSynchronizingGpuNodes()) return false;
    if (!configuration) throw std::invalid_argument("CPU generator requires a configuration");
    const auto version = owner->operations().cpuLayerEditTarget() ? owner->operations().preview() : owner->operations().snapshot();
    for (const auto &value : version->layers) {
        if (value.uuid != layer->uuid() || !value.cpuDefinition ||
            value.cpuDefinition->feature != Gpu::LayerFeature::Generator) continue;
        Gpu::RasterLayer rendered;
        rendered.name = value.name;
        rendered.uuid = value.uuid;
        rendered.cpuDefinition = value.cpuDefinition;
        rendered.cpuDefinition->id = configuration->name();
        rendered.cpuDefinition->version = configuration->version();
        rendered.cpuDefinition->xml = configuration->toXML().toUtf8();
        if (rendered.cpuDefinition->id == value.cpuDefinition->id && rendered.cpuDefinition->version == value.cpuDefinition->version &&
            rendered.cpuDefinition->xml == value.cpuDefinition->xml) return true;
        if (!renderGpuGeneratorLayer(rendered, version->size, image->colorSpace(), owner->limits().rasterBytes, {owner, version, {}}))
            throw std::runtime_error("CPU generator rendering failed");
        if (owner->replaceCpuLayer(value.id, rendered, version) != Gpu::Result::Success)
            throw std::runtime_error("CPU generator upload failed");
        return true;
    }
    return false;
}
