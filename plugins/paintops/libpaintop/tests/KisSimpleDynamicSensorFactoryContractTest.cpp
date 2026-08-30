/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <functional>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include <KisSimpleDynamicSensorFactory.h>

#include <QTest>

class KisSimpleDynamicSensorFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorAndAccessorsPreserveIndependentConfiguration();
    void maximumAccessorsIgnoreLengthsAndWidgetCreationIsAbsent();
};

void KisSimpleDynamicSensorFactoryContractTest::constructorAndAccessorsPreserveIndependentConfiguration()
{
    KisSimpleDynamicSensorFactory factory(QStringLiteral("筆圧/Δ"),
                                          -17,
                                          109,
                                          QStringLiteral("最小-α"),
                                          QStringLiteral("最大-β"),
                                          QStringLiteral("°/秒"));

    QCOMPARE(factory.m_id, QStringLiteral("筆圧/Δ"));
    QCOMPARE(factory.m_minimumValue, -17);
    QCOMPARE(factory.m_maximumValue, 109);
    QCOMPARE(factory.m_minimumLabel, QStringLiteral("最小-α"));
    QCOMPARE(factory.m_maximumLabel, QStringLiteral("最大-β"));
    QCOMPARE(factory.m_valueSuffix, QStringLiteral("°/秒"));
    QCOMPARE(factory.id(), QStringLiteral("筆圧/Δ"));
    QCOMPARE(factory.minimumValue(), -17);
    QCOMPARE(factory.maximumValue(83), 109);
    QCOMPARE(factory.minimumLabel(), QStringLiteral("最小-α"));
    QCOMPARE(factory.maximumLabel(127), QStringLiteral("最大-β"));
    QCOMPARE(factory.valueSuffix(), QStringLiteral("°/秒"));

    factory.m_id = QStringLiteral("傾き/γ");
    QCOMPARE(factory.id(), QStringLiteral("傾き/γ"));
    QCOMPARE(factory.minimumValue(), -17);

    factory.m_minimumValue = -31;
    QCOMPARE(factory.minimumValue(), -31);
    QCOMPARE(factory.maximumValue(83), 109);

    factory.m_maximumValue = 211;
    QCOMPARE(factory.maximumValue(83), 211);
    QCOMPARE(factory.minimumLabel(), QStringLiteral("最小-α"));

    factory.m_minimumLabel = QStringLiteral("下限-δ");
    QCOMPARE(factory.minimumLabel(), QStringLiteral("下限-δ"));
    QCOMPARE(factory.maximumLabel(127), QStringLiteral("最大-β"));

    factory.m_maximumLabel = QStringLiteral("上限-ε");
    QCOMPARE(factory.maximumLabel(127), QStringLiteral("上限-ε"));
    QCOMPARE(factory.valueSuffix(), QStringLiteral("°/秒"));

    factory.m_valueSuffix = QStringLiteral("px/秒");
    QCOMPARE(factory.valueSuffix(), QStringLiteral("px/秒"));
    QCOMPARE(factory.id(), QStringLiteral("傾き/γ"));
    QCOMPARE(factory.minimumValue(), -31);
    QCOMPARE(factory.maximumValue(83), 211);
    QCOMPARE(factory.minimumLabel(), QStringLiteral("下限-δ"));
    QCOMPARE(factory.maximumLabel(127), QStringLiteral("上限-ε"));
}

void KisSimpleDynamicSensorFactoryContractTest::maximumAccessorsIgnoreLengthsAndWidgetCreationIsAbsent()
{
    KisSimpleDynamicSensorFactory factory(QStringLiteral("距離/ζ"),
                                          -23,
                                          307,
                                          QStringLiteral("開始-η"),
                                          QStringLiteral("終了-θ"),
                                          QStringLiteral("px"));

    QCOMPARE(factory.maximumValue(83), 307);
    QCOMPARE(factory.maximumValue(-19), 307);
    QCOMPARE(factory.maximumLabel(127), QStringLiteral("終了-θ"));
    QCOMPARE(factory.maximumLabel(0), QStringLiteral("終了-θ"));

    lager::cursor<KisCurveOptionDataCommon> cursor;
    QVERIFY(!cursor);
    int parentToken = 41;
    auto *parent = reinterpret_cast<QWidget *>(&parentToken);
    QCOMPARE(factory.createConfigWidget(cursor, parent), nullptr);
    QCOMPARE(parentToken, 41);
}

QTEST_GUILESS_MAIN(KisSimpleDynamicSensorFactoryContractTest)

#include "KisSimpleDynamicSensorFactoryContractTest.moc"
