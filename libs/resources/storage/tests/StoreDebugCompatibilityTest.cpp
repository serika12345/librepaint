/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <StoreDebug.h>

#include <QTest>

class StoreDebugCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresTheSavedLogDockerFilter();
};

// Compatibility requirement: Log Docker's saved file_41008 filter depends on the krita.lib.store logging category.
void StoreDebugCompatibilityTest::restoresTheSavedLogDockerFilter()
{
    // Consumer: Log Docker users enabling the saved file loading and saving diagnostic filter.
    // Operation: LibrePaint applies the saved logging rule for the storage category.
    // Observable result: Storage debug messages are enabled under the established category name.
    // Failure impact: The user's file-operation diagnostics disappear after an upgrade.
    QLoggingCategory::setFilterRules(QStringLiteral("krita.lib.store=true"));

    const QLoggingCategory &category = STORE_LOG();
    QCOMPARE(QString::fromLatin1(category.categoryName()), QStringLiteral("krita.lib.store"));
    QVERIFY(category.isDebugEnabled());

    QLoggingCategory::setFilterRules(QString());
}

QTEST_GUILESS_MAIN(StoreDebugCompatibilityTest)

#include "StoreDebugCompatibilityTest.moc"
