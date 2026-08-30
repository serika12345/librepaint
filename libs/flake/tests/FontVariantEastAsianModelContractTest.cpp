/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FontVariantEastAsianModel.h"

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using EastAsian = KoSvgText::FontFeatureEastAsian;

void compareEastAsian(const lager::cursor<EastAsian> &cursor,
                      KoSvgText::EastAsianVariant variant,
                      KoSvgText::EastAsianWidth width,
                      bool ruby)
{
    const EastAsian actual = cursor.get();
    QCOMPARE(int(actual.variant), int(variant));
    QCOMPARE(int(actual.width), int(width));
    QCOMPARE(actual.ruby, ruby);
}

} // namespace

class FontVariantEastAsianModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void validValuesRoundTripIndependently();
    void qObjectBaseProvidesVirtualLifetime();
};

void FontVariantEastAsianModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    FontVariantEastAsianModel model;

    compareEastAsian(model.data, KoSvgText::EastAsianVariantNormal, KoSvgText::EastAsiantNormalWidth, false);
    QCOMPARE(model.variant(), int(KoSvgText::EastAsianVariantNormal));
    QCOMPARE(model.width(), int(KoSvgText::EastAsiantNormalWidth));
    QVERIFY(!model.ruby());

    for (const char *name : {"variant", "width", "ruby"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void FontVariantEastAsianModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(EastAsian(), lager::automatic_tag{});
    FontVariantEastAsianModel model(state);
    QSignalSpy variantSpy(&model, &FontVariantEastAsianModel::variantChanged);
    QSignalSpy widthSpy(&model, &FontVariantEastAsianModel::widthChanged);
    QSignalSpy rubySpy(&model, &FontVariantEastAsianModel::rubyChanged);

    QVERIFY(variantSpy.isValid());
    QVERIFY(widthSpy.isValid());
    QVERIFY(rubySpy.isValid());

    EastAsian external;
    external.variant = KoSvgText::EastAsianTraditional;
    external.width = KoSvgText::EastAsianProportionalWidth;
    external.ruby = true;
    state.set(external);

    compareEastAsian(model.data, KoSvgText::EastAsianTraditional, KoSvgText::EastAsianProportionalWidth, true);
    QCOMPARE(model.variant(), int(KoSvgText::EastAsianTraditional));
    QCOMPARE(model.width(), int(KoSvgText::EastAsianProportionalWidth));
    QVERIFY(model.ruby());
    QCOMPARE(variantSpy.size(), 1);
    QCOMPARE(widthSpy.size(), 1);
    QCOMPARE(rubySpy.size(), 1);

    QVERIFY(model.setProperty("variant", int(KoSvgText::EastAsianJis04)));
    QVERIFY(model.setProperty("width", int(KoSvgText::EastAsianFullWidth)));
    QVERIFY(model.setProperty("ruby", false));

    compareEastAsian(state, KoSvgText::EastAsianJis04, KoSvgText::EastAsianFullWidth, false);
    QCOMPARE(variantSpy.size(), 2);
    QCOMPARE(widthSpy.size(), 2);
    QCOMPARE(rubySpy.size(), 2);
}

void FontVariantEastAsianModelContractTest::validValuesRoundTripIndependently()
{
    auto state = lager::make_state(EastAsian(), lager::automatic_tag{});
    FontVariantEastAsianModel model(state);
    model.setruby(true);

    for (const KoSvgText::EastAsianVariant value : {KoSvgText::EastAsianVariantNormal,
                                                    KoSvgText::EastAsianJis78,
                                                    KoSvgText::EastAsianJis83,
                                                    KoSvgText::EastAsianJis90,
                                                    KoSvgText::EastAsianJis04,
                                                    KoSvgText::EastAsianSimplified,
                                                    KoSvgText::EastAsianTraditional}) {
        model.setvariant(int(value));
        QCOMPARE(int(state.get().variant), int(value));
        QCOMPARE(model.variant(), int(value));
        QCOMPARE(int(state.get().width), int(KoSvgText::EastAsiantNormalWidth));
        QVERIFY(state.get().ruby);
    }

    for (const KoSvgText::EastAsianWidth value :
         {KoSvgText::EastAsiantNormalWidth, KoSvgText::EastAsianFullWidth, KoSvgText::EastAsianProportionalWidth}) {
        model.setwidth(int(value));
        QCOMPARE(int(state.get().width), int(value));
        QCOMPARE(model.width(), int(value));
        QCOMPARE(int(state.get().variant), int(KoSvgText::EastAsianTraditional));
        QVERIFY(state.get().ruby);
    }

    model.setruby(false);
    QCOMPARE(int(state.get().variant), int(KoSvgText::EastAsianTraditional));
    QCOMPARE(int(state.get().width), int(KoSvgText::EastAsianProportionalWidth));
    QVERIFY(!state.get().ruby);
}

void FontVariantEastAsianModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<FontVariantEastAsianModel> model = new FontVariantEastAsianModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(FontVariantEastAsianModelContractTest)

#include "FontVariantEastAsianModelContractTest.moc"
