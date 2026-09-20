/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCETHUMBNAILSTORAGELOCATION_H
#define KISRESOURCETHUMBNAILSTORAGELOCATION_H

#include <QString>

class KisResourceThumbnailStorageLocation
{
public:
    static QString makeAbsolute(const QString &storageLocation);
};

#endif // KISRESOURCETHUMBNAILSTORAGELOCATION_H
