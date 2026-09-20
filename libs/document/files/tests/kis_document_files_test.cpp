/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <KoStore.h>

#include <files/kis_document_autosave_files.h>
#include <files/kis_document_backup_file.h>
#include <files/kis_document_save_target.h>

#include <memory>

using Krita::Document::KisDocumentAutoSaveFile;
using Krita::Document::KisDocumentAutoSaveFiles;
using Krita::Document::KisDocumentBackupFile;
using Krita::Document::KisDocumentBackupLocation;
using Krita::Document::KisDocumentBackupOptions;
using Krita::Document::KisDocumentBackupResult;
using Krita::Document::KisDocumentBackupStatus;
using Krita::Document::KisDocumentSaveTarget;

namespace
{

void writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(contents), contents.size());
}

void writeRecoveryArchive(const QString &path, const QByteArray &preview)
{
    std::unique_ptr<KoStore> store(
        KoStore::createStore(path, KoStore::Write, {}, KoStore::Zip, false));
    QVERIFY(store);
    QVERIFY(!store->bad());
    QVERIFY(store->open(QStringLiteral("Thumbnails/thumbnail.png")));
    QCOMPARE(store->write(preview), qint64(preview.size()));
    QVERIFY(store->close());
    QVERIFY(store->finalize());
}

}

class KisDocumentFilesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void classifiesSaveTargets();
    void backupOptionsHaveStableDefaults();
    void preservesSimpleAndNumberedBackups();
    void reportsBackupFailuresAndTemporaryLocations();
    void generatesAdjacentAndFallbackAutoSaveNames();
    void usesDefaultAutoSaveLocations();
    void discoversAndRemovesRecoveryFiles();
    void removesEveryAutoSaveForADocument();
};

void KisDocumentFilesTest::classifiesSaveTargets()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path = directory.filePath(QStringLiteral("painting.kra"));
    const KisDocumentSaveTarget missing = KisDocumentSaveTarget::inspect(path);
    QVERIFY(!missing.exists);

    writeFile(path, QByteArrayLiteral("document"));
    const KisDocumentSaveTarget writable = KisDocumentSaveTarget::inspect(path);
    QVERIFY(writable.exists);
    QVERIFY(writable.writable);

#ifndef Q_OS_ANDROID
    QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner));
    const KisDocumentSaveTarget readOnly = KisDocumentSaveTarget::inspect(path);
    QVERIFY(readOnly.exists);
    QVERIFY(!readOnly.writable);
#endif
}

void KisDocumentFilesTest::backupOptionsHaveStableDefaults()
{
    const KisDocumentBackupOptions options;
    QVERIFY(!options.enabled);
    QCOMPARE(options.location, KisDocumentBackupLocation::Adjacent);
    QCOMPARE(options.numberOfBackups, 1);
    QCOMPARE(options.suffix, QStringLiteral("~"));
    QVERIFY(!options.forceTemporaryDirectory);

    QVERIFY(KisDocumentBackupLocation::Adjacent != KisDocumentBackupLocation::Home);
    QVERIFY(KisDocumentBackupLocation::Home != KisDocumentBackupLocation::Temporary);

    const KisDocumentBackupResult result;
    QCOMPARE(result.status, KisDocumentBackupStatus::NotNeeded);
    QVERIFY(result.directory.isEmpty());
}

void KisDocumentFilesTest::preservesSimpleAndNumberedBackups()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path = directory.filePath(QStringLiteral("painting.kra"));
    writeFile(path, QByteArrayLiteral("first"));

    KisDocumentBackupOptions options;
    options.enabled = true;
    options.location = KisDocumentBackupLocation::Adjacent;
    options.numberOfBackups = 1;
    options.suffix = QStringLiteral("~");

    const KisDocumentBackupResult simpleBackup =
        KisDocumentBackupFile::create(path, options, directory.path());
    QCOMPARE(simpleBackup.status, KisDocumentBackupStatus::Created);
    QVERIFY(simpleBackup.directory.isEmpty());
    QVERIFY(QFileInfo::exists(path + QStringLiteral("~")));

    options.numberOfBackups = 2;
    QVERIFY(QFile::remove(path + QStringLiteral("~")));
    QCOMPARE(KisDocumentBackupFile::create(path, options, directory.path()).status,
             KisDocumentBackupStatus::Created);
    writeFile(path, QByteArrayLiteral("second"));
    QCOMPARE(KisDocumentBackupFile::create(path, options, directory.path()).status,
             KisDocumentBackupStatus::Created);

    QFile newest(path + QStringLiteral(".1~"));
    QVERIFY(newest.open(QIODevice::ReadOnly));
    QCOMPARE(newest.readAll(), QByteArrayLiteral("second"));

    QFile previous(path + QStringLiteral(".2~"));
    QVERIFY(previous.open(QIODevice::ReadOnly));
    QCOMPARE(previous.readAll(), QByteArrayLiteral("first"));
}

void KisDocumentFilesTest::reportsBackupFailuresAndTemporaryLocations()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString uniqueName = QFileInfo(directory.path()).fileName() + QStringLiteral(".kra");
    const QString path = directory.filePath(uniqueName);
    writeFile(path, QByteArrayLiteral("document"));

    KisDocumentBackupOptions options;
    options.enabled = true;
    options.suffix = QStringLiteral("/missing/backup");

    const KisDocumentBackupResult failed =
        KisDocumentBackupFile::create(path, options, directory.path());
    QCOMPARE(failed.status, KisDocumentBackupStatus::Failed);
    QVERIFY(failed.directory.isEmpty());

    options.location = KisDocumentBackupLocation::Temporary;
    options.suffix = QStringLiteral("~");
    const KisDocumentBackupResult temporary =
        KisDocumentBackupFile::create(path, options, directory.path());
    QCOMPARE(temporary.status, KisDocumentBackupStatus::Created);
    QCOMPARE(temporary.directory,
             QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    const QString temporaryBackup =
        QDir(temporary.directory).filePath(uniqueName + options.suffix);
    QVERIFY(QFileInfo::exists(temporaryBackup));
    QVERIFY(QFile::remove(temporaryBackup));

    options.location = KisDocumentBackupLocation::Adjacent;
    options.forceTemporaryDirectory = true;
    const KisDocumentBackupResult forcedTemporary =
        KisDocumentBackupFile::create(path, options, directory.path());
    QCOMPARE(forcedTemporary.status, KisDocumentBackupStatus::Created);
    QCOMPARE(forcedTemporary.directory, QDir::tempPath());
    const QString forcedBackup =
        QDir(forcedTemporary.directory).filePath(uniqueName + options.suffix);
    QVERIFY(QFileInfo::exists(forcedBackup));
    QVERIFY(QFile::remove(forcedBackup));
}

void KisDocumentFilesTest::generatesAdjacentAndFallbackAutoSaveNames()
{
    QTemporaryDir directory;
    QTemporaryDir recoveryDirectory;
    QVERIFY(directory.isValid());
    QVERIFY(recoveryDirectory.isValid());

    const QString path = directory.filePath(QStringLiteral("1-file.kra"));
    writeFile(path, QByteArrayLiteral("document"));

    QCOMPARE(KisDocumentAutoSaveFiles::filePath(path,
                                                recoveryDirectory.path(),
                                                42,
                                                QStringLiteral("document7"),
                                                true),
             directory.filePath(QStringLiteral(".1-file.kra-autosave.kra")));
    QCOMPARE(KisDocumentAutoSaveFiles::filePath(QString(),
                                                recoveryDirectory.path(),
                                                42,
                                                QStringLiteral("document7"),
                                                false),
             recoveryDirectory.filePath(QStringLiteral("krita-42-document7-autosave.kra")));
}

void KisDocumentFilesTest::usesDefaultAutoSaveLocations()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString recoveryDirectory = KisDocumentAutoSaveFiles::directory();
    QVERIFY(!recoveryDirectory.isEmpty());
    QVERIFY(QFileInfo(recoveryDirectory).isAbsolute());

    const QString path = directory.filePath(QStringLiteral("painting.kra"));
    writeFile(path, QByteArrayLiteral("document"));
    const QString autoSavePath = KisDocumentAutoSaveFiles::filePath(
        path, QStringLiteral("publicApiContractDocument"), true);
    QCOMPARE(autoSavePath,
             directory.filePath(QStringLiteral(".painting.kra-autosave.kra")));

    writeFile(autoSavePath, QByteArrayLiteral("autosave"));
    KisDocumentAutoSaveFiles::removeForDocument(
        path, false, QStringLiteral("publicApiContractDocument"), true);
    QVERIFY(!QFileInfo::exists(autoSavePath));
}

void KisDocumentFilesTest::discoversAndRemovesRecoveryFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString hidden = directory.filePath(QStringLiteral(".krita-1-first-autosave.kra"));
    const QString visible = directory.filePath(QStringLiteral("krita-2-second-autosave.kra"));
    const QByteArray preview = QByteArrayLiteral("preview-bytes");
    writeRecoveryArchive(hidden, preview);
    writeFile(visible, QByteArrayLiteral("visible"));
    const QString empty = directory.filePath(QStringLiteral("empty.kra"));
    writeFile(empty, {});
    writeFile(directory.filePath(QStringLiteral("unrelated.kra")), QByteArrayLiteral("other"));

    QVERIFY(KisDocumentAutoSaveFiles::isUsable(hidden));
    QVERIFY(!KisDocumentAutoSaveFiles::isUsable(empty));

    const QList<KisDocumentAutoSaveFile> files =
        KisDocumentAutoSaveFiles::recoverableFiles(directory.path());
    QCOMPARE(files.size(), 2);

    QStringList names;
    for (const KisDocumentAutoSaveFile &file : files) {
        names.append(file.fileName);
        QVERIFY(!file.path.isEmpty());
        QVERIFY(file.lastModified.isValid());
        if (file.fileName == QFileInfo(hidden).fileName()) {
            QCOMPARE(file.previewData, preview);
        } else {
            QVERIFY(file.previewData.isEmpty());
        }
    }
    QVERIFY(names.contains(QFileInfo(hidden).fileName()));
    QVERIFY(names.contains(QFileInfo(visible).fileName()));

    QVERIFY(KisDocumentAutoSaveFiles::remove(hidden));
    QVERIFY(!QFileInfo::exists(hidden));
    QVERIFY(KisDocumentAutoSaveFiles::remove(hidden));
}

void KisDocumentFilesTest::removesEveryAutoSaveForADocument()
{
    QTemporaryDir directory;
    QTemporaryDir recoveryDirectory;
    QVERIFY(directory.isValid());
    QVERIFY(recoveryDirectory.isValid());

    const QString path = directory.filePath(QStringLiteral("painting.kra"));
    writeFile(path, QByteArrayLiteral("document"));

    const QString adjacent = KisDocumentAutoSaveFiles::filePath(path,
                                                                recoveryDirectory.path(),
                                                                42,
                                                                QStringLiteral("document7"),
                                                                true);
    const QString fallback = KisDocumentAutoSaveFiles::filePath(QString(),
                                                                recoveryDirectory.path(),
                                                                42,
                                                                QStringLiteral("document7"),
                                                                true);
    writeFile(adjacent, QByteArrayLiteral("adjacent"));
    writeFile(fallback, QByteArrayLiteral("fallback"));

    KisDocumentAutoSaveFiles::removeForDocument(path,
                                                false,
                                                recoveryDirectory.path(),
                                                42,
                                                QStringLiteral("document7"),
                                                true);
    QVERIFY(!QFileInfo::exists(adjacent));
    QVERIFY(!QFileInfo::exists(fallback));

    writeFile(adjacent, QByteArrayLiteral("recovered"));
    KisDocumentAutoSaveFiles::removeForDocument(adjacent,
                                                true,
                                                recoveryDirectory.path(),
                                                42,
                                                QStringLiteral("document7"),
                                                true);
    QVERIFY(!QFileInfo::exists(adjacent));
}

QTEST_GUILESS_MAIN(KisDocumentFilesTest)

#include "kis_document_files_test.moc"
