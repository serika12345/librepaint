/*
 *    This file is part of the KDE project
 *    SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *    SPDX-FileCopyrightText: 2007 Jan Hambrecht <jaham@gmx.net>
 *    SPDX-FileCopyrightText: 2007 Sven Langkamp <sven.langkamp@gmail.com>
 *    SPDX-FileCopyrightText: 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
 *    SPDX-FileCopyrightText: 2013 Sascha Suelzer <s.suelzer@gmail.com>
 *    SPDX-FileCopyrightText: 2020 Agata Cacko <cacko.azh@gmail.com>
 *
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisTagChooserWidget.h"

#include <QAbstractItemModel>
#include <QComboBox>
#include <QGridLayout>

#include <klocalizedstring.h>

#include "KisTagChooserWidgetConstructionSource_p.h"
#include "KisTagChooserWidget_p.h"

KisTagChooserWidget::KisTagChooserWidget(KisTagModel *model,
                                         QString resourceType,
                                         QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    d->resourceType = resourceType;
    d->model = model;

    d->comboBox = new QComboBox(this);
    d->comboBox->setToolTip(i18n("Tag"));
    d->comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->comboBox->setMinimumContentsLength(1);
    d->comboBox->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon);
    d->comboBox->setInsertPolicy(QComboBox::InsertAlphabetically);
    d->comboBox->setModel(
        KisTagChooserWidgetConstructionSource::preparePresentationModel(model));

    d->allTagsModel =
        KisTagChooserWidgetConstructionSource::createAllTagsModel(resourceType,
                                                                  this);

    auto *comboLayout = new QGridLayout(this);
    comboLayout->addWidget(d->comboBox, 0, 0);

    d->tagToolButton = createTagToolButton(this);
    d->tagToolButton->setToolTip(i18n("Tag options"));
    comboLayout->addWidget(d->tagToolButton, 0, 1);
    comboLayout->setSpacing(0);
    comboLayout->setContentsMargins(0, 0, 0, 0);
    comboLayout->setColumnStretch(0, 3);
    setEnabled(true);

    connect(d->comboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &KisTagChooserWidget::tagChanged);
    KisTagChooserWidgetConstructionSource::connectTagToolButton(
        d->tagToolButton, this);
    connect(d->comboBox->model(),
            &QAbstractItemModel::modelAboutToBeReset,
            this,
            &KisTagChooserWidget::cacheSelectedTag);
    connect(d->comboBox->model(),
            &QAbstractItemModel::modelReset,
            this,
            &KisTagChooserWidget::restoreTagFromCache);
    connect(d->allTagsModel,
            &QAbstractItemModel::dataChanged,
            this,
            &KisTagChooserWidget::slotTagModelDataChanged);
}

KisTagChooserWidget::~KisTagChooserWidget()
{
    delete d;
}
