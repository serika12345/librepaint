/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2018 Emmet & Eoin O'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_painting_utils.h"

#include <KoColor.h>
#include <KoMixColorsOp.h>
#include <kis_group_layer.h>
#include <kis_sequential_iterator.h>

namespace KisPaintingUtils
{

bool sampleColor(KoColor &outColor,
                 KisPaintDeviceSP device,
                 const QPoint &position,
                 const KoColor *blendColor,
                 int radius,
                 int blend,
                 bool pure)
{
    KIS_ASSERT(device);

    static bool firstSample = true;
    if (firstSample) {
        pure = true;
        firstSample = false;
    }

    const KoColorSpace *colorSpace = device->colorSpace();
    KoColor sampledColor = KoColor::createTransparent(colorSpace);

    const bool supportedWraparound = device->supportsWraproundMode();
    device->setSupportsWraparoundMode(true);

    if (!pure && radius > 1) {
        QScopedPointer<KoMixColorsOp::Mixer> mixer(colorSpace->mixColorsOp()->createMixer());
        const int effectiveRadius = radius - 1;
        const QRect sampleRect(position.x() - effectiveRadius,
                               position.y() - effectiveRadius,
                               2 * effectiveRadius + 1,
                               2 * effectiveRadius + 1);
        KisSequentialConstIterator iterator(device, sampleRect);
        const int radiusSquared = pow2(effectiveRadius);

        int consecutivePixels = iterator.nConseqPixels();
        while (iterator.nextPixels(consecutivePixels)) {
            const QPoint realPosition(iterator.x(), iterator.y());
            if (kisSquareDistance(realPosition, position) < radiusSquared) {
                mixer->accumulateAverage(iterator.oldRawData(), consecutivePixels);
            }
        }
        mixer->computeMixedColor(sampledColor.data());
    } else {
        device->pixel(position.x(), position.y(), &sampledColor);
    }

    device->setSupportsWraparoundMode(supportedWraparound);

    if (!pure && blendColor && blend < 100) {
        const quint8 blendScaled = static_cast<quint8>(blend * 2.55f);
        const quint8 *colors[] = {blendColor->data(), sampledColor.data()};
        const qint16 weights[] = {static_cast<qint16>(255 - blendScaled), blendScaled};
        device->colorSpace()->mixColorsOp()->mixColors(colors, weights, 2, sampledColor.data());
    }

    sampledColor.convertTo(device->compositionSourceColorSpace());
    if (sampledColor.opacityU8() == OPACITY_TRANSPARENT_U8) {
        return false;
    }

    outColor = sampledColor;
    return true;
}

KisNodeSP findNode(KisNodeSP node, const QPoint &position, bool wholeGroup, bool editableOnly)
{
    KisNodeSP foundNode;
    while (node) {
        KisLayerSP layer = qobject_cast<KisLayer *>(node.data());
        if (!layer || !layer->isEditable()) {
            node = node->prevSibling();
            continue;
        }

        KoColor color(layer->projection()->colorSpace());
        layer->projection()->pixel(position.x(), position.y(), &color);
        KisGroupLayerSP group = dynamic_cast<KisGroupLayer *>(layer.data());

        if ((group && group->passThroughMode()) || color.opacityU8() != OPACITY_TRANSPARENT_U8) {
            if (layer->inherits("KisGroupLayer") && (!editableOnly || layer->isEditable())) {
                foundNode = findNode(node->lastChild(), position, wholeGroup, editableOnly);
            } else {
                foundNode = wholeGroup ? node->parent() : node;
            }
        }

        if (foundNode) {
            break;
        }
        node = node->prevSibling();
    }
    return foundNode;
}

KisNodeList findNodes(KisNodeSP node,
                      const QPoint &position,
                      bool wholeGroup,
                      bool includeGroups,
                      bool editableOnly)
{
    KisNodeList foundNodes;
    while (node) {
        KisLayerSP layer = qobject_cast<KisLayer *>(node.data());
        if (!layer || !layer->isEditable()) {
            node = node->nextSibling();
            continue;
        }

        KoColor color(layer->projection()->colorSpace());
        layer->projection()->pixel(position.x(), position.y(), &color);
        const bool isTransparent = color.opacityU8() == OPACITY_TRANSPARENT_U8;
        KisGroupLayerSP group = dynamic_cast<KisGroupLayer *>(layer.data());

        if (group) {
            if (!isTransparent || group->passThroughMode()) {
                foundNodes << findNodes(node->firstChild(), position, wholeGroup, includeGroups, editableOnly);
                if (includeGroups) {
                    foundNodes << node;
                }
            }
        } else if (!isTransparent) {
            if (wholeGroup) {
                if (!foundNodes.contains(node->parent())) {
                    foundNodes << node->parent();
                }
            } else {
                foundNodes << node;
            }
        }

        node = node->nextSibling();
    }
    return foundNodes;
}

}
