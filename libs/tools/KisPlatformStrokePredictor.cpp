/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPlatformStrokePredictor.h"

#include <cmath>

struct KisPlatformStrokePredictor::Private
{
    Private(const KisStrokePredictionSettings &input,
            KisStrokePredictionSource inputSource,
            const KisPlatformPredictionBuffer &inputBuffer,
            const QTransform &inputWidgetToDocument)
        : source(inputSource)
        , buffer(inputBuffer)
        , widgetToDocument(inputWidgetToDocument.m11(),
                           inputWidgetToDocument.m12(),
                           inputWidgetToDocument.m21(),
                           inputWidgetToDocument.m22(),
                           0.0, 0.0)
        , history(0.0, input.anchorMs, input.gapResetMs)
    {
    }

    KisStrokePredictionSource source;
    const KisPlatformPredictionBuffer &buffer;
    QTransform widgetToDocument;
    KisStrokeSampleHistory history;

    bool hasKey = false;
    quint64 keyMs = 0;
    int count = 0;
    bool entryFound = false;
    KisPlatformPredictionEntry entry;
    KisStrokePredictionMiss miss = KisStrokePredictionMiss::None;

    bool hasSource = false;
    QVector<QPointF> anchorPolyline;
    QPointF latestPosition;
    QVector<KisPlatformPredictedPoint> predictedPoints;
};

KisPlatformStrokePredictor::KisPlatformStrokePredictor(
    const KisStrokePredictionSettings &settings,
    KisStrokePredictionSource source,
    const KisPlatformPredictionBuffer &buffer,
    const QTransform &widgetToDocument)
    : m_d(new Private(settings, source, buffer, widgetToDocument))
{
}

KisPlatformStrokePredictor::~KisPlatformStrokePredictor() = default;

void KisPlatformStrokePredictor::reset()
{
    Private &d = *m_d;

    d.history.reset();
    d.hasKey = false;
    d.keyMs = 0;
    d.count = 0;
    d.entryFound = false;
    d.entry = KisPlatformPredictionEntry();
    d.miss = KisStrokePredictionMiss::None;
    d.hasSource = false;
    d.anchorPolyline.clear();
    d.latestPosition = QPointF();
    d.predictedPoints.clear();
}

void KisPlatformStrokePredictor::addSample(const KisStrokePredictionSample &sample)
{
    Private &d = *m_d;

    d.history.add(sample.documentPos, sample.timeMs, sample.pressure);

    const quint64 key = quint64(qRound64(sample.timeMs));
    d.miss = KisStrokePredictionMiss::None;

    if (d.hasKey && key == d.keyMs) {
        ++d.count;
    } else {
        if (d.hasKey && d.entryFound && d.count < d.entry.coalescedCount) {
            d.miss = KisStrokePredictionMiss::IncompleteEvent;
        }

        d.hasKey = true;
        d.keyMs = key;
        d.count = 1;

        KisPlatformPredictionEntry found;
        d.entryFound = d.buffer.find(key, &found);
        d.entry = d.entryFound ? found : KisPlatformPredictionEntry();
        if (!d.entryFound) {
            d.miss = KisStrokePredictionMiss::MissingEntry;
        }
    }

    if (d.entryFound && d.count >= d.entry.coalescedCount) {
        d.anchorPolyline = d.history.anchorPolyline();
        d.latestPosition = d.history.latest().position;
        d.predictedPoints = d.entry.points;
        d.hasSource = true;
    }
}

KisStrokePredictionResult KisPlatformStrokePredictor::predict(qreal horizonMs) const
{
    const Private &d = *m_d;
    KisStrokePredictionResult result;

    if (!d.hasSource) return result;

    QVector<QPointF> predicted;
    qreal lastDtMs = 0.0;
    for (const KisPlatformPredictedPoint &point : d.predictedPoints) {
        if (point.dtMs > horizonMs) continue;
        predicted.append(d.latestPosition + d.widgetToDocument.map(point.widgetDelta));
        lastDtMs = point.dtMs;
    }

    if (predicted.isEmpty()) return result;

    result.polyline = d.anchorPolyline;
    result.latestIndex = result.polyline.size() - 1;
    result.polyline += predicted;
    result.confidence = -1.0;
    result.source = d.source;
    result.valid = true;

    if (lastDtMs > 0.0) {
        result.velocity = (predicted.constLast() - d.latestPosition) / lastDtMs;
        result.speed = std::hypot(result.velocity.x(), result.velocity.y());
    }

    qreal moveLength = 0.0;
    QPointF previous = d.latestPosition;
    for (const QPointF &point : predicted) {
        moveLength += std::hypot(point.x() - previous.x(), point.y() - previous.y());
        previous = point;
    }
    result.moveLength = moveLength;

    return result;
}

KisStrokePredictionSource KisPlatformStrokePredictor::source() const
{
    return m_d->source;
}

KisStrokePredictionMiss KisPlatformStrokePredictor::lastSampleMiss() const
{
    return m_d->miss;
}
