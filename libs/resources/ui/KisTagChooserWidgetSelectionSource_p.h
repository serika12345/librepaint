/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISTAGCHOOSERWIDGETSELECTIONSOURCE_P_H
#define KISTAGCHOOSERWIDGETSELECTIONSOURCE_P_H

#include <KisTag.h>

class QString;
class QWidget;

namespace KisTagChooserWidgetSelectionSource
{
void persistSelectedTag(const QString &resourceType, KisTagSP tag);
}

#endif // KISTAGCHOOSERWIDGETSELECTIONSOURCE_P_H
