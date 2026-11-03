/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImportExportAsyncFeedback.h"
#include "KisImportExportErrorCode.h"

#include <QApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QtConcurrentRun>
#include <functional>
#include <utility>
#include <memory>
#include <qfuture.h>
#include <qnamespace.h>

KisImportExportErrorCode runImportExportActionWithFeedback(
    const QString &message,
    QWidget *parent,
    std::function<KisImportExportErrorCode()> action)
{
    return runImportExportActionWithFeedback(message, parent, std::move(action), false);
}

KisImportExportErrorCode runImportExportActionWithFeedback(
    const QString &message,
    QWidget *parent,
    std::function<KisImportExportErrorCode()> action,
    bool allowInteraction)
{
    return runImportExportActionWithFeedback(message, parent, std::move(action), allowInteraction, true);
}

KisImportExportErrorCode runImportExportActionWithFeedback(
    const QString &message,
    QWidget *parent,
    std::function<KisImportExportErrorCode()> action,
    bool allowInteraction,
    bool showProgress)
{
    std::unique_ptr<QProgressDialog> progress;
    if (showProgress) {
        progress = std::make_unique<QProgressDialog>(message, QString(), 0, 0, parent);
        progress->setWindowModality(allowInteraction ? Qt::NonModal : Qt::ApplicationModal);
        progress->setCancelButton(nullptr);
        progress->setMinimumDuration(1000);
        progress->setValue(0);
        progress->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    }

    QFuture<KisImportExportErrorCode> future = QtConcurrent::run(action);
    QFutureWatcher<KisImportExportErrorCode> watcher;
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher<KisImportExportErrorCode>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(future);
    if (watcher.isRunning())
        loop.exec(allowInteraction ? QEventLoop::AllEvents : QEventLoop::ExcludeUserInputEvents);

    watcher.waitForFinished();
    return watcher.result();
}
