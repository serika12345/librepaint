/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_IMPORT_EXPORT_FILTER_REGISTRY_H
#define KIS_IMPORT_EXPORT_FILTER_REGISTRY_H

#include <QJsonObject>
#include <QList>
#include <QStringList>

#include <kritaimpex_export.h>

class KisImportExportFilter;

/**
 * Discovers import/export plugins and selects their supported MIME types.
 *
 * This registry owns format selection independently of document lifetime and
 * user interaction. The caller owns every filter returned by createFilter().
 */
class KRITAIMPEX_EXPORT KisImportExportFilterRegistry
{
public:
    enum Direction {
        Import,
        Export
    };

    static QStringList supportedMimeTypes(Direction direction);
    static KisImportExportFilter *createFilter(const QString &mimeType, Direction direction);

    static QStringList mimeTypesFromMetadata(const QList<QJsonObject> &metadata,
                                             Direction direction);
};

#endif
