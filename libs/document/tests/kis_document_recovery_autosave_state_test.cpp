/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <session/kis_document_recovery_autosave_state.h>

using Krita::Document::RecoveryAutoSaveState;

class KisDocumentRecoveryAutoSaveStateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsWithoutRequest();
    void tracksPendingRequestAndJoinedSave();
    void defersCompletionReportedDuringSaveStart();
    void discardsDeferredCompletionWhenSaveDoesNotStart();
    void completesActiveRequestOnce();
};

void KisDocumentRecoveryAutoSaveStateTest::startsWithoutRequest()
{
    const RecoveryAutoSaveState state;

    QVERIFY(!state.hasPendingRequest());
    QVERIFY(state.joinedSavePath().isEmpty());
}

void KisDocumentRecoveryAutoSaveStateTest::tracksPendingRequestAndJoinedSave()
{
    RecoveryAutoSaveState state;

    state.beginRequest();
    QVERIFY(state.hasPendingRequest());

    state.recordJoinedSave(QStringLiteral("/tmp/recovery.kra"), true);
    QCOMPARE(state.joinedSavePath(), QStringLiteral("/tmp/recovery.kra"));

    state.clearJoinedSavePath();
    QVERIFY(state.joinedSavePath().isEmpty());

    state.recordJoinedSave(QStringLiteral("/tmp/recovery.kra"), true);
    state.recordJoinedSave(QStringLiteral("/tmp/failed.kra"), false);
    QVERIFY(state.joinedSavePath().isEmpty());

    state.recordJoinedSave(QStringLiteral("/tmp/recovery.kra"), true);
    state.cancelRequest();
    QVERIFY(!state.hasPendingRequest());
    QVERIFY(state.joinedSavePath().isEmpty());
}

void KisDocumentRecoveryAutoSaveStateTest::defersCompletionReportedDuringSaveStart()
{
    RecoveryAutoSaveState state;
    state.beginRequest();
    state.recordJoinedSave(QStringLiteral("/tmp/previous.kra"), true);
    state.beginSaveStart();

    QCOMPARE(state.recordCompletion(QStringLiteral("/tmp/recovery.kra"), true),
             RecoveryAutoSaveState::CompletionHandling::Deferred);
    QVERIFY(state.hasPendingRequest());

    const RecoveryAutoSaveState::Completion completion = state.markSaveStartSucceeded();
    QVERIFY(completion.available);
    QCOMPARE(completion.filePath, QStringLiteral("/tmp/recovery.kra"));
    QVERIFY(completion.success);
    QVERIFY(state.joinedSavePath().isEmpty());

    QCOMPARE(state.recordCompletion(completion.filePath, completion.success),
             RecoveryAutoSaveState::CompletionHandling::Ready);
    QVERIFY(!state.hasPendingRequest());
}

void KisDocumentRecoveryAutoSaveStateTest::discardsDeferredCompletionWhenSaveDoesNotStart()
{
    RecoveryAutoSaveState state;
    state.beginRequest();
    state.beginSaveStart();

    QCOMPARE(state.recordCompletion(QStringLiteral("/tmp/unusable.kra"), false),
             RecoveryAutoSaveState::CompletionHandling::Deferred);

    state.markSaveStartDidNotSucceed();
    QVERIFY(state.hasPendingRequest());

    state.beginSaveStart();
    const RecoveryAutoSaveState::Completion completion = state.markSaveStartSucceeded();
    QVERIFY(!completion.available);
}

void KisDocumentRecoveryAutoSaveStateTest::completesActiveRequestOnce()
{
    RecoveryAutoSaveState state;

    QCOMPARE(state.recordCompletion(QStringLiteral("/tmp/no-request.kra"), true),
             RecoveryAutoSaveState::CompletionHandling::Ignored);

    state.beginRequest();
    QCOMPARE(state.recordCompletion(QString(), true),
             RecoveryAutoSaveState::CompletionHandling::Ready);
    QVERIFY(!state.hasPendingRequest());

    QCOMPARE(state.recordCompletion(QString(), true),
             RecoveryAutoSaveState::CompletionHandling::Ignored);
}

QTEST_GUILESS_MAIN(KisDocumentRecoveryAutoSaveStateTest)

#include "kis_document_recovery_autosave_state_test.moc"
