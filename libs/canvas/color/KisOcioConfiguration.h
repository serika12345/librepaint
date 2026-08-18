/*
 * SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_OCIO_CONFIGURATION_H
#define KIS_OCIO_CONFIGURATION_H

#include <QString>

#include "kritacanvas_export.h"

class KRITACANVAS_EXPORT KisOcioConfiguration
{
public:
    enum Mode {
        INTERNAL = 0,
        OCIO_CONFIG,
        OCIO_ENVIRONMENT
    };

    Mode mode {INTERNAL};
    QString configurationPath;
    QString lutPath;
    QString inputColorSpace;
    QString displayDevice;
    QString displayView;
    QString look;
};

#endif
