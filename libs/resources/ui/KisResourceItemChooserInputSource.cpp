/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooserInputSource_p.h"

#include "KisResourceItemChooser.h"
#include "KisResourceItemChooserSync.h"
#include "KisResourceItemListView.h"

#include <KisKineticScroller.h>

#include <QObject>

namespace KisResourceItemChooserInputSource
{
void connectBaseLength(KisResourceItemChooser *chooser)
{
    auto *chooserSync = KisResourceItemChooserSync::instance();
    QObject::connect(chooserSync,
                     SIGNAL(baseLengthChanged(int)),
                     chooser,
                     SLOT(baseLengthChanged(int)));
}

void disconnectBaseLength(KisResourceItemChooser *chooser)
{
    KisResourceItemChooserSync::instance()->disconnect(chooser);
}

int baseLength()
{
    return KisResourceItemChooserSync::instance()->baseLength();
}

void setBaseLength(int length)
{
    KisResourceItemChooserSync::instance()->setBaseLength(length);
}

void setItemSize(KisResourceItemListView *view, int length)
{
    view->setItemSize(QSize(length, length));
}

void updateCursor(QWidget *widget, QScroller::State state)
{
    KisKineticScroller::updateCursor(widget, state);
}
}
