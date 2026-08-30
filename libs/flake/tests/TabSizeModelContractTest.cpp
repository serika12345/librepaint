/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TabSizeModel.h"

#include <QMetaEnum>
#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using Length = KoSvgText::CssLengthPercentage;
using TabSize = KoSvgText::TabSizeInfo;

void compareStorage(const lager::cursor<TabSize> &cursor,
                    qreal number,
                    bool isNumber,
                    qreal lengthValue,
                    Length::UnitType lengthUnit,
                    qreal extraSpacing = 0.0)
{
    const TabSize actual = cursor.get();
    QCOMPARE(actual.value, number);
    QCOMPARE(actual.isNumber, isNumber);
    QCOMPARE(actual.length.value, lengthValue);
    QCOMPARE(actual.length.unit, lengthUnit);
    QCOMPARE(actual.extraSpacing, extraSpacing);
}

} // namespace

class TabSizeModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void tabSizeTypeValuesAndMetaObjectRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void allUnitsMapToStorageWithoutConvertingMagnitude();
    void numberAndLengthMagnitudesRemainIndependentAcrossUnitSwitches();
    void qObjectBaseProvidesVirtualLifetime();
};

void TabSizeModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    TabSizeModel model;

    compareStorage(model.data, 8.0, true, 0.0, Length::Absolute);
    QCOMPARE(model.value(), 8.0);
    QCOMPARE(int(model.unit()), int(TabSizeModel::Spaces));

    for (const char *name : {"value", "unit"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void TabSizeModelContractTest::tabSizeTypeValuesAndMetaObjectRemainStable()
{
    const struct {
        TabSizeModel::TabSizeType value;
        int integer;
        const char *key;
    } cases[] = {
        {TabSizeModel::Absolute, 0, "Absolute"},
        {TabSizeModel::Em, 1, "Em"},
        {TabSizeModel::Ex, 2, "Ex"},
        {TabSizeModel::Cap, 3, "Cap"},
        {TabSizeModel::Ch, 4, "Ch"},
        {TabSizeModel::Ic, 5, "Ic"},
        {TabSizeModel::Lh, 6, "Lh"},
        {TabSizeModel::Spaces, 7, "Spaces"},
    };

    const QMetaEnum metaEnum = QMetaEnum::fromType<TabSizeModel::TabSizeType>();
    QVERIFY(metaEnum.isValid());
    QCOMPARE(metaEnum.keyCount(), 8);

    for (const auto &entry : cases) {
        QCOMPARE(int(entry.value), entry.integer);
        QCOMPARE(metaEnum.keyToValue(entry.key), entry.integer);
        QCOMPARE(QString::fromLatin1(metaEnum.valueToKey(entry.integer)), QString::fromLatin1(entry.key));
    }
}

void TabSizeModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(TabSize(), lager::automatic_tag{});
    TabSizeModel model(state);
    QSignalSpy valueSpy(&model, &TabSizeModel::valueChanged);
    QSignalSpy unitSpy(&model, &TabSizeModel::unitChanged);

    QVERIFY(valueSpy.isValid());
    QVERIFY(unitSpy.isValid());

    TabSize external;
    external.value = 11.0;
    external.isNumber = false;
    external.length = Length(2.5, Length::Em);
    state.set(external);

    compareStorage(model.data, 11.0, false, 2.5, Length::Em);
    QCOMPARE(model.value(), 2.5);
    QCOMPARE(int(model.unit()), int(TabSizeModel::Em));
    QCOMPARE(valueSpy.size(), 1);
    QCOMPARE(unitSpy.size(), 1);

    QVERIFY(model.setProperty("value", 4.5));
    QVERIFY(model.setProperty("unit", int(TabSizeModel::Ex)));

    compareStorage(state, 11.0, false, 4.5, Length::Ex);
    QCOMPARE(valueSpy.size(), 2);
    QCOMPARE(unitSpy.size(), 2);
}

void TabSizeModelContractTest::allUnitsMapToStorageWithoutConvertingMagnitude()
{
    const struct {
        TabSizeModel::TabSizeType modelUnit;
        Length::UnitType storageUnit;
    } cases[] = {
        {TabSizeModel::Absolute, Length::Absolute},
        {TabSizeModel::Em, Length::Em},
        {TabSizeModel::Ex, Length::Ex},
        {TabSizeModel::Cap, Length::Cap},
        {TabSizeModel::Ch, Length::Ch},
        {TabSizeModel::Ic, Length::Ic},
        {TabSizeModel::Lh, Length::Lh},
    };

    for (const auto &entry : cases) {
        TabSize initial;
        initial.value = 13.0;
        initial.isNumber = false;
        initial.length = Length(2.5, Length::Absolute);
        auto state = lager::make_state(initial, lager::automatic_tag{});
        TabSizeModel model(state);

        model.setunit(entry.modelUnit);

        compareStorage(state, 13.0, false, 2.5, entry.storageUnit);
        QCOMPARE(model.value(), 2.5);
        QCOMPARE(int(model.unit()), int(entry.modelUnit));
    }

    TabSize initial;
    initial.value = 13.0;
    initial.isNumber = false;
    initial.length = Length(2.5, Length::Em);
    auto state = lager::make_state(initial, lager::automatic_tag{});
    TabSizeModel model(state);

    model.setunit(TabSizeModel::Spaces);

    compareStorage(state, 13.0, true, 2.5, Length::Em);
    QCOMPARE(model.value(), 13.0);
    QCOMPARE(int(model.unit()), int(TabSizeModel::Spaces));
}

void TabSizeModelContractTest::numberAndLengthMagnitudesRemainIndependentAcrossUnitSwitches()
{
    TabSize initial;
    initial.value = 12.5;
    initial.isNumber = true;
    initial.length = Length(3.25, Length::Em);
    auto state = lager::make_state(initial, lager::automatic_tag{});
    TabSizeModel model(state);

    model.setunit(TabSizeModel::Ex);
    compareStorage(state, 12.5, false, 3.25, Length::Ex);
    QCOMPARE(model.value(), 3.25);

    model.setvalue(4.5);
    compareStorage(state, 12.5, false, 4.5, Length::Ex);

    model.setunit(TabSizeModel::Spaces);
    compareStorage(state, 12.5, true, 4.5, Length::Ex);
    QCOMPARE(model.value(), 12.5);

    model.setvalue(9.75);
    compareStorage(state, 9.75, true, 4.5, Length::Ex);

    model.setunit(TabSizeModel::Lh);
    compareStorage(state, 9.75, false, 4.5, Length::Lh);
    QCOMPARE(model.value(), 4.5);
}

void TabSizeModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<TabSizeModel> model = new TabSizeModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(TabSizeModelContractTest)

#include "TabSizeModelContractTest.moc"
