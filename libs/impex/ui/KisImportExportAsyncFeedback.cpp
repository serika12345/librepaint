/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImportExportAsyncFeedback.h"

#include <QApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QtConcurrentRun>

KisImportExportErrorCode runImportExportActionWithFeedback(
    const QString &message,
    QWidget *parent,
    std::function<KisImportExportErrorCode()> action)
{
    QProgressDialog progress(message, QString(), 0, 0, parent);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setCancelButton(nullptr);
    progress.setMinimumDuration(1000);
    progress.setValue(0);
    progress.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    QFuture<KisImportExportErrorCode> future = QtConcurrent::run(action);
    QFutureWatcher<KisImportExportErrorCode> watcher;
    watcher.setFuture(future);

    while (watcher.isRunning()) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    watcher.waitForFinished();
    return watcher.result();
}
