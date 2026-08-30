/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "CssFontStyleModel.h"

#include <QMetaEnum>
#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using StyleData = KoSvgText::CssFontStyleData;

void compareData(const lager::cursor<StyleData> &cursor, QFont::Style style, bool isAuto, qreal customValue)
{
    const StyleData actual = cursor.get();
    QCOMPARE(int(actual.style), int(style));
    QCOMPARE(actual.slantValue.isAuto, isAuto);
    QCOMPARE(actual.slantValue.customValue, customValue);
}

StyleData customSlant(QFont::Style style, qreal value)
{
    StyleData data(style);
    data.slantValue = KoSvgText::AutoValue(value);
    return data;
}

} // namespace

class CssFontStyleModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void fontStylesMirrorQFontAndAutomaticObliqueAngle();
    void nonObliqueValueWritesLeaveCursorUnchanged();
    void changingAwayFromObliqueResetsSlantToExplicitZero();
    void qObjectBaseProvidesVirtualLifetime();
};

void CssFontStyleModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    CssFontStyleModel model;

    compareData(model.data, QFont::StyleNormal, true, 0.0);
    QCOMPARE(model.style(), CssFontStyleModel::StyleNormal);
    QCOMPARE(model.value(), 0.0);

    for (const char *name : {"style", "value"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void CssFontStyleModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(StyleData(), lager::automatic_tag{});
    CssFontStyleModel model(state);
    QSignalSpy styleSpy(&model, &CssFontStyleModel::styleChanged);
    QSignalSpy valueSpy(&model, &CssFontStyleModel::valueChanged);

    QVERIFY(styleSpy.isValid());
    QVERIFY(valueSpy.isValid());

    state.set(StyleData(QFont::StyleOblique));

    compareData(model.data, QFont::StyleOblique, true, 0.0);
    QCOMPARE(model.style(), CssFontStyleModel::StyleOblique);
    QCOMPARE(model.value(), 14.0);
    QCOMPARE(styleSpy.size(), 1);
    QCOMPARE(valueSpy.size(), 1);

    model.setvalue(22.5);

    compareData(state, QFont::StyleOblique, false, 22.5);
    QCOMPARE(model.value(), 22.5);
    QCOMPARE(styleSpy.size(), 1);
    QCOMPARE(valueSpy.size(), 2);

    model.setstyle(CssFontStyleModel::StyleItalic);

    compareData(state, QFont::StyleItalic, false, 0.0);
    QCOMPARE(model.style(), CssFontStyleModel::StyleItalic);
    QCOMPARE(model.value(), 0.0);
    QCOMPARE(styleSpy.size(), 2);
    QCOMPARE(valueSpy.size(), 3);
}

void CssFontStyleModelContractTest::fontStylesMirrorQFontAndAutomaticObliqueAngle()
{
    QCOMPARE(int(CssFontStyleModel::StyleNormal), int(QFont::StyleNormal));
    QCOMPARE(int(CssFontStyleModel::StyleItalic), int(QFont::StyleItalic));
    QCOMPARE(int(CssFontStyleModel::StyleOblique), int(QFont::StyleOblique));

    const QMetaEnum styleEnum = QMetaEnum::fromType<CssFontStyleModel::FontStyle>();
    QVERIFY(styleEnum.isValid());
    QCOMPARE(styleEnum.keyToValue("StyleNormal"), int(QFont::StyleNormal));
    QCOMPARE(styleEnum.keyToValue("StyleItalic"), int(QFont::StyleItalic));
    QCOMPARE(styleEnum.keyToValue("StyleOblique"), int(QFont::StyleOblique));

    auto state = lager::make_state(StyleData(QFont::StyleOblique), lager::automatic_tag{});
    CssFontStyleModel model(state);

    compareData(state, QFont::StyleOblique, true, 0.0);
    QCOMPARE(model.value(), 14.0);

    model.setstyle(CssFontStyleModel::StyleItalic);
    compareData(state, QFont::StyleItalic, false, 0.0);
    QCOMPARE(model.value(), 0.0);

    model.setstyle(CssFontStyleModel::StyleOblique);
    compareData(state, QFont::StyleOblique, false, 0.0);
    QCOMPARE(model.value(), 0.0);

    model.setstyle(CssFontStyleModel::StyleNormal);
    compareData(state, QFont::StyleNormal, false, 0.0);
    QCOMPARE(model.value(), 0.0);
}

void CssFontStyleModelContractTest::nonObliqueValueWritesLeaveCursorUnchanged()
{
    for (const QFont::Style style : {QFont::StyleNormal, QFont::StyleItalic}) {
        auto state = lager::make_state(customSlant(style, 9.5), lager::automatic_tag{});
        CssFontStyleModel model(state);

        QCOMPARE(model.value(), 0.0);
        model.setvalue(-12.5);

        compareData(state, style, false, 9.5);
        QCOMPARE(model.value(), 0.0);
    }
}

void CssFontStyleModelContractTest::changingAwayFromObliqueResetsSlantToExplicitZero()
{
    for (const CssFontStyleModel::FontStyle destination :
         {CssFontStyleModel::StyleNormal, CssFontStyleModel::StyleItalic}) {
        auto state = lager::make_state(customSlant(QFont::StyleOblique, -17.25), lager::automatic_tag{});
        CssFontStyleModel model(state);

        QCOMPARE(model.value(), -17.25);
        model.setstyle(destination);

        compareData(state, QFont::Style(destination), false, 0.0);
        QCOMPARE(model.value(), 0.0);

        model.setstyle(CssFontStyleModel::StyleOblique);
        compareData(state, QFont::StyleOblique, false, 0.0);
        QCOMPARE(model.value(), 0.0);
    }
}

void CssFontStyleModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<CssFontStyleModel> model = new CssFontStyleModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(CssFontStyleModelContractTest)

#include "CssFontStyleModelContractTest.moc"
