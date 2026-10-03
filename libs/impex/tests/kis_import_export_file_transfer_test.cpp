/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <KisImportExportFileTransfer.h>

using Krita::ImportExport::KisImportExportFileTransfer;

namespace
{

void writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(contents), contents.size());
}

}

class KisImportExportFileTransferTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void replacesExistingFileContentsInPlace();
    void preservesTargetWhenSourceCannotBeOpened();
};

void KisImportExportFileTransferTest::replacesExistingFileContentsInPlace()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString sourcePath = directory.filePath(QStringLiteral("completed.kra"));
    const QString targetPath = directory.filePath(QStringLiteral("selected.kra"));
    writeFile(sourcePath, QByteArrayLiteral("complete document bytes"));
    writeFile(targetPath, QByteArrayLiteral("picker placeholder"));

    QFile existingTarget(targetPath);
    QVERIFY(existingTarget.open(QIODevice::ReadOnly));

    const auto result = KisImportExportFileTransfer::replaceFromLocalFile(sourcePath, targetPath);
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QCOMPARE(result.error, QFileDevice::NoError);

    QVERIFY(existingTarget.seek(0));
    QCOMPARE(existingTarget.readAll(), QByteArrayLiteral("complete document bytes"));
    QVERIFY(QFileInfo::exists(sourcePath));
}

void KisImportExportFileTransferTest::preservesTargetWhenSourceCannotBeOpened()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString sourcePath = directory.filePath(QStringLiteral("missing.kra"));
    const QString targetPath = directory.filePath(QStringLiteral("selected.kra"));
    writeFile(targetPath, QByteArrayLiteral("picker placeholder"));

    const auto result = KisImportExportFileTransfer::replaceFromLocalFile(sourcePath, targetPath);
    QVERIFY(!result.success);
    QVERIFY(result.error != QFileDevice::NoError);
    QVERIFY(!result.errorMessage.isEmpty());

    QFile target(targetPath);
    QVERIFY(target.open(QIODevice::ReadOnly));
    QCOMPARE(target.readAll(), QByteArrayLiteral("picker placeholder"));
}

QTEST_MAIN(KisImportExportFileTransferTest)

#include "kis_import_export_file_transfer_test.moc"
