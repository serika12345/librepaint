/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_commands_test.h"

#include "commands_new/KisDisableDirtyRequestsCommand.h"
#include "kis_image_interfaces.h"

#include <QRect>
#include <QStringList>
#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace KisCommandUtils
{

FlipFlopCommand::FlipFlopCommand(State initialState, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_currentState(initialState)
{
}

void FlipFlopCommand::redo()
{
    if (m_currentState == INITIALIZING) {
        partA();
    } else {
        partB();
    }

    m_firstRedo = false;
}

void FlipFlopCommand::undo()
{
    if (m_currentState == FINALIZING) {
        partA();
    } else {
        partB();
    }
}

void FlipFlopCommand::partA()
{
}

void FlipFlopCommand::partB()
{
}

} // namespace KisCommandUtils

KisUpdatesFacade::~KisUpdatesFacade() = default;

namespace
{

class RecordingUpdatesFacade final : public KisUpdatesFacade
{
public:
    explicit RecordingUpdatesFacade(bool *destroyed = nullptr)
        : m_destroyed(destroyed)
    {
    }

    ~RecordingUpdatesFacade() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    void blockUpdates() override
    {
    }
    void unblockUpdates() override
    {
    }
    void disableUIUpdates() override
    {
    }
    QVector<QRect> enableUIUpdates() override
    {
        return {};
    }
    bool hasUpdatesRunning() const override
    {
        return false;
    }
    void notifyBatchUpdateStarted() override
    {
    }
    void notifyBatchUpdateEnded() override
    {
    }
    void notifyUIUpdateCompleted(const QRect &) override
    {
    }
    QRect bounds() const override
    {
        return {};
    }

    void disableDirtyRequests() override
    {
        calls.append(QStringLiteral("disable"));
    }

    void enableDirtyRequests() override
    {
        calls.append(QStringLiteral("enable"));
    }

    void refreshGraphAsync(KisNodeSP, const QVector<QRect> &, const QRect &, KisProjectionUpdateFlags) override
    {
    }

    KisProjectionUpdatesFilterCookie addProjectionUpdatesFilter(KisProjectionUpdatesFilterSP) override
    {
        return nullptr;
    }

    KisProjectionUpdatesFilterSP removeProjectionUpdatesFilter(KisProjectionUpdatesFilterCookie) override
    {
        return {};
    }

    KisProjectionUpdatesFilterCookie currentProjectionUpdatesFilter() const override
    {
        return nullptr;
    }

    QStringList calls;

private:
    bool *m_destroyed;
};

} // namespace

void KisImageCommandsTest::initializingStateRoutesDirtyRequestCalls()
{
    RecordingUpdatesFacade facade;
    KisDisableDirtyRequestsCommand command(&facade, KisDisableDirtyRequestsCommand::INITIALIZING);

    command.redo();
    QCOMPARE(facade.calls, QStringList({QStringLiteral("disable")}));

    command.undo();
    QCOMPARE(facade.calls, QStringList({QStringLiteral("disable"), QStringLiteral("enable")}));

    command.redo();
    QCOMPARE(facade.calls,
             QStringList({QStringLiteral("disable"), QStringLiteral("enable"), QStringLiteral("disable")}));
}

void KisImageCommandsTest::finalizingStateRoutesDirtyRequestCalls()
{
    RecordingUpdatesFacade facade;
    KisDisableDirtyRequestsCommand command(&facade, KisDisableDirtyRequestsCommand::FINALIZING);

    command.redo();
    QCOMPARE(facade.calls, QStringList({QStringLiteral("enable")}));

    command.undo();
    QCOMPARE(facade.calls, QStringList({QStringLiteral("enable"), QStringLiteral("disable")}));

    command.redo();
    QCOMPARE(facade.calls,
             QStringList({QStringLiteral("enable"), QStringLiteral("disable"), QStringLiteral("enable")}));
}

void KisImageCommandsTest::publicPartsUseABorrowedFacade()
{
    bool facadeDestroyed = false;
    auto *facade = new RecordingUpdatesFacade(&facadeDestroyed);

    {
        KisDisableDirtyRequestsCommand command(facade, KisDisableDirtyRequestsCommand::INITIALIZING);
        command.partA();
        command.partB();
        QCOMPARE(facade->calls, QStringList({QStringLiteral("disable"), QStringLiteral("enable")}));
    }

    QVERIFY(!facadeDestroyed);
    delete facade;
    QVERIFY(facadeDestroyed);
}

QTEST_GUILESS_MAIN(KisImageCommandsTest)
