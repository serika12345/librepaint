/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_identity.h"

namespace Krita
{
namespace Document
{

QString Identity::path() const
{
    return m_path;
}

bool Identity::setPath(const QString &path)
{
    const bool changed = path != m_path;
    m_path = path;
    return changed;
}

QString Identity::localFilePath() const
{
    return m_localFilePath;
}

void Identity::setLocalFilePath(const QString &localFilePath)
{
    m_localFilePath = localFilePath;
}

QByteArray Identity::mimeType() const
{
    return m_mimeType;
}

void Identity::setMimeType(const QByteArray &mimeType)
{
    m_mimeType = mimeType;
}

void Identity::clearMimeType()
{
    m_mimeType.clear();
}

bool Identity::mimeTypeWasAutoDetected() const
{
    return m_mimeTypeWasAutoDetected;
}

void Identity::setMimeTypeWasAutoDetected(bool autoDetected)
{
    m_mimeTypeWasAutoDetected = autoDetected;
}

}
}
