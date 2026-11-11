/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <QPointF>
#include <QRect>
#include <QVector>

#include <input/ui/wintab/KisWintabMapping.h>

class KisWintabMappingTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testVirtualScreenMatchesQt();
    void testOneScreenOfTwo();
    void testNegativeOriginScreen();
    void testDevicePixelRatio();
    void testClampOutsideRange();
    void testNegativeExtent();
    void testTargetRect();
    void testConfiguredMode();
};

namespace
{

// The formula of QWindowsTabletDeviceData::scaleCoordinates() in official
// Qt 6.11.3, copied here so the test checks the sign handling against the
// reference and not against the implementation it is testing.
QPointF qtScaleCoordinates(int coordX, int coordY, int maxX, int maxY,
                           const QRect &targetArea)
{
    const auto sign = [](int value) { return value >= 0 ? 1 : -1; };

    const int minX = 0;
    const int minY = 0;
    const int targetX = targetArea.x();
    const int targetY = targetArea.y();
    const int targetWidth = targetArea.width();
    const int targetHeight = targetArea.height();

    const qreal x = sign(targetWidth) == sign(maxX) ?
        ((coordX - minX) * qAbs(targetWidth) / qAbs(qreal(maxX - minX))) + targetX :
        ((qAbs(maxX) - (coordX - minX)) * qAbs(targetWidth) / qAbs(qreal(maxX - minX))) + targetX;

    const qreal y = sign(targetHeight) == sign(maxY) ?
        ((coordY - minY) * qAbs(targetHeight) / qAbs(qreal(maxY - minY))) + targetY :
        ((qAbs(maxY) - (coordY - minY)) * qAbs(targetHeight) / qAbs(qreal(maxY - minY))) + targetY;

    return QPointF(x, y);
}

KisWintabMapping::Range range(int extX, int extY,
                              int sysOrgX = 0, int sysOrgY = 0,
                              int sysExtX = 0, int sysExtY = 0)
{
    KisWintabMapping::Range value;
    value.inOrgX = 0;
    value.inOrgY = 0;
    value.inExtX = extX;
    value.inExtY = extY;
    value.sysOrgX = sysOrgX;
    value.sysOrgY = sysOrgY;
    value.sysExtX = sysExtX;
    value.sysExtY = sysExtY;
    return value;
}

KisWintabMapping::Screen screen(const QRect &nativeRect,
                                const QRect &logicalRect,
                                qreal devicePixelRatio)
{
    KisWintabMapping::Screen value;
    value.nativeRect = nativeRect;
    value.logicalRect = logicalRect;
    value.devicePixelRatio = devicePixelRatio;
    return value;
}

} // namespace

void KisWintabMappingTest::testVirtualScreenMatchesQt()
{
    const QRect virtualDesktop(0, 0, 1920, 1080);
    const KisWintabMapping::Range packetRange = range(1000, 500);

    const QVector<QPoint> packets{
        QPoint(0, 0), QPoint(1, 1), QPoint(500, 250),
        QPoint(999, 499), QPoint(1000, 500)
    };

    for (const QPoint &packet : packets) {
        QCOMPARE(KisWintabMapping::nativePosition(packetRange, virtualDesktop,
                                                  packet.x(), packet.y()),
                 qtScaleCoordinates(packet.x(), packet.y(), 1000, 500,
                                    virtualDesktop));
    }

    // A target with a negative origin keeps Qt's sign handling.
    const QRect negativeTarget(-1920, -200, 1920, 1080);
    for (const QPoint &packet : packets) {
        QCOMPARE(KisWintabMapping::nativePosition(packetRange, negativeTarget,
                                                  packet.x(), packet.y()),
                 qtScaleCoordinates(packet.x(), packet.y(), 1000, 500,
                                    negativeTarget));
    }
}

void KisWintabMappingTest::testOneScreenOfTwo()
{
    const QRect leftNative(-1920, 0, 1920, 1080);
    const QRect rightNative(0, 0, 1920, 1080);
    const QVector<KisWintabMapping::Screen> screens{
        screen(leftNative, leftNative, 1.0),
        screen(rightNative, rightNative, 1.0)
    };

    const KisWintabMapping::Range packetRange = range(1000, 500);
    const QRect target = KisWintabMapping::targetRect(
        KisWintabMapping::Mode::Custom, packetRange,
        leftNative.united(rightNative), leftNative);

    QCOMPARE(target, leftNative);

    struct Corner {
        int packetX;
        int packetY;
        QPointF native;
        QPointF logical;
    };
    const QVector<Corner> corners{
        {0, 0, QPointF(-1920, 0), QPointF(-1920, 0)},
        {500, 250, QPointF(-960, 540), QPointF(-960, 540)},
        {1000, 500, QPointF(0, 1080), QPointF(0, 1080)}
    };

    for (const Corner &corner : corners) {
        const QPointF native = KisWintabMapping::nativePosition(
            packetRange, target, corner.packetX, corner.packetY);
        QCOMPARE(native, corner.native);
        QCOMPARE(KisWintabMapping::logicalPosition(native, screens),
                 corner.logical);
    }
}

void KisWintabMappingTest::testNegativeOriginScreen()
{
    const QRect native(-2560, -1440, 2560, 1440);
    const QVector<KisWintabMapping::Screen> screens{
        screen(native, native, 1.0)
    };
    const KisWintabMapping::Range packetRange = range(1000, 1000);

    const QPointF topLeft = KisWintabMapping::nativePosition(
        packetRange, native, 0, 0);
    QCOMPARE(topLeft, QPointF(-2560, -1440));
    QCOMPARE(KisWintabMapping::logicalPosition(topLeft, screens), topLeft);

    const QPointF center = KisWintabMapping::nativePosition(
        packetRange, native, 500, 500);
    QCOMPARE(center, QPointF(-1280, -720));
    QCOMPARE(KisWintabMapping::logicalPosition(center, screens), center);
}

void KisWintabMappingTest::testDevicePixelRatio()
{
    const QRect native(0, 0, 3840, 2160);
    const QRect logical(0, 0, 1920, 1080);
    const QVector<KisWintabMapping::Screen> screens{
        screen(native, logical, 2.0)
    };

    const KisWintabMapping::Range packetRange = range(1000, 500);
    const QRect target = KisWintabMapping::targetRect(
        KisWintabMapping::Mode::VirtualScreen, packetRange, native, QRect());

    QCOMPARE(target, native);

    QCOMPARE(KisWintabMapping::logicalPosition(
                 KisWintabMapping::nativePosition(packetRange, target, 0, 0),
                 screens),
             QPointF(0, 0));
    QCOMPARE(KisWintabMapping::logicalPosition(
                 KisWintabMapping::nativePosition(packetRange, target, 500, 250),
                 screens),
             QPointF(960, 540));
    QCOMPARE(KisWintabMapping::logicalPosition(
                 KisWintabMapping::nativePosition(packetRange, target, 1000, 500),
                 screens),
             QPointF(1920, 1080));
}

void KisWintabMappingTest::testClampOutsideRange()
{
    const QRect target(0, 0, 1920, 1080);
    const KisWintabMapping::Range packetRange = range(1000, 500);

    QCOMPARE(KisWintabMapping::nativePosition(packetRange, target, -100, -100),
             KisWintabMapping::nativePosition(packetRange, target, 0, 0));
    QCOMPARE(KisWintabMapping::nativePosition(packetRange, target, 2000, 900),
             KisWintabMapping::nativePosition(packetRange, target, 1000, 500));
}

void KisWintabMappingTest::testNegativeExtent()
{
    const QRect target(0, 0, 1920, 1080);
    const KisWintabMapping::Range packetRange = range(-1000, -500);

    // A negative extent flips the axis: packet 0 is the far edge.
    QCOMPARE(KisWintabMapping::nativePosition(packetRange, target, 0, 0),
             QPointF(1920, 1080));
    QCOMPARE(KisWintabMapping::nativePosition(packetRange, target, 500, 250),
             QPointF(960, 540));
    QCOMPARE(KisWintabMapping::nativePosition(packetRange, target, 1000, 500),
             QPointF(0, 0));

    // Outside the magnitude of the extent clamps like a positive extent.
    QCOMPARE(KisWintabMapping::nativePosition(packetRange, target, -100, -100),
             KisWintabMapping::nativePosition(packetRange, target, 0, 0));
    QCOMPARE(KisWintabMapping::nativePosition(packetRange, target, 2000, 900),
             KisWintabMapping::nativePosition(packetRange, target, 1000, 500));
}

void KisWintabMappingTest::testTargetRect()
{
    const QRect virtualDesktop(0, 0, 3840, 2160);
    const QRect custom(100, 200, 800, 600);
    const KisWintabMapping::Range packetRange =
        range(1000, 500, -50, -60, 3840, 2160);

    QCOMPARE(KisWintabMapping::targetRect(
                 KisWintabMapping::Mode::Wintab, packetRange,
                 virtualDesktop, custom),
             QRect(-50, -60, 3840, 2160));
    QCOMPARE(KisWintabMapping::targetRect(
                 KisWintabMapping::Mode::VirtualScreen, packetRange,
                 virtualDesktop, custom),
             virtualDesktop);
    QCOMPARE(KisWintabMapping::targetRect(
                 KisWintabMapping::Mode::Custom, packetRange,
                 virtualDesktop, custom),
             custom);

    // A Wintab context without a system rectangle cannot be corrected; the
    // virtual desktop is Qt's own target.
    const KisWintabMapping::Range emptySystem = range(1000, 500);
    QCOMPARE(KisWintabMapping::targetRect(
                 KisWintabMapping::Mode::Wintab, emptySystem,
                 virtualDesktop, custom),
             virtualDesktop);
}

void KisWintabMappingTest::testConfiguredMode()
{
    const KisWintabMapping::Mode savedMode = KisWintabMapping::configuredMode();
    const QRect savedRect = KisWintabMapping::configuredCustomRect();

    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Custom,
                                        QRect(10, 20, 30, 40));
    QCOMPARE(KisWintabMapping::configuredMode(),
             KisWintabMapping::Mode::Custom);
    QCOMPARE(KisWintabMapping::configuredCustomRect(), QRect(10, 20, 30, 40));

    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::VirtualScreen,
                                        QRect());
    QCOMPARE(KisWintabMapping::configuredMode(),
             KisWintabMapping::Mode::VirtualScreen);
    QCOMPARE(KisWintabMapping::configuredCustomRect(), QRect());

    KisWintabMapping::setConfiguredMode(savedMode, savedRect);
}

QTEST_GUILESS_MAIN(KisWintabMappingTest)

#include "KisWintabMappingTest.moc"
