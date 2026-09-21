/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 2026 Arkady Flury <arkady.flury@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "KoBackgroundColour.h"
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


KoBackgroundColour::KoBackgroundColour(KisCanvasResourceProvider *canvasResourceProvider,
                                     const KoColorDisplayRendererInterface *displayRenderer,
                                     QWidget *parent, QWidget *dialogParent)
    : KoDualColorButton(canvasResourceProvider, displayRenderer, parent, dialogParent)
{
    foregroundRect = QRect( 0, 0, 12, 12);
    backgroundRect = QRect( 6, 6, 22, 22);
    setColorDialogState(Background);
}


void KoBackgroundColour::paint_icons(QPainter &painter)
{
    Q_UNUSED(painter);
}

void KoBackgroundColour::mousePressEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
}

void KoBackgroundColour::mouseReleaseEvent( QMouseEvent *event )
{
    Q_UNUSED(event);
    
    KConfigGroup cfg =  KSharedConfig::openConfig()->group("colorselector");
    bool usePlatformDialog = cfg.readEntry("UsePlatformColorDialog", false);
    
    backgroundSelect(usePlatformDialog);

    update();
}

bool KoBackgroundColour::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        this->setToolTip(i18n("Background color selector"));
    }
    return QWidget::event(event);
}
