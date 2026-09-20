/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_multinode_property.h"

#include <KoChannelInfo.h>
#include <KoColorSpace.h>

#include <kis_layer.h>

QString BaseAdapter::NodeAccess::compositeOpId(KisNodeSP node)
{
    return node->compositeOpId();
}

void BaseAdapter::NodeAccess::setCompositeOpId(KisNodeSP node, const QString &value)
{
    node->setCompositeOpId(value);
}

QString BaseAdapter::NodeAccess::name(KisNodeSP node)
{
    return node->name();
}

void BaseAdapter::NodeAccess::setName(KisNodeSP node, const QString &value)
{
    node->setName(value);
}

int BaseAdapter::NodeAccess::colorLabelIndex(KisNodeSP node)
{
    return node->colorLabelIndex();
}

void BaseAdapter::NodeAccess::setColorLabelIndex(KisNodeSP node, int value)
{
    node->setColorLabelIndex(value);
}

quint8 BaseAdapter::NodeAccess::opacity(KisNodeSP node)
{
    return node->opacity();
}

void BaseAdapter::NodeAccess::setOpacity(KisNodeSP node, quint8 value)
{
    node->setOpacity(value);
}

KisBaseNode::PropertyList BaseAdapter::NodeAccess::sectionModelProperties(KisNodeSP node)
{
    return node->sectionModelProperties();
}

void BaseAdapter::NodeAccess::setSectionModelProperties(KisNodeSP node, const KisBaseNode::PropertyList &properties)
{
    node->setSectionModelProperties(properties);
}

bool BaseAdapter::NodeAccess::isLayer(KisNodeSP node)
{
    return bool(qobject_cast<KisLayer *>(node.data()));
}

const KoColorSpace *BaseAdapter::NodeAccess::colorSpace(KisNodeSP node)
{
    return node->colorSpace();
}

QStringList BaseAdapter::NodeAccess::channelNames(KisNodeSP node)
{
    QStringList names;
    const QList<KoChannelInfo *> channels = node->colorSpace()->channels();
    for (const KoChannelInfo *channel : channels) {
        names.append(channel->name());
    }
    return names;
}

int BaseAdapter::NodeAccess::channelCount(KisNodeSP node)
{
    return node->colorSpace()->channelCount();
}

QBitArray BaseAdapter::NodeAccess::channelFlags(KisNodeSP node)
{
    auto *layer = qobject_cast<KisLayer *>(node.data());
    Q_ASSERT(layer);
    return layer->channelFlags();
}

void BaseAdapter::NodeAccess::setChannelFlags(KisNodeSP node, const QBitArray &flags)
{
    auto *layer = qobject_cast<KisLayer *>(node.data());
    Q_ASSERT(layer);
    layer->setChannelFlags(flags);
}
