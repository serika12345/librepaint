/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_autosave_files.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QStandardPaths>

#include <KoStore.h>
#include <KisUsageLogger.h>

namespace Krita::Document
{

namespace
{

bool hasAutoSaveName(const QString &fileName)
{
    static const QRegularExpression hiddenPattern(QStringLiteral("^\\..+-autosave.kra$"));
    static const QRegularExpression visiblePattern(QStringLiteral("^.+-autosave.kra$"));
    return hiddenPattern.match(fileName).hasMatch() || visiblePattern.match(fileName).hasMatch();
}

QByteArray readPreview(const QString &filePath)
{
    QScopedPointer<KoStore> store(KoStore::createStore(filePath, KoStore::Read));
    if (!store) {
        return {};
    }

    const QString thumbnailPath = QStringLiteral("Thumbnails/thumbnail.png");
    const QString previewPath = QStringLiteral("preview.png");
    const QString pathToUse = store->hasFile(thumbnailPath)
        ? thumbnailPath
        : (store->hasFile(previewPath) ? previewPath : QString());

    if (pathToUse.isEmpty() || !store->open(pathToUse)) {
        return {};
    }

    const QByteArray bytes = store->read(store->size());
    store->close();
    return bytes;
}

}

QString KisDocumentAutoSaveFiles::directory()
{
#if defined(Q_OS_WIN)
    return QDir::tempPath();
#elif defined(Q_OS_ANDROID)
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QStringLiteral("/krita-backup");
    QDir().mkpath(path);
    return path;
#elif defined(Q_OS_IOS)
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/krita-autosave");
    QDir().mkpath(path);
    return path;
#else
    return QDir::homePath();
#endif
}

QString KisDocumentAutoSaveFiles::filePath(const QString &documentPath,
                                           const QString &documentObjectName,
                                           bool hidden)
{
    return filePath(documentPath,
                    directory(),
                    QCoreApplication::applicationPid(),
                    documentObjectName,
                    hidden);
}

QString KisDocumentAutoSaveFiles::filePath(const QString &documentPath,
                                           const QString &recoveryDirectory,
                                           qint64 processId,
                                           const QString &documentObjectName,
                                           bool hidden)
{
    const QString prefix = hidden ? QStringLiteral(".") : QString();
    const QFileInfo fileInfo(documentPath);
    QString targetDirectory = fileInfo.absolutePath();

#ifdef Q_OS_ANDROID
    if (documentPath.startsWith(QLatin1String("content://"))) {
        targetDirectory = recoveryDirectory;
        QDir().mkpath(targetDirectory);
    }
#endif

    if (documentPath.isEmpty() || hasAutoSaveName(fileInfo.fileName()) || !fileInfo.isWritable()) {
        return QStringLiteral("%1/%2krita-%3-%4-autosave.kra")
            .arg(recoveryDirectory, prefix, QString::number(processId), documentObjectName);
    }

    return QStringLiteral("%1/%2%3-autosave.kra")
        .arg(targetDirectory, prefix, fileInfo.fileName());
}

QList<KisDocumentAutoSaveFile> KisDocumentAutoSaveFiles::recoverableFiles(const QString &recoveryDirectory)
{
    const QDir targetDirectory(recoveryDirectory.isEmpty() ? directory() : recoveryDirectory);
    QStringList fileNames = targetDirectory.entryList(
        {QStringLiteral(".krita-*-*-autosave.kra")}, QDir::Files | QDir::Hidden);
    fileNames += targetDirectory.entryList(
        {QStringLiteral("krita-*-*-autosave.kra")}, QDir::Files);

    QList<KisDocumentAutoSaveFile> result;
    result.reserve(fileNames.size());
    for (const QString &fileName : fileNames) {
        const QString path = targetDirectory.absoluteFilePath(fileName);
        result.append({fileName,
                       path,
                       QFileInfo(path).lastModified(),
                       readPreview(path)});
    }
    return result;
}

bool KisDocumentAutoSaveFiles::isUsable(const QString &filePath)
{
    const QFileInfo file(filePath);
    return file.isFile() && file.size() > 0;
}

bool KisDocumentAutoSaveFiles::remove(const QString &filePath)
{
    return !QFileInfo::exists(filePath) || QFile::remove(filePath);
}

void KisDocumentAutoSaveFiles::removeForDocument(const QString &autoSaveBaseName,
                                                 bool wasRecovered,
                                                 const QString &documentObjectName,
                                                 bool hidden)
{
    removeForDocument(autoSaveBaseName,
                      wasRecovered,
                      directory(),
                      QCoreApplication::applicationPid(),
                      documentObjectName,
                      hidden);
}

void KisDocumentAutoSaveFiles::removeForDocument(const QString &autoSaveBaseName,
                                                 bool wasRecovered,
                                                 const QString &recoveryDirectory,
                                                 qint64 processId,
                                                 const QString &documentObjectName,
                                                 bool hidden)
{
    const QString adjacent = filePath(autoSaveBaseName,
                                      recoveryDirectory,
                                      processId,
                                      documentObjectName,
                                      hidden);
    const QString fallback = filePath(QString(),
                                      recoveryDirectory,
                                      processId,
                                      documentObjectName,
                                      hidden);

    for (const QString &path : {adjacent, fallback}) {
        if (QFileInfo::exists(path)) {
            KisUsageLogger::log(QStringLiteral("Removing autosave file: %1").arg(path));
            QFile::remove(path);
        }
    }

    if (wasRecovered && !autoSaveBaseName.isEmpty()
        && hasAutoSaveName(QFileInfo(autoSaveBaseName).fileName())
        && QFileInfo::exists(autoSaveBaseName)) {
        KisUsageLogger::log(QStringLiteral("Removing autosave file: %1").arg(autoSaveBaseName));
        QFile::remove(autoSaveBaseName);
    }
}

}
