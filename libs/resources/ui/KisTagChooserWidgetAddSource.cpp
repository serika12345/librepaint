/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisTagChooserWidgetAddSource_p.h"

#include <QMessageBox>

#include <klocalizedstring.h>

#include <KisTagModel.h>
#include <KisTagResourceModel.h>

#include "KisTagChooserWidget.h"

namespace
{
QVector<KoResourceSP> resourcesFor(KoResourceSP resource)
{
    return resource.isNull()
        ? QVector<KoResourceSP>()
        : QVector<KoResourceSP>{resource};
}
}

namespace KisTagChooserWidgetAddSource
{
QString tagName(KisTagSP tag)
{
    return tag->name();
}

QString tagUrl(KisTagSP tag)
{
    return tag->url();
}

KisTagSP tagForUrl(KisTagModel *model, const QString &url)
{
    return model->tagForUrl(url);
}

bool tagIsActive(KisTagSP tag)
{
    return tag->active();
}

void warnReservedName(QWidget *parent, bool tagValueInput)
{
    QMessageBox::information(
        parent,
        tagValueInput
            ? i18nc("Dialog title", "Can't rename the tag")
            : i18nc("Dialog title", "Can't create the tag"),
        i18nc("Dialog message", "You can't use this name for your custom tags."),
        QMessageBox::Ok);
}

void reactivateTag(KisTagModel *model,
                   const QString &resourceType,
                   KisTagSP tag,
                   KoResourceSP resource)
{
    model->setTagActive(tag);
    if (!resource.isNull()) {
        KisTagResourceModel(resourceType)
            .tagResources(tag, {resource->resourceId()});
    }
    model->sort(KisAllTagsModel::Name);
}

void addNewTag(KisTagModel *model,
               const QString &tagName,
               KoResourceSP resource)
{
    model->addTag(tagName, true, resourcesFor(resource));
    model->sort(KisAllTagsModel::Name);
}

void addNewTag(KisTagModel *model,
               KisTagSP tag,
               KoResourceSP resource)
{
    model->addTag(tag, true, resourcesFor(resource));
    model->sort(KisAllTagsModel::Name);
}
}

QMessageBox::ButtonRole KisTagChooserWidget::overwriteTagDialog(
    KisTagChooserWidget *parent,
    bool tagIsActive)
{
    const QString existingTagOption =
        !tagIsActive
        ? i18nc("Option in a dialog to undelete (reactivate) existing tag with its old assigned resources", "Restore previous tag")
        : i18nc("Option in a dialog to use existing tag with its old assigned resources", "Use existing tag");
    QMessageBox question(
        QMessageBox::Question,
        i18nc("Dialog title", "Overwrite tag?"),
        i18nc("Question to the user in a dialog about creating a tag",
              "A tag with this unique name already exists. Do you want to replace it?"));
    question.setParent(parent);
    question.addButton(
        i18nc("Option in a dialog to discard the previously existing tag and creating a new one in its place", "Replace (overwrite) tag"),
        QMessageBox::DestructiveRole);
    question.addButton(existingTagOption, QMessageBox::AcceptRole);
    question.addButton(QMessageBox::Cancel);
    question.exec();
    return question.buttonRole(question.clickedButton());
}
