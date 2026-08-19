/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <session/kis_document_recovery_status.h>

using Krita::Document::RecoveryStatus;

class KisDocumentRecoveryStatusTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsAsAnOrdinaryDocument();
    void reportsOnlyRecoveryStatusTransitions();
    void copiesTheDocumentStatus();
};

void KisDocumentRecoveryStatusTest::startsAsAnOrdinaryDocument()
{
    const RecoveryStatus status;

    QVERIFY(!status.isRecovered());
}

void KisDocumentRecoveryStatusTest::reportsOnlyRecoveryStatusTransitions()
{
    RecoveryStatus status;

    QVERIFY(status.setRecovered(true));
    QVERIFY(status.isRecovered());
    QVERIFY(!status.setRecovered(true));

    QVERIFY(status.setRecovered(false));
    QVERIFY(!status.isRecovered());
    QVERIFY(!status.setRecovered(false));
}

void KisDocumentRecoveryStatusTest::copiesTheDocumentStatus()
{
    RecoveryStatus recoveredStatus;
    recoveredStatus.setRecovered(true);

    const RecoveryStatus copy = recoveredStatus;

    QVERIFY(copy.isRecovered());
}

QTEST_GUILESS_MAIN(KisDocumentRecoveryStatusTest)

#include "kis_document_recovery_status_test.moc"
