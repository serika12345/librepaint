/*
 *  SPDX-FileCopyrightText: 2020 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ResourceDebug.h"
#include <qlogging.h>
#include <qloggingcategory.h>

const QLoggingCategory &RESOURCE_LOG() \
{
    static const QLoggingCategory category("krita.lib.resources", QtInfoMsg);
    return category;
}

