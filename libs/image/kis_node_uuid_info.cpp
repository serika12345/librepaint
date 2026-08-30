/*
 *  Clone info stores information about clone layer's target
 *  SPDX-FileCopyrightText: 2011 Torio Mlshi <mlshi@lavabit.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_uuid_info.h"

#include "KisNodeUuidInfoNodeAccess_p.h"

KisNodeUuidInfo::KisNodeUuidInfo()
{
}

KisNodeUuidInfo::KisNodeUuidInfo(const QUuid &uuid)
{
    m_uuid = uuid;
}

KisNodeUuidInfo::KisNodeUuidInfo(const QString &name)
{
    m_name = name;
}

KisNodeUuidInfo::KisNodeUuidInfo(KisNodeSP node)
{
    m_uuid = KisNodeUuidInfoNodeAccess::uuid(node.data());
    m_name = KisNodeUuidInfoNodeAccess::name(node.data());
}

KisNodeSP KisNodeUuidInfo::findNode(KisNodeSP rootNode)
{
    if (check(rootNode))
        return rootNode;

    KisNodeSP child = KisNodeUuidInfoNodeAccess::firstChild(rootNode.data());
    KisNodeSP node = 0;
    while (child && !node) {
        node = findNode(child);
        child = KisNodeUuidInfoNodeAccess::nextSibling(child.data());
    }
    return node;
}

bool KisNodeUuidInfo::check(KisNodeSP node)
{
    // Every node has a valid UUID.
    if (m_uuid == KisNodeUuidInfoNodeAccess::uuid(node.data()))
        return true;
    // Some nodes may have empty names.
    if (m_uuid.isNull() && m_name == KisNodeUuidInfoNodeAccess::name(node.data()))
        return true;
    return false;
}
