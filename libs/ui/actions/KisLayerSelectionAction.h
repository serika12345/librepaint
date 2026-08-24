/*
 * SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_LAYER_SELECTION_ACTION_H
#define KIS_LAYER_SELECTION_ACTION_H

#include <kritaui_export.h>

#include <QPoint>

class KisCanvas2;

class KRITAUI_EXPORT KisLayerSelectionAction
{
public:
    static void select(KisCanvas2 *canvas,
                       const QPoint &imagePosition,
                       const QPoint &widgetPosition,
                       int layerSelectionMode,
                       int selectionOverrideMode);
};

#endif
