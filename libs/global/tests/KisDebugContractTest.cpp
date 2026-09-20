/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_debug.h"

#include "config-debug.h"

#include <QTest>

class KisDebugContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void methodNameRemovesReturnTypeAndArguments();
    void backtraceFollowsConfiguredFormat();
};

void KisDebugContractTest::methodNameRemovesReturnTypeAndArguments()
{
#ifdef __GNUC__
    QCOMPARE(__methodName("void Example::Worker::execute(int, const QString &)"),
             QStringLiteral("Example::Worker::execute()"));
    QCOMPARE(__methodName("int freeFunction(double)"),
             QStringLiteral("freeFunction()"));
#else
    QSKIP("__methodName is exposed only for GCC-compatible compilers");
#endif
}

void KisDebugContractTest::backtraceFollowsConfiguredFormat()
{
    const QString trace = kisBacktrace();
#if HAVE_BACKTRACE
    QVERIFY(trace.startsWith(QStringLiteral("[\n")));
    QVERIFY(trace.endsWith(QStringLiteral("]\n")));
#else
    QVERIFY(trace.isEmpty());
#endif
}

QTEST_GUILESS_MAIN(KisDebugContractTest)

#include "KisDebugContractTest.moc"
