/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilterRegistry.h"

#include <QJsonValue>
#include <QSet>

#include <KPluginFactory>
#include <KoJsonTrader.h>
#include <kis_assert.h>
#include <kis_debug.h>

#include "KisImportExportFilter.h"

namespace
{
QString metadataKey(KisImportExportFilterRegistry::Direction direction)
{
    return direction == KisImportExportFilterRegistry::Export
        ? QStringLiteral("X-KDE-Export")
        : QStringLiteral("X-KDE-Import");
}

QStringList &mimeTypeCache(KisImportExportFilterRegistry::Direction direction)
{
    static QStringList importMimeTypes;
    static QStringList exportMimeTypes;
    return direction == KisImportExportFilterRegistry::Export
        ? exportMimeTypes
        : importMimeTypes;
}
}

QStringList KisImportExportFilterRegistry::mimeTypesFromMetadata(
    const QList<QJsonObject> &metadata,
    Direction direction)
{
    QSet<QString> mimeTypes;
    const QString key = metadataKey(direction);

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

KisImportExportFilter *KisImportExportFilterRegistry::createFilter(
    const QString &mimeType,
    Direction direction)
{
    int selectedWeight = -1;
    KisImportExportFilter *selectedFilter = nullptr;
    const QString key = metadataKey(direction);
    const QList<KoJsonTrader::Plugin> plugins =
        KoJsonTrader::instance()->query(QStringLiteral("Krita/FileFilter"), QString());

    for (const KoJsonTrader::Plugin &plugin : plugins) {
        const QJsonObject metadata =
            plugin.metaData().value(QStringLiteral("MetaData")).toObject();
        const QStringList formats = metadata.value(key).toString().split(',', Qt::SkipEmptyParts);
        if (!formats.contains(mimeType)) {
            continue;
        }

        KPluginFactory *factory = qobject_cast<KPluginFactory *>(plugin.instance());
        if (!factory) {
            warnPlugins << plugin.errorString();
            continue;
        }

        QObject *object = factory->create<KisImportExportFilter>(nullptr);
        KisImportExportFilter *candidate = qobject_cast<KisImportExportFilter *>(object);
        if (!candidate) {
            delete object;
            continue;
        }

        KIS_ASSERT_RECOVER_NOOP(metadata.value(QStringLiteral("X-KDE-Weight")).isDouble());
        const int weight = metadata.value(QStringLiteral("X-KDE-Weight")).toInt();
        if (weight > selectedWeight) {
            delete selectedFilter;
            selectedFilter = candidate;
            selectedFilter->setObjectName(plugin.fileName());
            selectedWeight = weight;
        } else {
            delete candidate;
        }
    }

    if (selectedFilter) {
        selectedFilter->setMimeType(mimeType);
    }
    return selectedFilter;
}
