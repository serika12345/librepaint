/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_count_visitor.h"

#include "KisQStringListFwd.h"
#include "generator/kis_generator_layer.h"
#include "kis_adjustment_layer.h"
#include "kis_clone_layer.h"
#include "kis_external_layer_iface.h"
#include "kis_filter_mask.h"
#include "kis_group_layer.h"
#include "kis_node.h"
#include "kis_paint_layer.h"
#include "kis_selection_mask.h"
#include "kis_transform_mask.h"
#include "kis_transparency_mask.h"
#include "lazybrush/kis_colorize_mask.h"
#include <QtGlobal>

KisCountVisitor::KisCountVisitor(const QStringList &nodeTypes, const KoProperties &properties)
    : m_nodeTypes(nodeTypes)
    , m_properties(properties)
    , m_count(0)
{
}

quint32 KisCountVisitor::count()
{
    return m_count;
}

bool KisCountVisitor::visit(KisNode *node)
{
    return check(node);
}

bool KisCountVisitor::visit(KisPaintLayer *layer)
{
    return check(layer);
}

bool KisCountVisitor::visit(KisGroupLayer *layer)
{
    return check(layer);
}

bool KisCountVisitor::visit(KisAdjustmentLayer *layer)
{
    return check(layer);
}

bool KisCountVisitor::visit(KisExternalLayer *layer)
{
    return check(layer);
}

bool KisCountVisitor::visit(KisCloneLayer *layer)
{
    return check(layer);
}

bool KisCountVisitor::visit(KisFilterMask *mask)
{
    return check(mask);
}

bool KisCountVisitor::visit(KisTransformMask *mask)
{
    return check(mask);
}

bool KisCountVisitor::visit(KisTransparencyMask *mask)
{
    return check(mask);
}

bool KisCountVisitor::visit(KisGeneratorLayer *layer)
{
    return check(layer);
}

bool KisCountVisitor::visit(KisSelectionMask *mask)
{
    return check(mask);
}

bool KisCountVisitor::visit(KisColorizeMask *mask)
{
    return check(mask);
}

bool KisCountVisitor::inList(KisNode *node)
{
    Q_FOREACH (const QString &nodeType, m_nodeTypes) {
        if (node->inherits(nodeType.toLatin1()))
            return true;
    }
    return false;
}

bool KisCountVisitor::check(KisNode *node)
{
    if (m_nodeTypes.isEmpty() || inList(node)) {
        if (m_properties.isEmpty() || node->check(m_properties)) {
            m_count++;
        }
    }
    visitAll(node);

    return true;
}
