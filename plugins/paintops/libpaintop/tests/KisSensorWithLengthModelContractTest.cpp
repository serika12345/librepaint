/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDynamicSensorIds.h>
#include <KisSensorWithLengthModel.h>

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

#include <type_traits>

namespace
{

KisSensorWithLengthData sensorData(int length = 137, bool isPeriodic = true)
{
    KisSensorWithLengthData data(TimeId, QLatin1String("duration"));
    data.length = length;
    data.isPeriodic = isPeriodic;
    return data;
}

void compareSensorState(const lager::cursor<KisSensorWithLengthData> &cursor, int length, bool isPeriodic)
{
    const KisSensorWithLengthData actual = cursor.get();
    QCOMPARE(actual.length, length);
    QCOMPARE(actual.isPeriodic, isPeriodic);
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

class KisSensorWithLengthModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void injectedCursorParentAndPropertiesRemainStable();
    void externalStateUpdatesGettersAndSignals();
    void settersAndPropertiesUpdateFieldsIndependently();
    void parentOwnsModelLifetime();
};

void KisSensorWithLengthModelContractTest::injectedCursorParentAndPropertiesRemainStable()
{
    static_assert(std::is_base_of_v<QObject, KisSensorWithLengthModel>);

    auto state = lager::make_state(sensorData(), lager::automatic_tag{});
    QObject parent;
    KisSensorWithLengthModel model(state, &parent);

    QCOMPARE(model.parent(), &parent);
    compareSensorState(model.m_data, 137, true);
    QCOMPARE(model.length(), 137);
    QVERIFY(model.isPeriodic());
    verifyProperty(model, "length", QMetaType::Int);
    verifyProperty(model, "isPeriodic", QMetaType::Bool);

    model.m_data.set(sensorData(211, false));
    compareSensorState(state, 211, false);
}

void KisSensorWithLengthModelContractTest::externalStateUpdatesGettersAndSignals()
{
    auto state = lager::make_state(sensorData(), lager::automatic_tag{});
    KisSensorWithLengthModel model(state, nullptr);
    QSignalSpy lengthSpy(&model, &KisSensorWithLengthModel::lengthChanged);
    QSignalSpy periodicSpy(&model, &KisSensorWithLengthModel::isPeriodicChanged);

    state.set(sensorData(311, false));

    compareSensorState(model.m_data, 311, false);
    QCOMPARE(model.length(), 311);
    QVERIFY(!model.isPeriodic());
    QCOMPARE(lengthSpy.size(), 1);
    QCOMPARE(periodicSpy.size(), 1);
}

void KisSensorWithLengthModelContractTest::settersAndPropertiesUpdateFieldsIndependently()
{
    auto state = lager::make_state(sensorData(), lager::automatic_tag{});
    KisSensorWithLengthModel model(state, nullptr);

    model.setlength(509);
    compareSensorState(state, 509, true);

    model.setisPeriodic(false);
    compareSensorState(state, 509, false);

    QVERIFY(model.setProperty("length", 613));
    compareSensorState(state, 613, false);

    QVERIFY(model.setProperty("isPeriodic", true));
    compareSensorState(state, 613, true);
}

void KisSensorWithLengthModelContractTest::parentOwnsModelLifetime()
{
    auto state = lager::make_state(sensorData(), lager::automatic_tag{});
    QObject *parent = new QObject;
    QPointer<KisSensorWithLengthModel> model = new KisSensorWithLengthModel(state, parent);

    delete parent;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(KisSensorWithLengthModelContractTest)

#include "KisSensorWithLengthModelContractTest.moc"
