/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_IMPORT_EXPORT_FILTER_P_H
#define KIS_IMPORT_EXPORT_FILTER_P_H

#include "KisImportExportFilter.h"

class Q_DECL_HIDDEN KisImportExportFilter::Private
{
public:
    Private();
    ~Private();

    QPointer<KoUpdater> updater;
    QByteArray mime;
    QString filename;
    QString realFilename;
    bool batchmode;
    KisImportUserFeedbackInterface *importUserFeedBackInterface{nullptr};
    QMap<QString, KisExportCheckBase *> capabilities;
};

#endif // KIS_IMPORT_EXPORT_FILTER_P_H
