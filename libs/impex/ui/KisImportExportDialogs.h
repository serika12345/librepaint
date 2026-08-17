/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_IMPORT_EXPORT_DIALOGS_H
#define KIS_IMPORT_EXPORT_DIALOGS_H

#include <optional>

#include <QString>

class QWidget;

namespace KisImportExportDialogs
{
std::optional<int> choosePasteSource(QWidget *parent,
                                     bool remoteAvailable,
                                     bool localAvailable,
                                     bool bitmapAvailable,
                                     bool *remember);

std::optional<int> chooseClipboardColorSource(QWidget *parent, bool *remember);

QString choosePngProfile(const QString &path,
                         const QString &colorModel,
                         const QString &colorDepth);
}

#endif
