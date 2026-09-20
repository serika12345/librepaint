/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FontVariantNumericModel.h"

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using Numeric = KoSvgText::FontFeatureNumeric;

void compareNumeric(const lager::cursor<Numeric> &cursor,
                    KoSvgText::NumericFigureStyle style,
                    KoSvgText::NumericFigureSpacing spacing,
                    KoSvgText::NumericFractions fractions,
                    bool ordinals,
                    bool slashedZero)
{
    const Numeric actual = cursor.get();
    QCOMPARE(int(actual.style), int(style));
    QCOMPARE(int(actual.spacing), int(spacing));
    QCOMPARE(int(actual.fractions), int(fractions));
    QCOMPARE(actual.ordinals, ordinals);
    QCOMPARE(actual.slashedZero, slashedZero);
}

} // namespace

class FontVariantNumericModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void validEnumerationsRoundTripIndependently();
    void booleanPropertiesRemainIndependent();
    void qObjectBaseProvidesVirtualLifetime();
};

void FontVariantNumericModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    FontVariantNumericModel model;

    compareNumeric(model.data,
                   KoSvgText::NumericFigureStyleNormal,
                   KoSvgText::NumericFigureSpacingNormal,
                   KoSvgText::NumericFractionsNormal,
                   false,
                   false);
    QCOMPARE(model.figureStyle(), int(KoSvgText::NumericFigureStyleNormal));
    QCOMPARE(model.figureSpacing(), int(KoSvgText::NumericFigureSpacingNormal));
    QCOMPARE(model.fractions(), int(KoSvgText::NumericFractionsNormal));
    QVERIFY(!model.ordinals());
    QVERIFY(!model.slashedZero());

    for (const char *name : {"figureStyle", "figureSpacing", "fractions", "ordinals", "slashedZero"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void FontVariantNumericModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(Numeric(), lager::automatic_tag{});
    FontVariantNumericModel model(state);
    QSignalSpy styleSpy(&model, &FontVariantNumericModel::figureStyleChanged);
    QSignalSpy spacingSpy(&model, &FontVariantNumericModel::figureSpacingChanged);
    QSignalSpy fractionsSpy(&model, &FontVariantNumericModel::fractionsChanged);
    QSignalSpy ordinalsSpy(&model, &FontVariantNumericModel::ordinalsChanged);
    QSignalSpy zeroSpy(&model, &FontVariantNumericModel::slashedZeroChanged);

    QVERIFY(styleSpy.isValid());
    QVERIFY(spacingSpy.isValid());
    QVERIFY(fractionsSpy.isValid());
    QVERIFY(ordinalsSpy.isValid());
    QVERIFY(zeroSpy.isValid());

    Numeric external;
    external.style = KoSvgText::NumericFigureStyleOld;
    external.spacing = KoSvgText::NumericFigureSpacingTabular;
    external.fractions = KoSvgText::NumericFractionsStacked;
    external.ordinals = true;
    external.slashedZero = true;
    state.set(external);

    compareNumeric(model.data,
                   KoSvgText::NumericFigureStyleOld,
                   KoSvgText::NumericFigureSpacingTabular,
                   KoSvgText::NumericFractionsStacked,
                   true,
                   true);
    QCOMPARE(model.figureStyle(), int(KoSvgText::NumericFigureStyleOld));
    QCOMPARE(model.figureSpacing(), int(KoSvgText::NumericFigureSpacingTabular));
    QCOMPARE(model.fractions(), int(KoSvgText::NumericFractionsStacked));
    QVERIFY(model.ordinals());
    QVERIFY(model.slashedZero());
    QCOMPARE(styleSpy.size(), 1);
    QCOMPARE(spacingSpy.size(), 1);
    QCOMPARE(fractionsSpy.size(), 1);
    QCOMPARE(ordinalsSpy.size(), 1);
    QCOMPARE(zeroSpy.size(), 1);

    QVERIFY(model.setProperty("figureStyle", int(KoSvgText::NumericFigureStyleLining)));
    QVERIFY(model.setProperty("figureSpacing", int(KoSvgText::NumericFigureSpacingProportional)));
    QVERIFY(model.setProperty("fractions", int(KoSvgText::NumericFractionsDiagonal)));
    QVERIFY(model.setProperty("ordinals", false));
    QVERIFY(model.setProperty("slashedZero", false));

    compareNumeric(state,
                   KoSvgText::NumericFigureStyleLining,
                   KoSvgText::NumericFigureSpacingProportional,
                   KoSvgText::NumericFractionsDiagonal,
                   false,
                   false);
    QCOMPARE(styleSpy.size(), 2);
    QCOMPARE(spacingSpy.size(), 2);
    QCOMPARE(fractionsSpy.size(), 2);
    QCOMPARE(ordinalsSpy.size(), 2);
    QCOMPARE(zeroSpy.size(), 2);
}

void FontVariantNumericModelContractTest::validEnumerationsRoundTripIndependently()
{
    auto state = lager::make_state(Numeric(), lager::automatic_tag{});
    FontVariantNumericModel model(state);

    model.setordinals(true);
    model.setslashedZero(true);

    for (const KoSvgText::NumericFigureStyle value :
         {KoSvgText::NumericFigureStyleNormal, KoSvgText::NumericFigureStyleLining, KoSvgText::NumericFigureStyleOld}) {
        model.setfigureStyle(int(value));
        QCOMPARE(int(state.get().style), int(value));
        QCOMPARE(model.figureStyle(), int(value));
        QVERIFY(state.get().ordinals);
        QVERIFY(state.get().slashedZero);
    }

    for (const KoSvgText::NumericFigureSpacing value : {KoSvgText::NumericFigureSpacingNormal,
                                                        KoSvgText::NumericFigureSpacingProportional,
                                                        KoSvgText::NumericFigureSpacingTabular}) {
        model.setfigureSpacing(int(value));
        QCOMPARE(int(state.get().spacing), int(value));
        QCOMPARE(model.figureSpacing(), int(value));
        QCOMPARE(int(state.get().style), int(KoSvgText::NumericFigureStyleOld));
    }

    for (const KoSvgText::NumericFractions value :
         {KoSvgText::NumericFractionsNormal, KoSvgText::NumericFractionsDiagonal, KoSvgText::NumericFractionsStacked}) {
        model.setfractions(int(value));
        QCOMPARE(int(state.get().fractions), int(value));
        QCOMPARE(model.fractions(), int(value));
        QCOMPARE(int(state.get().spacing), int(KoSvgText::NumericFigureSpacingTabular));
    }
}

void FontVariantNumericModelContractTest::booleanPropertiesRemainIndependent()
{
    auto state = lager::make_state(Numeric(), lager::automatic_tag{});
    FontVariantNumericModel model(state);

    model.setordinals(true);
    QVERIFY(state.get().ordinals);
    QVERIFY(!state.get().slashedZero);

    model.setslashedZero(true);
    QVERIFY(state.get().ordinals);
    QVERIFY(state.get().slashedZero);

    model.setordinals(false);
    QVERIFY(!state.get().ordinals);
    QVERIFY(state.get().slashedZero);
}

void FontVariantNumericModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<FontVariantNumericModel> model = new FontVariantNumericModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(FontVariantNumericModelContractTest)

#include "FontVariantNumericModelContractTest.moc"
