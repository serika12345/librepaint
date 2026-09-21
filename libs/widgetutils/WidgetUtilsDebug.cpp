/*
 *  SPDX-FileCopyrightText: 2015 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WidgetUtilsDebug.h"
#include <qlogging.h>
#include <qloggingcategory.h>

const QLoggingCategory &KRITAWIDGETUTILS_LOG()
{
    static const QLoggingCategory category("krita.lib.widgetutils", QtInfoMsg);
    return category;
}


