/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisColorOptionModel.h>

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

void compareColorData(const lager::cursor<KisColorOptionData> &cursor, const KisColorOptionData &expected)
{
    const KisColorOptionData actual = cursor.get();
    QCOMPARE(actual.useRandomHSV, expected.useRandomHSV);
    QCOMPARE(actual.useRandomOpacity, expected.useRandomOpacity);
    QCOMPARE(actual.sampleInputColor, expected.sampleInputColor);
    QCOMPARE(actual.fillBackground, expected.fillBackground);
    QCOMPARE(actual.colorPerParticle, expected.colorPerParticle);
    QCOMPARE(actual.mixBgColor, expected.mixBgColor);
    QCOMPARE(actual.hue, expected.hue);
    QCOMPARE(actual.saturation, expected.saturation);
    QCOMPARE(actual.value, expected.value);
}

void verifyProperty(const QObject &object, const char *name, int expectedType)
{
    const int propertyIndex = object.metaObject()->indexOfProperty(name);
    QVERIFY(propertyIndex >= 0);

    const QMetaProperty property = object.metaObject()->property(propertyIndex);
    QCOMPARE(property.metaType().id(), expectedType);
    QVERIFY(property.isReadable());
    QVERIFY(property.isWritable());
    QVERIFY(property.hasNotifySignal());
}

KisColorOptionData enabledData()
{
    KisColorOptionData data;
    data.useRandomHSV = true;
    data.useRandomOpacity = true;
    data.sampleInputColor = true;
    data.fillBackground = true;
    data.colorPerParticle = true;
    data.mixBgColor = true;
    data.hue = -37;
    data.saturation = 42;
    data.value = 73;
    return data;
}

} // namespace

class KisColorOptionModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void injectedCursorDefaultsAndQtPropertiesRemainStable();
    void externalStateUpdatesAllGettersAndSignals();
    void settersUpdateFieldsIndependently();
    void qObjectBaseProvidesVirtualLifetime();
};

void KisColorOptionModelContractTest::injectedCursorDefaultsAndQtPropertiesRemainStable()
{
    const KisColorOptionData defaults;
    auto state = lager::make_state(defaults, lager::automatic_tag{});
    KisColorOptionModel model(state);

    compareColorData(model.optionData, defaults);
    QVERIFY(!model.useRandomHSV());
    QVERIFY(!model.useRandomOpacity());
    QVERIFY(!model.sampleInputColor());
    QVERIFY(!model.fillBackground());
    QVERIFY(!model.colorPerParticle());
    QVERIFY(!model.mixBgColor());
    QCOMPARE(model.hue(), 0);
    QCOMPARE(model.saturation(), 0);
    QCOMPARE(model.value(), 0);

    for (const char *name :
         {"useRandomHSV", "useRandomOpacity", "sampleInputColor", "fillBackground", "colorPerParticle", "mixBgColor"}) {
        verifyProperty(model, name, QMetaType::Bool);
    }
    for (const char *name : {"hue", "saturation", "value"}) {
        verifyProperty(model, name, QMetaType::Int);
    }
}

void KisColorOptionModelContractTest::externalStateUpdatesAllGettersAndSignals()
{
    auto state = lager::make_state(KisColorOptionData(), lager::automatic_tag{});
    KisColorOptionModel model(state);
    QSignalSpy randomHsvSpy(&model, &KisColorOptionModel::useRandomHSVChanged);
    QSignalSpy randomOpacitySpy(&model, &KisColorOptionModel::useRandomOpacityChanged);
    QSignalSpy sampleInputSpy(&model, &KisColorOptionModel::sampleInputColorChanged);
    QSignalSpy fillBackgroundSpy(&model, &KisColorOptionModel::fillBackgroundChanged);
    QSignalSpy colorPerParticleSpy(&model, &KisColorOptionModel::colorPerParticleChanged);
    QSignalSpy mixBackgroundSpy(&model, &KisColorOptionModel::mixBgColorChanged);
    QSignalSpy hueSpy(&model, &KisColorOptionModel::hueChanged);
    QSignalSpy saturationSpy(&model, &KisColorOptionModel::saturationChanged);
    QSignalSpy valueSpy(&model, &KisColorOptionModel::valueChanged);

    const KisColorOptionData enabled = enabledData();
    state.set(enabled);

    compareColorData(model.optionData, enabled);
    QVERIFY(model.useRandomHSV());
    QVERIFY(model.useRandomOpacity());
    QVERIFY(model.sampleInputColor());
    QVERIFY(model.fillBackground());
    QVERIFY(model.colorPerParticle());
    QVERIFY(model.mixBgColor());
    QCOMPARE(model.hue(), -37);
    QCOMPARE(model.saturation(), 42);
    QCOMPARE(model.value(), 73);
    QCOMPARE(randomHsvSpy.size(), 1);
    QCOMPARE(randomOpacitySpy.size(), 1);
    QCOMPARE(sampleInputSpy.size(), 1);
    QCOMPARE(fillBackgroundSpy.size(), 1);
    QCOMPARE(colorPerParticleSpy.size(), 1);
    QCOMPARE(mixBackgroundSpy.size(), 1);
    QCOMPARE(hueSpy.size(), 1);
    QCOMPARE(saturationSpy.size(), 1);
    QCOMPARE(valueSpy.size(), 1);
}

void KisColorOptionModelContractTest::settersUpdateFieldsIndependently()
{
    KisColorOptionData expected;
    auto state = lager::make_state(expected, lager::automatic_tag{});
    KisColorOptionModel model(state);

    model.setuseRandomHSV(true);
    expected.useRandomHSV = true;
    compareColorData(state, expected);

    model.setuseRandomOpacity(true);
    expected.useRandomOpacity = true;
    compareColorData(state, expected);

    model.setsampleInputColor(true);
    expected.sampleInputColor = true;
    compareColorData(state, expected);

    model.setfillBackground(true);
    expected.fillBackground = true;
    compareColorData(state, expected);

    model.setcolorPerParticle(true);
    expected.colorPerParticle = true;
    compareColorData(state, expected);

    model.setmixBgColor(true);
    expected.mixBgColor = true;
    compareColorData(state, expected);

    model.sethue(-23);
    expected.hue = -23;
    compareColorData(state, expected);

    model.setsaturation(54);
    expected.saturation = 54;
    compareColorData(state, expected);

    model.setvalue(81);
    expected.value = 81;
    compareColorData(state, expected);

    QVERIFY(model.setProperty("useRandomHSV", false));
    expected.useRandomHSV = false;
    compareColorData(state, expected);

    QVERIFY(model.setProperty("hue", 17));
    expected.hue = 17;
    compareColorData(state, expected);
}

void KisColorOptionModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    auto state = lager::make_state(KisColorOptionData(), lager::automatic_tag{});
    QPointer<KisColorOptionModel> model = new KisColorOptionModel(state);
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(KisColorOptionModelContractTest)

#include "KisColorOptionModelContractTest.moc"
