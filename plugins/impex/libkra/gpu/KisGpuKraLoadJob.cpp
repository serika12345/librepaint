/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuKraLoadJob.h"
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <metadata/KoDocumentInfo.h>
#include <canvas/kis_grid_config.h>
#include <canvas/kis_guides_config.h>
#include <canvas/KisMirrorAxisConfig.h>
#include <QDomDocument>
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoCompositeOpRegistry.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_psd_layer_style.h>
#include <KisRegion.h>
#include <QSet>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace {
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
constexpr int TileSize = 64;
bool supportedColorSpace(const KoColorSpace *cs)
{
    const auto profile = KoColorSpaceRegistry::instance()->p709SRGBProfile();
    return cs && cs->pixelSize() == 4 && cs->colorModelId() == RGBAColorModelID
        && cs->colorDepthId() == Integer8BitsColorDepthID && cs->profile() && profile
        && cs->profile()->rawData() == profile->rawData();
}
bool zeroDefault(KisPaintDeviceSP device)
{
    const auto pixel = device->defaultPixel();
    const auto zero = KoColor::createTransparent(device->colorSpace());
    return std::memcmp(pixel.data(), zero.data(), device->pixelSize()) == 0;
}
bool supportedNode(KisNodeSP node)
{
    return node->keyframeChannels().isEmpty()
        && node->colorLabelIndex() == 0;
}
}

struct KisGpuKraLoadJob::Private {
    QString path;
    quint64 maximumBytes = 0, usedBytes = 0;
    State state = State::Reading;
    Error error = Error::None;
    KisGpuTileStore::Error gpuError = KisGpuTileStore::Error::None;
    std::optional<KisGpuKraMetadata> metadata;
    std::shared_ptr<Document> document;
    std::optional<Document::Recovery> pendingRecovery;

    void fail(Error reason)
    {
        state = State::Failed;
        error = reason;
        metadata.reset();
        pendingRecovery.reset();
        document.reset();
    }
    bool reserve(quint64 count, quint64 stride)
    {
        if (count > (maximumBytes - usedBytes) / stride) {
            fail(Error::SnapshotBudgetExceeded);
            return false;
        }
        usedBytes += count * stride;
        return true;
    }
    Document::RasterRecovery readRaster(KisPaintDeviceSP source, bool mask)
    {
        if (!reserve(1, sizeof(Session::Recovery))) return {};
        auto result = std::make_shared<Session::Recovery>();
        QSet<QPoint> coordinates;
        const auto floorTile = [](qint64 coordinate) {
            return coordinate >= 0 ? coordinate / TileSize : (coordinate - TileSize + 1) / TileSize;
        };
        for (const auto &rectangle : source->region().rects()) {
            for (qint64 y = floorTile(rectangle.top()); y <= floorTile(rectangle.bottom()); ++y) {
                for (qint64 x = floorTile(rectangle.left()); x <= floorTile(rectangle.right()); ++x) {
                    const QPoint coordinate(x, y);
                    if (coordinates.contains(coordinate)) continue;
                    if (x * TileSize < std::numeric_limits<int>::min()
                        || x * TileSize + TileSize - 1 > std::numeric_limits<int>::max()
                        || y * TileSize < std::numeric_limits<int>::min()
                        || y * TileSize + TileSize - 1 > std::numeric_limits<int>::max()) {
                        fail(Error::UnsupportedDocument);
                        return {};
                    }
                    if (!reserve(1, KisGpuTileStore::TileBytes + sizeof(Session::RecoveryTile) + sizeof(QPoint))) return {};
                    coordinates.insert(coordinate);
                    QByteArray pixels(KisGpuTileStore::TileBytes, '\0');
                    const QRect bounds(int(x * TileSize), int(y * TileSize), TileSize, TileSize);
                    if (mask) {
                        quint8 alpha[TileSize * TileSize];
                        source->readBytes(alpha, bounds);
                        for (qsizetype i = 0; i < TileSize * TileSize; ++i) pixels[i * 4 + 3] = char(alpha[i]);
                    } else {
                        source->readBytes(reinterpret_cast<quint8 *>(pixels.data()), bounds);
                        for (qsizetype i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
                    }
                    result->tiles.append({coordinate, std::move(pixels)});
                }
            }
        }
        return result;
    }
    std::optional<Document::Recovery> readNative()
    {
        std::unique_ptr<KisDocument> native(KisPart::instance()->createDocument());
        native->setFileBatchMode(true);
        if (!native->loadNativeFormat(path)) { fail(Error::LoadFailed); return {}; }
        auto image = native->image();
        if (!image) { fail(Error::LoadFailed); return {}; }
        image->waitForDone();
        if (!supportedColorSpace(image->colorSpace()) || image->width() <= 0 || image->height() <= 0
            || !std::isfinite(image->xRes()) || image->xRes() <= 0
            || !std::isfinite(image->yRes()) || image->yRes() <= 0
            || image->defaultProjectionColor().opacityU8() || image->globalSelection()
            || !native->gridConfig().isDefault() || !native->guidesConfig().isDefault()
            || !native->mirrorAxisConfig().isDefault()) {
            fail(Error::UnsupportedDocument); return {};
        }
        auto info = KisDocument::createDomDocument(QStringLiteral("document-info"),
            QStringLiteral("document-info"), QStringLiteral("1.1"));
        const auto infoXml = native->documentInfo()->save(info, true, false).toByteArray();
        if (!reserve(quint64(infoXml.size()), 1)) return {};
        Document::Recovery recovery;
        if (!reserve(1, sizeof(Document::RecoveryFrame))) return {};
        recovery.history.append(Document::RecoveryFrame{});
        for (KisNodeSP node = image->root()->firstChild(); node; node = node->nextSibling()) {
            auto layer = dynamic_cast<KisPaintLayer *>(node.data());
            if (!layer || !supportedNode(node) || layer->channelLockFlags().count(false)
                || layer->channelFlags().count(false) || layer->onionSkinEnabled()
                || layer->paintDevice()->keyframeChannel() || !supportedColorSpace(layer->colorSpace())
                || !zeroDefault(layer->paintDevice()) || (layer->layerStyle() && !layer->layerStyle()->isEmpty())
                || (layer->compositeOpId() != COMPOSITE_OVER && layer->compositeOpId() != COMPOSITE_ERASE)
                || layer->childCount() > 1) {
                fail(Error::UnsupportedDocument); return {};
            }
            if (!reserve(1, sizeof(Document::RecoveryLayer) + sizeof(std::shared_ptr<const Document::RecoveryLayer>))
                || !reserve(quint64(layer->name().size()), sizeof(QChar))) return {};
            auto record = std::make_shared<Document::RecoveryLayer>();
            record->id = recovery.nextId++;
            record->name = layer->name();
            record->opacity = layer->opacity();
            record->visible = layer->visible();
            record->locked = layer->userLocked();
            record->operation = layer->compositeOpId() == COMPOSITE_ERASE
                ? KisGpuTileStore::CompositeOp::Erase : KisGpuTileStore::CompositeOp::Over;
            record->pixels = readRaster(layer->paintDevice(), false);
            if (!record->pixels) return {};
            if (layer->childCount()) {
                auto mask = dynamic_cast<KisTransparencyMask *>(layer->firstChild().data());
                if (!mask || !supportedNode(mask) || !mask->visible() || mask->userLocked() || mask->opacity() != 255
                    || !mask->selection() || mask->selection()->hasShapeSelection()) {
                    fail(Error::UnsupportedDocument); return {};
                }
                auto selection = mask->selection()->pixelSelection();
                if (selection->pixelSize() != 1 || !zeroDefault(selection)) {
                    fail(Error::UnsupportedDocument); return {};
                }
                record->mask = readRaster(selection, true);
                if (!record->mask) return {};
            }
            recovery.history[0].append(std::move(record));
        }
        if (!Document::validRecovery(recovery, 0, maximumBytes - quint64(infoXml.size()))) {
            fail(Error::SnapshotBudgetExceeded); return {};
        }
        metadata = KisGpuKraMetadata{image->size(), image->objectName(), image->xRes(), image->yRes(), infoXml};
        return recovery;
    }
};

KisGpuKraLoadJob::KisGpuKraLoadJob(std::shared_ptr<Document> document, QString path,
    quint64 maximumRecoveryBytes) : d(std::make_unique<Private>())
{
    if (!document || path.isEmpty() || document->state() != Document::State::Idle
        || document->historySize() != 1 || !document->layers().empty()
        || document->recovery().nextId != 1) { d->fail(Error::InvalidInput); return; }
    d->document = std::move(document);
    d->path = std::move(path);
    d->maximumBytes = maximumRecoveryBytes;
}
KisGpuKraLoadJob::~KisGpuKraLoadJob() = default;
void KisGpuKraLoadJob::poll()
{
    if (d->state == State::Reading) {
        if (d->document->state() == Document::State::Suspended || d->document->state() == Document::State::Restoring) return;
        auto recovery = d->readNative();
        if (!recovery) return;
        d->pendingRecovery = std::move(*recovery);
        d->state = State::Restoring;
    }
    if (d->state == State::Restoring) {
        if (d->document->state() == Document::State::Suspended) return;
        if (d->pendingRecovery) {
            if (d->document->state() == Document::State::Restoring) return;
            const auto result = d->document->restoreInitial(std::move(*d->pendingRecovery));
            d->pendingRecovery.reset();
            if (result != Document::Result::Accepted) {
                d->gpuError = d->document->recoveryError();
                d->fail(result == Document::Result::GpuRejected ? Error::GpuRejected : Error::InvalidInput);
            }
            return;
        }
        d->document->poll();
        if (d->document->state() == Document::State::Failed) {
            d->gpuError = d->document->recoveryError();
            d->fail(Error::GpuRejected);
        } else if (d->document->state() == Document::State::Idle) {
            d->state = State::Succeeded;
        }
    }
}
KisGpuKraLoadJob::State KisGpuKraLoadJob::state() const { return d->state; }
KisGpuKraLoadJob::Error KisGpuKraLoadJob::error() const { return d->error; }
KisGpuTileStore::Error KisGpuKraLoadJob::gpuError() const { return d->gpuError; }
std::optional<KisGpuKraMetadata> KisGpuKraLoadJob::metadata() const
{
    return d->state == State::Succeeded ? d->metadata : std::nullopt;
}
