/*
 * tool_polyline.cc -- Part of Krita
 *
 * SPDX-FileCopyrightText: 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tool_polyline.h"
#include <qcontainerfwd.h>
#include <qobject.h>
#include <stdlib.h>

#include <QPoint>

#include <klocalizedstring.h>

#include <kpluginfactory.h>

#include <KoToolRegistry.h>


#include "kis_tool_polyline.h"

K_PLUGIN_FACTORY_WITH_JSON(ToolPolylineFactory, "kritatoolpolyline.json", registerPlugin<ToolPolyline>();)


ToolPolyline::ToolPolyline(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KoToolRegistry::instance()->add(new KisToolPolylineFactory());
}

ToolPolyline::~ToolPolyline()
{
}

#include "tool_polyline.moc"
