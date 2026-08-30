/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TextIndentModel.h"

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using Indent = KoSvgText::TextIndentInfo;
using Length = KoSvgText::CssLengthPercentage;

void compareLength(const lager::cursor<Length> &cursor, qreal value, Length::UnitType unit)
{
    const Length actual = cursor.get();
    QCOMPARE(actual.value, value);
    QCOMPARE(actual.unit, unit);
}

void compareIndent(const lager::cursor<Indent> &cursor,
                   qreal lengthValue,
                   Length::UnitType lengthUnit,
                   bool hanging,
                   bool eachLine)
{
    const Indent actual = cursor.get();
    QCOMPARE(actual.length.value, lengthValue);
    QCOMPARE(actual.length.unit, lengthUnit);
    QCOMPARE(actual.hanging, hanging);
    QCOMPARE(actual.eachLine, eachLine);
}

} // namespace

class TextIndentModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void publicLengthViewsRemainAliased();
    void booleanFlagsRoundTripIndependently();
    void firstParentChangeInitializesLengthObservationBeforeLengthOnlyUpdates();
    void qObjectBaseAndChildProvideStableLifetime();
};

void TextIndentModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    TextIndentModel model;

    compareIndent(model.data, 0.0, Length::Absolute, false, false);
    compareLength(model.lengthData, 0.0, Length::Absolute);
    compareLength(model.lengthModel.length, 0.0, Length::Absolute);
    QCOMPARE(model.length(), &model.lengthModel);
    QVERIFY(!model.hanging());
    QVERIFY(!model.eachLine());

    const int lengthPropertyIndex = model.metaObject()->indexOfProperty("length");
    QVERIFY(lengthPropertyIndex >= 0);
    const QMetaProperty lengthProperty = model.metaObject()->property(lengthPropertyIndex);
    QVERIFY(lengthProperty.isReadable());
    QVERIFY(!lengthProperty.isWritable());
    QVERIFY(lengthProperty.hasNotifySignal());

    for (const char *name : {"hanging", "eachLine"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void TextIndentModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(Indent(), lager::automatic_tag{});
    TextIndentModel model(state);
    QSignalSpy lengthSpy(&model, &TextIndentModel::lengthChanged);
    QSignalSpy hangingSpy(&model, &TextIndentModel::hangingChanged);
    QSignalSpy eachLineSpy(&model, &TextIndentModel::eachLineChanged);

    QVERIFY(lengthSpy.isValid());
    QVERIFY(hangingSpy.isValid());
    QVERIFY(eachLineSpy.isValid());

    Indent external;
    external.length = Length(0.25, Length::Percentage);
    external.hanging = true;
    external.eachLine = true;
    state.set(external);

    compareIndent(model.data, 0.25, Length::Percentage, true, true);
    compareLength(model.lengthData, 0.25, Length::Percentage);
    compareLength(model.lengthModel.length, 0.25, Length::Percentage);
    QCOMPARE(model.lengthModel.value(), 25.0);
    QCOMPARE(model.lengthModel.unitType(), int(Length::Percentage));
    QVERIFY(model.hanging());
    QVERIFY(model.eachLine());
    QCOMPARE(lengthSpy.size(), 1);
    QCOMPARE(hangingSpy.size(), 1);
    QCOMPARE(eachLineSpy.size(), 1);

    QVERIFY(model.setProperty("hanging", false));
    QVERIFY(model.setProperty("eachLine", false));
    QVERIFY(model.length()->setProperty("value", 50.0));

    compareIndent(state, 0.5, Length::Percentage, false, false);
    QCOMPARE(lengthSpy.size(), 2);
    QCOMPARE(hangingSpy.size(), 2);
    QCOMPARE(eachLineSpy.size(), 2);
}

void TextIndentModelContractTest::publicLengthViewsRemainAliased()
{
    TextIndentModel model;

    QCOMPARE(model.length(), &model.lengthModel);

    Indent direct;
    direct.length = Length(3.0, Length::Em);
    direct.hanging = true;
    model.data.set(direct);

    compareLength(model.lengthData, 3.0, Length::Em);
    compareLength(model.lengthModel.length, 3.0, Length::Em);

    model.lengthData.set(Length(4.0, Length::Ex));
    compareIndent(model.data, 4.0, Length::Ex, true, false);
    compareLength(model.lengthModel.length, 4.0, Length::Ex);

    model.lengthModel.setunitType(int(Length::Cap));
    compareIndent(model.data, 4.0, Length::Cap, true, false);
    compareLength(model.lengthData, 4.0, Length::Cap);
    QCOMPARE(model.length(), &model.lengthModel);
}

void TextIndentModelContractTest::booleanFlagsRoundTripIndependently()
{
    Indent initial;
    initial.length = Length(4.25, Length::Cap);
    auto state = lager::make_state(initial, lager::automatic_tag{});
    TextIndentModel model(state);

    const struct {
        bool hanging;
        bool eachLine;
    } cases[] = {
        {false, false},
        {true, false},
        {false, true},
        {true, true},
    };

    for (const auto &entry : cases) {
        model.sethanging(entry.hanging);
        model.seteachLine(entry.eachLine);

        compareIndent(state, 4.25, Length::Cap, entry.hanging, entry.eachLine);
        QCOMPARE(model.hanging(), entry.hanging);
        QCOMPARE(model.eachLine(), entry.eachLine);
    }
}

void TextIndentModelContractTest::firstParentChangeInitializesLengthObservationBeforeLengthOnlyUpdates()
{
    TextIndentModel model;
    QSignalSpy lengthSpy(&model, &TextIndentModel::lengthChanged);

    QVERIFY(lengthSpy.isValid());
    compareLength(model.lengthData, 0.0, Length::Absolute);
    QCOMPARE(lengthSpy.size(), 0);

    model.sethanging(true);
    compareLength(model.lengthData, 0.0, Length::Absolute);
    QCOMPARE(lengthSpy.size(), 1);

    model.seteachLine(true);
    compareLength(model.lengthData, 0.0, Length::Absolute);
    QCOMPARE(lengthSpy.size(), 1);

    model.lengthData.set(Length(2.0, Length::Em));
    QCOMPARE(lengthSpy.size(), 2);

    model.sethanging(false);
    model.seteachLine(false);
    QCOMPARE(lengthSpy.size(), 2);
}

void TextIndentModelContractTest::qObjectBaseAndChildProvideStableLifetime()
{
    QPointer<TextIndentModel> model = new TextIndentModel;
    QPointer<CssLengthPercentageModel> length = model->length();
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
    QVERIFY(length.isNull());
}

QTEST_GUILESS_MAIN(TextIndentModelContractTest)

#include "TextIndentModelContractTest.moc"
