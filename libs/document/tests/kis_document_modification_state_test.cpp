/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <session/kis_document_modification_state.h>

using Krita::Document::ModificationState;

class KisDocumentModificationStateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsWithoutChanges();
    void tracksChangesSinceAutoSave();
    void tracksChangesDuringSave();
    void snapshotsDocumentStateWithoutOperationProgress();
};

void KisDocumentModificationStateTest::startsWithoutChanges()
{
    const ModificationState state;

    QVERIFY(!state.isModified());
    QVERIFY(!state.hasChangesAfterAutoSave());
    QVERIFY(!state.wasModifiedWhileSaving());
    QVERIFY(!state.imageModifiedWithoutUndo());
}

void KisDocumentModificationStateTest::tracksChangesSinceAutoSave()
{
    ModificationState state;

    QVERIFY(state.setModified(true));
    QVERIFY(state.isModified());
    QVERIFY(state.hasChangesAfterAutoSave());

    state.markAutoSaveStarted();
    QVERIFY(!state.hasChangesAfterAutoSave());

    QVERIFY(!state.setModified(true));
    QVERIFY(state.hasChangesAfterAutoSave());

    state.restoreAutoSaveRequirement();
    QVERIFY(state.hasChangesAfterAutoSave());
}

void KisDocumentModificationStateTest::tracksChangesDuringSave()
{
    ModificationState state;
    state.setModified(true);

    state.markSavingStarted();
    QVERIFY(!state.wasModifiedWhileSaving());

    QVERIFY(!state.setModified(true));
    QVERIFY(state.wasModifiedWhileSaving());

    state.markImageModifiedWithoutUndo();
    QVERIFY(state.imageModifiedWithoutUndo());

    state.clearImageModifiedWithoutUndo();
    QVERIFY(!state.imageModifiedWithoutUndo());

    state.markImageModifiedWithoutUndo();
    QVERIFY(state.setModified(false));
    QVERIFY(!state.isModified());
    QVERIFY(!state.wasModifiedWhileSaving());
    QVERIFY(!state.imageModifiedWithoutUndo());
}

void KisDocumentModificationStateTest::snapshotsDocumentStateWithoutOperationProgress()
{
    ModificationState state;
    state.markImageModifiedWithoutUndo();
    state.setModified(true);

    const ModificationState snapshot = state.snapshotState();

    QVERIFY(snapshot.isModified());
    QVERIFY(snapshot.imageModifiedWithoutUndo());
    QVERIFY(!snapshot.hasChangesAfterAutoSave());
    QVERIFY(!snapshot.wasModifiedWhileSaving());
}

QTEST_GUILESS_MAIN(KisDocumentModificationStateTest)

#include "kis_document_modification_state_test.moc"
