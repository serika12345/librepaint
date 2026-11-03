/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSamplerStroke.h"
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoMixColorsOp.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_paint_device.h>
#include <strokes/kis_color_sampler_stroke_strategy.h>
#include <QPointer>
#include <QTimer>
#include <chrono>
#include <optional>

namespace Gpu = LibrePaint::Image::Gpu;

struct KisColorSamplerStroke::Private
{
    explicit Private(KisColorSamplerStroke *owner) : q(owner) {}
    KisColorSamplerStroke *q;
    KisImageSP image;
    KisStrokeId strokeId;
    QList<QMetaObject::Connection> connections;
    bool active = false;
    bool gpu = false;
    quint64 serial = 0;
    struct Sample {
        Gpu::Snapshot version;
        Gpu::LayerId layer = 0;
        QPoint point;
        KoColor currentColor;
    };
    Gpu::PaintImageSP owner;
    QUuid documentId;
    std::optional<Sample> sample, nextSample;
    std::future<Gpu::ColorSampleOutcome> readback;
    std::optional<KoColor> lastColor;
    QTimer poll;
    int radius = 1, blend = 100;

    void fail(const QString &diagnostic)
    {
        const QPointer<KisColorSamplerStroke> guard(q);
        q->cancel();
        if (guard) Q_EMIT q->sigSamplingFailed(diagnostic);
    }
    void complete()
    {
        if (active || sample || nextSample) return;
        const auto selected = lastColor;
        const QPointer<KisColorSamplerStroke> guard(q);
        q->cancel();
        if (guard && selected) Q_EMIT q->sigFinalColorSelected(*selected);
    }
    void advance()
    {
        if (!owner || !owner->handle() ||
            owner->operations().snapshot()->documentId() != documentId) {
            fail(QStringLiteral("The sampled document is no longer available"));
            return;
        }
        if (!sample) { complete(); return; }
        const auto current = *sample;
        const auto source = owner;
        if (!readback.valid())
            readback = source->execution().readColorSampleAsync(source->handle(), current.version,
                                                               current.layer, current.point, radius);
        const auto polled = source->execution().pollReadbacks();
        if (readback.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            if (polled.code != Gpu::ExecutionCode::Success && polled.code != Gpu::ExecutionCode::Busy)
                fail(polled.diagnostic);
            return;
        }
        const auto read = readback.get();
        if (read.code == Gpu::ExecutionCode::Busy) return;
        sample = std::move(nextSample);
        nextSample.reset();
        if (read.code != Gpu::ExecutionCode::Success) {
            fail(read.diagnostic);
            return;
        }
        if (!read.document || read.document != current.version) {
            fail(QStringLiteral("The sampled pixel version does not match the retained request"));
            return;
        }
        auto *registry = KoColorSpaceRegistry::instance();
        const auto format = read.document->format;
        const QString depth = format == Gpu::PixelFormat::Rgba16 ?
            QStringLiteral("U16") : QStringLiteral("U8");
        const KoColorSpace *space = nullptr;
        if (read.document->colorProfile.isEmpty()) {
            // Operation-only documents use the format's declared default encoding.
            space = format == Gpu::PixelFormat::Rgba16 ? registry->rgb16() : registry->rgb8();
        } else {
            const auto *profile = registry->createColorProfileFromData(
                QStringLiteral("RGBA"), depth, read.document->colorProfile);
            if (profile) space = registry->colorSpace(QStringLiteral("RGBA"), depth, profile);
        }
        if (!space || (!read.document->colorProfile.isEmpty() &&
            space->profile()->rawData() != read.document->colorProfile)) {
            fail(QStringLiteral("The retained sampling color encoding is unavailable"));
            return;
        }
        KoColor color(space);
        const auto &rgba = read.rgba;
        if (format == Gpu::PixelFormat::Rgba16) {
            const quint16 bgra[]{rgba[2], rgba[1], rgba[0], rgba[3]};
            color = KoColor(reinterpret_cast<const quint8 *>(bgra), space);
        } else {
            const quint8 bgra[]{quint8(rgba[2]), quint8(rgba[1]), quint8(rgba[0]), quint8(rgba[3])};
            color = KoColor(bgra, space);
        }
        if (blend < 100) {
            KoColor previous(current.currentColor);
            previous.convertTo(space);
            const quint8 *colors[]{previous.data(), color.data()};
            const qint16 weight = blend * 255 / 100;
            const qint16 weights[]{qint16(255 - weight), weight};
            space->mixColorsOp()->mixColors(colors, weights, 2, color.data());
        }
        const auto action = serial;
        const QPointer<KisColorSamplerStroke> guard(q);
        if (color.opacityU8()) {
            lastColor = color;
            Q_EMIT q->sigColorUpdated(color);
        }
        if (!guard || serial != action) return;
        if (!owner || !owner->handle()) { q->cancel(); return; }
        if (!sample) poll.stop();
        complete();
    }
};

KisColorSamplerStroke::KisColorSamplerStroke(QObject *parent)
    : QObject(parent), m_d(new Private(this))
{
    m_d->poll.setInterval(1);
    connect(&m_d->poll, &QTimer::timeout, this, [this] { m_d->advance(); });
}
KisColorSamplerStroke::~KisColorSamplerStroke() { cancel(); }
bool KisColorSamplerStroke::isActive() const { return m_d->active; }

void KisColorSamplerStroke::cancel()
{
    auto &d = *m_d;
    d.poll.stop();
    ++d.serial;
    for (const auto &connection : d.connections) QObject::disconnect(connection);
    d.connections.clear();
    auto image = d.image;
    d.image.clear();
    const auto stroke = d.strokeId;
    d.strokeId.clear();
    d.active = false;
    d.gpu = false;
    d.readback = {};
    d.sample.reset();
    d.nextSample.reset();
    d.lastColor.reset();
    d.documentId = {};
    const auto owner = std::move(d.owner);
    if (image && stroke) image->cancelStroke(stroke);
}

bool KisColorSamplerStroke::start(const KisImageSP &image, int radius, int blend)
{
    cancel();
    if (!image) {
        Q_EMIT sigSamplingFailed(QStringLiteral("Color sampling requires an image"));
        return false;
    }
    auto &d = *m_d;
    const auto owner = image->gpuImage();
    if (owner && (radius < 1 || radius > 900 || blend < 0 || blend > 100 || !owner->handle())) {
        Q_EMIT sigSamplingFailed(QStringLiteral("These color sampling settings are not available for this document"));
        return false;
    }
    d.active = true;
    d.gpu = bool(owner);
    d.radius = radius;
    d.blend = blend;
    if (owner) {
        d.owner = owner;
        d.documentId = owner->operations().snapshot()->documentId();
    } else {
        d.image = image;
        const auto action = d.serial;
        auto *strategy = new KisColorSamplerStrokeStrategy(radius, blend);
        d.connections.append(connect(strategy, &KisColorSamplerStrokeStrategy::sigColorUpdated,
            this, [this, action](const KoColor &color) {
                if (m_d->serial == action) Q_EMIT sigColorUpdated(color);
            }));
        d.connections.append(connect(strategy, &KisColorSamplerStrokeStrategy::sigFinalColorSelected,
            this, [this, action](const KoColor &color) {
                if (m_d->serial != action) return;
                Q_EMIT sigFinalColorSelected(color);
            }));
        d.connections.append(connect(strategy, &QObject::destroyed,
            this, [this, action] {
            if (m_d->serial != action) return;
            const auto image = m_d->image;
            m_d->image.clear();
            m_d->active = false;
            m_d->strokeId.clear();
            for (const auto &connection : m_d->connections) QObject::disconnect(connection);
            m_d->connections.clear();
        }, Qt::QueuedConnection));
        d.strokeId = d.image->startStroke(strategy);
    }
    return true;
}

void KisColorSamplerStroke::addSample(const KisNodeSP &node, const QPoint &point, const KoColor &currentColor)
{
    auto &d = *m_d;
    if (!d.active) return;
    if (d.gpu) {
        const auto owner = d.owner;
        if (!owner || !owner->handle()) {
            d.fail(QStringLiteral("The sampled document is no longer available"));
            return;
        }
        Private::Sample sample{owner->operations().snapshot(), 0, point, currentColor};
        if (node) {
            for (const auto &layer : sample.version->layers)
                if (layer.uuid == node->uuid()) sample.layer = layer.id;
            if (!sample.layer) return;
        }
        if (d.sample) d.nextSample = std::move(sample);
        else d.sample = std::move(sample);
        d.poll.start();
    } else {
        const auto device = node ? node->colorSampleSourceDevice() : d.image->projection();
        if (!device) {
            Q_EMIT sigSamplingFailed(QStringLiteral("Color sampler does not work on this layer"));
            return;
        }
        d.image->addJob(d.strokeId, new KisColorSamplerStrokeStrategy::Data(device, point, currentColor));
    }
}
void KisColorSamplerStroke::addReferenceColor(const KoColor &color)
{
    if (!m_d->active) return;
    if (m_d->gpu) m_d->lastColor = color;
    Q_EMIT sigColorUpdated(color);
}
void KisColorSamplerStroke::finish()
{
    auto &d = *m_d;
    if (!d.active) return;
    d.active = false;
    if (d.gpu) d.complete();
    else {
        const auto stroke = d.strokeId;
        d.strokeId.clear();
        d.image->addJob(stroke, new KisColorSamplerStrokeStrategy::FinalizeData());
        d.image->endStroke(stroke);
    }
}
