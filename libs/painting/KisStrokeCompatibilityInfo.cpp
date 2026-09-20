/*
 *  SPDX-FileCopyrightText: 2023 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisStrokeCompatibilityInfo.h"

#include <kis_resources_snapshot.h>

#include "kis_node.h"

KisStrokeCompatibilityInfo::KisStrokeCompatibilityInfo()
{
}

KisStrokeCompatibilityInfo::KisStrokeCompatibilityInfo(KisResourcesSnapshot &resourcesSnapshot)
    : currentFgColor(resourcesSnapshot.currentFgColor())
    , currentBgColor(resourcesSnapshot.currentBgColor())
    , currentPattern(resourcesSnapshot.currentPatternSignature())
    , currentGradient(resourcesSnapshot.currentGradientSignature())
    , currentPreset(resourcesSnapshot.currentPaintOpPresetSignature())
    , currentGeneratorXml(resourcesSnapshot.currentGenerator() ? resourcesSnapshot.currentGenerator()->toXML() : "")
    , currentNode(resourcesSnapshot.currentNode() ? resourcesSnapshot.currentNode()->uuid() : QUuid())
    , opacity(resourcesSnapshot.opacity())
    , compositeOpId(resourcesSnapshot.compositeOpId())
    , channelLockFlags(resourcesSnapshot.channelLockFlags())
{
}

bool operator==(const KisStrokeCompatibilityInfo &lhs, const KisStrokeCompatibilityInfo &rhs)
{
    return
        lhs.currentFgColor == rhs.currentFgColor &&
        lhs.currentBgColor == rhs.currentBgColor &&
        lhs.currentPattern == rhs.currentPattern &&
        lhs.currentGradient == rhs.currentGradient &&
        lhs.currentPreset == rhs.currentPreset &&
        lhs.currentGeneratorXml == rhs.currentGeneratorXml &&
        lhs.currentNode == rhs.currentNode &&
        qFuzzyCompare(lhs.opacity, rhs.opacity) &&
        lhs.compositeOpId == rhs.compositeOpId &&
        lhs.channelLockFlags == rhs.channelLockFlags;
}
