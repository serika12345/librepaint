/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PLATFORM_PREDICTION_BUFFER_H
#define KIS_PLATFORM_PREDICTION_BUFFER_H

#include <QMutex>
#include <QPointF>
#include <QVector>

#include <atomic>

#include <kritatools_export.h>

/// One predicted point of the operating system: the offset from the last real
/// touch of the same OS event and the time after that touch.
struct KisPlatformPredictedPoint {
    QPointF widgetDelta; // from the last real touch of the same OS event, Qt logical pixels
    qreal dtMs = 0.0;    // time after that real touch
};

/// The prediction points of one OS event, keyed by the event time of its real
/// touches.
struct KisPlatformPredictionEntry {
    quint64 keyMs = 0;       // Qt event time shared by the real touches of the OS event
    int coalescedCount = 0;  // real touches the OS event delivers to Qt
    QVector<KisPlatformPredictedPoint> points;
};

/**
 * Hands the predicted points of the operating system from the supplier to the
 * predictor, keyed by the event time of the real touches of one OS event.
 *
 * On Android the supplier runs on another thread, so every entry and every
 * lookup takes the mutex.
 *
 * The 250 ms capacity keeps a pushed entry from being overwritten before the
 * delayed Qt thread reads it.
 */
class KRITATOOLS_EXPORT KisPlatformPredictionBuffer
{
public:
    static constexpr quint64 keepMs = 250;
    static constexpr int maxEntries = 256;

    static KisPlatformPredictionBuffer &instance();

    void push(const KisPlatformPredictionEntry &entry);
    bool find(quint64 keyMs, KisPlatformPredictionEntry *entry) const;
    void clear();

    void setSupplierInstalled(bool installed);
    bool supplierInstalled() const;

private:
    QVector<KisPlatformPredictionEntry> m_entries;
    mutable QMutex m_mutex;
    std::atomic<bool> m_supplierInstalled {false};
};

#endif // KIS_PLATFORM_PREDICTION_BUFFER_H
