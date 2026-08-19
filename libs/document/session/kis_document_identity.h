/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_IDENTITY_H
#define KIS_DOCUMENT_IDENTITY_H

#include <QByteArray>
#include <QString>

#include <kritadocument_export.h>

namespace Krita
{
namespace Document
{

/**
 * Stores the file and format identity of one document session.
 *
 * The display path may be an operating-system URI while localFilePath() is
 * the file passed to import and export implementations. Format detection is
 * tracked independently so a caller can discard a previous inferred format
 * before opening another document.
 */
class KRITADOCUMENT_EXPORT Identity
{
public:
    QString path() const;
    bool setPath(const QString &path);

    QString localFilePath() const;
    void setLocalFilePath(const QString &localFilePath);
    void resetPaths();

    QByteArray mimeType() const;
    void setMimeType(const QByteArray &mimeType);
    void clearMimeType();

    bool mimeTypeWasAutoDetected() const;
    void setMimeTypeWasAutoDetected(bool autoDetected);

private:
    QString m_path;
    QString m_localFilePath;
    QByteArray m_mimeType;
    bool m_mimeTypeWasAutoDetected = false;
};

}
}

#endif
