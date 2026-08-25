/*
 *  This file is part of KimageShop^WKrayon^WKrita
 *
 *  SPDX-FileCopyrightText: 1999 Matthias Elter <elter@kde.org>
 *  SPDX-FileCopyrightText: 1999 Michael Koch <koch@kde.org>
 *  SPDX-FileCopyrightText: 1999 Carsten Pfeiffer <pfeiffer@kde.org>
 *  SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2003-2011 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2004 Clarence Dang <dang@kde.org>
 *  SPDX-FileCopyrightText: 2011 José Luis Vergara <pentalis@gmail.com>
 *  SPDX-FileCopyrightText: 2017 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "workspace/KisViewManager.h"

#include <QSet>

#include <KoToolBase.h>
#include <KoToolManager.h>

#include <kis_assert.h>
#include <kis_paint_device.h>
#include <kis_tool.h>

#include "canvas/kis_canvas2.h"

void KisViewManager::slotActivateTransformTool()
{
    if(KoToolManager::instance()->activeToolId() == "KisToolTransform") {
        KoToolBase* tool = KoToolManager::instance()->toolById(canvasBase(), "KisToolTransform");

        QSet<KoShape*> dummy;
        // Start a new stroke
        tool->deactivate();
        tool->activate(dummy);
    }

    KoToolManager::instance()->switchToolRequested("KisToolTransform");
}

void KisViewManager::activateTransformToolWithExternalSource(KisPaintDeviceSP externalSource)
{
    KisTool *tool = dynamic_cast<KisTool*>(KoToolManager::instance()->toolById(canvasBase(), "KisToolTransform"));
    KIS_ASSERT(tool);
    tool->newActivationWithExternalSource(externalSource);
}
