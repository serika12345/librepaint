/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <session/kis_document_autosave_state.h>

using Krita::Document::AutoSaveState;

class KisDocumentAutoSaveStateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsWithoutOperationProgress();
    void tracksAutoSaveExportLifetime();
    void requestsCloneFallbackAfterRepeatedFailures();
};

void KisDocumentAutoSaveStateTest::startsWithoutOperationProgress()
{
    const AutoSaveState state;

    QVERIFY(!state.isExportingAutoSave());
    QVERIFY(!state.shouldUseCloneFallback());
}

void KisDocumentAutoSaveStateTest::tracksAutoSaveExportLifetime()
{
    AutoSaveState state;

    state.markAutoSaveExportStarted();
    QVERIFY(state.isExportingAutoSave());

    state.markAutoSaveExportFinished();
    QVERIFY(!state.isExportingAutoSave());
}

void KisDocumentAutoSaveStateTest::requestsCloneFallbackAfterRepeatedFailures()
{
    AutoSaveState state;

    for (int failure = 0; failure < 3; ++failure) {
        QVERIFY(!state.shouldUseCloneFallback());
        state.recordFailure();
    }

    QVERIFY(state.shouldUseCloneFallback());

    state.clearFailures();
    QVERIFY(!state.shouldUseCloneFallback());
}

QTEST_GUILESS_MAIN(KisDocumentAutoSaveStateTest)

#include "kis_document_autosave_state_test.moc"
