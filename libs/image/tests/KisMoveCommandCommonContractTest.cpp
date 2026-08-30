/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commands_new/kis_move_command_common.h"

#include <QSharedPointer>
#include <QTest>
#include <QVector>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace
{

struct CoordinateUpdate {
    enum Axis {
        X,
        Y
    };

    Axis axis;
    int value;
    const void *object;
};

class MoveProbe
{
public:
    void setX(int value)
    {
        x = value;
        updates.append({CoordinateUpdate::X, value, this});
    }

    void setY(int value)
    {
        y = value;
        updates.append({CoordinateUpdate::Y, value, this});
    }

    int x = 0;
    int y = 0;
    QVector<CoordinateUpdate> updates;
};

void verifyUpdate(const MoveProbe &probe,
                  int index,
                  CoordinateUpdate::Axis axis,
                  int value,
                  const MoveProbe *expectedObject)
{
    QCOMPARE(probe.updates.at(index).axis, axis);
    QCOMPARE(probe.updates.at(index).value, value);
    QCOMPARE(probe.updates.at(index).object, expectedObject);
}

} // namespace

class KisMoveCommandCommonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void redoAndUndoDispatchCoordinatesInOrderAndRetainOwnedObject();
};

void KisMoveCommandCommonContractTest::redoAndUndoDispatchCoordinatesInOrderAndRetainOwnedObject()
{
    const QPoint oldPosition(-13, 21);
    const QPoint newPosition(34, -55);
    QWeakPointer<MoveProbe> weakProbe;

    {
        KUndo2Command parent;
        QSharedPointer<MoveProbe> probe = QSharedPointer<MoveProbe>::create();
        weakProbe = probe;
        const MoveProbe *const probeAddress = probe.data();

        using Command = KisMoveCommandCommon<QSharedPointer<MoveProbe>>;
        auto *command = new Command(probe, oldPosition, newPosition, &parent);

        QCOMPARE(parent.childCount(), 1);
        QCOMPARE(parent.child(0), command);

        probe.clear();
        QVERIFY(!weakProbe.isNull());
        QSharedPointer<MoveProbe> retainedProbe = weakProbe.toStrongRef();

        KUndo2Command *baseCommand = command;
        baseCommand->redo();
        baseCommand->redo();
        baseCommand->undo();
        baseCommand->undo();

        QCOMPARE(retainedProbe->updates.size(), 8);
        verifyUpdate(*retainedProbe, 0, CoordinateUpdate::X, newPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 1, CoordinateUpdate::Y, newPosition.y(), probeAddress);
        verifyUpdate(*retainedProbe, 2, CoordinateUpdate::X, newPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 3, CoordinateUpdate::Y, newPosition.y(), probeAddress);
        verifyUpdate(*retainedProbe, 4, CoordinateUpdate::X, oldPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 5, CoordinateUpdate::Y, oldPosition.y(), probeAddress);
        verifyUpdate(*retainedProbe, 6, CoordinateUpdate::X, oldPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 7, CoordinateUpdate::Y, oldPosition.y(), probeAddress);
        QCOMPARE(retainedProbe->x, oldPosition.x());
        QCOMPARE(retainedProbe->y, oldPosition.y());

        retainedProbe.clear();
        QVERIFY(!weakProbe.isNull());
    }

    QVERIFY(weakProbe.isNull());
}

QTEST_GUILESS_MAIN(KisMoveCommandCommonContractTest)

#include "KisMoveCommandCommonContractTest.moc"
