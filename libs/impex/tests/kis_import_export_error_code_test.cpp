/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisImportExportErrorCode.h>

class KisImportExportErrorCodeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resultClassification();
};

void KisImportExportErrorCodeTest::resultClassification()
{
    const KisImportExportErrorCode success(ImportExportCodes::OK);
    const KisImportExportErrorCode cancellation(ImportExportCodes::Cancelled);
    const KisImportExportErrorCode failure(ImportExportCodes::ErrorWhileWriting);

    QVERIFY(success.isOk());
    QVERIFY(!success.isCancelled());
    QVERIFY(!failure.isOk());
    QVERIFY(!failure.isCancelled());
    QVERIFY(cancellation.isCancelled());
}

QTEST_GUILESS_MAIN(KisImportExportErrorCodeTest)

#include "kis_import_export_error_code_test.moc"
