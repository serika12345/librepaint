/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisTagChooserWidgetConstructionSource_p.h"

#include "KisTagChooserWidget.h"
#include "KisTagToolButton.h"

#include <KisTagModel.h>

namespace KisTagChooserWidgetConstructionSource
{
QAbstractItemModel *preparePresentationModel(KisTagModel *model)
{
    model->sort(KisAllTagsModel::Name);
    return model;
}

QAbstractItemModel *createAllTagsModel(const QString &resourceType,
                                       QObject *parent)
{
    auto *model = new KisTagModel(resourceType, parent);
    model->setTagFilter(KisTagModel::ShowAllTags);
    return model;
}

void connectTagToolButton(QWidget *toolButton,
                          KisTagChooserWidget *chooser)
{
    auto *button = static_cast<KisTagToolButton *>(toolButton);
    QObject::connect(button,
                     SIGNAL(popupMenuAboutToShow()),
                     chooser,
                     SLOT(tagToolContextMenuAboutToShow()));
    QObject::connect(button,
                     SIGNAL(newTagRequested(QString)),
                     chooser,
                     SLOT(addTag(QString)));
    QObject::connect(button,
                     SIGNAL(deletionOfCurrentTagRequested()),
                     chooser,
                     SLOT(tagToolDeleteCurrentTag()));
    QObject::connect(button,
                     SIGNAL(renamingOfCurrentTagRequested(const QString &)),
                     chooser,
                     SLOT(tagToolRenameCurrentTag(const QString &)));
    QObject::connect(button,
                     SIGNAL(undeletionOfTagRequested(KisTagSP)),
                     chooser,
                     SLOT(tagToolUndeleteLastTag(KisTagSP)));
}
}

QWidget *KisTagChooserWidget::createTagToolButton(QWidget *parent)
{
    return new KisTagToolButton(parent);
}
