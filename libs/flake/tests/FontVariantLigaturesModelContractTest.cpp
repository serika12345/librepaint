/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FontVariantLigaturesModel.h"

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using Ligatures = KoSvgText::FontFeatureLigatures;

void compareLigatures(const lager::cursor<Ligatures> &cursor,
                      bool commonLigatures,
                      bool discretionaryLigatures,
                      bool historicalLigatures,
                      bool contextualAlternates)
{
    const Ligatures actual = cursor.get();
    QCOMPARE(actual.commonLigatures, commonLigatures);
    QCOMPARE(actual.discretionaryLigatures, discretionaryLigatures);
    QCOMPARE(actual.historicalLigatures, historicalLigatures);
    QCOMPARE(actual.contextualAlternates, contextualAlternates);
}

} // namespace

class FontVariantLigaturesModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void booleanPropertiesRemainIndependent();
    void qObjectBaseProvidesVirtualLifetime();
};

void FontVariantLigaturesModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    FontVariantLigaturesModel model;

    compareLigatures(model.data, true, false, false, true);
    QVERIFY(model.commonLigatures());
    QVERIFY(!model.discretionaryLigatures());
    QVERIFY(!model.historicalLigatures());
    QVERIFY(model.contextualAlternates());

    for (const char *name :
         {"commonLigatures", "discretionaryLigatures", "historicalLigatures", "contextualAlternates"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void FontVariantLigaturesModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(Ligatures(), lager::automatic_tag{});
    FontVariantLigaturesModel model(state);
    QSignalSpy commonSpy(&model, &FontVariantLigaturesModel::commonLigaturesChanged);
    QSignalSpy discretionarySpy(&model, &FontVariantLigaturesModel::discretionaryLigaturesChanged);
    QSignalSpy historicalSpy(&model, &FontVariantLigaturesModel::historicalLigaturesChanged);
    QSignalSpy contextualSpy(&model, &FontVariantLigaturesModel::contextualAlternatesChanged);

    QVERIFY(commonSpy.isValid());
    QVERIFY(discretionarySpy.isValid());
    QVERIFY(historicalSpy.isValid());
    QVERIFY(contextualSpy.isValid());

    Ligatures external;
    external.commonLigatures = false;
    external.discretionaryLigatures = true;
    external.historicalLigatures = true;
    external.contextualAlternates = false;
    state.set(external);

    compareLigatures(model.data, false, true, true, false);
    QVERIFY(!model.commonLigatures());
    QVERIFY(model.discretionaryLigatures());
    QVERIFY(model.historicalLigatures());
    QVERIFY(!model.contextualAlternates());
    QCOMPARE(commonSpy.size(), 1);
    QCOMPARE(discretionarySpy.size(), 1);
    QCOMPARE(historicalSpy.size(), 1);
    QCOMPARE(contextualSpy.size(), 1);

    QVERIFY(model.setProperty("commonLigatures", true));
    QVERIFY(model.setProperty("discretionaryLigatures", false));
    QVERIFY(model.setProperty("historicalLigatures", false));
    QVERIFY(model.setProperty("contextualAlternates", true));

    compareLigatures(state, true, false, false, true);
    QCOMPARE(commonSpy.size(), 2);
    QCOMPARE(discretionarySpy.size(), 2);
    QCOMPARE(historicalSpy.size(), 2);
    QCOMPARE(contextualSpy.size(), 2);
}

void FontVariantLigaturesModelContractTest::booleanPropertiesRemainIndependent()
{
    auto state = lager::make_state(Ligatures(), lager::automatic_tag{});
    FontVariantLigaturesModel model(state);

    model.setcommonLigatures(false);
    compareLigatures(state, false, false, false, true);

    model.setdiscretionaryLigatures(true);
    compareLigatures(state, false, true, false, true);

    model.sethistoricalLigatures(true);
    compareLigatures(state, false, true, true, true);

    model.setcontextualAlternates(false);
    compareLigatures(state, false, true, true, false);
}

void FontVariantLigaturesModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<FontVariantLigaturesModel> model = new FontVariantLigaturesModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(FontVariantLigaturesModelContractTest)

#include "FontVariantLigaturesModelContractTest.moc"
