/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 2026 Arkady Flury <arkady.flury@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "KoSwapBgFgColours.h"
#include "KoColorDisplayRendererInterface.h"
#include "KoDualColorButton.h"
#include "KoID.h"
#include <QPainter>
#include <QEvent>
#include <QtGlobal>
#include <qwidget.h>



KoSwapBgFgColours::KoSwapBgFgColours(KisCanvasResourceProvider *canvasResourceProvider,
                                     const KoColorDisplayRendererInterface *displayRenderer,
                                     QWidget *parent, QWidget *dialogParent)
    : KoDualColorButton(canvasResourceProvider, displayRenderer, parent, dialogParent)
{
    foregroundRect = QRect( 0, 7, 14, 14);
    backgroundRect = QRect( 7, 14, 14, 14);
}


void KoSwapBgFgColours::paint_icons(QPainter &painter)
{
    painter.drawPixmap( 15, 1, arrowBitmap );
}

void KoSwapBgFgColours::mousePressEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
    swapColours();
}

void KoSwapBgFgColours::mouseReleaseEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
}

bool KoSwapBgFgColours::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        this->setToolTip(i18n("Swap foreground and background colors"));
    }
    return QWidget::event(event);
}

void KoSwapBgFgColours::mouseMoveEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
}