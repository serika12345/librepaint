/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_WINDOWS_POINTER_TIMES_H
#define KIS_WINDOWS_POINTER_TIMES_H

#include "kritainputui_export.h"

#include <QPointF>
#include <QVector>
#include <QtGlobal>

/**
 * The high-resolution time of each pen sample of a Windows pointer message.
 *
 * Qt takes the timestamp of a tablet event from msg.time, which advances in
 * steps of the system timer (about 15.6 ms). A 240 Hz pen then reports three
 * or four samples with the same time, and a predictor cannot estimate the
 * pen speed. Windows also reports POINTER_INFO.PerformanceCount for every
 * sample; this class keeps those samples and gives a tablet event the time of
 * its sample in the msg.time base.
 *
 * Samples arrive from the native event filter and are taken from the input
 * manager on the GUI thread.
 */
class KRITAINPUTUI_EXPORT KisWindowsPointerTimes
{
public:
    struct Sample {
        quint32 messageTime = 0;        // msg.time of the WM_POINTER message
        QPointF logicalPosition;        // ptPixelLocation mapped to logical pixels
        quint64 performanceCount = 0;   // POINTER_INFO.PerformanceCount
    };

    void setFrequency(quint64 countsPerSecond);
    // Samples of one message, oldest first.
    void addSamples(const QVector<Sample> &samples);
    // Finds the sample of a Qt tablet event and returns its time in the
    // msg.time base. Returns false when no sample matches.
    bool take(quint64 eventTimestamp, const QPointF &logicalPosition, quint64 *timestampMs);

    static KisWindowsPointerTimes *instance();   // nullptr until install()
    static void install(KisWindowsPointerTimes *times);

private:
    struct Entry {
        quint32 messageTime = 0;
        quint32 timestampMs = 0;
        QPointF logicalPosition;
    };

    quint64 m_frequency = 0;
    qint64 m_offset = 0;
    bool m_haveOffset = false;
    QVector<Entry> m_entries;

    static KisWindowsPointerTimes *s_instance;
};

#endif // KIS_WINDOWS_POINTER_TIMES_H
