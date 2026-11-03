/*
 *  SPDX-FileCopyrightText: 2016 Alvin Wong <alvinhochun-at-gmail-com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STABILIZER_DELAYED_PAINT_HELPER_H
#define KIS_STABILIZER_DELAYED_PAINT_HELPER_H

#include <QElapsedTimer>
#include <QQueue>
#include <QPair>
#include <QTimer>
#include <QVector>

#include <functional>
#include <utility>

#include "kis_paint_information.h"
#include "kritatools_export.h"

class KRITATOOLS_EXPORT KisStabilizerDelayedPaintHelper : public QObject
{
    Q_OBJECT

public:
    using PaintLines = QVector<QPair<KisPaintInformation, KisPaintInformation>>;

private:
    struct TimedPaintInfo
    {
        int elapsedTime;
        KisPaintInformation paintInfo;
        TimedPaintInfo(int elapsedTime, KisPaintInformation paintInfo);
    };

    QTimer m_paintTimer;
    QQueue<TimedPaintInfo> m_paintQueue;
    int m_lastPendingTime {0};
    int m_lastPaintTime {0};
    QElapsedTimer m_elapsedTimer;

    // Callbacks
    std::function<void(const KisPaintInformation &, const KisPaintInformation &)> m_paintLine;
    std::function<void()> m_requestUpdateOutline;
    std::function<void(const PaintLines &)> m_paintLines;
    std::function<void()> m_batchAllocationFailed;

public:
    KisStabilizerDelayedPaintHelper();
    ~KisStabilizerDelayedPaintHelper() override {}

    bool running() const {
        return m_paintTimer.isActive();
    }

    bool hasLastPaintInformation() const {
        return !m_paintQueue.isEmpty();
    }

    KisPaintInformation lastPaintInformation() const {
        // Please call hasLastPaintInformation before this
        return m_paintQueue.head().paintInfo;
    }

    void setPaintLineCallback(std::function<void(const KisPaintInformation &, const KisPaintInformation &)> paintLine) {
        m_paintLine = paintLine;
    }

    /** A GPU consumer adopts all currently due lines in one document preview.
     * Allocation failure cancels the queue before notifying the consumer.
     * An empty callback restores the native per-line consumer. */
    void setPaintLinesCallback(std::function<void(const PaintLines &)> paintLines,
                              std::function<void()> allocationFailed = {}) {
        m_paintLines = std::move(paintLines);
        m_batchAllocationFailed = std::move(allocationFailed);
    }

    void setUpdateOutlineCallback(std::function<void()> requestUpdateOutline) {
        m_requestUpdateOutline = requestUpdateOutline;
    }

    void start(const KisPaintInformation &firstPaintInfo);
    void update(const QVector<KisPaintInformation> &newPaintInfos);
    void paintSome();
    void end();
    void cancel();

private Q_SLOTS:
    void stabilizerDelayedPaintTimer();
};

#endif // KIS_STABILIZER_DELAYED_PAINT_HELPER_H
