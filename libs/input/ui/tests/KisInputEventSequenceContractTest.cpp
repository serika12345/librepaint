/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisInputEventSequence.h>

#include <QCoreApplication>
#include <QEventPoint>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QSharedPointer>
#include <QTabletEvent>
#include <QTest>
#include <QTouchEvent>

namespace
{
bool isRecordedInputType(QEvent::Type type)
{
    switch (type) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        return true;
    default:
        return false;
    }
}

class EventReceiver final : public QObject
{
public:
    QVector<QSharedPointer<QEvent>> events;

protected:
    bool event(QEvent *event) override
    {
        if (isRecordedInputType(event->type())) {
            events.append(QSharedPointer<QEvent>(event->clone()));
            return true;
        }
        return QObject::event(event);
    }
};

void sendMouseEvent(QObject *receiver,
                    QEvent::Type type,
                    const QPointF &position,
                    const QPointF &globalPosition,
                    Qt::MouseButton button,
                    Qt::MouseButtons buttons,
                    Qt::MouseEventSource source,
                    quint64 timestamp)
{
    QMouseEvent event(type,
                      position,
                      position,
                      globalPosition,
                      button,
                      buttons,
                      Qt::ShiftModifier,
                      source);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(receiver, &event);
}

void sendTabletEvent(QObject *receiver,
                     QEvent::Type type,
                     const QPointingDevice *device,
                     const QPointF &position,
                     const QPointF &globalPosition,
                     qreal pressure,
                     float xTilt,
                     float yTilt,
                     float tangentialPressure,
                     qreal rotation,
                     float z,
                     Qt::MouseButton button,
                     Qt::MouseButtons buttons,
                     quint64 timestamp)
{
    QTabletEvent event(type,
                       device,
                       position,
                       globalPosition,
                       pressure,
                       xTilt,
                       yTilt,
                       tangentialPressure,
                       rotation,
                       z,
                       Qt::AltModifier,
                       button,
                       buttons);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(receiver, &event);
}

QEventPoint sendTouchEvent(QObject *receiver,
                           QEvent::Type type,
                           const QPointingDevice *device,
                           QEventPoint::State state,
                           int pointId,
                           const QPointF &position,
                           const QPointF &globalPosition,
                           quint64 timestamp)
{
    QEventPoint point(pointId, state, position, globalPosition);
    QTouchEvent event(type,
                      device,
                      Qt::ControlModifier,
                      QList<QEventPoint>{point});
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(receiver, &event);
    return event.points().first();
}
}

class KisInputEventSequenceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void replaysMouseSequenceWithSynthesizedSource();
    void replaysTabletSequenceWithStylusValues();
    void replaysSingleTouchSequence();
    void rejectsUnsupportedEventsAndInvalidReplay();
};

void KisInputEventSequenceContractTest::replaysMouseSequenceWithSynthesizedSource()
{
    QObject source;
    KisInputEventSequence sequence;
    QVERIFY(sequence.startRecording(&source));

    sendMouseEvent(&source,
                   QEvent::MouseButtonPress,
                   QPointF(10.5, 20.25),
                   QPointF(110.5, 220.25),
                   Qt::LeftButton,
                   Qt::LeftButton,
                   Qt::MouseEventSynthesizedBySystem,
                   101);
    sendMouseEvent(&source,
                   QEvent::MouseMove,
                   QPointF(12.5, 24.25),
                   QPointF(112.5, 224.25),
                   Qt::NoButton,
                   Qt::LeftButton,
                   Qt::MouseEventSynthesizedBySystem,
                   102);
    sendMouseEvent(&source,
                   QEvent::MouseButtonRelease,
                   QPointF(12.5, 24.25),
                   QPointF(112.5, 224.25),
                   Qt::LeftButton,
                   Qt::NoButton,
                   Qt::MouseEventSynthesizedBySystem,
                   103);

    sequence.stopRecording();
    QCOMPARE(sequence.size(), 3);

    EventReceiver target;
    QVERIFY(sequence.replay(&target));
    QCOMPARE(target.events.size(), 3);

    const QList<QEvent::Type> expectedTypes{
        QEvent::MouseButtonPress,
        QEvent::MouseMove,
        QEvent::MouseButtonRelease,
    };
    const QList<quint64> expectedTimestamps{101, 102, 103};
    for (qsizetype i = 0; i < target.events.size(); ++i) {
        const auto *event = static_cast<const QMouseEvent *>(target.events.at(i).data());
        QCOMPARE(event->type(), expectedTypes.at(i));
        QCOMPARE(event->modifiers(), Qt::KeyboardModifiers(Qt::ShiftModifier));
        QCOMPARE(event->source(), Qt::MouseEventSynthesizedBySystem);
        QCOMPARE(event->timestamp(), expectedTimestamps.at(i));
    }

    const auto *press = static_cast<const QMouseEvent *>(target.events.at(0).data());
    QCOMPARE(press->position(), QPointF(10.5, 20.25));
    QCOMPARE(press->globalPosition(), QPointF(110.5, 220.25));
    QCOMPARE(press->button(), Qt::LeftButton);
    QCOMPARE(press->buttons(), Qt::MouseButtons(Qt::LeftButton));

    const auto *move = static_cast<const QMouseEvent *>(target.events.at(1).data());
    QCOMPARE(move->position(), QPointF(12.5, 24.25));
    QCOMPARE(move->globalPosition(), QPointF(112.5, 224.25));
    QCOMPARE(move->button(), Qt::NoButton);
    QCOMPARE(move->buttons(), Qt::MouseButtons(Qt::LeftButton));

    const auto *release = static_cast<const QMouseEvent *>(target.events.at(2).data());
    QCOMPARE(release->button(), Qt::LeftButton);
    QCOMPARE(release->buttons(), Qt::MouseButtons(Qt::NoButton));
}

void KisInputEventSequenceContractTest::replaysTabletSequenceWithStylusValues()
{
    const QInputDevice::Capabilities capabilities =
        QInputDevice::Capability::Position |
        QInputDevice::Capability::Pressure |
        QInputDevice::Capability::XTilt |
        QInputDevice::Capability::YTilt |
        QInputDevice::Capability::TangentialPressure |
        QInputDevice::Capability::Rotation |
        QInputDevice::Capability::ZPosition;
    QPointingDevice stylus(QStringLiteral("contract-stylus"),
                           41,
                           QInputDevice::DeviceType::Stylus,
                           QPointingDevice::PointerType::Pen,
                           capabilities,
                           1,
                           3,
                           QString(),
                           QPointingDeviceUniqueId::fromNumericId(9001));

    QObject source;
    KisInputEventSequence sequence;
    QVERIFY(sequence.startRecording(&source));

    sendTabletEvent(&source,
                    QEvent::TabletPress,
                    &stylus,
                    QPointF(30.25, 40.5),
                    QPointF(130.25, 240.5),
                    0.25,
                    12.0f,
                    -8.0f,
                    0.125f,
                    33.0,
                    2.0f,
                    Qt::LeftButton,
                    Qt::LeftButton,
                    201);
    sendTabletEvent(&source,
                    QEvent::TabletMove,
                    &stylus,
                    QPointF(31.25, 42.5),
                    QPointF(131.25, 242.5),
                    0.75,
                    15.0f,
                    -4.0f,
                    0.25f,
                    44.0,
                    3.0f,
                    Qt::NoButton,
                    Qt::LeftButton,
                    202);
    sendTabletEvent(&source,
                    QEvent::TabletRelease,
                    &stylus,
                    QPointF(31.25, 42.5),
                    QPointF(131.25, 242.5),
                    0.0,
                    15.0f,
                    -4.0f,
                    0.25f,
                    44.0,
                    3.0f,
                    Qt::LeftButton,
                    Qt::NoButton,
                    203);

    sequence.stopRecording();
    EventReceiver target;
    QVERIFY(sequence.replay(&target));
    QCOMPARE(target.events.size(), 3);

    const auto *press = static_cast<const QTabletEvent *>(target.events.at(0).data());
    QCOMPARE(press->type(), QEvent::TabletPress);
    QCOMPARE(press->position(), QPointF(30.25, 40.5));
    QCOMPARE(press->globalPosition(), QPointF(130.25, 240.5));
    QCOMPARE(press->pressure(), 0.25);
    QCOMPARE(press->xTilt(), 12.0);
    QCOMPARE(press->yTilt(), -8.0);
    QCOMPARE(press->tangentialPressure(), 0.125);
    QCOMPARE(press->rotation(), 33.0);
    QCOMPARE(press->z(), 2.0);
    QCOMPARE(press->pointingDevice(), &stylus);
    QCOMPARE(press->timestamp(), quint64(201));
    QCOMPARE(press->modifiers(), Qt::KeyboardModifiers(Qt::AltModifier));

    const auto *move = static_cast<const QTabletEvent *>(target.events.at(1).data());
    QCOMPARE(move->type(), QEvent::TabletMove);
    QCOMPARE(move->pressure(), 0.75);
    QCOMPARE(move->xTilt(), 15.0);
    QCOMPARE(move->yTilt(), -4.0);
    QCOMPARE(move->rotation(), 44.0);
    QCOMPARE(move->timestamp(), quint64(202));

    const auto *release = static_cast<const QTabletEvent *>(target.events.at(2).data());
    QCOMPARE(release->type(), QEvent::TabletRelease);
    QCOMPARE(release->pressure(), 0.0);
    QCOMPARE(release->button(), Qt::LeftButton);
    QCOMPARE(release->buttons(), Qt::MouseButtons(Qt::NoButton));
    QCOMPARE(release->timestamp(), quint64(203));
}

void KisInputEventSequenceContractTest::replaysSingleTouchSequence()
{
    QPointingDevice touchScreen(QStringLiteral("contract-touchscreen"),
                                51,
                                QInputDevice::DeviceType::TouchScreen,
                                QPointingDevice::PointerType::Finger,
                                QInputDevice::Capability::Position |
                                    QInputDevice::Capability::Pressure,
                                10,
                                0);

    QObject source;
    KisInputEventSequence sequence;
    QVERIFY(sequence.startRecording(&source));

    const QList<QEventPoint> inputPoints{
        sendTouchEvent(&source,
                       QEvent::TouchBegin,
                       &touchScreen,
                       QEventPoint::State::Pressed,
                       7,
                       QPointF(50.5, 60.25),
                       QPointF(150.5, 260.25),
                       301),
        sendTouchEvent(&source,
                       QEvent::TouchUpdate,
                       &touchScreen,
                       QEventPoint::State::Updated,
                       7,
                       QPointF(52.5, 64.25),
                       QPointF(152.5, 264.25),
                       302),
        sendTouchEvent(&source,
                       QEvent::TouchEnd,
                       &touchScreen,
                       QEventPoint::State::Released,
                       7,
                       QPointF(52.5, 64.25),
                       QPointF(152.5, 264.25),
                       303),
        sendTouchEvent(&source,
                       QEvent::TouchCancel,
                       &touchScreen,
                       QEventPoint::State::Released,
                       7,
                       QPointF(52.5, 64.25),
                       QPointF(152.5, 264.25),
                       304),
    };

    sequence.stopRecording();
    EventReceiver target;
    QVERIFY(sequence.replay(&target));
    QCOMPARE(target.events.size(), 4);

    const QList<QEvent::Type> expectedTypes{
        QEvent::TouchBegin,
        QEvent::TouchUpdate,
        QEvent::TouchEnd,
        QEvent::TouchCancel,
    };
    const QList<QEventPoint::State> expectedStates{
        QEventPoint::State::Pressed,
        QEventPoint::State::Updated,
        QEventPoint::State::Released,
        QEventPoint::State::Released,
    };

    for (qsizetype i = 0; i < target.events.size(); ++i) {
        const auto *event = static_cast<const QTouchEvent *>(target.events.at(i).data());
        QCOMPARE(event->type(), expectedTypes.at(i));
        QCOMPARE(event->pointingDevice(), &touchScreen);
        QCOMPARE(event->modifiers(), Qt::KeyboardModifiers(Qt::ControlModifier));
        QCOMPARE(event->timestamp(), quint64(301 + i));
        QCOMPARE(event->points().size(), 1);
        const QEventPoint &replayedPoint = event->points().first();
        const QEventPoint &inputPoint = inputPoints.at(i);
        QCOMPARE(replayedPoint.id(), 7);
        QCOMPARE(replayedPoint.state(), expectedStates.at(i));
        QCOMPARE(replayedPoint.position(), inputPoint.position());
        QCOMPARE(replayedPoint.scenePosition(), inputPoint.scenePosition());
        QCOMPARE(replayedPoint.globalPosition(), inputPoint.globalPosition());
        QCOMPARE(replayedPoint.pressure(), inputPoint.pressure());
        QCOMPARE(replayedPoint.rotation(), inputPoint.rotation());
    }

    const QEventPoint &pressPoint =
        static_cast<const QTouchEvent *>(target.events.at(0).data())->points().first();
    QCOMPARE(pressPoint.position(), QPointF());
    QCOMPARE(pressPoint.scenePosition(), QPointF(50.5, 60.25));
    QCOMPARE(pressPoint.globalPosition(), QPointF(150.5, 260.25));
}

void KisInputEventSequenceContractTest::rejectsUnsupportedEventsAndInvalidReplay()
{
    QObject source;
    EventReceiver target;
    KisInputEventSequence sequence;

    QVERIFY(!sequence.startRecording(nullptr));
    QVERIFY(sequence.startRecording(&source));
    QVERIFY(sequence.isRecording());

    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QCoreApplication::sendEvent(&source, &keyEvent);
    QCOMPARE(sequence.size(), 0);

    QVERIFY(!sequence.replay(nullptr));
    QVERIFY(!sequence.replay(&target));

    sequence.stopRecording();
    QVERIFY(!sequence.isRecording());
    QVERIFY(sequence.replay(&target));
    QCOMPARE(target.events.size(), 0);

    QVERIFY(sequence.startRecording(&source));
    sendMouseEvent(&source,
                   QEvent::MouseButtonDblClick,
                   QPointF(1.0, 2.0),
                   QPointF(3.0, 4.0),
                   Qt::LeftButton,
                   Qt::LeftButton,
                   Qt::MouseEventSynthesizedByApplication,
                   401);
    sequence.stopRecording();
    QCOMPARE(sequence.size(), 1);
    sequence.clear();
    QCOMPARE(sequence.size(), 0);
}

QTEST_GUILESS_MAIN(KisInputEventSequenceContractTest)

#include "KisInputEventSequenceContractTest.moc"
