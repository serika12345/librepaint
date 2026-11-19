/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_SAMPLE_HISTORY_H
#define KIS_STROKE_SAMPLE_HISTORY_H

#include <QPointF>
#include <QVector>

#include <kritatools_export.h>

/**
 * The recent samples of one stroke: position, time and pressure.
 *
 * The rules are the ones of KisStrokePrediction::addSample() and
 * KisStrokePrediction::predict(): a sample at the same time replaces the
 * previous one, a time going backwards or a gap of gapResetMs restarts the
 * history, only the samples inside the kept time span and the last 64 remain,
 * and the anchor is the oldest sample within anchorMs of the latest one, or
 * the previous sample when only the latest one is inside.
 */
class KRITATOOLS_EXPORT KisStrokeSampleHistory
{
public:
    struct Sample {
        QPointF position;
        qreal time = 0.0;
        qreal pressure = 0.0;
    };

    /**
     * keepMs is the time span the caller needs besides the anchor and the gap
     * reset; the kept span is the largest of the three.
     */
    KisStrokeSampleHistory(qreal keepMs, qreal anchorMs, qreal gapResetMs);

    /// Add one sample. Returns true when a gap or a clock going backwards
    /// dropped the history and this sample became the first one.
    bool add(const QPointF &pos, qreal timeMs, qreal pressure);

    void reset();

    int size() const;
    const Sample &latest() const;

    /// Positions from the anchor to the latest sample.
    QVector<QPointF> anchorPolyline() const;

private:
    void trim();

    qreal m_keepMs = 0.0;
    qreal m_anchorMs = 0.0;
    qreal m_gapResetMs = 0.0;
    QVector<Sample> m_samples;
};

#endif // KIS_STROKE_SAMPLE_HISTORY_H
