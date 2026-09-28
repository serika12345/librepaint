/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAndroidCrashHandler.h"

#include <QFile>
#include <QStandardPaths>
#include <QTest>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

class KisAndroidCrashHandlerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void signalCallbackWritesAnUnwindstackBacktrace();
};

void KisAndroidCrashHandlerContractTest::signalCallbackWritesAnUnwindstackBacktrace()
{
    const QString crashLogPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/kritacrashlog.txt");
    QVERIFY(QFile::remove(crashLogPath) || !QFile::exists(crashLogPath));

    const pid_t childProcess = fork();
    QVERIFY(childProcess >= 0);

    if (childProcess == 0) {
        KisAndroidCrashHandler::handler_init();
        raise(SIGTERM);
        _exit(125);
    }

    int childStatus = 0;
    QCOMPARE(waitpid(childProcess, &childStatus, 0), childProcess);
    QVERIFY(WIFSIGNALED(childStatus));
    QCOMPARE(WTERMSIG(childStatus), SIGTERM);

    QFile crashLog(crashLogPath);
    QVERIFY(crashLog.open(QIODevice::ReadOnly));
    const QByteArray backtrace = crashLog.readAll();
    QVERIFY(backtrace.contains("Dumping backtrace"));
    QVERIFY(backtrace.contains("Signal: 15 (SIGTERM)"));
    QVERIFY(backtrace.contains("#00 pc "));
    crashLog.close();
    QVERIFY(QFile::remove(crashLogPath));
}

QTEST_APPLESS_MAIN(KisAndroidCrashHandlerContractTest)

#include "KisAndroidCrashHandlerContractTest.moc"
