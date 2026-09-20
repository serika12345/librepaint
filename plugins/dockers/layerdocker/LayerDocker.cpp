/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "LayerDocker.h"


#include <kpluginfactory.h>

#include <KoDockFactoryBase.h>
#include <KoDockRegistry.h>

#include "LayerBox.h"

K_PLUGIN_CLASS_WITH_JSON(KritaLayerDockerPlugin, "kritalayerdocker.json")

KritaLayerDockerPlugin::KritaLayerDockerPlugin(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KoDockRegistry::instance()->add(new LayerBoxFactory());
}

KritaLayerDockerPlugin::~KritaLayerDockerPlugin()
{
}

#include "LayerDocker.moc"
