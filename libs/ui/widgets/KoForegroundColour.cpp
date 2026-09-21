/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 1999 Daniel M. Duley <mosfet@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "KoForegroundColour.h"
#include "KoColorDisplayRendererInterface.h"
#include "KoDualColorButton.h"
#include "KoID.h"
#include <QPainter>
#include <QColorDialog>
#include <QEvent>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <qtpreprocessorsupport.h>
#include <qwidget.h>

KoForegroundColour::KoForegroundColour(KisCanvasResourceProvider *canvasResourceProvider,
                                     const KoColorDisplayRendererInterface *displayRenderer,
                                     QWidget *parent, QWidget *dialogParent)
    : KoDualColorButton(canvasResourceProvider, displayRenderer, parent, dialogParent)
{
    foregroundRect = QRect( 0, 0, 22, 22);
    backgroundRect = QRect( 16, 16, 12, 12);
    setColorDialogState(Foreground);
}


void KoForegroundColour::paint_icons(QPainter &painter)
{
    Q_UNUSED(painter);
}

void KoForegroundColour::mousePressEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
}

void KoForegroundColour::mouseReleaseEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
    
    KConfigGroup cfg =  KSharedConfig::openConfig()->group("colorselector");
    bool usePlatformDialog = cfg.readEntry("UsePlatformColorDialog", false);
    
    foregroundSelect(usePlatformDialog);

    update();
}

bool KoForegroundColour::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        this->setToolTip(i18n("Foreground color selector"));
    }
    return QWidget::event(event);
}
