/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisTagChooserWidget.h"

#include <QAbstractItemModel>
#include <QComboBox>

#include "KisTagChooserWidgetSelectionSource_p.h"
#include "KisTagChooserWidget_p.h"

void KisTagChooserWidget::setCurrentIndex(int index)
{
    d->comboBox->setCurrentIndex(index);
}

int KisTagChooserWidget::currentIndex() const
{
    return d->comboBox->currentIndex();
}

void KisTagChooserWidget::setCurrentItem(const QString &tag)
{
    QAbstractItemModel *model = d->comboBox->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        const QString currentRowTag =
            model->data(index, Qt::UserRole + KisAllTagsModel::Url).toString();
        if (currentRowTag == tag) {
            setCurrentIndex(row);
        }
    }
}

KisTagSP KisTagChooserWidget::currentlySelectedTag()
{
    const int row = d->comboBox->currentIndex();
    if (row < 0) {
        return nullptr;
    }
    const QModelIndex index = d->comboBox->model()->index(row, 0);
    return d->comboBox->model()
        ->data(index, Qt::UserRole + KisAllTagsModel::KisTagRole)
        .value<KisTagSP>();
}

void KisTagChooserWidget::tagChanged(int tagIndex)
{
    if (tagIndex >= 0) {
        const KisTagSP tag = currentlySelectedTag();
        setTagToolButtonCurrentTag(d->tagToolButton, tag);
        KisTagChooserWidgetSelectionSource::persistSelectedTag(d->resourceType,
                                                               tag);
        d->comboBox->model()->sort(KisAllTagsModel::Name);
        Q_EMIT sigTagChosen(tag);
    } else {
        setCurrentIndex(0);
    }
}

void KisTagChooserWidget::updateIcons()
{
    refreshTagToolButtonIcons(d->tagToolButton);
}
