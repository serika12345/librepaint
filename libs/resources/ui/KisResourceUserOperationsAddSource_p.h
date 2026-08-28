/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEUSEROPERATIONSADDSOURCE_P_H
#define KISRESOURCEUSEROPERATIONSADDSOURCE_P_H

#include <KoResource.h>

class QString;
class QWidget;

namespace KisResourceUserOperationsAddSource
{
bool userAllowsOverwrite(QWidget *widgetParent);
bool resourceNameIsAlreadyUsed(KoResourceSP resource);
bool userAllowsDuplicateName(QWidget *widgetParent);
bool addResource(KoResourceSP resource, const QString &storageLocation);
void warnAddFailed(QWidget *widgetParent);
}

#endif // KISRESOURCEUSEROPERATIONSADDSOURCE_P_H
