/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commands_new/kis_transaction_based_command.h"

#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace
{

struct CommandObservation {
    QStringList calls;
    int childDestructions = 0;
};

class TransactionCommand final : public KUndo2Command
{
public:
    explicit TransactionCommand(CommandObservation *observation)
        : m_observation(observation)
    {
    }

    ~TransactionCommand() override
    {
        ++m_observation->childDestructions;
    }

    void redo() override
    {
        m_observation->calls.append(QStringLiteral("child-redo"));
    }

    void undo() override
    {
        m_observation->calls.append(QStringLiteral("child-undo"));
    }

private:
    CommandObservation *m_observation;
};

class TransactionBasedCommandProbe final : public KisTransactionBasedCommand
{
public:
    TransactionBasedCommandProbe(const KUndo2MagicString &text,
                                 CommandObservation *observation,
                                 int nullPaintResults,
                                 KUndo2Command *parent = nullptr)
        : KisTransactionBasedCommand(text, parent)
        , m_observation(observation)
        , m_nullPaintResults(nullPaintResults)
    {
    }

    int paintCalls() const
    {
        return m_paintCalls;
    }

protected:
    KUndo2Command *paint() override
    {
        m_observation->calls.append(QStringLiteral("paint"));
        ++m_paintCalls;

        if (m_paintCalls <= m_nullPaintResults) {
            return nullptr;
        }

        return new TransactionCommand(m_observation);
    }

private:
    CommandObservation *m_observation;
    int m_nullPaintResults;
    int m_paintCalls = 0;
};

} // namespace

class KisTransactionBasedCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createsTransactionOnceAndOwnsItThroughTheParentedWrapper();
    void nullTransactionsAreRetriedAndUndoBeforeCreationIsEmpty();
};

void KisTransactionBasedCommandContractTest::createsTransactionOnceAndOwnsItThroughTheParentedWrapper()
{
    CommandObservation observation;

    {
        KUndo2Command parent;
        auto *command = new TransactionBasedCommandProbe(kundo2_noi18n(QStringLiteral("Transaction Contract")),
                                                         &observation,
                                                         0,
                                                         &parent);

        QCOMPARE(parent.childCount(), 1);
        QCOMPARE(parent.child(0), command);
        QCOMPARE(command->text().toString(), QStringLiteral("Transaction Contract"));

        command->undo();
        QCOMPARE(command->paintCalls(), 0);
        QVERIFY(observation.calls.isEmpty());

        command->redo();
        QCOMPARE(command->paintCalls(), 1);
        QCOMPARE(observation.calls, QStringList({QStringLiteral("paint"), QStringLiteral("child-redo")}));

        command->redo();
        command->undo();
        QCOMPARE(command->paintCalls(), 1);
        QCOMPARE(observation.calls,
                 QStringList({QStringLiteral("paint"),
                              QStringLiteral("child-redo"),
                              QStringLiteral("child-redo"),
                              QStringLiteral("child-undo")}));
        QCOMPARE(observation.childDestructions, 0);
    }

    QCOMPARE(observation.childDestructions, 1);
}

void KisTransactionBasedCommandContractTest::nullTransactionsAreRetriedAndUndoBeforeCreationIsEmpty()
{
    CommandObservation observation;

    {
        TransactionBasedCommandProbe command(KUndo2MagicString(), &observation, 1);

        command.undo();
        QVERIFY(observation.calls.isEmpty());

        command.redo();
        QCOMPARE(command.paintCalls(), 1);
        QCOMPARE(observation.calls, QStringList({QStringLiteral("paint")}));

        command.undo();
        QCOMPARE(observation.calls, QStringList({QStringLiteral("paint")}));

        command.redo();
        command.redo();
        QCOMPARE(command.paintCalls(), 2);
        QCOMPARE(observation.calls,
                 QStringList({QStringLiteral("paint"),
                              QStringLiteral("paint"),
                              QStringLiteral("child-redo"),
                              QStringLiteral("child-redo")}));
        QCOMPARE(observation.childDestructions, 0);
    }

    QCOMPARE(observation.childDestructions, 1);
}

QTEST_GUILESS_MAIN(KisTransactionBasedCommandContractTest)

#include "KisTransactionBasedCommandContractTest.moc"
