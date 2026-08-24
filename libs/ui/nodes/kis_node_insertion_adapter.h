/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_NODE_INSERTION_ADAPTER_H
#define __KIS_NODE_INSERTION_ADAPTER_H

#include <QScopedPointer>

#include "nodes/kis_node_manager.h"
#include "kis_node.h"
#include <kis_mimedata.h>


class KisNodeInsertionAdapter : public KisMimeData::NodeInsertionInterface
{
public:
    KisNodeInsertionAdapter(KisNodeManager *nodeManager);
    ~KisNodeInsertionAdapter();

    void moveNodes(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis) override;
    void copyNodes(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis);
    void addNodes(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis) override;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_NODE_INSERTION_ADAPTER_H */
