/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../ui/KisImportExportAsyncFeedback.h"
#include <QApplication>
#include <QProgressDialog>
#include <QSemaphore>
#include <QThread>
#include <QTimer>
#include <QTest>
#include <atomic>

class KisImportExportAsyncFeedbackTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void workerCompletionKeepsTheRequestedInteractionMode_data();
    void workerCompletionKeepsTheRequestedInteractionMode();
};

void KisImportExportAsyncFeedbackTest::workerCompletionKeepsTheRequestedInteractionMode_data()
{
    QTest::addColumn<bool>("interactive");
    QTest::addColumn<bool>("showProgress");
    QTest::newRow("existing-exclusive-caller") << false << true;
    QTest::newRow("fixed-snapshot-interactive-caller") << true << true;
    QTest::newRow("batch-fixed-snapshot-caller") << true << false;
}

void KisImportExportAsyncFeedbackTest::workerCompletionKeepsTheRequestedInteractionMode()
{
    QFETCH(bool, interactive);
    QFETCH(bool, showProgress);
    QSemaphore finish;
    std::atomic_bool workerThread{false};
    bool guiEvent = false;
    bool foundProgress = false;
    bool modal = false;
    QTimer interaction;
    interaction.setSingleShot(true);
    connect(&interaction, &QTimer::timeout, this, [&] {
        guiEvent = QThread::currentThread() == qApp->thread();
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (auto *progress = qobject_cast<QProgressDialog *>(widget)) {
                foundProgress = true;
                progress->show();
                modal = QApplication::activeModalWidget() == progress;
            }
        }
        finish.release();
    });
    interaction.start(0);
    auto action = [&] {
        workerThread.store(QThread::currentThread() != qApp->thread());
        return KisImportExportErrorCode(finish.tryAcquire(1, 5000) ? ImportExportCodes::OK : ImportExportCodes::Failure);
    };
    const auto result = !showProgress ?
        runImportExportActionWithFeedback(QStringLiteral("Encoding image"), nullptr, action, true, false) : interactive ?
        runImportExportActionWithFeedback(QStringLiteral("Encoding image"), nullptr, action, true) :
        runImportExportActionWithFeedback(QStringLiteral("Encoding image"), nullptr, action);
    interaction.stop();
    QVERIFY(result.isOk());
    QVERIFY(workerThread.load());
    QVERIFY(guiEvent);
    QCOMPARE(foundProgress, showProgress);
    QCOMPARE(modal, !interactive);
    QVERIFY(!QApplication::activeModalWidget());
}

QTEST_MAIN(KisImportExportAsyncFeedbackTest)
#include "KisImportExportAsyncFeedbackTest.moc"
