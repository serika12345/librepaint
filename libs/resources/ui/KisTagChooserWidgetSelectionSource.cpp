/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisTagChooserWidgetSelectionSource_p.h"

#include <kconfiggroup.h>
#include <ksharedconfig.h>

#include "KisTagChooserWidget.h"
#include "KisTagToolButton.h"

namespace KisTagChooserWidgetSelectionSource
{
void persistSelectedTag(const QString &resourceType, KisTagSP tag)
{
    KConfigGroup group = KSharedConfig::openConfig()->group("SelectedTags");
    group.writeEntry(resourceType, tag->url());
}
}

void KisTagChooserWidget::setTagToolButtonCurrentTag(QWidget *toolButton,
                                                      KisTagSP tag)
{
    static_cast<KisTagToolButton *>(toolButton)->setCurrentTag(tag);
}

void KisTagChooserWidget::refreshTagToolButtonIcons(QWidget *toolButton)
{
    static_cast<KisTagToolButton *>(toolButton)->loadIcon();
}
