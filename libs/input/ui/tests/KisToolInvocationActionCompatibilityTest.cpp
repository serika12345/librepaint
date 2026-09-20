/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_alternate_invocation_action.h>
#include <kis_shortcut_configuration.h>
#include <kis_tool_invocation_action.h>

#include <QTest>

// Compatibility requirement: Saved input profile files depend on Tool Invocation and Alternate Invocation shortcut mode values.

namespace
{
void verifySavedMode(KisAbstractInputAction &action, int mode, const QString &expectedAction)
{
    KisShortcutConfiguration savedShortcut;
    savedShortcut.setActionId(action.id());

    QVERIFY(savedShortcut.unserialize(
        QStringLiteral("{%1;2;[];1;0;0}").arg(QString::number(mode, 16))));
    QCOMPARE(action.shortcutIndexes().key(savedShortcut.mode()), expectedAction);
}
} // namespace

class KisToolInvocationActionCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void savedToolInvocationModesKeepTheirActions_data();
    void savedToolInvocationModesKeepTheirActions();
    void savedAlternateInvocationModesKeepTheirActions_data();
    void savedAlternateInvocationModesKeepTheirActions();
};

void KisToolInvocationActionCompatibilityTest::savedToolInvocationModesKeepTheirActions_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QString>("expectedAction");

    QTest::newRow("activate") << 0 << QStringLiteral("Activate");
    QTest::newRow("confirm") << 1 << QStringLiteral("Confirm");
    QTest::newRow("cancel") << 2 << QStringLiteral("Cancel");
    QTest::newRow("line") << 3 << QStringLiteral("Activate Line Tool");
    QTest::newRow("ellipse") << 4 << QStringLiteral("Activate Ellipse Tool");
    QTest::newRow("rectangle") << 5 << QStringLiteral("Activate Rectangle Tool");
    QTest::newRow("move") << 6 << QStringLiteral("Activate Move Tool");
    QTest::newRow("fill") << 7 << QStringLiteral("Activate Fill Tool");
    QTest::newRow("gradient") << 8 << QStringLiteral("Activate Gradient Tool");
    QTest::newRow("measure") << 9 << QStringLiteral("Activate Measure Tool");
    QTest::newRow("elliptical-selection") << 10 << QStringLiteral("Activate Elliptical Selection Tool");
    QTest::newRow("rectangular-selection") << 11 << QStringLiteral("Activate Rectangular Selection Tool");
    QTest::newRow("contiguous-selection") << 12 << QStringLiteral("Activate Contiguous Selection Tool");
    QTest::newRow("freehand-selection") << 13 << QStringLiteral("Activate Freehand Selection Tool");
    QTest::newRow("other-color") << 14 << QStringLiteral("Activate with Other Color");
}

void KisToolInvocationActionCompatibilityTest::savedToolInvocationModesKeepTheirActions()
{
    QFETCH(int, mode);
    QFETCH(QString, expectedAction);

    KisToolInvocationAction action;
    verifySavedMode(action, mode, expectedAction);
}

void KisToolInvocationActionCompatibilityTest::savedAlternateInvocationModesKeepTheirActions_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QString>("expectedAction");

    QTest::newRow("primary") << 0 << QStringLiteral("Primary Mode");
    QTest::newRow("secondary") << 1 << QStringLiteral("Secondary Mode");
    QTest::newRow("foreground-layer") << 2 << QStringLiteral("Sample Foreground Color from Current Layer");
    QTest::newRow("background-layer") << 3 << QStringLiteral("Sample Background Color from Current Layer");
    QTest::newRow("foreground-image") << 4 << QStringLiteral("Sample Foreground Color from Merged Image");
    QTest::newRow("background-image") << 5 << QStringLiteral("Sample Background Color from Merged Image");
    QTest::newRow("tertiary") << 6 << QStringLiteral("Tertiary Mode");
}

void KisToolInvocationActionCompatibilityTest::savedAlternateInvocationModesKeepTheirActions()
{
    QFETCH(int, mode);
    QFETCH(QString, expectedAction);

    KisAlternateInvocationAction action;
    verifySavedMode(action, mode, expectedAction);
}

QTEST_GUILESS_MAIN(KisToolInvocationActionCompatibilityTest)

#include "KisToolInvocationActionCompatibilityTest.moc"
