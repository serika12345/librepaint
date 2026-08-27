/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilterRegistry.h"

#include <QJsonValue>
#include <QSet>

#include <KoJsonTrader.h>

namespace
{
QStringList &mimeTypeCache(KisImportExportFilterRegistry::Direction direction)
{
    static QStringList importMimeTypes;
    static QStringList exportMimeTypes;
    return direction == KisImportExportFilterRegistry::Export ? exportMimeTypes : importMimeTypes;
}
} // namespace

QStringList KisImportExportFilterRegistry::mimeTypesFromMetadata(const QList<QJsonObject> &metadata,
                                                                 Direction direction)
{
    QSet<QString> mimeTypes;
    const QString key = direction == Export ? QStringLiteral("X-KDE-Export") : QStringLiteral("X-KDE-Import");

    for (const QJsonObject &entry : metadata) {
        const QStringList formats = entry.value(key).toString().split(',', Qt::SkipEmptyParts);
        for (const QString &format : formats) {
            mimeTypes.insert(format.trimmed());
        }
    }

    QStringList result(mimeTypes.begin(), mimeTypes.end());
    result.sort();
    return result;
}

QStringList KisImportExportFilterRegistry::supportedMimeTypes(Direction direction)
{
    QStringList &cachedMimeTypes = mimeTypeCache(direction);
    if (!cachedMimeTypes.isEmpty()) {
        return cachedMimeTypes;
    }

    QList<QJsonObject> metadata;
    const QList<KoJsonTrader::Plugin> plugins =
        KoJsonTrader::instance()->query(QStringLiteral("Krita/FileFilter"), QString());

    metadata.reserve(plugins.size());

    for (const KoJsonTrader::Plugin &plugin : plugins) {
        metadata.append(plugin.metaData().value(QStringLiteral("MetaData")).toObject());
    }

    cachedMimeTypes = mimeTypesFromMetadata(metadata, direction);
    return cachedMimeTypes;
}
