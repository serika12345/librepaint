/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TAGACTIONSSOURCE_P_H
#define TAGACTIONSSOURCE_P_H

class QIcon;
class QLineEdit;
class QString;
class QWidget;

namespace TagActionsSource
{
QLineEdit *createLineEdit(QWidget *parent);
QIcon loadIcon(const QString &name);
}

#endif // TAGACTIONSSOURCE_P_H
