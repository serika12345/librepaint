/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PLATFORM_STROKE_PREDICTOR_H
#define KIS_PLATFORM_STROKE_PREDICTOR_H

#include <KisPlatformPredictionBuffer.h>
#include <KisStrokePrediction.h>
#include <KisStrokePredictor.h>
#include <KisStrokeSampleHistory.h>

#include <QScopedPointer>
#include <QTransform>

#include <kritatools_export.h>

/**
 * The predictor that follows the prediction points of the operating system.
 *
 * addSample() looks up the buffer with the event time of the sample. The
 * source of the prediction changes when the number of samples of the same time
 * reaches coalescedCount, that is at the last real touch of the OS event; the
 * intermediate coalesced touches keep the previous result.
 *
 * The predicted document point is the document position of the latest real
 * touch plus the widget delta mapped by the linear part of the widget to
 * document transform, so rotation, mirror and scale apply.
 * KoPointerEvent::pos() rounds to integers, so the delta does not come from it.
 *
 * A predicted point whose dtMs is larger than the horizon is left out of the
 * polyline.
 *
 * When the buffer has no entry for a sample, the previous result is kept and
 * lastSampleMiss() reports the miss, so the caller records prediction_miss and
 * the display disappears with the expiry after the stroke stops. The tool does
 * not switch to its own predictor in the middle of a stroke, so the shape of
 * the prediction does not change.
 */
class KRITATOOLS_EXPORT KisPlatformStrokePredictor : public KisStrokePredictor
{
public:
    /// widgetToDocument is the linear part of the canvas widget to document
    /// transform; its translation is ignored.
    KisPlatformStrokePredictor(const KisStrokePredictionSettings &settings,
                               KisStrokePredictionSource source,
                               const KisPlatformPredictionBuffer &buffer,
                               const QTransform &widgetToDocument);
    ~KisPlatformStrokePredictor() override;

    void reset() override;
    void addSample(const KisStrokePredictionSample &sample) override;
    KisStrokePredictionResult predict(qreal horizonMs) const override;
    KisStrokePredictionSource source() const override;
    KisStrokePredictionMiss lastSampleMiss() const override;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KIS_PLATFORM_STROKE_PREDICTOR_H
