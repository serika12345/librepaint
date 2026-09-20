/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisBackup.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

namespace
{

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(contents) == contents.size();
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

} // namespace

class KisBackupTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultBackupReplacesExistingFile();
    void simpleBackupUsesRequestedLocation();
    void numberedBackupRotatesAndLimitsHistory();
};

void KisBackupTest::defaultBackupReplacesExistingFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString source = directory.filePath(QStringLiteral("document.kra"));
    const QString backup = source + QLatin1Char('~');
    QVERIFY(writeFile(source, QByteArrayLiteral("current document")));
    QVERIFY(writeFile(backup, QByteArrayLiteral("obsolete backup")));

    QVERIFY(KisBackup::backupFile(source));

    QCOMPARE(readFile(source), QByteArrayLiteral("current document"));
    QCOMPARE(readFile(backup), QByteArrayLiteral("current document"));
}

void KisBackupTest::simpleBackupUsesRequestedLocation()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString source = directory.filePath(QStringLiteral("document.kra"));
    const QString backupDirectory = directory.filePath(QStringLiteral("backups"));
    QVERIFY(QDir().mkpath(backupDirectory));
    QVERIFY(writeFile(source, QByteArrayLiteral("document data")));

    QVERIFY(KisBackup::simpleBackupFile(source, backupDirectory, QStringLiteral(".backup")));

    const QString backup = QDir(backupDirectory).filePath(QStringLiteral("document.kra.backup"));
    QCOMPARE(readFile(backup), QByteArrayLiteral("document data"));
}

void KisBackupTest::numberedBackupRotatesAndLimitsHistory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString source = directory.filePath(QStringLiteral("document.kra"));
    const QString backupDirectory = directory.filePath(QStringLiteral("backups"));
    QVERIFY(QDir().mkpath(backupDirectory));

    const auto backupPath = [&](int number) {
        return QDir(backupDirectory).filePath(QStringLiteral("document.kra.%1.backup").arg(number));
    };
    const auto createBackup = [&](const QByteArray &contents) {
        return writeFile(source, contents)
            && KisBackup::numberedBackupFile(source, backupDirectory, QStringLiteral(".backup"), 3);
    };

    QVERIFY(createBackup(QByteArrayLiteral("version 1")));
    QVERIFY(createBackup(QByteArrayLiteral("version 2")));
    QVERIFY(createBackup(QByteArrayLiteral("version 3")));
    QVERIFY(writeFile(backupPath(9), QByteArrayLiteral("stale history")));
    QVERIFY(createBackup(QByteArrayLiteral("version 4")));

    QCOMPARE(readFile(backupPath(1)), QByteArrayLiteral("version 4"));
    QCOMPARE(readFile(backupPath(2)), QByteArrayLiteral("version 3"));
    QCOMPARE(readFile(backupPath(3)), QByteArrayLiteral("version 2"));
    QVERIFY(!QFileInfo::exists(backupPath(4)));
    QVERIFY(!QFileInfo::exists(backupPath(9)));
}

QTEST_GUILESS_MAIN(KisBackupTest)

#include "KisBackupTest.moc"
