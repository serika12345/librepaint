/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilterRegistry.h"

#include <QJsonValue>

#include <KPluginFactory>
#include <KoJsonTrader.h>
#include <kis_assert.h>
#include <kis_debug.h>

#include "KisImportExportFilter.h"

KisImportExportFilter *KisImportExportFilterRegistry::createFilter(const QString &mimeType, Direction direction)
{
    int selectedWeight = -1;
    KisImportExportFilter *selectedFilter = nullptr;
    const QString key = direction == Export ? QStringLiteral("X-KDE-Export") : QStringLiteral("X-KDE-Import");
    const QList<KoJsonTrader::Plugin> plugins =
        KoJsonTrader::instance()->query(QStringLiteral("Krita/FileFilter"), QString());

    for (const KoJsonTrader::Plugin &plugin : plugins) {
        const QJsonObject metadata = plugin.metaData().value(QStringLiteral("MetaData")).toObject();
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
