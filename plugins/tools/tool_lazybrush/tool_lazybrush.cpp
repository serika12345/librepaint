/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tool_lazybrush.h"

#include <kpluginfactory.h>

#include <KoToolRegistry.h>
#include <qcontainerfwd.h>
#include <qobject.h>

#include "kis_tool_lazy_brush.h"


K_PLUGIN_FACTORY_WITH_JSON(ToolLazyBrushPluginFactory, "kritatoollazybrush.json", registerPlugin<ToolLazyBrush>();)


ToolLazyBrush::ToolLazyBrush(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KoToolRegistry::instance()->add(new KisToolLazyBrushFactory());
}

ToolLazyBrush::~ToolLazyBrush()
{
}

#include "tool_lazybrush.moc"
