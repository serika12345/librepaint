/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisTagChooserWidget.h"

#include "KisTagChooserWidgetAddSource_p.h"
#include "KisTagChooserWidget_p.h"

void KisTagChooserWidget::addTag(const QString &tag)
{
    addTag(tag, nullptr);
}

void KisTagChooserWidget::addTag(const QString &tagName, KoResourceSP resource)
{
    if (tagName == KisAllTagsModel::urlAll()
        || tagName == KisAllTagsModel::urlAllUntagged()) {
        KisTagChooserWidgetAddSource::warnReservedName(this, false);
        return;
    }
    if (tagName.isEmpty()) {
        return;
    }

    const KisTagSP existingTag =
        KisTagChooserWidgetAddSource::tagForUrl(d->model, tagName);
    if (!existingTag.isNull()) {
        const QMessageBox::ButtonRole response = overwriteTagDialog(
            this,
            KisTagChooserWidgetAddSource::tagIsActive(existingTag));
        if (response == QMessageBox::AcceptRole) {
            KisTagChooserWidgetAddSource::reactivateTag(d->model,
                                                        d->resourceType,
                                                        existingTag,
                                                        resource);
            return;
        }
        if (response == QMessageBox::RejectRole) {
            return;
        }
    }

    KisTagChooserWidgetAddSource::addNewTag(d->model, tagName, resource);
}

void KisTagChooserWidget::addTag(KisTagSP tag, KoResourceSP resource)
{
    const QString tagName = KisTagChooserWidgetAddSource::tagName(tag);
    if (tagName == KisAllTagsModel::urlAll()
        || tagName == KisAllTagsModel::urlAllUntagged()) {
        KisTagChooserWidgetAddSource::warnReservedName(this, true);
        return;
    }

    const KisTagSP existingTag = KisTagChooserWidgetAddSource::tagForUrl(
        d->model,
        KisTagChooserWidgetAddSource::tagUrl(tag));
    if (!existingTag.isNull()) {
        const QMessageBox::ButtonRole response = overwriteTagDialog(
            this,
            KisTagChooserWidgetAddSource::tagIsActive(existingTag));
        if (response == QMessageBox::AcceptRole) {
            KisTagChooserWidgetAddSource::reactivateTag(d->model,
                                                        d->resourceType,
                                                        existingTag,
                                                        resource);
            return;
        }
        if (response == QMessageBox::RejectRole) {
            return;
        }
    }

    KisTagChooserWidgetAddSource::addNewTag(d->model, tag, resource);
}
