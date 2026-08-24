/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2018 Emmet & Eoin O'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TOOL_UTILS_H
#define KIS_TOOL_UTILS_H

#include <kis_types.h>
#include <kritatools_export.h>

#include <vector>

class QPoint;
class KoColor;

namespace KisToolUtils {

struct KRITATOOLS_EXPORT ColorSamplerConfig {
    ColorSamplerConfig();

    bool toForegroundColor;
    bool updateColor;
    bool addColorToCurrentPalette;
    bool normaliseValues;
    bool sampleMerged;
    int radius;
    int blend;

    void save() const;
    void load();
private:
    static const QString CONFIG_GROUP_NAME;
};

/**
 * @brief nodeEditableMessage
 * @param node -- active node.
 * @param blockedNoIndirectPainting -- whether editing is blocked due no-indirect painting being possible.
 * @return the message for why the node is not editable. Will return empty if it is editable after all.
 */
QString KRITATOOLS_EXPORT nodeEditableMessage(KisNodeSP node, bool blockedNoIndirectPainting = false);

/**
 * return true if success
 * Clears the image. Selection is optional, use 0 to clear everything.
 */
bool KRITATOOLS_EXPORT clearImage(KisImageSP image, KisNodeList nodes, KisSelectionSP selection);

/**
 * Moves the cursor (hot spot) of the primary screen to the global screen position (x, y).
 *
 * This function is provided as a replacement to `QCursor::setPos` which does
 * not properly handle multiple monitors with different scale factors.
 */
void KRITATOOLS_EXPORT setCursorPos(const QPoint &point);

class KRITATOOLS_EXPORT StandardBrushSizes {
public:
    StandardBrushSizes(int minSize, int maxSize);

    int increaseBrushSize(qreal size);
    int decreaseBrushSize(qreal size);

private:
    std::vector<int> m_sizes;
};
}

#endif // KIS_TOOL_UTILS_H
