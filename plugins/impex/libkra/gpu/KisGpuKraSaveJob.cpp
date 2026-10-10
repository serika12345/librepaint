/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuKraSaveJob.h"
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpRegistry.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <cmath>
#include <utility>

namespace {
constexpr int TileSize = 64;
bool validXmlText(const QString &text)
{
    for (qsizetype i = 0; i < text.size(); ++i) {
        const auto value = text[i].unicode();
        if (QChar::isHighSurrogate(value)) {
            if (++i == text.size() || !text[i].isLowSurrogate()) return false;
        } else if (QChar::isLowSurrogate(value) || value >= 0xFFFE
                   || (value < 0x20 && value != 9 && value != 10 && value != 13)) {
            return false;
        }
    }
    return true;
}

void writeTiles(KisPaintDeviceSP destination, const KisGpuEditSession::Recovery &source, bool mask)
{
    for (const auto &tile : source.tiles) {
        QByteArray pixels;
        if (mask) {
            pixels.resize(TileSize * TileSize);
            for (qsizetype i = 0; i < pixels.size(); ++i) pixels[i] = tile.pixels[i * 4 + 3];
        } else {
            pixels = tile.pixels;
            for (qsizetype i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
        }
        destination->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()),
            tile.coordinate.x() * TileSize, tile.coordinate.y() * TileSize, TileSize, TileSize);
    }
}
}

struct KisGpuKraSaveJob::Private {
    Metadata metadata;
    QString path, message;
    State state = State::Reading;
    Error error = Error::None;
    KisGpuDocumentReadback::Error readbackError = KisGpuDocumentReadback::Error::None;
    std::unique_ptr<KisGpuDocumentReadback> readback;
    std::unique_ptr<KisDocument> document;
    QMetaObject::Connection completion;

    void fail(Error reason, QString diagnostic)
    {
        state = State::Failed;
        error = reason;
        message = std::move(diagnostic);
        readback.reset();
    }

    void prepare(const KisGpuLayerDocument::Recovery &snapshot)
    {
        const auto registry = KoColorSpaceRegistry::instance();
        const auto profile = registry->p709SRGBProfile();
        const auto colorSpace = profile ? registry->rgb8(profile) : nullptr;
        if (!colorSpace) {
            fail(Error::SaveFailed, QStringLiteral("The RGBA8 sRGB profile is unavailable."));
            return;
        }
        KisImageSP image = new KisImage(nullptr, metadata.canvasSize.width(), metadata.canvasSize.height(),
                                       colorSpace, metadata.name);
        image->setResolution(metadata.xResolution, metadata.yResolution);
        for (const auto &record : snapshot.history[snapshot.cursor]) {
            KisPaintLayerSP layer = new KisPaintLayer(image, record->name, record->opacity);
            layer->setCompositeOpId(record->operation == KisGpuTileStore::CompositeOp::Erase
                                        ? COMPOSITE_ERASE : COMPOSITE_OVER);
            layer->setVisible(record->visible, true);
            layer->setUserLocked(record->locked);
            writeTiles(layer->paintDevice(), *record->pixels, false);
            image->addNode(layer, image->root());
            if (record->mask) {
                KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
                mask->initSelection(layer);
                auto selection = mask->selection()->pixelSelection();
                selection->setDefaultPixel(KoColor::createTransparent(selection->colorSpace()));
                writeTiles(selection, *record->mask, true);
                image->addNode(mask, layer);
            }
        }
        document.reset(KisPart::instance()->createDocument());
        document->setFileBatchMode(true);
        document->setCurrentImage(image, false);
        image->initialRefreshGraph();
        state = State::Preparing;
    }
};

KisGpuKraSaveJob::KisGpuKraSaveJob(std::shared_ptr<KisGpuTileStore> store,
    QVector<KisGpuLayerDocument::Layer> layers, KisGpuLayerDocument::LayerId nextId,
    Metadata metadata, QString path, quint64 maximumSnapshotBytes)
    : d(std::make_unique<Private>())
{
    if (metadata.canvasSize.width() <= 0 || metadata.canvasSize.height() <= 0 || path.isEmpty()
        || !std::isfinite(metadata.xResolution) || metadata.xResolution <= 0
        || !std::isfinite(metadata.yResolution) || metadata.yResolution <= 0) {
        d->fail(Error::InvalidInput, QStringLiteral("Invalid canvas size, resolution or destination."));
        return;
    }
    if (!validXmlText(metadata.name)) {
        d->fail(Error::InvalidInput, QStringLiteral("The document name cannot be represented in KRA XML."));
        return;
    }
    for (const auto &layer : layers) {
        if (!validXmlText(layer.name)) {
            d->fail(Error::InvalidInput, QStringLiteral("A layer name cannot be represented in KRA XML."));
            return;
        }
    }
    d->metadata = std::move(metadata);
    d->path = std::move(path);
    d->readback = std::make_unique<KisGpuDocumentReadback>(std::move(store), std::move(layers),
                                                         nextId, maximumSnapshotBytes);
    if (d->readback->status() == KisGpuTileStore::Status::Failed) {
        d->readbackError = d->readback->error();
        d->fail(Error::ReadbackFailed, QStringLiteral("The frozen GPU document could not be read."));
    }
}

KisGpuKraSaveJob::~KisGpuKraSaveJob()
{
    QObject::disconnect(d->completion);
    if (d->document) d->document->waitForSavingToComplete();
}

void KisGpuKraSaveJob::poll()
{
    if (d->state == State::Reading) {
        d->readback->poll();
        if (d->readback->status() == KisGpuTileStore::Status::Pending) return;
        if (d->readback->status() == KisGpuTileStore::Status::Failed) {
            d->readbackError = d->readback->error();
            d->fail(Error::ReadbackFailed, QStringLiteral("The frozen GPU document could not be read."));
            return;
        }
        const auto snapshot = *d->readback->result();
        d->readback.reset();
        d->prepare(snapshot);
    }
    if (d->state == State::Preparing && d->document->image()->isIdle()) {
        d->completion = QObject::connect(d->document.get(), &KisDocument::sigCompleteBackgroundSaving,
            d->document.get(), [state = d.get()](const KritaUtils::ExportFileJob &,
                KisImportExportErrorCode result, const QString &message, const QString &) {
                if (result.isOk()) state->state = State::Succeeded;
                else state->fail(Error::SaveFailed, message);
            });
        d->state = State::Writing;
        if (!d->document->exportDocument(d->path, "application/x-krita", false, false)
            && d->state == State::Writing) {
            d->fail(Error::SaveFailed, QStringLiteral("The native KRA writer could not start."));
        }
    }
}

KisGpuKraSaveJob::State KisGpuKraSaveJob::state() const { return d->state; }
KisGpuKraSaveJob::Error KisGpuKraSaveJob::error() const { return d->error; }
KisGpuDocumentReadback::Error KisGpuKraSaveJob::readbackError() const { return d->readbackError; }
QString KisGpuKraSaveJob::errorMessage() const { return d->message; }
