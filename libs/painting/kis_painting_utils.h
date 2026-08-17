/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2018 Emmet & Eoin O'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PAINTING_UTILS_H
#define KIS_PAINTING_UTILS_H

#include <kis_types.h>
#include <kritapainting_export.h>

class KoColor;
class QPoint;

namespace KisPaintingUtils
{

KRITAPAINTING_EXPORT bool sampleColor(KoColor &outColor,
                                      KisPaintDeviceSP device,
                                      const QPoint &position,
                                      const KoColor *blendColor = nullptr,
                                      int radius = 1,
                                      int blend = 100,
                                      bool pure = false);

KRITAPAINTING_EXPORT KisNodeSP findNode(KisNodeSP node,
                                        const QPoint &position,
                                        bool wholeGroup,
                                        bool editableOnly = true);

KRITAPAINTING_EXPORT KisNodeList findNodes(KisNodeSP node,
                                           const QPoint &position,
                                           bool wholeGroup,
                                           bool includeGroups = true,
                                           bool editableOnly = true);

}

#endif
