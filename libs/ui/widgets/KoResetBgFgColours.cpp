/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 2026 Arkady Flury <arkady.flury@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "KoResetBgFgColours.h"
#include "KoColorDisplayRendererInterface.h"
#include "KoDualColorButton.h"
#include "KoID.h"
#include "ui/workspace/KisView.h"
#include <QPainter>
#include <QEvent>
#include <klocalizedstring.h>
#include <QtGlobal>
#include <qwidget.h>



KoResetBgFgColours::KoResetBgFgColours(KisCanvasResourceProvider *canvasResourceProvider,
                                     const KoColorDisplayRendererInterface *displayRenderer,
                                     QWidget *parent, QWidget *dialogParent)
    : KoDualColorButton(canvasResourceProvider, displayRenderer, parent, dialogParent)
{
    foregroundRect = QRect( 0, 7, 14, 14);
    backgroundRect = QRect( 7, 14, 14, 14);
    resetColours = true;
}


void KoResetBgFgColours::paint_icons(QPainter &painter)
{
    painter.drawPixmap(16, 0, resetArrowPixmap);
}

void KoResetBgFgColours::mousePressEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
    ResetColours();
}

void KoResetBgFgColours::mouseReleaseEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
}

bool KoResetBgFgColours::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        this->setToolTip(i18n("Set foreground and background colors to black and white"));
    }
    return QWidget::event(event);
}

void KoResetBgFgColours::mouseMoveEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
}
