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

#include <QComboBox>
#include <QMessageBox>

#include <klocalizedstring.h>
#include <kis_assert.h>

#include <KisTagResourceModel.h>

#include "KisTagChooserWidget_p.h"
#include "KisTagToolButton.h"

void KisTagChooserWidget::tagToolDeleteCurrentTag()
{
    KisTagSP currentTag = currentlySelectedTag();
    if (!currentTag.isNull() && currentTag->id() >= 0) {
        d->model->setTagInactive(currentTag);
        setCurrentIndex(0);
        d->model->sort(KisAllTagsModel::Name);
    }
}

void KisTagChooserWidget::tagToolRenameCurrentTag(const QString &tagName)
{
    KisTagSP tag = currentlySelectedTag();
    bool canRenameCurrentTag = !tag.isNull() && (tagName != tag->name());

    if (tagName == KisAllTagsModel::urlAll()
        || tagName == KisAllTagsModel::urlAllUntagged()) {
        QMessageBox::information(this,
                                 i18nc("Dialog title", "Can't rename the tag"),
                                 i18nc("Dialog message", "You can't use this name for your custom tags."),
                                 QMessageBox::Ok);
        return;
    }

    bool result = false;
    if (canRenameCurrentTag && !tagName.isEmpty()) {
        result = d->model->renameTag(tag, tagName, false);
        if (!result) {
            KisTagSP tagToRemove = d->model->tagForUrl(tagName);
            if (tagToRemove
                && QMessageBox::question(
                       this,
                       i18nc("Dialog title", "Remove existing tag with that name?"),
                       i18nc("Dialog message (the arguments are both somewhat user readable nouns or adjectives (names of the tags), can be treated as nouns since they represent the tags)",
                             "A tag with this unique name already exists. In order to continue renaming, the existing tag needs to be removed. Do you want to continue?\n"
                             "Tag to be removed: %1\n"
                             "Tag's unique name: %2",
                             tagToRemove->name(),
                             tagToRemove->url()),
                       QMessageBox::Yes | QMessageBox::Cancel,
                       QMessageBox::Cancel)
                    != QMessageBox::Cancel) {
                result = d->model->renameTag(tag, tagName, true);
                KIS_SAFE_ASSERT_RECOVER_RETURN(result);
            }
        }
    }

    if (result) {
        KisTagSP renamedTag = d->model->tagForUrl(tagName);
        KIS_SAFE_ASSERT_RECOVER_RETURN(renamedTag);
        const QModelIndex idx = d->model->indexForTag(renamedTag);
        setCurrentIndex(idx.row());
    }
}

void KisTagChooserWidget::tagToolUndeleteLastTag(KisTagSP tag)
{
    int previousIndex = d->comboBox->currentIndex();
    bool success = d->model->setTagActive(tag);
    setCurrentIndex(previousIndex);
    if (success) {
        setCurrentItem(tag->name());
        d->model->sort(KisAllTagsModel::Name);
    }
}

void KisTagChooserWidget::cacheSelectedTag()
{
    d->cachedTag = currentlySelectedTag();
}

void KisTagChooserWidget::restoreTagFromCache()
{
    if (d->cachedTag) {
        QModelIndex cachedIndex = d->model->indexForTag(d->cachedTag);
        setCurrentIndex(cachedIndex.row());
        d->cachedTag = nullptr;
    }
}

void KisTagChooserWidget::slotTagModelDataChanged(const QModelIndex &topLeft,
                                                   const QModelIndex &bottomRight,
                                                   const QVector<int> roles)
{
    if (!roles.isEmpty() && !roles.contains(Qt::CheckStateRole)) {
        return;
    }

    auto *tagToolButton = static_cast<KisTagToolButton *>(d->tagToolButton);
    auto *allTagsModel = static_cast<KisTagModel *>(d->allTagsModel);
    const QModelIndex currIdx =
        allTagsModel->indexForTag(tagToolButton->undeletionCandidate());

    if (currIdx.isValid()
        && currIdx.row() >= topLeft.row() && currIdx.row() <= bottomRight.row()
        && currIdx.column() >= topLeft.column()
        && currIdx.column() <= bottomRight.column()) {
        const bool isNowActive =
            allTagsModel->data(currIdx, Qt::CheckStateRole).toBool();
        if (isNowActive) {
            tagToolButton->setUndeletionCandidate(KisTagSP());
        }
    }

    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        for (int column = topLeft.column(); column <= bottomRight.column(); ++column) {
            const QModelIndex idx = allTagsModel->index(row, column);
            const bool isActive =
                allTagsModel->data(idx, Qt::CheckStateRole).toBool();
            if (idx != currIdx && !isActive) {
                tagToolButton->setUndeletionCandidate(
                    allTagsModel->tagForIndex(idx));
                break;
            }
        }
    }
}

void KisTagChooserWidget::addTag(const QString &tag)
{
    addTag(tag, nullptr);
}

QMessageBox::ButtonRole KisTagChooserWidget::overwriteTagDialog(
    KisTagChooserWidget *parent,
    bool tagIsActive)
{
    QString undeleteOption =
        !tagIsActive
        ? i18nc("Option in a dialog to undelete (reactivate) existing tag with its old assigned resources", "Restore previous tag")
        : i18nc("Option in a dialog to use existing tag with its old assigned resources", "Use existing tag");
    QMessageBox question(QMessageBox::Question,
                         i18nc("Dialog title", "Overwrite tag?"),
                         i18nc("Question to the user in a dialog about creating a tag",
                               "A tag with this unique name already exists. Do you want to replace it?"));
    question.setParent(parent);
    question.addButton(
        i18nc("Option in a dialog to discard the previously existing tag and creating a new one in its place", "Replace (overwrite) tag"),
        QMessageBox::DestructiveRole);
    question.addButton(undeleteOption, QMessageBox::AcceptRole);
    question.addButton(QMessageBox::Cancel);
    question.exec();
    return question.buttonRole(question.clickedButton());
}

void KisTagChooserWidget::addTag(const QString &tagName, KoResourceSP resource)
{
    if (tagName == KisAllTagsModel::urlAll()
        || tagName == KisAllTagsModel::urlAllUntagged()) {
        QMessageBox::information(this,
                                 i18nc("Dialog title", "Can't create the tag"),
                                 i18nc("Dialog message", "You can't use this name for your custom tags."),
                                 QMessageBox::Ok);
        return;
    }
    if (tagName.isEmpty()) {
        return;
    }

    KisTagSP tagForUrl = d->model->tagForUrl(tagName);
    if (!tagForUrl.isNull()) {
        int response = overwriteTagDialog(this, tagForUrl->active());
        if (response == QMessageBox::AcceptRole) {
            d->model->setTagActive(tagForUrl);
            if (!resource.isNull()) {
                KisTagResourceModel(d->resourceType)
                    .tagResources(tagForUrl, {resource->resourceId()});
            }
            d->model->sort(KisAllTagsModel::Name);
            return;
        } else if (response == QMessageBox::RejectRole) {
            return;
        }
    }
    QVector<KoResourceSP> resources = resource.isNull()
        ? QVector<KoResourceSP>()
        : QVector<KoResourceSP>{resource};
    d->model->addTag(tagName, true, resources);
    d->model->sort(KisAllTagsModel::Name);
}

void KisTagChooserWidget::addTag(KisTagSP tag, KoResourceSP resource)
{
    if (tag->name() == KisAllTagsModel::urlAll()
        || tag->name() == KisAllTagsModel::urlAllUntagged()) {
        QMessageBox::information(this,
                                 i18nc("Dialog title", "Can't rename the tag"),
                                 i18nc("Dialog message", "You can't use this name for your custom tags."),
                                 QMessageBox::Ok);
        return;
    }

    KisTagSP tagForUrl = d->model->tagForUrl(tag->url());
    if (!tagForUrl.isNull()) {
        int response = overwriteTagDialog(this, tagForUrl->active());
        if (response == QMessageBox::AcceptRole) {
            d->model->setTagActive(tagForUrl);
            if (!resource.isNull()) {
                KisTagResourceModel(d->resourceType)
                    .tagResources(tagForUrl, {resource->resourceId()});
            }
            d->model->sort(KisAllTagsModel::Name);
            return;
        } else if (response == QMessageBox::RejectRole) {
            return;
        }
    }
    QVector<KoResourceSP> resources = resource.isNull()
        ? QVector<KoResourceSP>()
        : QVector<KoResourceSP>{resource};
    d->model->addTag(tag, true, resources);
    d->model->sort(KisAllTagsModel::Name);
}

void KisTagChooserWidget::tagToolContextMenuAboutToShow()
{
    auto *tagToolButton = static_cast<KisTagToolButton *>(d->tagToolButton);
    if (currentlySelectedTag()) {
        tagToolButton->readOnlyMode(currentlySelectedTag()->id() < 0);
    } else {
        tagToolButton->readOnlyMode(true);
    }
}
