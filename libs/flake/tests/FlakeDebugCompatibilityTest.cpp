/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FlakeDebug.h"

#include <QTest>

class FlakeDebugCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresTheSavedLogDockerFilter();
};

// Compatibility requirement: Log Docker's saved tools_41003 filter depends on the krita.lib.flake logging category.
void FlakeDebugCompatibilityTest::restoresTheSavedLogDockerFilter()
{
    // Consumer: Log Docker users enabling the saved tools diagnostic filter.
    // Operation: LibrePaint applies the saved logging rule for Flake tools.
    // Observable result: Flake debug messages are enabled under the established category name.
    // Failure impact: The user's vector-tool diagnostics disappear after an upgrade.
    QLoggingCategory::setFilterRules(QStringLiteral("krita.lib.flake=true"));

    const QLoggingCategory &category = FLAKE_LOG();
    QCOMPARE(QString::fromLatin1(category.categoryName()), QStringLiteral("krita.lib.flake"));
    QVERIFY(category.isDebugEnabled());

    QLoggingCategory::setFilterRules(QString());
}

QTEST_GUILESS_MAIN(FlakeDebugCompatibilityTest)

#include "FlakeDebugCompatibilityTest.moc"
