/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <functional>
#include <optional>

#include <KisMirrorOptionModel.h>

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

void compareMirrorData(const lager::cursor<KisMirrorOptionMixIn> &cursor,
                       const QString &prefix,
                       bool verticalMirror,
                       bool horizontalMirror)
{
    const KisMirrorOptionMixIn actual = cursor.get();
    QCOMPARE(actual.prefix, prefix);
    QCOMPARE(actual.enableVerticalMirror, verticalMirror);
    QCOMPARE(actual.enableHorizontalMirror, horizontalMirror);
}

} // namespace

class KisMirrorOptionModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void injectedCursorAndQtPropertiesRemainStable();
    void externalStateUpdatesBothProperties();
    void settersUpdateFieldsIndependently();
    void qObjectBaseProvidesVirtualLifetime();
};

void KisMirrorOptionModelContractTest::injectedCursorAndQtPropertiesRemainStable()
{
    const QString prefix = QStringLiteral("覆い/鏡/");
    auto state = lager::make_state(KisMirrorOptionMixIn(prefix), lager::automatic_tag{});
    KisMirrorOptionModel model(state);

    compareMirrorData(model.mirrorOptionData, prefix, false, false);
    QVERIFY(!model.enableVerticalMirror());
    QVERIFY(!model.enableHorizontalMirror());

    for (const char *name : {"enableVerticalMirror", "enableHorizontalMirror"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);

        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void KisMirrorOptionModelContractTest::externalStateUpdatesBothProperties()
{
    const QString prefix = QStringLiteral("mask/鏡/");
    auto state = lager::make_state(KisMirrorOptionMixIn(prefix), lager::automatic_tag{});
    KisMirrorOptionModel model(state);
    QSignalSpy verticalSpy(&model, &KisMirrorOptionModel::enableVerticalMirrorChanged);
    QSignalSpy horizontalSpy(&model, &KisMirrorOptionModel::enableHorizontalMirrorChanged);

    QVERIFY(verticalSpy.isValid());
    QVERIFY(horizontalSpy.isValid());

    KisMirrorOptionMixIn updated = state.get();
    updated.enableVerticalMirror = true;
    updated.enableHorizontalMirror = true;
    state.set(updated);

    compareMirrorData(model.mirrorOptionData, prefix, true, true);
    QVERIFY(model.enableVerticalMirror());
    QVERIFY(model.enableHorizontalMirror());
    QCOMPARE(verticalSpy.size(), 1);
    QCOMPARE(horizontalSpy.size(), 1);
}

void KisMirrorOptionModelContractTest::settersUpdateFieldsIndependently()
{
    const QString prefix = QStringLiteral("マスク/");
    auto state = lager::make_state(KisMirrorOptionMixIn(prefix), lager::automatic_tag{});
    KisMirrorOptionModel model(state);

    model.setenableVerticalMirror(true);
    compareMirrorData(state, prefix, true, false);

    model.setenableHorizontalMirror(true);
    compareMirrorData(state, prefix, true, true);

    QVERIFY(model.setProperty("enableVerticalMirror", false));
    compareMirrorData(state, prefix, false, true);

    QVERIFY(model.setProperty("enableHorizontalMirror", false));
    compareMirrorData(state, prefix, false, false);
}

void KisMirrorOptionModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    auto state = lager::make_state(KisMirrorOptionMixIn(QStringLiteral("lifetime/")), lager::automatic_tag{});
    QPointer<KisMirrorOptionModel> model = new KisMirrorOptionModel(state);
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(KisMirrorOptionModelContractTest)

#include "KisMirrorOptionModelContractTest.moc"
