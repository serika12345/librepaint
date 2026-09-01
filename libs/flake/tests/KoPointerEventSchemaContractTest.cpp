/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPointerEvent.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_POINTER_EVENT_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPointerEvent::method)), signature>)

#define ASSERT_POINTER_EVENT_STATIC_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPointerEvent::method)), signature>)
} // namespace

class KoPointerEventSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pointerEventTypeLifetimeAndWrapperSchemaRemainsStable();
    void pointerEventRoutingAndAcceptanceSignaturesRemainStable();
    void pointerEventPositionSignaturesRemainStable();
    void pointerEventDeviceMeasurementSignaturesRemainStable();
    void pointerEventDeepCopyCompatibilitySignaturesRemainStable();
};

void KoPointerEventSchemaContractTest::pointerEventTypeLifetimeAndWrapperSchemaRemainsStable()
{
    static_assert(std::is_class_v<KoPointerEvent>);
    static_assert(!std::is_default_constructible_v<KoPointerEvent>);
    static_assert(std::is_constructible_v<KoPointerEvent, QMouseEvent *, const QPointF &>);
    static_assert(std::is_constructible_v<KoPointerEvent, QTabletEvent *, const QPointF &>);
    static_assert(std::is_constructible_v<KoPointerEvent, QTouchEvent *, const QPointF &>);
    static_assert(std::is_constructible_v<KoPointerEvent, KoPointerEvent *, const QPointF &>);
    static_assert(std::is_copy_constructible_v<KoPointerEvent>);
    static_assert(std::is_copy_assignable_v<KoPointerEvent>);
    static_assert(std::is_destructible_v<KoPointerEvent>);
    static_assert(!std::has_virtual_destructor_v<KoPointerEvent>);

    ASSERT_POINTER_EVENT_SIGNATURE(operator=, KoPointerEvent & (KoPointerEvent::*)(const KoPointerEvent &));

    static_assert(std::is_class_v<KoPointerEventWrapper>);
    static_assert(std::is_constructible_v<KoPointerEventWrapper, QMouseEvent *, const QPointF &>);
    static_assert(std::is_constructible_v<KoPointerEventWrapper, QTabletEvent *, const QPointF &>);
    static_assert(std::is_constructible_v<KoPointerEventWrapper, QTouchEvent *, const QPointF &>);
    static_assert(std::is_same_v<decltype(KoPointerEventWrapper::event), KoPointerEvent>);
    static_assert(std::is_same_v<decltype(KoPointerEventWrapper::baseQtEvent), QSharedPointer<QEvent>>);
}

void KoPointerEventSchemaContractTest::pointerEventRoutingAndAcceptanceSignaturesRemainStable()
{
    ASSERT_POINTER_EVENT_SIGNATURE(accept, void (KoPointerEvent::*)());
    ASSERT_POINTER_EVENT_SIGNATURE(ignore, void (KoPointerEvent::*)());
    ASSERT_POINTER_EVENT_SIGNATURE(isAccepted, bool (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(spontaneous, bool (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(modifiers, Qt::KeyboardModifiers (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(button, Qt::MouseButton (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(buttons, Qt::MouseButtons (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(time, ulong (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_STATIC_SIGNATURE(tabletInputReceived, bool (*)());
}

void KoPointerEventSchemaContractTest::pointerEventPositionSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(KoPointerEvent::point), QPointF>);

    ASSERT_POINTER_EVENT_SIGNATURE(globalPos, QPoint (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(pos, QPoint (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(x, int (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(y, int (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_STATIC_SIGNATURE(fetchGlobalPositionFromPointerEvent, std::optional<QPointF> (*)(QEvent *));
}

void KoPointerEventSchemaContractTest::pointerEventDeviceMeasurementSignaturesRemainStable()
{
    ASSERT_POINTER_EVENT_SIGNATURE(pressure, qreal (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(rotation, qreal (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(tangentialPressure, qreal (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(xTilt, qreal (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(yTilt, qreal (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(z, int (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(isTabletEvent, bool (KoPointerEvent::*)() const);
    ASSERT_POINTER_EVENT_SIGNATURE(isTouchEvent, bool (KoPointerEvent::*)() const);
}

void KoPointerEventSchemaContractTest::pointerEventDeepCopyCompatibilitySignaturesRemainStable()
{
    ASSERT_POINTER_EVENT_SIGNATURE(deepCopyEvent, KoPointerEventWrapper (KoPointerEvent::*)() const);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ASSERT_POINTER_EVENT_STATIC_SIGNATURE(copyQtPointerEvent, void (*)(const QMouseEvent *, QScopedPointer<QEvent> &));
    ASSERT_POINTER_EVENT_STATIC_SIGNATURE(copyQtPointerEvent, void (*)(const QTabletEvent *, QScopedPointer<QEvent> &));
    ASSERT_POINTER_EVENT_STATIC_SIGNATURE(copyQtPointerEvent, void (*)(const QTouchEvent *, QScopedPointer<QEvent> &));
#endif
}

QTEST_GUILESS_MAIN(KoPointerEventSchemaContractTest)

#include "KoPointerEventSchemaContractTest.moc"
