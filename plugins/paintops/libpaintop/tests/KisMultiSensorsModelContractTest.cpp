/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisMultiSensorsModel.h>

#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtCore/qtmetamacros.h>
#include <QtTest/qtest.h>
#include <QtTest/qtestcase.h>

#include <KoID.h>
#include <lager/state.hpp>
#include <lager/tags.hpp>

namespace
{

KisMultiSensorsModel::MultiSensorData sensorData(bool pressureActive, bool speedActive)
{
    return {
        {KoID(QStringLiteral("pressure"), QStringLiteral("Pressure")), pressureActive},
        {KoID(QStringLiteral("speed"), QStringLiteral("Speed")), speedActive},
    };
}

} // namespace

class KisMultiSensorsModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void presentsSensorNamesAndActiveStates();
    void keepsAtLeastOneSensorActive();
};

void KisMultiSensorsModelContractTest::presentsSensorNamesAndActiveStates()
{
    auto state = lager::make_state(sensorData(true, false), lager::automatic_tag{});
    KisMultiSensorsModel model(state);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0), Qt::DisplayRole).toString(), QStringLiteral("Pressure"));
    QCOMPARE(model.data(model.index(0), Qt::CheckStateRole).toInt(), int(Qt::Checked));
    QCOMPARE(model.data(model.index(1), Qt::DisplayRole).toString(), QStringLiteral("Speed"));
    QCOMPARE(model.data(model.index(1), Qt::CheckStateRole).toInt(), int(Qt::Unchecked));
    QCOMPARE(model.getSensorId(model.index(1)), QStringLiteral("speed"));
    QCOMPARE(model.sensorIndex(QStringLiteral("pressure")), model.index(0));
}

void KisMultiSensorsModelContractTest::keepsAtLeastOneSensorActive()
{
    auto state = lager::make_state(sensorData(true, false), lager::automatic_tag{});
    KisMultiSensorsModel model(state);

    QVERIFY(model.setData(model.index(0), Qt::Unchecked, Qt::CheckStateRole));
    QVERIFY(state.get().at(0).second);

    QVERIFY(model.setData(model.index(1), Qt::Checked, Qt::CheckStateRole));
    QVERIFY(state.get().at(1).second);
    QVERIFY(model.setData(model.index(0), Qt::Unchecked, Qt::CheckStateRole));
    QVERIFY(!state.get().at(0).second);
    QVERIFY(state.get().at(1).second);
}

QTEST_GUILESS_MAIN(KisMultiSensorsModelContractTest)

#include "KisMultiSensorsModelContractTest.moc"
