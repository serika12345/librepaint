/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "CssLengthPercentageModel.h"

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using Length = KoSvgText::CssLengthPercentage;

void compareLength(const lager::cursor<Length> &cursor, qreal value, Length::UnitType unit)
{
    const Length actual = cursor.get();
    QCOMPARE(actual.value, value);
    QCOMPARE(actual.unit, unit);
}

} // namespace

class CssLengthPercentageModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void absoluteAndRelativeValuesRemainUnscaled();
    void percentageValuesUseUiScaling();
    void unitChangesDoNotConvertStoredMagnitude();
    void qObjectBaseProvidesVirtualLifetime();
};

void CssLengthPercentageModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    CssLengthPercentageModel model;

    compareLength(model.length, 0.0, Length::Absolute);
    QCOMPARE(model.value(), 0.0);
    QCOMPARE(model.unitType(), int(Length::Absolute));

    for (const char *name : {"value", "unitType"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void CssLengthPercentageModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(Length(2.0, Length::Em), lager::automatic_tag{});
    CssLengthPercentageModel model(state);
    QSignalSpy valueSpy(&model, &CssLengthPercentageModel::valueChanged);
    QSignalSpy unitSpy(&model, &CssLengthPercentageModel::unitTypeChanged);

    QVERIFY(valueSpy.isValid());
    QVERIFY(unitSpy.isValid());
    compareLength(model.length, 2.0, Length::Em);

    state.set(Length(0.25, Length::Percentage));
    compareLength(model.length, 0.25, Length::Percentage);
    QCOMPARE(model.value(), 25.0);
    QCOMPARE(model.unitType(), int(Length::Percentage));
    QCOMPARE(valueSpy.size(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), 25.0);
    QCOMPARE(unitSpy.size(), 1);
    QCOMPARE(unitSpy.at(0).at(0).toInt(), int(Length::Percentage));

    QVERIFY(model.setProperty("value", 50.0));
    compareLength(state, 0.5, Length::Percentage);
    QCOMPARE(valueSpy.size(), 2);
    QCOMPARE(valueSpy.at(1).at(0).toDouble(), 50.0);

    QVERIFY(model.setProperty("unitType", int(Length::Em)));
    compareLength(state, 0.5, Length::Em);
    QCOMPARE(model.value(), 0.5);
    QCOMPARE(unitSpy.size(), 2);
    QCOMPARE(unitSpy.at(1).at(0).toInt(), int(Length::Em));
}

void CssLengthPercentageModelContractTest::absoluteAndRelativeValuesRemainUnscaled()
{
    for (const Length::UnitType unit : {Length::Absolute, Length::Em}) {
        auto state = lager::make_state(Length(3.25, unit), lager::automatic_tag{});
        CssLengthPercentageModel model(state);
        QSignalSpy valueSpy(&model, &CssLengthPercentageModel::valueChanged);

        QCOMPARE(model.value(), 3.25);
        model.setvalue(7.5);

        compareLength(state, 7.5, unit);
        QCOMPARE(model.value(), 7.5);
        QCOMPARE(valueSpy.size(), 1);
        QCOMPARE(valueSpy.at(0).at(0).toDouble(), 7.5);
    }
}

void CssLengthPercentageModelContractTest::percentageValuesUseUiScaling()
{
    auto state = lager::make_state(Length(0.375, Length::Percentage), lager::automatic_tag{});
    CssLengthPercentageModel model(state);
    QSignalSpy valueSpy(&model, &CssLengthPercentageModel::valueChanged);

    QCOMPARE(model.value(), 37.5);
    model.setvalue(62.5);

    compareLength(state, 0.625, Length::Percentage);
    QCOMPARE(model.value(), 62.5);
    QCOMPARE(valueSpy.size(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), 62.5);
}

void CssLengthPercentageModelContractTest::unitChangesDoNotConvertStoredMagnitude()
{
    auto state = lager::make_state(Length(0.5, Length::Percentage), lager::automatic_tag{});
    CssLengthPercentageModel model(state);
    QSignalSpy valueSpy(&model, &CssLengthPercentageModel::valueChanged);
    QSignalSpy unitSpy(&model, &CssLengthPercentageModel::unitTypeChanged);

    QCOMPARE(model.value(), 50.0);
    model.setunitType(int(Length::Absolute));

    compareLength(state, 0.5, Length::Absolute);
    QCOMPARE(model.value(), 0.5);
    QCOMPARE(valueSpy.size(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), 0.5);
    QCOMPARE(unitSpy.size(), 1);
    QCOMPARE(unitSpy.at(0).at(0).toInt(), int(Length::Absolute));
}

void CssLengthPercentageModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<CssLengthPercentageModel> model = new CssLengthPercentageModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(CssLengthPercentageModelContractTest)

#include "CssLengthPercentageModelContractTest.moc"
