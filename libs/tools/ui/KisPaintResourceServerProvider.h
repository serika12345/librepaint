/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PAINT_RESOURCE_SERVER_PROVIDER_H
#define KIS_PAINT_RESOURCE_SERVER_PROVIDER_H

#include <QObject>

#include <KoResourceServer.h>
#include <brushengine/kis_paintop_preset.h>

#include <kritatoolsui_export.h>

class KisPSDLayerStyle;

using KisPaintOpPresetResourceServer = KoResourceServer<KisPaintOpPreset>;

class KRITATOOLSUI_EXPORT KisPaintResourceServerProvider : public QObject
{
    Q_OBJECT

public:
    KisPaintResourceServerProvider();
    ~KisPaintResourceServerProvider() override;

    static KisPaintResourceServerProvider *instance();

    KisPaintOpPresetResourceServer *paintOpPresetServer();
    KoResourceServer<KisPSDLayerStyle> *layerStyleServer();

private:
    KisPaintResourceServerProvider(const KisPaintResourceServerProvider &) = delete;
    KisPaintResourceServerProvider &operator=(const KisPaintResourceServerProvider &) = delete;

    KisPaintOpPresetResourceServer *m_paintOpPresetServer;
    KoResourceServer<KisPSDLayerStyle> *m_layerStyleServer;
};

#endif
