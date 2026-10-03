/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisExportCheckBase.h"
#include "KisImportExportFilter.h"
#include "KisImportExportFilter_p.h"
#include <qalgorithms.h>
#include <qmap.h>
#include <qobject.h>

QMap<QString, KisExportCheckBase *> KisImportExportFilter::exportChecks()
{
    qDeleteAll(d->capabilities);
    initializeCapabilities();
    return d->capabilities;
}

void KisImportExportFilter::initializeCapabilities()
{
}

void KisImportExportFilter::addCapability(KisExportCheckBase *capability)
{
    d->capabilities[capability->id()] = capability;
}
