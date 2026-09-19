/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QColor>
#include <QPen>
#include <QTest>
#include <QTransform>

#include <canvas/kis_guides_config.h>
#include <canvas/kis_guides_manager.h>


namespace
{
} // namespace

class KisGuidesConfigSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void guideValueIdentityAndEqualitySchemaRemainStable();
};

void KisGuidesConfigSchemaContractTest::guideValueIdentityAndEqualitySchemaRemainStable()
{

    QCOMPARE(int(KisGuidesConfig::LINE_SOLID), 0);
    QCOMPARE(int(KisGuidesConfig::LINE_DASHED), 1);
    QCOMPARE(int(KisGuidesConfig::LINE_DOTTED), 2);

}

QTEST_APPLESS_MAIN(KisGuidesConfigSchemaContractTest)

#include "KisGuidesConfigSchemaContractTest.moc"
