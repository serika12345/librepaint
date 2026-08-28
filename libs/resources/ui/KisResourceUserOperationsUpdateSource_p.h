/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEUSEROPERATIONSUPDATESOURCE_P_H
#define KISRESOURCEUSEROPERATIONSUPDATESOURCE_P_H

#include <KoResource.h>

class QWidget;

namespace KisResourceUserOperationsUpdateSource
{
bool isExternalResource(KoResourceSP resource);
bool resourceNameChanged(KoResourceSP resource);
bool resourceNameIsAlreadyUsed(KoResourceSP resource);
bool userAllowsDuplicateName(QWidget *widgetParent);
bool updateResource(KoResourceSP resource);
void warnUpdateFailed(QWidget *widgetParent);
}

#endif // KISRESOURCEUSEROPERATIONSUPDATESOURCE_P_H
