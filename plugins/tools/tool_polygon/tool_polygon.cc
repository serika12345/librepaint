/*
 * tool_polygon.cc -- Part of Krita
 *
 * SPDX-FileCopyrightText: 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "tool_polygon.h"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <stdlib.h>

#include <QPoint>

#include <klocalizedstring.h>

#include <kpluginfactory.h>

#include <KoToolRegistry.h>

#include "kis_tool_polygon.h"

K_PLUGIN_FACTORY_WITH_JSON(ToolPolygonFactory, "kritatoolpolygon.json", registerPlugin<ToolPolygon>();)


ToolPolygon::ToolPolygon(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KoToolRegistry::instance()->add(new KisToolPolygonFactory());
}

ToolPolygon::~ToolPolygon()
{
}

#include "tool_polygon.moc"
