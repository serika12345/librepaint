/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_NODE_GROUP_OPERATIONS_H
#define KIS_NODE_GROUP_OPERATIONS_H

#include "kis_types.h"

class KisNodeOperationBatch;
class QString;

namespace KisNodeGroupOperations
{

bool createGroup(KisNodeOperationBatch *batch,
                 KisImageSP image,
                 const KisNodeList &nodes,
                 KisNodeSP activeNode,
                 const QString &groupName,
                 KisNodeSP *newGroup,
                 KisNodeSP *newLastChild);

bool ungroupNodes(KisNodeOperationBatch *batch,
                  const KisNodeList &selectedNodes,
                  KisNodeSP activeNode,
                  KisNodeSP *incompatibleNode,
                  KisNodeSP *destinationParent);

}

#endif
