/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_types.h"

class KisSelection
{
public:
    void setX(qint32 value);
    void setY(qint32 value);
    void notifySelectionChanged();
};

#include "commands_new/kis_move_command_common.h"
#include "commands_new/kis_selection_move_command2.h"

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
    enum Type {
        SetX,
        SetY,
        Notify
    };

    Type type;
    int value;
    const void *object;
};

struct SelectionToken {
    int references = 0;
    int finalReleases = 0;
    QVector<CoordinateUpdate> updates;
};

KisSelection *selectionPointer(SelectionToken *token)
{
    return reinterpret_cast<KisSelection *>(token);
}

SelectionToken *selectionToken(KisSelection *selection)
{
    return reinterpret_cast<SelectionToken *>(selection);
}

class MoveProbe
{
public:
    void setX(int value)
    {
        x = value;
        updates.append({CoordinateUpdate::SetX, value, this});
    }

    void setY(int value)
    {
        y = value;
        updates.append({CoordinateUpdate::SetY, value, this});
    }

    int x = 0;
    int y = 0;
    QVector<CoordinateUpdate> updates;
};

void verifyUpdate(const MoveProbe &probe,
                  int index,
                  CoordinateUpdate::Type type,
                  int value,
                  const MoveProbe *expectedObject)
{
    QCOMPARE(probe.updates.at(index).type, type);
    QCOMPARE(probe.updates.at(index).value, value);
    QCOMPARE(probe.updates.at(index).object, expectedObject);
}

void verifySelectionUpdate(const SelectionToken &token,
                           int index,
                           CoordinateUpdate::Type type,
                           int value,
                           const KisSelection *expectedObject)
{
    QCOMPARE(token.updates.at(index).type, type);
    QCOMPARE(token.updates.at(index).value, value);
    QCOMPARE(token.updates.at(index).object, expectedObject);
}

} // namespace

void kisSharedPtrAddReference(KisSelection *selection)
{
    ++selectionToken(selection)->references;
}

bool kisSharedPtrRelease(KisSelection *selection)
{
    SelectionToken *const token = selectionToken(selection);
    --token->references;
    if (token->references == 0) {
        ++token->finalReleases;
    }
    return true;
}

void KisSelection::setX(qint32 value)
{
    selectionToken(this)->updates.append({CoordinateUpdate::SetX, value, this});
}

void KisSelection::setY(qint32 value)
{
    selectionToken(this)->updates.append({CoordinateUpdate::SetY, value, this});
}

void KisSelection::notifySelectionChanged()
{
    selectionToken(this)->updates.append({CoordinateUpdate::Notify, 0, this});
}

class KisMoveCommandCommonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void redoAndUndoDispatchCoordinatesInOrderAndRetainOwnedObject();
    void selectionMoveRetainsSelectionAndParentOwnership();
    void selectionMoveRedoAndUndoDispatchCoordinatesBeforeNotification();
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
        verifyUpdate(*retainedProbe, 0, CoordinateUpdate::SetX, newPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 1, CoordinateUpdate::SetY, newPosition.y(), probeAddress);
        verifyUpdate(*retainedProbe, 2, CoordinateUpdate::SetX, newPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 3, CoordinateUpdate::SetY, newPosition.y(), probeAddress);
        verifyUpdate(*retainedProbe, 4, CoordinateUpdate::SetX, oldPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 5, CoordinateUpdate::SetY, oldPosition.y(), probeAddress);
        verifyUpdate(*retainedProbe, 6, CoordinateUpdate::SetX, oldPosition.x(), probeAddress);
        verifyUpdate(*retainedProbe, 7, CoordinateUpdate::SetY, oldPosition.y(), probeAddress);
        QCOMPARE(retainedProbe->x, oldPosition.x());
        QCOMPARE(retainedProbe->y, oldPosition.y());

        retainedProbe.clear();
        QVERIFY(!weakProbe.isNull());
    }

    QVERIFY(weakProbe.isNull());
}

void KisMoveCommandCommonContractTest::selectionMoveRetainsSelectionAndParentOwnership()
{
    SelectionToken token;

    {
        KUndo2Command parent;
        KisSelectionSP selection(selectionPointer(&token));
        QCOMPARE(token.references, 1);

        auto *command = new KisSelectionMoveCommand2(selection, QPoint(-13, 21), QPoint(34, -55), &parent);

        QCOMPARE(parent.childCount(), 1);
        QCOMPARE(parent.child(0), command);

        selection.clear();
        QCOMPARE(token.references, 1);
        QCOMPARE(token.finalReleases, 0);
    }

    QCOMPARE(token.references, 0);
    QCOMPARE(token.finalReleases, 1);
}

void KisMoveCommandCommonContractTest::selectionMoveRedoAndUndoDispatchCoordinatesBeforeNotification()
{
    const QPoint oldPosition(-13, 21);
    const QPoint newPosition(34, -55);
    SelectionToken token;
    KisSelection *const selectionAddress = selectionPointer(&token);
    KisSelectionSP selection(selectionAddress);
    KisSelectionMoveCommand2 command(selection, oldPosition, newPosition);

    KUndo2Command *const baseCommand = &command;
    baseCommand->redo();
    baseCommand->redo();
    baseCommand->undo();
    baseCommand->undo();

    QCOMPARE(token.updates.size(), 12);
    verifySelectionUpdate(token, 0, CoordinateUpdate::SetX, newPosition.x(), selectionAddress);
    verifySelectionUpdate(token, 1, CoordinateUpdate::SetY, newPosition.y(), selectionAddress);
    verifySelectionUpdate(token, 2, CoordinateUpdate::Notify, 0, selectionAddress);
    verifySelectionUpdate(token, 3, CoordinateUpdate::SetX, newPosition.x(), selectionAddress);
    verifySelectionUpdate(token, 4, CoordinateUpdate::SetY, newPosition.y(), selectionAddress);
    verifySelectionUpdate(token, 5, CoordinateUpdate::Notify, 0, selectionAddress);
    verifySelectionUpdate(token, 6, CoordinateUpdate::SetX, oldPosition.x(), selectionAddress);
    verifySelectionUpdate(token, 7, CoordinateUpdate::SetY, oldPosition.y(), selectionAddress);
    verifySelectionUpdate(token, 8, CoordinateUpdate::Notify, 0, selectionAddress);
    verifySelectionUpdate(token, 9, CoordinateUpdate::SetX, oldPosition.x(), selectionAddress);
    verifySelectionUpdate(token, 10, CoordinateUpdate::SetY, oldPosition.y(), selectionAddress);
    verifySelectionUpdate(token, 11, CoordinateUpdate::Notify, 0, selectionAddress);
}

QTEST_GUILESS_MAIN(KisMoveCommandCommonContractTest)

#include "KisMoveCommandCommonContractTest.moc"
