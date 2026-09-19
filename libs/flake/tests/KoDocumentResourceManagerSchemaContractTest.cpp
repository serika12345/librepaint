/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDocumentResourceManager.h>

#include <QTest>


namespace
{
} // namespace

class KoDocumentResourceManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentResourceKeySchemaRemainsStable();
};

void KoDocumentResourceManagerSchemaContractTest::documentResourceKeySchemaRemainsStable()
{
    QCOMPARE(int(KoDocumentResourceManager::UndoStack), 0);
    QCOMPARE(int(KoDocumentResourceManager::OdfDocument), 1);
    QCOMPARE(int(KoDocumentResourceManager::GrabSensitivity), 2);
    QCOMPARE(int(KoDocumentResourceManager::MarkerCollection), 3);
    QCOMPARE(int(KoDocumentResourceManager::DocumentResolution), 4);
    QCOMPARE(int(KoDocumentResourceManager::DocumentRectInPixels), 5);
    QCOMPARE(int(KoDocumentResourceManager::KarbonStart), 1000);
    QCOMPARE(int(KoDocumentResourceManager::KexiStart), 2000);
    QCOMPARE(int(KoDocumentResourceManager::FlowStart), 3000);
    QCOMPARE(int(KoDocumentResourceManager::PlanStart), 4000);
    QCOMPARE(int(KoDocumentResourceManager::StageStart), 5000);
    QCOMPARE(int(KoDocumentResourceManager::KritaStart), 6000);
    QCOMPARE(int(KoDocumentResourceManager::SheetsStart), 7000);
    QCOMPARE(int(KoDocumentResourceManager::WordsStart), 8000);
    QCOMPARE(int(KoDocumentResourceManager::KoPageAppStart), 9000);
    QCOMPARE(int(KoDocumentResourceManager::KoTextStart), 10000);
}

QTEST_APPLESS_MAIN(KoDocumentResourceManagerSchemaContractTest)

#include "KoDocumentResourceManagerSchemaContractTest.moc"
