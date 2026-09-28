/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_INPUT_EVENT_SEQUENCE_H
#define KIS_INPUT_EVENT_SEQUENCE_H

#include <QObject>
#include <QScopedPointer>

#include <kritainputui_export.h>

class QEvent;

/**
 * Records pointer input events delivered to one Qt receiver and replays them
 * synchronously through another receiver's normal event path.
 *
 * A recording owns clones of the events. Pointing-device objects referenced by
 * those events are borrowed from Qt and must remain alive until replay ends.
 */
class KRITAINPUTUI_EXPORT KisInputEventSequence final : public QObject
{
public:
    explicit KisInputEventSequence(QObject *parent = nullptr);
    ~KisInputEventSequence() override;

    /**
     * Clears the current sequence and starts observing @p receiver.
     * Returns false for a null receiver or when called outside this object's
     * thread.
     */
    bool startRecording(QObject *receiver);
    void stopRecording();
    bool isRecording() const;

    qsizetype size() const;
    void clear();

    /**
     * Sends fresh clones of the recorded events to @p receiver in order.
     * Replay is synchronous and returns false while recording, for a null
     * receiver, or outside the receiver's thread.
     */
    bool replay(QObject *receiver) const;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    class Private;
    const QScopedPointer<Private> m_d;
};

#endif // KIS_INPUT_EVENT_SEQUENCE_H
