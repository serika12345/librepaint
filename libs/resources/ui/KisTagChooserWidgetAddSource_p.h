/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISTAGCHOOSERWIDGETADDSOURCE_P_H
#define KISTAGCHOOSERWIDGETADDSOURCE_P_H

#include <KoResource.h>
#include <KisTag.h>

class QString;
class QWidget;
class KisTagModel;

namespace KisTagChooserWidgetAddSource
{
QString tagName(KisTagSP tag);
QString tagUrl(KisTagSP tag);
KisTagSP tagForUrl(KisTagModel *model, const QString &url);
bool tagIsActive(KisTagSP tag);
void warnReservedName(QWidget *parent, bool tagValueInput);
void reactivateTag(KisTagModel *model,
                   const QString &resourceType,
                   KisTagSP tag,
                   KoResourceSP resource);
void addNewTag(KisTagModel *model,
               const QString &tagName,
               KoResourceSP resource);
void addNewTag(KisTagModel *model,
               KisTagSP tag,
               KoResourceSP resource);
}

#endif // KISTAGCHOOSERWIDGETADDSOURCE_P_H
