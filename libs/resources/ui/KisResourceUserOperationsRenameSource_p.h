/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEUSEROPERATIONSRENAMESOURCE_P_H
#define KISRESOURCEUSEROPERATIONSRENAMESOURCE_P_H

#include <KoResource.h>

class QString;
class QWidget;

namespace KisResourceUserOperationsRenameSource
{
bool resourceNameIsAlreadyUsed(KoResourceSP resource, const QString &resourceName);
bool userAllowsDuplicateName(QWidget *widgetParent);
bool renameResource(KoResourceSP resource, const QString &resourceName);
void warnRenameFailed(QWidget *widgetParent);
}

#endif // KISRESOURCEUSEROPERATIONSRENAMESOURCE_P_H
