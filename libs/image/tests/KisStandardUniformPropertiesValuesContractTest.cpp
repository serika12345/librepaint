/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_standard_uniform_properties_factory.h"

#include <QTest>

class KisStandardUniformPropertiesValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void standardPropertyIdentifiersRemainDistinctAndStable();
};

void KisStandardUniformPropertiesValuesContractTest::standardPropertyIdentifiersRemainDistinctAndStable()
{
    QCOMPARE(KisStandardUniformPropertiesFactory::size.id(), QStringLiteral("size"));
    QCOMPARE(KisStandardUniformPropertiesFactory::opacity.id(), QStringLiteral("opacity"));
    QCOMPARE(KisStandardUniformPropertiesFactory::flow.id(), QStringLiteral("flow"));
    QCOMPARE(KisStandardUniformPropertiesFactory::angle.id(), QStringLiteral("angle"));
    QCOMPARE(KisStandardUniformPropertiesFactory::spacing.id(), QStringLiteral("spacing"));
}

QTEST_APPLESS_MAIN(KisStandardUniformPropertiesValuesContractTest)

#include "KisStandardUniformPropertiesValuesContractTest.moc"
