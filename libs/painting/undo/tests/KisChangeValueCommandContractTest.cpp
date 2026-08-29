/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <memory>
#include <type_traits>

#include <QTest>

#include <KisChangeValueCommand.h>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{
struct DirectState {
    int value{0};
};

struct IndirectState {
    int *value{nullptr};
};
} // namespace

class KisChangeValueCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directCommandCapturesAndRestoresTheConstructionValue();
    void indirectCommandResolvesTheCurrentPointeeForEveryAssignment();
    void factoriesDeduceValueTypesAndJoinParentOwnership();
};

void KisChangeValueCommandContractTest::directCommandCapturesAndRestoresTheConstructionValue()
{
    DirectState state{3};
    KisChangeValueCommand<&DirectState::value, short> command(&state, short{9});
    KUndo2Command *base = &command;

    QCOMPARE(state.value, 3);

    base->redo();
    QCOMPARE(state.value, 9);

    state.value = 17;
    base->undo();
    QCOMPARE(state.value, 3);

    base->redo();
    QCOMPARE(state.value, 9);
}

void KisChangeValueCommandContractTest::indirectCommandResolvesTheCurrentPointeeForEveryAssignment()
{
    int firstValue = 2;
    int secondValue = 4;
    IndirectState state{&firstValue};
    KisChangeIndirectValueCommand<&IndirectState::value, short> command(&state, short{9});
    KUndo2Command *base = &command;

    firstValue = 7;
    state.value = &secondValue;
    base->redo();
    QCOMPARE(firstValue, 7);
    QCOMPARE(secondValue, 9);

    base->undo();
    QCOMPARE(firstValue, 7);
    QCOMPARE(secondValue, 2);

    state.value = &firstValue;
    base->redo();
    QCOMPARE(firstValue, 9);
    base->undo();
    QCOMPARE(firstValue, 2);
}

void KisChangeValueCommandContractTest::factoriesDeduceValueTypesAndJoinParentOwnership()
{
    DirectState directState{5};
    int indirectValue = 6;
    IndirectState indirectState{&indirectValue};
    KUndo2Command parent;

    auto *directCommand = makeChangeValueCommand<&DirectState::value>(&directState, short{11}, &parent);
    auto *indirectCommand = makeChangeIndirectValueCommand<&IndirectState::value>(&indirectState, short{12}, &parent);

    static_assert(std::is_same_v<decltype(directCommand), KisChangeValueCommand<&DirectState::value, short> *>);
    static_assert(
        std::is_same_v<decltype(indirectCommand), KisChangeIndirectValueCommand<&IndirectState::value, short> *>);
    static_assert(std::is_base_of_v<KUndo2Command, KisChangeValueCommand<&DirectState::value, short>>);
    static_assert(std::is_base_of_v<KUndo2Command, KisChangeIndirectValueCommand<&IndirectState::value, short>>);

    QCOMPARE(parent.childCount(), 2);
    QCOMPARE(parent.child(0), directCommand);
    QCOMPARE(parent.child(1), indirectCommand);
    QCOMPARE(directState.value, 5);
    QCOMPARE(indirectValue, 6);

    parent.redo();
    QCOMPARE(directState.value, 11);
    QCOMPARE(indirectValue, 12);

    parent.undo();
    QCOMPARE(directState.value, 5);
    QCOMPARE(indirectValue, 6);
}

QTEST_MAIN(KisChangeValueCommandContractTest)

#include "KisChangeValueCommandContractTest.moc"
