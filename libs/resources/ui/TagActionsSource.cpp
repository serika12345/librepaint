/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TagActionsSource_p.h"

#include <KoIcon.h>
#include <KisPopupSelfActivatingLineEdit.h>

namespace TagActionsSource
{
QLineEdit *createLineEdit(QWidget *parent)
{
    return new KisPopupSelfActivatingLineEdit(parent);
}

QIcon loadIcon(const QString &name)
{
    return KisIconUtils::loadIcon(name);
}
}
