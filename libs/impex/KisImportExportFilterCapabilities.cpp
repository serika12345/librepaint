/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilter_p.h"

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
