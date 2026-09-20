/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCppQuirks.h>

#include <QDebug>
#include <QTest>

#include <optional>
#include <type_traits>

class KisCppQuirksTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void conditionalConstTypes();
    void optionalDebugOutput();
};

void KisCppQuirksTest::conditionalConstTypes()
{
    QVERIFY((std::is_same_v<typename std::add_const_if<true, int>::type, const int>));
    QVERIFY((std::is_same_v<typename std::add_const_if<false, int>::type, int>));
    QVERIFY((std::is_same_v<std::add_const_if_t<true, int>, const int>));
    QVERIFY((std::is_same_v<std::add_const_if_t<false, int>, int>));

    QVERIFY((std::is_same_v<typename std::copy_const<const int, double>::type, const double>));
    QVERIFY((std::is_same_v<typename std::copy_const<int, const double>::type, double>));
    QVERIFY((std::is_same_v<std::copy_const_t<const int, double>, const double>));
    QVERIFY((std::is_same_v<std::copy_const_t<int, const double>, double>));
}

void KisCppQuirksTest::optionalDebugOutput()
{
    QString valueOutput;
    {
        QDebug debug(&valueOutput);
        debug << std::optional<int>(42);
    }
    QVERIFY(valueOutput.contains("std::optional(42)"));

    QString emptyOutput;
    {
        QDebug debug(&emptyOutput);
        debug << std::optional<int>();
    }
    QVERIFY(emptyOutput.contains("nullopt"));
}

QTEST_GUILESS_MAIN(KisCppQuirksTest)

#include "KisCppQuirksTest.moc"
