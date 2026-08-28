/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISSTORAGECHOOSERWIDGET_H
#define KISSTORAGECHOOSERWIDGET_H

#include <QWidget>
#include <KisPopupButton.h>

#include "KisStorageChooserDelegate.h"
#include "kritaresourceui_export.h"


class KRITARESOURCEUI_EXPORT KisStorageChooserWidget : public KisPopupButton
{
    Q_OBJECT
public:
    KisStorageChooserWidget(const QString &resourceType, QWidget *parent = 0);

    ~KisStorageChooserWidget();

private Q_SLOTS:
    void activated(const QModelIndex &index);

private:
    QString m_resourceType;

};


#endif // KISSTORAGECHOOSERWIDGET_H
