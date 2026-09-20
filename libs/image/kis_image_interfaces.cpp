/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_interfaces.h"

#include <QRect>

KisStrokesFacade::~KisStrokesFacade()
{
}

KisUpdatesFacade::~KisUpdatesFacade()
{
}

void KisUpdatesFacade::refreshGraphAsync(KisNodeSP root, KisProjectionUpdateFlags flags)
{
    refreshGraphAsync(root, bounds(), bounds(), flags);
}

void KisUpdatesFacade::refreshGraphAsync(KisNodeSP root, const QRect &rc, KisProjectionUpdateFlags flags)
{
    refreshGraphAsync(root, rc, bounds(), flags);
}

void KisUpdatesFacade::refreshGraphAsync(KisNodeSP root,
                                         const QRect &rc,
                                         const QRect &cropRect,
                                         KisProjectionUpdateFlags flags)
{
    refreshGraphAsync(root, QVector<QRect>({rc}), cropRect, flags);
}

KisProjectionUpdateListener::~KisProjectionUpdateListener()
{
}

KisStrokeUndoFacade::~KisStrokeUndoFacade()
{
}
