/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_debug.h"

#include "config-debug.h"

#include <QTest>

#include <array>

class KisDebugContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loggingCategoriesExposeStableNames();
    void methodNameRemovesReturnTypeAndArguments();
    void backtraceFollowsConfiguredFormat();
};

void KisDebugContractTest::loggingCategoriesExposeStableNames()
{
    using CategoryFunction = const QLoggingCategory &(*)();
    struct ExpectedCategory {
        CategoryFunction function;
        const char *name;
    };
    const std::array<ExpectedCategory, 21> categories {{
        {_30009, "krita.lib.resources"},
        {_30010, "krita.db.migration"},
        {_41000, "krita.general"},
        {_41001, "krita.core"},
        {_41002, "krita.registry"},
        {_41003, "krita.tools"},
        {_41004, "krita.tiles"},
        {_41005, "krita.filters"},
        {_41006, "krita.plugins"},
        {_41007, "krita.ui"},
        {_41008, "krita.file"},
        {_41009, "krita.math"},
        {_41010, "krita.render"},
        {_41011, "krita.scripting"},
        {_41012, "krita.input"},
        {_41013, "krita.action"},
        {_41014, "krita.tabletlog"},
        {_41015, "krita.opengl"},
        {_41016, "krita.metadata"},
        {_41017, "krita.android"},
        {_41018, "krita.locale"},
    }};

    for (const ExpectedCategory &category : categories) {
        QCOMPARE(QString::fromLatin1(category.function().categoryName()),
                 QString::fromLatin1(category.name));
    }
}

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
