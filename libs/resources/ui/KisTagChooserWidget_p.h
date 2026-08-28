/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISTAGCHOOSERWIDGET_P_H
#define KISTAGCHOOSERWIDGET_P_H

#include "KisTagChooserWidget.h"

class QAbstractItemModel;
class QComboBox;

class Q_DECL_HIDDEN KisTagChooserWidget::Private
{
public:
    QComboBox *comboBox = nullptr;
    QWidget *tagToolButton = nullptr;
    KisTagModel *model = nullptr;
    KisTagSP cachedTag;
    QString resourceType;
    QAbstractItemModel *allTagsModel = nullptr;
};

#endif // KISTAGCHOOSERWIDGET_P_H
