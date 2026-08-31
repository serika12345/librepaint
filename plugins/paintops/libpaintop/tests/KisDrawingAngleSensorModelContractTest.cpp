/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDrawingAngleSensorModel.h>

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

#include <type_traits>

namespace
{

KisDrawingAngleSensorData sensorData(bool fanCornersEnabled = true,
                                     int fanCornersStep = 47,
                                     qreal angleOffset = 12.5,
                                     bool lockedAngleMode = true)
{
    KisDrawingAngleSensorData data;
    data.fanCornersEnabled = fanCornersEnabled;
    data.fanCornersStep = fanCornersStep;
    data.angleOffset = angleOffset;
    data.lockedAngleMode = lockedAngleMode;
    return data;
}

void compareSensorState(const lager::cursor<KisDrawingAngleSensorData> &cursor,
                        const KisDrawingAngleSensorData &expected)
{
    const KisDrawingAngleSensorData actual = cursor.get();
    QCOMPARE(actual.fanCornersEnabled, expected.fanCornersEnabled);
    QCOMPARE(actual.fanCornersStep, expected.fanCornersStep);
    QCOMPARE(actual.angleOffset, expected.angleOffset);
    QCOMPARE(actual.lockedAngleMode, expected.lockedAngleMode);
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

} // namespace

void kis_assert_exception(const char *, const char *, int)
{
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

class KisDrawingAngleSensorModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void injectedCursorParentAndQtPropertiesRemainStable();
    void externalStateUpdatesGettersAndSignals();
    void settersAndPropertiesUpdateFieldsIndependently();
    void parentOwnsModelLifetime();
};

void KisDrawingAngleSensorModelContractTest::injectedCursorParentAndQtPropertiesRemainStable()
{
    static_assert(std::is_base_of_v<QObject, KisDrawingAngleSensorModel>);
    static_assert(
        std::is_same_v<decltype(KisDrawingAngleSensorModel::m_data), lager::cursor<KisDrawingAngleSensorData>>);

    const KisDrawingAngleSensorData injected = sensorData();
    auto state = lager::make_state(injected, lager::automatic_tag{});
    QObject parent;
    KisDrawingAngleSensorModel model(state, &parent);

    QCOMPARE(model.parent(), &parent);
    compareSensorState(model.m_data, injected);
    QVERIFY(model.fanCornersEnabled());
    QCOMPARE(model.fanCornersStep(), 47);
    QCOMPARE(model.angleOffset(), 12.5);
    QCOMPARE(model.angleOffsetInverted(), -12.5);
    QVERIFY(model.lockedAngleMode());

    verifyProperty(model, "fanCornersEnabled", QMetaType::Bool);
    verifyProperty(model, "fanCornersStep", QMetaType::Int);
    verifyProperty(model, "angleOffset", QMetaType::Double);
    verifyProperty(model, "angleOffsetInverted", QMetaType::Double);
    verifyProperty(model, "lockedAngleMode", QMetaType::Bool);

    const KisDrawingAngleSensorData replacement = sensorData(false, 59, -7.25, false);
    model.m_data.set(replacement);
    compareSensorState(state, replacement);
}

void KisDrawingAngleSensorModelContractTest::externalStateUpdatesGettersAndSignals()
{
    auto state = lager::make_state(sensorData(), lager::automatic_tag{});
    KisDrawingAngleSensorModel model(state, nullptr);
    QSignalSpy fanCornersEnabledSpy(&model, &KisDrawingAngleSensorModel::fanCornersEnabledChanged);
    QSignalSpy fanCornersStepSpy(&model, &KisDrawingAngleSensorModel::fanCornersStepChanged);
    QSignalSpy angleOffsetSpy(&model, &KisDrawingAngleSensorModel::angleOffsetChanged);
    QSignalSpy angleOffsetInvertedSpy(&model, &KisDrawingAngleSensorModel::angleOffsetInvertedChanged);
    QSignalSpy lockedAngleModeSpy(&model, &KisDrawingAngleSensorModel::lockedAngleModeChanged);

    const KisDrawingAngleSensorData updated = sensorData(false, 83, -21.25, false);
    state.set(updated);

    compareSensorState(model.m_data, updated);
    QVERIFY(!model.fanCornersEnabled());
    QCOMPARE(model.fanCornersStep(), 83);
    QCOMPARE(model.angleOffset(), -21.25);
    QCOMPARE(model.angleOffsetInverted(), 21.25);
    QVERIFY(!model.lockedAngleMode());
    QCOMPARE(fanCornersEnabledSpy.size(), 1);
    QCOMPARE(fanCornersStepSpy.size(), 1);
    QCOMPARE(angleOffsetSpy.size(), 1);
    QCOMPARE(angleOffsetInvertedSpy.size(), 1);
    QCOMPARE(lockedAngleModeSpy.size(), 1);
}

void KisDrawingAngleSensorModelContractTest::settersAndPropertiesUpdateFieldsIndependently()
{
    KisDrawingAngleSensorData expected;
    auto state = lager::make_state(expected, lager::automatic_tag{});
    KisDrawingAngleSensorModel model(state, nullptr);

    model.setfanCornersEnabled(true);
    expected.fanCornersEnabled = true;
    compareSensorState(state, expected);

    model.setfanCornersStep(61);
    expected.fanCornersStep = 61;
    compareSensorState(state, expected);

    model.setangleOffset(32.5);
    expected.angleOffset = 32.5;
    compareSensorState(state, expected);
    QCOMPARE(model.angleOffsetInverted(), -32.5);

    model.setangleOffsetInverted(14.25);
    expected.angleOffset = -14.25;
    compareSensorState(state, expected);
    QCOMPARE(model.angleOffsetInverted(), 14.25);

    model.setlockedAngleMode(true);
    expected.lockedAngleMode = true;
    compareSensorState(state, expected);

    QVERIFY(model.setProperty("fanCornersEnabled", false));
    expected.fanCornersEnabled = false;
    compareSensorState(state, expected);

    QVERIFY(model.setProperty("fanCornersStep", 73));
    expected.fanCornersStep = 73;
    compareSensorState(state, expected);

    QVERIFY(model.setProperty("angleOffset", -18.5));
    expected.angleOffset = -18.5;
    compareSensorState(state, expected);

    QVERIFY(model.setProperty("angleOffsetInverted", -27.75));
    expected.angleOffset = 27.75;
    compareSensorState(state, expected);
    QCOMPARE(model.angleOffsetInverted(), -27.75);

    QVERIFY(model.setProperty("lockedAngleMode", false));
    expected.lockedAngleMode = false;
    compareSensorState(state, expected);
}

void KisDrawingAngleSensorModelContractTest::parentOwnsModelLifetime()
{
    auto state = lager::make_state(sensorData(), lager::automatic_tag{});
    QObject *parent = new QObject;
    QPointer<KisDrawingAngleSensorModel> model = new KisDrawingAngleSensorModel(state, parent);

    delete parent;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(KisDrawingAngleSensorModelContractTest)

#include "KisDrawingAngleSensorModelContractTest.moc"
