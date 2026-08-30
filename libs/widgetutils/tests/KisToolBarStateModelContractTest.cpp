/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisToolBarStateModel.h"

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

class KisToolBarStateModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsLockedWithWritableQtProperty();
    void directAndQtPropertyWritesNotifyObservers();
    void qObjectBaseProvidesVirtualLifetime();
};

void KisToolBarStateModelContractTest::startsLockedWithWritableQtProperty()
{
    KisToolBarStateModel model;

    QCOMPARE(model.toolBarsLocked(), true);
    QCOMPARE(model.property("toolBarsLocked").toBool(), true);

    const int propertyIndex = model.metaObject()->indexOfProperty("toolBarsLocked");
    QVERIFY(propertyIndex >= 0);
    const QMetaProperty property = model.metaObject()->property(propertyIndex);
    QVERIFY(property.isReadable());
    QVERIFY(property.isWritable());
    QVERIFY(property.hasNotifySignal());
}

void KisToolBarStateModelContractTest::directAndQtPropertyWritesNotifyObservers()
{
    KisToolBarStateModel model;
    QSignalSpy spy(&model, &KisToolBarStateModel::toolBarsLockedChanged);

    QVERIFY(spy.isValid());

    model.settoolBarsLocked(false);
    QCOMPARE(model.toolBarsLocked(), false);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);

    QVERIFY(model.setProperty("toolBarsLocked", true));
    QCOMPARE(model.toolBarsLocked(), true);
    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.at(1).at(0).toBool(), true);
}

void KisToolBarStateModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<KisToolBarStateModel> model = new KisToolBarStateModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(KisToolBarStateModelContractTest)

#include "KisToolBarStateModelContractTest.moc"
