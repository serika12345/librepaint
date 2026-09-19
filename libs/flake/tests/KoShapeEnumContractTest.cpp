/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeFactoryBase.h>

#include <QTest>

class KoShapeEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeTemplateDefaultsAndValues();
};

void KoShapeEnumContractTest::shapeTemplateDefaultsAndValues()
{
    KoShapeTemplate value;
    QVERIFY(value.id.isEmpty());
    QVERIFY(value.templateId.isEmpty());
    QVERIFY(value.name.isEmpty());
    QVERIFY(value.family.isEmpty());
    QVERIFY(value.toolTip.isEmpty());
    QVERIFY(value.iconName.isEmpty());
    QCOMPARE(value.properties, nullptr);

    value.name = QStringLiteral("source");
    const auto *propertiesMarker = reinterpret_cast<const KoProperties *>(&value);
    value.properties = propertiesMarker;
    KoShapeTemplate copy = value;
    value.name = QStringLiteral("changed");
    QCOMPARE(copy.name, QStringLiteral("source"));
    QCOMPARE(copy.properties, propertiesMarker);
}

QTEST_GUILESS_MAIN(KoShapeEnumContractTest)

#include "KoShapeEnumContractTest.moc"
