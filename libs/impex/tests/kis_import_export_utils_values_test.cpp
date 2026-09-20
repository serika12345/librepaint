/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisImportExportUtils.h>

class KisImportExportUtilsValuesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicEnumValuesRemainStable();
    void defaultExportJobIsInvalid();
    void configuredExportJobPreservesValues();
};

void KisImportExportUtilsValuesTest::publicEnumValuesRemainStable()
{
    QCOMPARE(int(KritaUtils::SaveNone), 0);
    QCOMPARE(int(KritaUtils::SaveShowWarnings), 0x1);
    QCOMPARE(int(KritaUtils::SaveIsExporting), 0x2);
    QCOMPARE(int(KritaUtils::SaveInAutosaveMode), 0x4);

    QCOMPARE(int(KritaUtils::Success), 0);
    QCOMPARE(int(KritaUtils::Failure), 1);
    QCOMPARE(int(KritaUtils::AnotherSavingInProgress), 2);
    QCOMPARE(int(KritaUtils::ImageLockFailure), 3);
    QCOMPARE(int(KritaUtils::Cancelled), 4);

    const KritaUtils::SaveFlags flags =
        KritaUtils::SaveShowWarnings | KritaUtils::SaveIsExporting | KritaUtils::SaveInAutosaveMode;
    QVERIFY(flags.testFlag(KritaUtils::SaveShowWarnings));
    QVERIFY(flags.testFlag(KritaUtils::SaveIsExporting));
    QVERIFY(flags.testFlag(KritaUtils::SaveInAutosaveMode));
}

void KisImportExportUtilsValuesTest::defaultExportJobIsInvalid()
{
    const KritaUtils::ExportFileJob job;

    QVERIFY(job.filePath.isEmpty());
    QVERIFY(job.mimeType.isEmpty());
    QCOMPARE(job.flags, KritaUtils::SaveFlags(KritaUtils::SaveNone));
    QVERIFY(!job.isValid());
}

void KisImportExportUtilsValuesTest::configuredExportJobPreservesValues()
{
    const KritaUtils::SaveFlags flags = KritaUtils::SaveShowWarnings | KritaUtils::SaveIsExporting;
    const KritaUtils::ExportFileJob job(QStringLiteral("/tmp/contract.png"), QByteArray("image/png"), flags);

    QCOMPARE(job.filePath, QStringLiteral("/tmp/contract.png"));
    QCOMPARE(job.mimeType, QByteArray("image/png"));
    QCOMPARE(job.flags, flags);
    QVERIFY(job.isValid());

    const KritaUtils::ExportFileJob defaultFlagsJob(QStringLiteral("contract.kra"), QByteArray("application/x-krita"));
    QCOMPARE(defaultFlagsJob.flags, KritaUtils::SaveFlags(KritaUtils::SaveNone));
    QVERIFY(defaultFlagsJob.isValid());
}

QTEST_GUILESS_MAIN(KisImportExportUtilsValuesTest)

#include "kis_import_export_utils_values_test.moc"
