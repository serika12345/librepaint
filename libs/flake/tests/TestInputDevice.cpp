/*
 * SPDX-FileCopyrightText: 2011 Hanna Skott <hannaetscott@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TestInputDevice.h"

#include "KoInputDevice.h"

#include <QDebug>
#include <QPointingDevice>
#include <QTest>

#include <array>
#include <memory>
#include <utility>

namespace
{
using DeviceMapping = std::pair<QInputDevice::DeviceType, KoInputDevice::InputDevice>;
using PointerMapping = std::pair<QPointingDevice::PointerType, KoInputDevice::Pointer>;

constexpr std::array<DeviceMapping, 9> deviceMappings{{
    {QInputDevice::DeviceType::Unknown, KoInputDevice::InputDevice::Unknown},
    {QInputDevice::DeviceType::Mouse, KoInputDevice::InputDevice::Mouse},
    {QInputDevice::DeviceType::TouchScreen, KoInputDevice::InputDevice::TouchScreen},
    {QInputDevice::DeviceType::TouchPad, KoInputDevice::InputDevice::TouchPad},
    {QInputDevice::DeviceType::Puck, KoInputDevice::InputDevice::Puck},
    {QInputDevice::DeviceType::Stylus, KoInputDevice::InputDevice::Stylus},
    {QInputDevice::DeviceType::Airbrush, KoInputDevice::InputDevice::Airbrush},
    {QInputDevice::DeviceType::Keyboard, KoInputDevice::InputDevice::Keyboard},
    {QInputDevice::DeviceType::AllDevices, KoInputDevice::InputDevice::AllDevices},
}};

constexpr std::array<PointerMapping, 7> pointerMappings{{
    {QPointingDevice::PointerType::Unknown, KoInputDevice::Pointer::Unknown},
    {QPointingDevice::PointerType::Generic, KoInputDevice::Pointer::Generic},
    {QPointingDevice::PointerType::Finger, KoInputDevice::Pointer::Finger},
    {QPointingDevice::PointerType::Pen, KoInputDevice::Pointer::Pen},
    {QPointingDevice::PointerType::Eraser, KoInputDevice::Pointer::Eraser},
    {QPointingDevice::PointerType::Cursor, KoInputDevice::Pointer::Cursor},
    {QPointingDevice::PointerType::AllPointerTypes, KoInputDevice::Pointer::AllPointerTypes},
}};

class TabletEventFixture
{
public:
    TabletEventFixture(QInputDevice::DeviceType deviceType, QPointingDevice::PointerType pointerType, qint64 systemId)
        : device(QStringLiteral("input-device-contract"),
                 systemId,
                 deviceType,
                 pointerType,
                 QInputDevice::Capability::Position | QInputDevice::Capability::Pressure,
                 1,
                 3)
        , event(QEvent::TabletMove,
                &device,
                QPointF(1.25, 2.5),
                QPointF(11.25, 12.5),
                0.75,
                1.0F,
                -2.0F,
                0.0F,
                15.0,
                0.0F,
                Qt::NoModifier,
                Qt::NoButton,
                Qt::NoButton)
    {
    }

    QPointingDevice device;
    QTabletEvent event;
};

QStringList debugTokens(const KoInputDevice &device)
{
    QString output;
    {
        QDebug debug(&output);
        debug.nospace() << device;
    }
    return output.split(QLatin1Char(' '), Qt::SkipEmptyParts);
}
} // namespace

void TestInputDevice::enumValuesMatchQtInputClassifications()
{
    for (const DeviceMapping &mapping : deviceMappings) {
        QCOMPARE(int(mapping.second), int(mapping.first));
    }
    for (const PointerMapping &mapping : pointerMappings) {
        QCOMPARE(int(mapping.second), int(mapping.first));
    }
}

void TestInputDevice::tabletEventsMapQtDeviceAndPointerTypes()
{
    qint64 systemId = 100;
    for (const DeviceMapping &mapping : deviceMappings) {
        TabletEventFixture fixture(mapping.first, QPointingDevice::PointerType::Pen, systemId++);
        QCOMPARE(KoInputDevice::convertDeviceType(&fixture.event), mapping.second);
    }
    for (const PointerMapping &mapping : pointerMappings) {
        TabletEventFixture fixture(QInputDevice::DeviceType::Stylus, mapping.first, systemId++);
        QCOMPARE(KoInputDevice::convertPointerType(&fixture.event), mapping.second);
    }
}

void TestInputDevice::constructionAndMouseFallbackPreserveState()
{
    const KoInputDevice defaultDevice;
    QCOMPARE(defaultDevice.device(), KoInputDevice::InputDevice::Unknown);
    QCOMPARE(defaultDevice.pointer(), KoInputDevice::Pointer::Unknown);
    QCOMPARE(defaultDevice.uniqueTabletId(), qint64(-1));
    QVERIFY(defaultDevice.isMouse());

    const qint64 tabletId = Q_INT64_C(0x123456789A);
    const KoInputDevice tablet(KoInputDevice::InputDevice::Airbrush, KoInputDevice::Pointer::Pen, tabletId);
    QCOMPARE(tablet.device(), KoInputDevice::InputDevice::Airbrush);
    QCOMPARE(tablet.pointer(), KoInputDevice::Pointer::Pen);
    QCOMPARE(tablet.uniqueTabletId(), tabletId);
    QVERIFY(!tablet.isMouse());

    const KoInputDevice unknownDevice(KoInputDevice::InputDevice::Unknown, KoInputDevice::Pointer::Pen, 7);
    const KoInputDevice unknownPointer(KoInputDevice::InputDevice::Stylus, KoInputDevice::Pointer::Unknown, 8);
    QVERIFY(unknownDevice.isMouse());
    QVERIFY(unknownPointer.isMouse());
}

void TestInputDevice::copyAssignmentEqualityAndLifetime()
{
    const KoInputDevice source(KoInputDevice::InputDevice::Stylus, KoInputDevice::Pointer::Pen, 71);
    const std::unique_ptr<KoInputDevice> copied = [&source] {
        KoInputDevice temporary(source);
        return std::make_unique<KoInputDevice>(temporary);
    }();

    QVERIFY(*copied == source);
    QVERIFY(!(*copied != source));

    KoInputDevice assigned;
    KoInputDevice &assignmentResult = (assigned = source);
    QCOMPARE(&assignmentResult, &assigned);
    QVERIFY(assigned == source);
    QCOMPARE(&(assigned = assigned), &assigned);

    QVERIFY(!(source == KoInputDevice(KoInputDevice::InputDevice::Airbrush, KoInputDevice::Pointer::Pen, 71)));
    QVERIFY(!(source == KoInputDevice(KoInputDevice::InputDevice::Stylus, KoInputDevice::Pointer::Eraser, 71)));
    QVERIFY(!(source == KoInputDevice(KoInputDevice::InputDevice::Stylus, KoInputDevice::Pointer::Pen, 72)));
    QVERIFY(!(KoInputDevice::invalid() == KoInputDevice::mouse()));
}

void TestInputDevice::factoriesExposeCanonicalDevices()
{
    const KoInputDevice invalid = KoInputDevice::invalid();
    QCOMPARE(invalid.device(), KoInputDevice::InputDevice::Unknown);
    QCOMPARE(invalid.pointer(), KoInputDevice::Pointer::Unknown);
    QCOMPARE(invalid.uniqueTabletId(), qint64(-1));
    QVERIFY(invalid.isMouse());

    const KoInputDevice mouse = KoInputDevice::mouse();
    QCOMPARE(mouse.device(), KoInputDevice::InputDevice::Unknown);
    QCOMPARE(mouse.pointer(), KoInputDevice::Pointer::Unknown);
    QCOMPARE(mouse.uniqueTabletId(), qint64(-1));
    QVERIFY(mouse.isMouse());
    QVERIFY(mouse == KoInputDevice());

    const KoInputDevice stylus = KoInputDevice::stylus();
    QCOMPARE(stylus.device(), KoInputDevice::InputDevice::Stylus);
    QCOMPARE(stylus.pointer(), KoInputDevice::Pointer::Pen);
    QCOMPARE(stylus.uniqueTabletId(), qint64(-1));
    QVERIFY(!stylus.isMouse());

    const KoInputDevice eraser = KoInputDevice::eraser();
    QCOMPARE(eraser.device(), KoInputDevice::InputDevice::Stylus);
    QCOMPARE(eraser.pointer(), KoInputDevice::Pointer::Eraser);
    QCOMPARE(eraser.uniqueTabletId(), qint64(-1));
    QVERIFY(!eraser.isMouse());
}

void TestInputDevice::equalDevicesProduceEqualHashes()
{
    const KoInputDevice first(KoInputDevice::InputDevice::Puck, KoInputDevice::Pointer::Cursor, 93);
    const KoInputDevice second(first);

    QVERIFY(first == second);
    QCOMPARE(qHash(first), qHash(second));
}

void TestInputDevice::debugOutputKeepsCurrentLabels()
{
    QCOMPARE(debugTokens(KoInputDevice::mouse()), QStringList{QStringLiteral("mouse")});

    const QStringList stylusTokens = debugTokens(KoInputDevice::stylus());
    QCOMPARE(stylusTokens.value(0), QStringLiteral("pen"));
    QCOMPARE(stylusTokens.value(1), QStringLiteral("stylus"));
    QVERIFY(stylusTokens.contains(QStringLiteral("-1")));

    const QStringList eraserTokens = debugTokens(KoInputDevice::eraser());
    QCOMPARE(eraserTokens.value(0), QStringLiteral("eraser"));
    QCOMPARE(eraserTokens.value(1), QStringLiteral("stylus"));

    const QStringList cursorTokens =
        debugTokens(KoInputDevice(KoInputDevice::InputDevice::Puck, KoInputDevice::Pointer::Cursor, 47));
    QCOMPARE(cursorTokens.value(0), QStringLiteral("cursor"));
    QCOMPARE(cursorTokens.value(1), QStringLiteral("puck"));
    QVERIFY(cursorTokens.contains(QStringLiteral("47")));
}

void TestInputDevice::concreteMouseDeviceIsNotReportedAsMouse()
{
    const KoInputDevice mouse(KoInputDevice::InputDevice::Mouse, KoInputDevice::Pointer::Generic, 5);

    QVERIFY(!mouse.isMouse());
}

void TestInputDevice::qt6AdditionalTypesUseUnsupportedDebugLabels()
{
    const KoInputDevice touchScreen(KoInputDevice::InputDevice::TouchScreen, KoInputDevice::Pointer::Finger, 17);
    const QStringList tokens = debugTokens(touchScreen);

    QVERIFY(tokens.contains(QStringLiteral("Unsupported")));
    QVERIFY(tokens.contains(QStringLiteral("pointer")));
    QVERIFY(tokens.contains(QStringLiteral("device")));
}

QTEST_GUILESS_MAIN(TestInputDevice)
