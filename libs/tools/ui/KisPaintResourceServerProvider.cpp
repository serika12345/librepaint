/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintResourceServerProvider.h"

#include <QGlobalStatic>

#include <kis_psd_layer_style.h>

Q_GLOBAL_STATIC(KisPaintResourceServerProvider, s_instance)

KisPaintResourceServerProvider::KisPaintResourceServerProvider()
    : m_paintOpPresetServer(new KisPaintOpPresetResourceServer(ResourceType::PaintOpPresets))
    , m_layerStyleServer(new KoResourceServer<KisPSDLayerStyle>(ResourceType::LayerStyles))
{
}

KisPaintResourceServerProvider::~KisPaintResourceServerProvider()
{
    delete m_paintOpPresetServer;
    delete m_layerStyleServer;
}

KisPaintResourceServerProvider *KisPaintResourceServerProvider::instance()
{
    return s_instance;
}

KisPaintOpPresetResourceServer *KisPaintResourceServerProvider::paintOpPresetServer()
{
    return m_paintOpPresetServer;
}

KoResourceServer<KisPSDLayerStyle> *KisPaintResourceServerProvider::layerStyleServer()
{
    return m_layerStyleServer;
}
