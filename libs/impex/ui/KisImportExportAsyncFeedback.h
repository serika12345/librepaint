/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_IMPORT_EXPORT_ASYNC_FEEDBACK_H
#define KIS_IMPORT_EXPORT_ASYNC_FEEDBACK_H

#include <functional>

#include <QString>

#include <KisImportExportErrorCode.h>

class QWidget;

KisImportExportErrorCode runImportExportActionWithFeedback(
    const QString &message,
    QWidget *parent,
    std::function<KisImportExportErrorCode()> action);

#endif
