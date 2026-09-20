/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "LineHeightModel.h"

#include <QMetaEnum>
#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using Length = KoSvgText::CssLengthPercentage;
using LineHeight = KoSvgText::LineHeightInfo;

void compareStorage(const lager::cursor<LineHeight> &cursor,
                    qreal number,
                    bool isNumber,
                    qreal lengthValue,
                    Length::UnitType lengthUnit,
                    bool isNormal)
{
    const LineHeight actual = cursor.get();
    QCOMPARE(actual.value, number);
    QCOMPARE(actual.isNumber, isNumber);
    QCOMPARE(actual.length.value, lengthValue);
    QCOMPARE(actual.length.unit, lengthUnit);
    QCOMPARE(actual.isNormal, isNormal);
}

} // namespace

class LineHeightModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void lineHeightTypeValuesAndMetaObjectRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void allUnitsMapToStorageWithoutConvertingMagnitude();
    void percentageAndOtherMagnitudesUseStableScaling();
    void numberLengthAndNormalStateRemainIndependent();
    void qObjectBaseProvidesVirtualLifetime();
};

void LineHeightModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    LineHeightModel model;

    compareStorage(model.data, 1.0, false, 0.0, Length::Absolute, true);
    QVERIFY(model.isNormal());
    QCOMPARE(model.value(), 0.0);
    QCOMPARE(int(model.unit()), int(LineHeightModel::Absolute));

    for (const char *name : {"isNormal", "value", "unit"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void LineHeightModelContractTest::lineHeightTypeValuesAndMetaObjectRemainStable()
{
    const struct {
        LineHeightModel::LineHeightType value;
        int integer;
        const char *key;
    } cases[] = {
        {LineHeightModel::Absolute, 0, "Absolute"},
        {LineHeightModel::Em, 1, "Em"},
        {LineHeightModel::Ex, 2, "Ex"},
        {LineHeightModel::Cap, 3, "Cap"},
        {LineHeightModel::Ch, 4, "Ch"},
        {LineHeightModel::Ic, 5, "Ic"},
        {LineHeightModel::Lh, 6, "Lh"},
        {LineHeightModel::Percentage, 7, "Percentage"},
        {LineHeightModel::Lines, 8, "Lines"},
    };

    const QMetaEnum metaEnum = QMetaEnum::fromType<LineHeightModel::LineHeightType>();
    QVERIFY(metaEnum.isValid());
    QCOMPARE(metaEnum.keyCount(), 9);

    for (const auto &entry : cases) {
        QCOMPARE(int(entry.value), entry.integer);
        QCOMPARE(metaEnum.keyToValue(entry.key), entry.integer);
        QCOMPARE(QString::fromLatin1(metaEnum.valueToKey(entry.integer)), QString::fromLatin1(entry.key));
    }
}

void LineHeightModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(LineHeight(), lager::automatic_tag{});
    LineHeightModel model(state);
    QSignalSpy normalSpy(&model, &LineHeightModel::isNormalChanged);
    QSignalSpy valueSpy(&model, &LineHeightModel::valueChanged);
    QSignalSpy unitSpy(&model, &LineHeightModel::unitChanged);

    QVERIFY(normalSpy.isValid());
    QVERIFY(valueSpy.isValid());
    QVERIFY(unitSpy.isValid());

    LineHeight external;
    external.length = Length(0.375, Length::Percentage);
    external.value = 1.5;
    external.isNumber = false;
    external.isNormal = false;
    state.set(external);

    compareStorage(model.data, 1.5, false, 0.375, Length::Percentage, false);
    QVERIFY(!model.isNormal());
    QCOMPARE(model.value(), 37.5);
    QCOMPARE(int(model.unit()), int(LineHeightModel::Percentage));
    QCOMPARE(normalSpy.size(), 1);
    QCOMPARE(valueSpy.size(), 1);
    QCOMPARE(unitSpy.size(), 1);

    QVERIFY(model.setProperty("value", 62.5));
    QVERIFY(model.setProperty("unit", int(LineHeightModel::Lines)));
    QVERIFY(model.setProperty("value", 2.25));
    QVERIFY(model.setProperty("isNormal", true));

    compareStorage(state, 2.25, true, 0.625, Length::Percentage, true);
    QCOMPARE(normalSpy.size(), 2);
    QCOMPARE(valueSpy.size(), 4);
    QCOMPARE(unitSpy.size(), 2);
}

void LineHeightModelContractTest::allUnitsMapToStorageWithoutConvertingMagnitude()
{
    const struct {
        LineHeightModel::LineHeightType modelUnit;
        Length::UnitType storageUnit;
    } cases[] = {
        {LineHeightModel::Absolute, Length::Absolute},
        {LineHeightModel::Em, Length::Em},
        {LineHeightModel::Ex, Length::Ex},
        {LineHeightModel::Cap, Length::Cap},
        {LineHeightModel::Ch, Length::Ch},
        {LineHeightModel::Ic, Length::Ic},
        {LineHeightModel::Lh, Length::Lh},
        {LineHeightModel::Percentage, Length::Percentage},
    };

    for (const auto &entry : cases) {
        LineHeight initial;
        initial.length = Length(2.5, Length::Absolute);
        initial.value = 1.75;
        initial.isNumber = false;
        initial.isNormal = false;
        auto state = lager::make_state(initial, lager::automatic_tag{});
        LineHeightModel model(state);

        model.setunit(entry.modelUnit);

        compareStorage(state, 1.75, false, 2.5, entry.storageUnit, false);
        QCOMPARE(int(model.unit()), int(entry.modelUnit));
    }

    LineHeight initial;
    initial.length = Length(0.625, Length::Percentage);
    initial.value = 1.75;
    initial.isNumber = false;
    initial.isNormal = false;
    auto state = lager::make_state(initial, lager::automatic_tag{});
    LineHeightModel model(state);

    model.setunit(LineHeightModel::Lines);

    compareStorage(state, 1.75, true, 0.625, Length::Percentage, false);
    QCOMPARE(model.value(), 1.75);
    QCOMPARE(int(model.unit()), int(LineHeightModel::Lines));
}

void LineHeightModelContractTest::percentageAndOtherMagnitudesUseStableScaling()
{
    LineHeight percentage;
    percentage.length = Length(0.375, Length::Percentage);
    percentage.isNumber = false;
    auto percentageState = lager::make_state(percentage, lager::automatic_tag{});
    LineHeightModel percentageModel(percentageState);

    QCOMPARE(percentageModel.value(), 37.5);
    percentageModel.setvalue(62.5);
    compareStorage(percentageState, 1.0, false, 0.625, Length::Percentage, true);

    for (const Length::UnitType unit : {Length::Absolute, Length::Em}) {
        LineHeight length;
        length.length = Length(3.25, unit);
        length.isNumber = false;
        auto state = lager::make_state(length, lager::automatic_tag{});
        LineHeightModel model(state);

        QCOMPARE(model.value(), 3.25);
        model.setvalue(7.5);

        compareStorage(state, 1.0, false, 7.5, unit, true);
        QCOMPARE(model.value(), 7.5);
    }

    LineHeight lines;
    lines.length = Length(0.625, Length::Percentage);
    lines.value = 1.75;
    lines.isNumber = true;
    auto linesState = lager::make_state(lines, lager::automatic_tag{});
    LineHeightModel linesModel(linesState);

    QCOMPARE(linesModel.value(), 1.75);
    linesModel.setvalue(2.25);
    compareStorage(linesState, 2.25, true, 0.625, Length::Percentage, true);
}

void LineHeightModelContractTest::numberLengthAndNormalStateRemainIndependent()
{
    LineHeight initial;
    initial.length = Length(0.625, Length::Percentage);
    initial.value = 1.75;
    initial.isNumber = true;
    initial.isNormal = true;
    auto state = lager::make_state(initial, lager::automatic_tag{});
    LineHeightModel model(state);

    model.setunit(LineHeightModel::Percentage);
    compareStorage(state, 1.75, false, 0.625, Length::Percentage, true);
    QCOMPARE(model.value(), 62.5);

    model.setunit(LineHeightModel::Em);
    compareStorage(state, 1.75, false, 0.625, Length::Em, true);
    QCOMPARE(model.value(), 0.625);

    model.setisNormal(false);
    compareStorage(state, 1.75, false, 0.625, Length::Em, false);

    model.setunit(LineHeightModel::Lines);
    compareStorage(state, 1.75, true, 0.625, Length::Em, false);
    QCOMPARE(model.value(), 1.75);

    model.setvalue(2.25);
    compareStorage(state, 2.25, true, 0.625, Length::Em, false);

    model.setunit(LineHeightModel::Percentage);
    compareStorage(state, 2.25, false, 0.625, Length::Percentage, false);
    QCOMPARE(model.value(), 62.5);
}

void LineHeightModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<LineHeightModel> model = new LineHeightModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(LineHeightModelContractTest)

#include "LineHeightModelContractTest.moc"
