/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEUSEROPERATIONSIMPORTSOURCE_P_H
#define KISRESOURCEUSEROPERATIONSIMPORTSOURCE_P_H

#include <KoResource.h>

class QString;
class QWidget;

namespace KisResourceUserOperationsImportSource
{
struct ImportAttempt
{
    KoResourceSP resource;
    bool wouldOverwrite = false;
};

ImportAttempt importResourceFile(const QString &resourceType,
                                 const QString &resourceFilepath,
                                 const QString &storageLocation,
                                 bool overwrite);
void warnImportFailed(QWidget *widgetParent);
}

#endif // KISRESOURCEUSEROPERATIONSIMPORTSOURCE_P_H
