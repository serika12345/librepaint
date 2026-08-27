/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <KisImportExportAdditionalChecks.h>
#include <KisImportExportFilterRegistry.h>
#include <KisImportExportMimeType.h>
#include <kis_store_paintdevice_writer.h>

class TestImportExportBoundary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filePreconditions();
    void mimeSelectionUsesTheRequestedDirection();
};

void TestImportExportBoundary::filePreconditions()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path = directory.filePath(QStringLiteral("input.bin"));
    QVERIFY(!KisImportExportAdditionalChecks::doesFileExist(path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("data"), qint64(4));
    file.close();

    QVERIFY(KisImportExportAdditionalChecks::doesFileExist(path));
    QVERIFY(KisImportExportAdditionalChecks::isFileReadable(path));
    QVERIFY(KisImportExportAdditionalChecks::isFileWritable(path));
}

void TestImportExportBoundary::mimeSelectionUsesTheRequestedDirection()
{
    QList<QJsonObject> metadata;
    metadata.append({
        {QStringLiteral("X-KDE-Import"), QStringLiteral("image/png,image/webp")},
        {QStringLiteral("X-KDE-Export"), QStringLiteral("image/png,image/avif")},
    });
    metadata.append({
        {QStringLiteral("X-KDE-Import"), QStringLiteral("image/png,image/tiff")},
        {QStringLiteral("X-KDE-Export"), QStringLiteral("image/tiff")},
    });

    QCOMPARE(KisImportExportFilterRegistry::mimeTypesFromMetadata(metadata, KisImportExportFilterRegistry::Import),
             QStringList({QStringLiteral("image/png"), QStringLiteral("image/tiff"), QStringLiteral("image/webp")}));
    QCOMPARE(KisImportExportFilterRegistry::mimeTypesFromMetadata(metadata, KisImportExportFilterRegistry::Export),
             QStringList({QStringLiteral("image/avif"), QStringLiteral("image/png"), QStringLiteral("image/tiff")}));
}

QTEST_MAIN(TestImportExportBoundary)

#include "TestImportExportBoundary.moc"
