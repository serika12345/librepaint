/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisWindowsPointerEventFilter.h>

#include <QTest>

#include <algorithm>

namespace
{

// The same packing the Windows message uses: two signed 16-bit values, low
// word first.
qintptr packedPoint(int x, int y)
{
    return qintptr(quint32(quint16(x)) | (quint32(quint16(y)) << 16));
}

} // namespace

/**
 * Contract of the Windows non-client pointer handling for Q4 (official Qt):
 * c41d66281e (Enter/Leave and the cursor when the stylus touches the frame)
 * and 33a49f25d1 (the press-and-hold gesture of Windows Ink).
 *
 * The tests use only the pure functions, so they run on the development host
 * without a HWND or the Windows SDK.
 */
class KisWindowsPointerFilterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void classifiesTheNonClientPointerMessages();
    void ignoresClientPointerAndOtherMessages();
    void decodesThePackedScreenPoint();
    void mapsTheScreenPointToTheCanvas();
    void treatsEveryEdgePixelAsInsideAndTheRestAsOutside();
    void scalesTheCanvasRectByTheDevicePixelRatio();
    void restoresTheCanvasCursorOnlyWhenTheCanvasWasEntered();
    void disablesOnlyThePressAndTapGesture();
};

void KisWindowsPointerFilterContractTest::classifiesTheNonClientPointerMessages()
{
    using namespace KisWindowsPointer;

    QCOMPARE(nonClientMessage(NonClientPointerUpdate), NonClientMessage::Move);
    QCOMPARE(nonClientMessage(NonClientMouseMove), NonClientMessage::Move);
    QCOMPARE(nonClientMessage(NonClientPointerDown), NonClientMessage::Press);
    QCOMPARE(nonClientMessage(NonClientPointerUp), NonClientMessage::Release);
}

void KisWindowsPointerFilterContractTest::ignoresClientPointerAndOtherMessages()
{
    using namespace KisWindowsPointer;

    // The client area stays on Qt's WM_POINTER* path.
    QCOMPARE(nonClientMessage(0x0245), NonClientMessage::None); // WM_POINTERUPDATE
    QCOMPARE(nonClientMessage(0x0246), NonClientMessage::None); // WM_POINTERDOWN
    QCOMPARE(nonClientMessage(0x0247), NonClientMessage::None); // WM_POINTERUP
    QCOMPARE(nonClientMessage(0x0200), NonClientMessage::None); // WM_MOUSEMOVE
    QCOMPARE(nonClientMessage(0x0000), NonClientMessage::None);
}

void KisWindowsPointerFilterContractTest::decodesThePackedScreenPoint()
{
    using namespace KisWindowsPointer;

    QCOMPARE(screenPointFromLParam(packedPoint(0, 0)), QPoint(0, 0));
    QCOMPARE(screenPointFromLParam(packedPoint(100, 200)), QPoint(100, 200));
    QCOMPARE(screenPointFromLParam(packedPoint(2735, 1823)), QPoint(2735, 1823));

    // A monitor to the left of or above the primary one has negative
    // coordinates in the signed words.
    QCOMPARE(screenPointFromLParam(packedPoint(-1920, 10)), QPoint(-1920, 10));
    QCOMPARE(screenPointFromLParam(packedPoint(-1, -2)), QPoint(-1, -2));

    // Only the low 32 bits carry the point.
    QCOMPARE(screenPointFromLParam(qintptr(0x7fff00000000) | packedPoint(3, 4)), QPoint(3, 4));
}

void KisWindowsPointerFilterContractTest::mapsTheScreenPointToTheCanvas()
{
    using namespace KisWindowsPointer;

    const QPoint origin(300, 200);
    QCOMPARE(canvasLocalPoint(QPoint(300, 200), origin), QPoint(0, 0));
    QCOMPARE(canvasLocalPoint(QPoint(500, 400), origin), QPoint(200, 200));
    QCOMPARE(canvasLocalPoint(QPoint(299, 199), origin), QPoint(-1, -1));

    // Second monitor with a negative origin.
    const QPoint leftMonitorOrigin(-1920, 0);
    QCOMPARE(canvasLocalPoint(QPoint(-1900, 30), leftMonitorOrigin), QPoint(20, 30));
}

void KisWindowsPointerFilterContractTest::treatsEveryEdgePixelAsInsideAndTheRestAsOutside()
{
    using namespace KisWindowsPointer;

    const QSize size(100, 50);

    QVERIFY(isInsideCanvas(QPoint(0, 0), size));
    QVERIFY(isInsideCanvas(QPoint(99, 49), size));
    QVERIFY(isInsideCanvas(QPoint(50, 25), size));

    QVERIFY(!isInsideCanvas(QPoint(100, 0), size));
    QVERIFY(!isInsideCanvas(QPoint(0, 50), size));
    QVERIFY(!isInsideCanvas(QPoint(100, 50), size));
    QVERIFY(!isInsideCanvas(QPoint(-1, 0), size));
    QVERIFY(!isInsideCanvas(QPoint(0, -1), size));

    // A canvas that is not laid out yet has no inside.
    QVERIFY(!isInsideCanvas(QPoint(0, 0), QSize(0, 0)));
}

void KisWindowsPointerFilterContractTest::scalesTheCanvasRectByTheDevicePixelRatio()
{
    using namespace KisWindowsPointer;

    QCOMPARE(nativeCanvasSize(QSize(100, 50), 1.0), QSize(100, 50));
    QCOMPARE(nativeCanvasSize(QSize(100, 50), 2.0), QSize(200, 100));
    QCOMPARE(nativeCanvasSize(QSize(101, 51), 1.5), QSize(152, 77));

    // An unset ratio is treated as 1, not as a zero-sized rect.
    QCOMPARE(nativeCanvasSize(QSize(100, 50), 0.0), QSize(100, 50));
    QCOMPARE(nativeCanvasSize(QSize(100, 50), -1.0), QSize(100, 50));
}

void KisWindowsPointerFilterContractTest::restoresTheCanvasCursorOnlyWhenTheCanvasWasEntered()
{
    using namespace KisWindowsPointer;

    QVERIFY(restoresCanvasCursor(true));
    QVERIFY(!restoresCanvasCursor(false));
}

void KisWindowsPointerFilterContractTest::disablesOnlyThePressAndTapGesture()
{
    using namespace KisWindowsPointer;

    const auto config = canvasGestureConfig();

    // Only the press-and-hold gesture is listed, so SetGestureConfig leaves
    // zoom, pan, rotate and two-finger tap untouched.
    QCOMPARE(config.size(), size_t(1));

    const GestureConfig &pressAndTap = config.front();
    QCOMPARE(pressAndTap.id, GestureIdPressAndTap);
    QCOMPARE(pressAndTap.want & GestureConfigPressAndTap, quint32(0));
    QCOMPARE(pressAndTap.block & GestureConfigPressAndTap, GestureConfigPressAndTap);

    const std::array<quint32, 4> otherGestures {
        GestureIdZoom, GestureIdPan, GestureIdRotate, GestureIdTwoFingerTap};
    QVERIFY(std::none_of(config.cbegin(), config.cend(), [&otherGestures](const GestureConfig &entry) {
        return std::find(otherGestures.cbegin(), otherGestures.cend(), entry.id) != otherGestures.cend();
    }));
}

QTEST_MAIN(KisWindowsPointerFilterContractTest)

#include "KisWindowsPointerFilterContractTest.moc"
