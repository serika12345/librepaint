/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_debug.h"

#include <QTest>

class KisDebugCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresTheSavedLogDockerFilters();
    void cleanup();
};

// Compatibility requirement: Log Docker's saved diagnostic filters depend on the established krita.* logging categories.
void KisDebugCompatibilityTest::restoresTheSavedLogDockerFilters()
{
    // Consumer: Log Docker users restoring saved diagnostic categories.
    // Operation: LibrePaint applies the saved logging rules for every supported global category.
    // Observable result: Each selected diagnostic category enables its debug messages.
    // Failure impact: An upgraded application silently omits diagnostics that the user explicitly enabled.
    QLoggingCategory::setFilterRules(QStringLiteral(
        "krita.lib.resources=true\n"
        "krita.general=true\n"
        "krita.core=true\n"
        "krita.registry=true\n"
        "krita.tools=true\n"
        "krita.tiles=true\n"
        "krita.filters=true\n"
        "krita.plugins=true\n"
        "krita.ui=true\n"
        "krita.file=true\n"
        "krita.math=true\n"
        "krita.grender=true\n"
        "krita.scripting=true\n"
        "krita.input=true\n"
        "krita.action=true\n"
        "krita.tablet=true\n"
        "krita.opengl=true\n"
        "krita.metadata=true"));

    QVERIFY(_30009().isDebugEnabled());
    QVERIFY(_41000().isDebugEnabled());
    QVERIFY(_41001().isDebugEnabled());
    QVERIFY(_41002().isDebugEnabled());
    QVERIFY(_41003().isDebugEnabled());
    QVERIFY(_41004().isDebugEnabled());
    QVERIFY(_41005().isDebugEnabled());
    QVERIFY(_41006().isDebugEnabled());
    QVERIFY(_41007().isDebugEnabled());
    QVERIFY(_41008().isDebugEnabled());
    QVERIFY(_41009().isDebugEnabled());
    QVERIFY(_41010().isDebugEnabled());
    QVERIFY(_41011().isDebugEnabled());
    QVERIFY(_41012().isDebugEnabled());
    QVERIFY(_41013().isDebugEnabled());
    QVERIFY(_41014().isDebugEnabled());
    QVERIFY(_41015().isDebugEnabled());
    QVERIFY(_41016().isDebugEnabled());

}

void KisDebugCompatibilityTest::cleanup()
{
    QLoggingCategory::setFilterRules(QString());
}

QTEST_GUILESS_MAIN(KisDebugCompatibilityTest)

#include "KisDebugCompatibilityTest.moc"
