/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_time_span.h"

#include "kis_keyframe_channel.h"
#include "kis_node.h"
#include "kis_layer_utils.h"

KisTimeSpan KisTimeSpan::calculateIdenticalFramesRecursive(const KisNode *node, int time)
{
    KisTimeSpan range = KisTimeSpan::infinite(0);

    KisLayerUtils::recursiveApplyNodes(node,
        [&range, time] (const KisNode *node) {
            if (node->visible()) {
                range &= calculateNodeIdenticalFrames(node, time);
            }
    });

    return range;
}

KisTimeSpan KisTimeSpan::calculateAffectedFramesRecursive(const KisNode *node, int time)
{
    KisTimeSpan range;

    KisLayerUtils::recursiveApplyNodes(node,
        [&range, time] (const KisNode *node) {
            if (node->visible()) {
                range |= calculateNodeIdenticalFrames(node, time);
            }
    });

    return range;
}

KisTimeSpan KisTimeSpan::calculateNodeIdenticalFrames(const KisNode *node, int time)
{
    KisTimeSpan range = KisTimeSpan::infinite(0);

    const QMap<QString, KisKeyframeChannel*> channels =
        node->keyframeChannels();

    Q_FOREACH (const KisKeyframeChannel *channel, channels) {
        // Intersection
        range &= channel->identicalFrames(time);
    }

    return range;
}

KisTimeSpan KisTimeSpan::calculateNodeAffectedFrames(const KisNode *node, int time)
{
    KisTimeSpan range;

    if (!node->visible()) return range;

    const QMap<QString, KisKeyframeChannel*> channels =
        node->keyframeChannels();

    // TODO: channels should report to the image which channel exactly has changed
    //       to avoid the dirty range to be stretched into infinity!

    if (channels.isEmpty() ||
        !channels.contains(KisKeyframeChannel::Raster.id())) {
        range = KisTimeSpan::infinite(0);
        return range;
    }

    Q_FOREACH (const KisKeyframeChannel *channel, channels) {
        // Union
        range |= channel->affectedFrames(time);
    }

    return range;
}
