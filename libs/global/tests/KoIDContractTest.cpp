/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoID.h"

#include <QDebug>
#include <QTest>

#include <klocalizedstring.h>

class KoIDContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultAndStringValues();
    void localizedCopyAndAssignmentPreserveValues();
    void comparisonOperatorsUseIdentifierAndDisplayName();
    void debugOutputContainsNameAndIdentifier();
};

void KoIDContractTest::defaultAndStringValues()
{
    const KoID empty;
    QCOMPARE(empty.id(), QString());
    QCOMPARE(empty.name(), QString());

    const KoID value(QStringLiteral("stable-id"), QStringLiteral("Visible name"));
    QCOMPARE(value.id(), QStringLiteral("stable-id"));
    QCOMPARE(value.name(), QStringLiteral("Visible name"));
}

void KoIDContractTest::localizedCopyAndAssignmentPreserveValues()
{
    const KoID localized(QStringLiteral("localized-id"), ki18n("Localized name"));
    QCOMPARE(localized.id(), QStringLiteral("localized-id"));
    QCOMPARE(localized.name(), QStringLiteral("Localized name"));

    const KoID copied(localized);
    QCOMPARE(copied.id(), localized.id());
    QCOMPARE(copied.name(), localized.name());

    KoID assigned;
    assigned = localized;
    QCOMPARE(assigned.id(), localized.id());
    QCOMPARE(assigned.name(), localized.name());
}

void KoIDContractTest::comparisonOperatorsUseIdentifierAndDisplayName()
{
    const KoID alpha(QStringLiteral("alpha"), QStringLiteral("Zulu"));
    const KoID beta(QStringLiteral("beta"), QStringLiteral("Alpha"));
    const KoID sameAlpha(QStringLiteral("alpha"), QStringLiteral("Other name"));

    QVERIFY(alpha != beta);
    QVERIFY(alpha < beta);
    QVERIFY(beta > alpha);
    QVERIFY(alpha == sameAlpha);
    QVERIFY(KoID::compareNames(beta, alpha));
    QVERIFY(!KoID::compareNames(alpha, beta));
}

void KoIDContractTest::debugOutputContainsNameAndIdentifier()
{
    const KoID value(QStringLiteral("debug-id"), QStringLiteral("Debug name"));
    QString output;
    {
        QDebug debug(&output);
        debug << value;
    }

    QVERIFY(output.contains(QStringLiteral("Debug name")));
    QVERIFY(output.contains(QStringLiteral("debug-id")));
}

QTEST_GUILESS_MAIN(KoIDContractTest)

#include "KoIDContractTest.moc"
