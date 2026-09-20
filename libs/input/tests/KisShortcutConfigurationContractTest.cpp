/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_shortcut_configuration.h>

#include <QTest>

#include <array>

class KisShortcutConfigurationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumValuesRemainStable();
    void defaultConfigurationIsNoOp();
    void storesAllPublicValues();
    void copiesAndAssignsIndependentValues();
    void equalityUsesPhysicalInputOnly();
    void serializesAndRestoresProfileValues();
    void rejectsInvalidSerializedValues();
    void noOpReflectsSelectedInputType();
};

void KisShortcutConfigurationContractTest::enumValuesRemainStable()
{
    const std::array<KisShortcutConfiguration::ShortcutType, 6> types {{
        KisShortcutConfiguration::UnknownType,
        KisShortcutConfiguration::KeyCombinationType,
        KisShortcutConfiguration::MouseButtonType,
        KisShortcutConfiguration::MouseWheelType,
        KisShortcutConfiguration::GestureType,
        KisShortcutConfiguration::MacOSGestureType,
    }};
    for (std::size_t index = 0; index < types.size(); ++index) {
        QCOMPARE(int(types[index]), int(index));
    }

    const std::array<KisShortcutConfiguration::MouseWheelMovement, 6> wheelMovements {{
        KisShortcutConfiguration::NoMovement,
        KisShortcutConfiguration::WheelUp,
        KisShortcutConfiguration::WheelDown,
        KisShortcutConfiguration::WheelLeft,
        KisShortcutConfiguration::WheelRight,
        KisShortcutConfiguration::WheelTrackpad,
    }};
    for (std::size_t index = 0; index < wheelMovements.size(); ++index) {
        QCOMPARE(int(wheelMovements[index]), int(index));
    }

#ifdef Q_OS_MACOS
    const std::array<KisShortcutConfiguration::GestureAction, 6> gestures {{
        KisShortcutConfiguration::NoGesture,
        KisShortcutConfiguration::PinchGesture,
        KisShortcutConfiguration::PanGesture,
        KisShortcutConfiguration::RotateGesture,
        KisShortcutConfiguration::SmartZoomGesture,
        KisShortcutConfiguration::MaxGesture,
    }};
#else
    const std::array<KisShortcutConfiguration::GestureAction, 13> gestures {{
        KisShortcutConfiguration::NoGesture,
        KisShortcutConfiguration::OneFingerTap,
        KisShortcutConfiguration::TwoFingerTap,
        KisShortcutConfiguration::ThreeFingerTap,
        KisShortcutConfiguration::FourFingerTap,
        KisShortcutConfiguration::FiveFingerTap,
        KisShortcutConfiguration::OneFingerDrag,
        KisShortcutConfiguration::TwoFingerDrag,
        KisShortcutConfiguration::ThreeFingerDrag,
        KisShortcutConfiguration::FourFingerDrag,
        KisShortcutConfiguration::FiveFingerDrag,
        KisShortcutConfiguration::OneFingerHold,
        KisShortcutConfiguration::MaxGesture,
    }};
#endif
    for (std::size_t index = 0; index < gestures.size(); ++index) {
        QCOMPARE(int(gestures[index]), int(index));
    }
}

void KisShortcutConfigurationContractTest::defaultConfigurationIsNoOp()
{
    const KisShortcutConfiguration shortcut;

    QCOMPARE(shortcut.actionId(), QString());
    QCOMPARE(shortcut.type(), KisShortcutConfiguration::UnknownType);
    QCOMPARE(shortcut.mode(), 0U);
    QVERIFY(shortcut.keys().isEmpty());
    QCOMPARE(shortcut.buttons(), Qt::MouseButtons(Qt::NoButton));
    QCOMPARE(shortcut.wheel(), KisShortcutConfiguration::NoMovement);
    QCOMPARE(shortcut.gesture(), KisShortcutConfiguration::NoGesture);
    QVERIFY(shortcut.isNoOp());
}

void KisShortcutConfigurationContractTest::storesAllPublicValues()
{
    KisShortcutConfiguration shortcut;
    shortcut.setActionId(QStringLiteral("freehand-brush"));
    shortcut.setType(KisShortcutConfiguration::MouseWheelType);
    shortcut.setMode(0x2aU);
    shortcut.setKeys({Qt::Key_Control, Qt::Key_X});
    shortcut.setButtons(Qt::LeftButton | Qt::RightButton);
    shortcut.setWheel(KisShortcutConfiguration::WheelLeft);
#ifdef Q_OS_MACOS
    shortcut.setGesture(KisShortcutConfiguration::PinchGesture);
#else
    shortcut.setGesture(KisShortcutConfiguration::OneFingerTap);
#endif

    QCOMPARE(shortcut.actionId(), QStringLiteral("freehand-brush"));
    QCOMPARE(shortcut.type(), KisShortcutConfiguration::MouseWheelType);
    QCOMPARE(shortcut.mode(), 0x2aU);
    QCOMPARE(shortcut.keys(), QList<Qt::Key>({Qt::Key_Control, Qt::Key_X}));
    QCOMPARE(shortcut.buttons(), Qt::MouseButtons(Qt::LeftButton | Qt::RightButton));
    QCOMPARE(shortcut.wheel(), KisShortcutConfiguration::WheelLeft);
#ifdef Q_OS_MACOS
    QCOMPARE(shortcut.gesture(), KisShortcutConfiguration::PinchGesture);
#else
    QCOMPARE(shortcut.gesture(), KisShortcutConfiguration::OneFingerTap);
#endif
}

void KisShortcutConfigurationContractTest::copiesAndAssignsIndependentValues()
{
    KisShortcutConfiguration source;
    source.setActionId(QStringLiteral("source"));
    source.setType(KisShortcutConfiguration::KeyCombinationType);
    source.setMode(7U);
    source.setKeys({Qt::Key_A});
    source.setButtons(Qt::MiddleButton);
    source.setWheel(KisShortcutConfiguration::WheelDown);
#ifdef Q_OS_MACOS
    source.setGesture(KisShortcutConfiguration::RotateGesture);
#else
    source.setGesture(KisShortcutConfiguration::ThreeFingerDrag);
#endif

    KisShortcutConfiguration copied(source);
    KisShortcutConfiguration assigned;
    assigned = source;

    source.setActionId(QStringLiteral("changed"));
    source.setKeys({Qt::Key_B});

    QCOMPARE(copied.actionId(), QStringLiteral("source"));
    QCOMPARE(copied.keys(), QList<Qt::Key>({Qt::Key_A}));
    QCOMPARE(assigned.actionId(), QStringLiteral("source"));
    QCOMPARE(assigned.keys(), QList<Qt::Key>({Qt::Key_A}));
}

void KisShortcutConfigurationContractTest::equalityUsesPhysicalInputOnly()
{
    KisShortcutConfiguration first;
    first.setActionId(QStringLiteral("first-action"));
    first.setMode(1U);
    first.setType(KisShortcutConfiguration::MouseButtonType);
    first.setButtons(Qt::LeftButton);

    KisShortcutConfiguration second(first);
    second.setActionId(QStringLiteral("second-action"));
    second.setMode(9U);

    QVERIFY(first == second);

    second.setButtons(Qt::RightButton);
    QVERIFY(!(first == second));
}

void KisShortcutConfigurationContractTest::serializesAndRestoresProfileValues()
{
    KisShortcutConfiguration shortcut;
    shortcut.setActionId(QStringLiteral("profile-owned-action"));
    shortcut.setType(KisShortcutConfiguration::GestureType);
    shortcut.setMode(0x2aU);
    shortcut.setKeys({Qt::Key_Control, Qt::Key_X});
    shortcut.setButtons(Qt::LeftButton | Qt::RightButton);
    shortcut.setWheel(KisShortcutConfiguration::WheelLeft);
#ifdef Q_OS_MACOS
    shortcut.setGesture(KisShortcutConfiguration::PinchGesture);
    const QString expected = QStringLiteral("{2a;5;[1000021,58];3;3;1}");
#else
    shortcut.setGesture(KisShortcutConfiguration::OneFingerTap);
    const QString expected = QStringLiteral("{2a;4;[1000021,58];3;3;1}");
#endif

    QCOMPARE(shortcut.serialize(), expected);

    KisShortcutConfiguration restored;
    QVERIFY(restored.unserialize(expected));
    QCOMPARE(restored.actionId(), QString());
    QCOMPARE(restored.type(), KisShortcutConfiguration::GestureType);
    QCOMPARE(restored.mode(), 0x2aU);
    QCOMPARE(restored.keys(), QList<Qt::Key>({Qt::Key_Control, Qt::Key_X}));
    QCOMPARE(restored.buttons(), Qt::MouseButtons(Qt::LeftButton | Qt::RightButton));
    QCOMPARE(restored.wheel(), KisShortcutConfiguration::WheelLeft);
    QVERIFY(restored == shortcut);
}

void KisShortcutConfigurationContractTest::rejectsInvalidSerializedValues()
{
    KisShortcutConfiguration shortcut;

    QVERIFY(!shortcut.unserialize(QStringLiteral("invalid")));
    QVERIFY(!shortcut.unserialize(QStringLiteral("{}")));
    QVERIFY(!shortcut.unserialize(QStringLiteral("{0;0;[];0;0;0}")));
#ifdef Q_OS_MACOS
    QVERIFY(!shortcut.unserialize(QStringLiteral("{0;4;[];0;0;1}")));
#else
    QVERIFY(!shortcut.unserialize(QStringLiteral("{0;5;[];0;0;1}")));
#endif
}

void KisShortcutConfigurationContractTest::noOpReflectsSelectedInputType()
{
    KisShortcutConfiguration shortcut;

    shortcut.setType(KisShortcutConfiguration::KeyCombinationType);
    QVERIFY(shortcut.isNoOp());
    shortcut.setKeys({Qt::Key_A});
    QVERIFY(!shortcut.isNoOp());

    shortcut.setType(KisShortcutConfiguration::MouseButtonType);
    QVERIFY(shortcut.isNoOp());
    shortcut.setButtons(Qt::LeftButton);
    QVERIFY(!shortcut.isNoOp());

    shortcut.setType(KisShortcutConfiguration::MouseWheelType);
    QVERIFY(shortcut.isNoOp());
    shortcut.setWheel(KisShortcutConfiguration::WheelUp);
    QVERIFY(!shortcut.isNoOp());

    shortcut.setType(KisShortcutConfiguration::GestureType);
    QVERIFY(shortcut.isNoOp());
#ifdef Q_OS_MACOS
    shortcut.setGesture(KisShortcutConfiguration::PanGesture);
#else
    shortcut.setGesture(KisShortcutConfiguration::TwoFingerTap);
#endif
    QVERIFY(!shortcut.isNoOp());
    shortcut.setGesture(KisShortcutConfiguration::MaxGesture);
    QVERIFY(shortcut.isNoOp());
}

QTEST_GUILESS_MAIN(KisShortcutConfigurationContractTest)

#include "KisShortcutConfigurationContractTest.moc"
