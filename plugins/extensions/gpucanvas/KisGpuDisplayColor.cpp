/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDisplayColor.h"
#include "KisGpuDocumentWindow.h"
#include "KisRgb8DisplayLookup.h"
#include <KoColorModelStandardIds.h>
#include <KoColorSpace.h>
#include <QThread>
#include <optional>

struct KisGpuDisplayColor::Private {
    struct Settings {
        const KoColorSpace *destination;
        KoColorConversionTransformation::Intent intent;
        KoColorConversionTransformation::ConversionFlags flags;
        bool operator==(const Settings &other) const {
            return destination == other.destination && intent == other.intent && flags == other.flags;
        }
    };
    struct Job {
        std::atomic_bool cancelled{false};
        KisRgb8DisplayLookup::Result result;
        quint64 generation;
    };
    KisGpuDocumentWindow &window;
    const KoColorSpace *source;
    std::optional<Settings> requested, pending;
    std::shared_ptr<Job> job;
    std::unique_ptr<QThread> thread;
    quint64 generation = 0;
    Private(KisGpuDocumentWindow &window, const KoColorSpace *source) : window(window), source(source) {}
};

KisGpuDisplayColor::KisGpuDisplayColor(KisGpuDocumentWindow &window, const KoColorSpace *source)
    : QObject(&window), d(std::make_unique<Private>(window, source)) {}
KisGpuDisplayColor::~KisGpuDisplayColor()
{
    if (d->job) d->job->cancelled.store(true);
    if (d->thread) d->thread->wait();
}
bool KisGpuDisplayColor::busy() const { return bool(d->thread) || bool(d->pending); }

bool KisGpuDisplayColor::request(const KoColorSpace *destination,
    KoColorConversionTransformation::Intent intent, KoColorConversionTransformation::ConversionFlags flags)
{
    const auto supported = [](const KoColorSpace *space) {
        return space && space->colorModelId() == RGBAColorModelID
            && space->colorDepthId() == Integer8BitsColorDepthID && space->pixelSize() == 4;
    };
    if (!supported(d->source) || !supported(destination)) return false;
    Private::Settings settings{destination, intent, flags};
    if (d->requested && *d->requested == settings) return true;
    d->requested = settings;
    d->pending = settings;
    ++d->generation;
    if (d->job) d->job->cancelled.store(true);
    startNext();
    return true;
}
void KisGpuDisplayColor::startNext()
{
    if (d->thread || !d->pending) return;
    const auto settings = *d->pending;
    d->pending.reset();
    if (*d->source == *settings.destination) {
        d->window.setDisplayLookup({});
        Q_EMIT ready();
        return;
    }
    d->job = std::make_shared<Private::Job>();
    d->job->generation = d->generation;
    const auto job = d->job;
    const auto *source = d->source;
    d->thread.reset(QThread::create([job, source, settings] {
        try {
            job->result = KisRgb8DisplayLookup::build(source, settings.destination,
                settings.intent, settings.flags, job->cancelled);
        } catch (...) {
            job->result = {};
        }
    }));
    connect(d->thread.get(), &QThread::finished, this, [this] {
        d->thread->wait();
        d->thread.reset();
        const auto job = std::move(d->job);
        const bool current = job->generation == d->generation && !job->cancelled.load();
        const bool applied = current && job->result.status == KisRgb8DisplayLookup::Status::Ready;
        const bool rejected = current && !applied;
        if (applied) d->window.setDisplayLookup(job->result.values);
        if (rejected) d->requested.reset();
        startNext();
        if (applied) Q_EMIT ready();
        else if (rejected) Q_EMIT failed();
    }, Qt::QueuedConnection);
    d->thread->start();
}
