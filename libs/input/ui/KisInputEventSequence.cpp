/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisInputEventSequence.h"

#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QThread>
#include <QVector>

namespace
{
bool isRecordedInputType(QEvent::Type type)
{
    switch (type) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        return true;
    default:
        return false;
    }
}
}

class KisInputEventSequence::Private
{
public:
    QPointer<QObject> recordingReceiver;
    QVector<QSharedPointer<QEvent>> events;
};

KisInputEventSequence::KisInputEventSequence(QObject *parent)
    : QObject(parent)
    , m_d(new Private)
{
}

KisInputEventSequence::~KisInputEventSequence()
{
    stopRecording();
}

bool KisInputEventSequence::startRecording(QObject *receiver)
{
    if (!receiver ||
        thread() != QThread::currentThread() ||
        receiver->thread() != QThread::currentThread()) {
        return false;
    }

    stopRecording();
    clear();
    m_d->recordingReceiver = receiver;
    receiver->installEventFilter(this);
    return true;
}

void KisInputEventSequence::stopRecording()
{
    if (m_d->recordingReceiver) {
        m_d->recordingReceiver->removeEventFilter(this);
    }
    m_d->recordingReceiver.clear();
}

bool KisInputEventSequence::isRecording() const
{
    return !m_d->recordingReceiver.isNull();
}

qsizetype KisInputEventSequence::size() const
{
    return m_d->events.size();
}

void KisInputEventSequence::clear()
{
    m_d->events.clear();
}

bool KisInputEventSequence::replay(QObject *receiver) const
{
    if (!receiver ||
        isRecording() ||
        thread() != QThread::currentThread() ||
        receiver->thread() != QThread::currentThread()) {
        return false;
    }

    for (const QSharedPointer<QEvent> &event : m_d->events) {
        QScopedPointer<QEvent> replayEvent(event->clone());
        QCoreApplication::sendEvent(receiver, replayEvent.data());
    }
    return true;
}

bool KisInputEventSequence::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_d->recordingReceiver &&
        event &&
        isRecordedInputType(event->type())) {
        m_d->events.append(QSharedPointer<QEvent>(event->clone()));
    }
    return false;
}
