/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEITEMCHOOSERINPUTSOURCE_P_H
#define KISRESOURCEITEMCHOOSERINPUTSOURCE_P_H

#include <QScroller>

class QWidget;
class KisResourceItemChooser;
class KisResourceItemListView;

namespace KisResourceItemChooserInputSource
{
void connectBaseLength(KisResourceItemChooser *chooser);
void disconnectBaseLength(KisResourceItemChooser *chooser);
int baseLength();
void setBaseLength(int length);
void setItemSize(KisResourceItemListView *view, int length);
void updateCursor(QWidget *widget, QScroller::State state);
}

#endif // KISRESOURCEITEMCHOOSERINPUTSOURCE_P_H
