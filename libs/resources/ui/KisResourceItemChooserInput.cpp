/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooser.h"

#include "KisResourceItemChooserInputSource_p.h"
#include "KisResourceItemChooser_p.h"

#include <QWheelEvent>

void KisResourceItemChooser::setSynced(bool sync)
{
    if (d->synced == sync) {
        return;
    }

    d->synced = sync;
    if (sync) {
        KisResourceItemChooserInputSource::connectBaseLength(this);
        baseLengthChanged(KisResourceItemChooserInputSource::baseLength());
    } else {
        KisResourceItemChooserInputSource::disconnectBaseLength(this);
    }
}

void KisResourceItemChooser::slotScrollerStateChanged(QScroller::State state)
{
    KisResourceItemChooserInputSource::updateCursor(this, state);
}

void KisResourceItemChooser::baseLengthChanged(int length)
{
    if (d->synced) {
        KisResourceItemChooserInputSource::setItemSize(d->view, length);
    }
}

bool KisResourceItemChooser::eventFilter(QObject *object, QEvent *event)
{
    if (d->synced && event->type() == QEvent::Wheel) {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            const int degrees = wheelEvent->angleDelta().y() / 8;
            const int newBaseLength =
                KisResourceItemChooserInputSource::baseLength() +
                degrees / 15 * 10;
            KisResourceItemChooserInputSource::setBaseLength(newBaseLength);
            return true;
        }
    }
    return QObject::eventFilter(object, event);
}
