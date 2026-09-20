/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ResourceDebug.h"

#include <QTest>

class ResourceDebugCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresTheSavedLogDockerFilter();
};

// Compatibility requirement: Log Docker's saved resources_30009 filter depends on the krita.lib.resources logging category.
void ResourceDebugCompatibilityTest::restoresTheSavedLogDockerFilter()
{
    // Consumer: Log Docker users enabling the saved resource-management diagnostic filter.
    // Operation: LibrePaint applies the saved logging rule for resource management.
    // Observable result: Resource debug messages are enabled under the established category name.
    // Failure impact: The user's resource-management diagnostics disappear after an upgrade.
    QLoggingCategory::setFilterRules(QStringLiteral("krita.lib.resources=true"));

    const QLoggingCategory &category = RESOURCE_LOG();
    QCOMPARE(QString::fromLatin1(category.categoryName()), QStringLiteral("krita.lib.resources"));
    QVERIFY(category.isDebugEnabled());

    QLoggingCategory::setFilterRules(QString());
}

QTEST_GUILESS_MAIN(ResourceDebugCompatibilityTest)

#include "ResourceDebugCompatibilityTest.moc"
