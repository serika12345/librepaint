/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilter.h"

#include <kis_image_config.h>

KisPropertiesConfigurationSP KisImportExportFilter::defaultConfiguration(const QByteArray &from,
                                                                         const QByteArray &to) const
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    return 0;
}

KisPropertiesConfigurationSP KisImportExportFilter::lastSavedConfiguration(const QByteArray &from,
                                                                           const QByteArray &to) const
{
    KisPropertiesConfigurationSP cfg = defaultConfiguration(from, to);
    const QString filterConfig = KisImageConfig(true).exportConfigurationXML(to);
    if (cfg && !filterConfig.isEmpty()) {
        cfg->fromXML(filterConfig, false);
    }
    return cfg;
}

KisConfigWidget *
KisImportExportFilter::createConfigurationWidget(QWidget *, const QByteArray &from, const QByteArray &to) const
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    return 0;
}
