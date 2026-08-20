/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <files/kis_document_autosave_files.h>
#include <files/kis_document_backup_file.h>
#include <files/kis_document_save_target.h>

using Krita::Document::KisDocumentAutoSaveFile;
using Krita::Document::KisDocumentAutoSaveFiles;
using Krita::Document::KisDocumentBackupFile;
using Krita::Document::KisDocumentBackupLocation;
using Krita::Document::KisDocumentBackupOptions;
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

}

class KisDocumentFilesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void classifiesSaveTargets();
    void preservesSimpleAndNumberedBackups();
    void generatesAdjacentAndFallbackAutoSaveNames();
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

    QCOMPARE(KisDocumentBackupFile::create(path, options, directory.path()).status,
             KisDocumentBackupStatus::Created);
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

void KisDocumentFilesTest::discoversAndRemovesRecoveryFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString hidden = directory.filePath(QStringLiteral(".krita-1-first-autosave.kra"));
    const QString visible = directory.filePath(QStringLiteral("krita-2-second-autosave.kra"));
    writeFile(hidden, QByteArrayLiteral("hidden"));
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
    }
    QVERIFY(names.contains(QFileInfo(hidden).fileName()));
    QVERIFY(names.contains(QFileInfo(visible).fileName()));

    QVERIFY(KisDocumentAutoSaveFiles::remove(hidden));
    QVERIFY(!QFileInfo::exists(hidden));
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
