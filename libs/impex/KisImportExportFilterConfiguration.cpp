/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilter.h"
#include "kis_types.h"
#include <qobject.h>
#include <QtGlobal>

KisPropertiesConfigurationSP KisImportExportFilter::defaultConfiguration(const QByteArray &from,
                                                                         const QByteArray &to) const
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    return 0;
}

KisConfigWidget *
KisImportExportFilter::createConfigurationWidget(QWidget *, const QByteArray &from, const QByteArray &to) const
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    return 0;
}
