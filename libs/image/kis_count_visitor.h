/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_COUNT_VISITOR
#define KIS_COUNT_VISITOR

#include "kritaimage_export.h"

#include <KoProperties.h>
#include <QStringList>

#include "kis_node_visitor.h"

/**
 * The count visitor traverses the node stack for nodes that conform
 * to certain properties. You can set the types of nodes to count and
 * add a list of properties to check. The children of nodes that are
 * not counted will be checked and counted if they conform to the
 * requirements.
 */
class KRITAIMAGE_EXPORT KisCountVisitor : public KisNodeVisitor
{
public:
    using KisNodeVisitor::visit;

    KisCountVisitor(const QStringList &nodeTypes, const KoProperties &properties);

    quint32 count();

    bool visit(KisNode *node) override;
    bool visit(KisPaintLayer *layer) override;
    bool visit(KisGroupLayer *layer) override;
    bool visit(KisAdjustmentLayer *layer) override;
    bool visit(KisExternalLayer *layer) override;
    bool visit(KisCloneLayer *layer) override;
    bool visit(KisFilterMask *mask) override;
    bool visit(KisTransformMask *mask) override;
    bool visit(KisTransparencyMask *mask) override;
    bool visit(KisGeneratorLayer *layer) override;
    bool visit(KisSelectionMask *mask) override;
    bool visit(KisColorizeMask *mask) override;

private:
    bool inList(KisNode *node);
    bool check(KisNode *node);

    const QStringList m_nodeTypes;
    const KoProperties m_properties;
    quint32 m_count;
};
#endif
