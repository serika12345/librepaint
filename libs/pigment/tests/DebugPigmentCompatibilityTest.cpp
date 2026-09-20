/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DebugPigment.h"

#include <QTest>

class DebugPigmentCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresTheSavedLogDockerFilter();
};

// Compatibility requirement: Log Docker's saved pigment filter depends on the krita.lib.pigment logging category.
void DebugPigmentCompatibilityTest::restoresTheSavedLogDockerFilter()
{
    // Consumer: Log Docker users enabling the saved color-management diagnostic filter.
    // Operation: LibrePaint applies the saved logging rule for Pigment.
    // Observable result: Pigment debug messages are enabled under the established category name.
    // Failure impact: The user's color-management diagnostics disappear after an upgrade.
    QLoggingCategory::setFilterRules(QStringLiteral("krita.lib.pigment=true"));

    const QLoggingCategory &category = PIGMENT_log();
    QCOMPARE(QString::fromLatin1(category.categoryName()), QStringLiteral("krita.lib.pigment"));
    QVERIFY(category.isDebugEnabled());

    QLoggingCategory::setFilterRules(QString());
}

QTEST_GUILESS_MAIN(DebugPigmentCompatibilityTest)

#include "DebugPigmentCompatibilityTest.moc"
