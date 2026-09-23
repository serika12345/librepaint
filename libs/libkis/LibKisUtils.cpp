/*
 * SPDX-FileCopyrightText: 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "LibKisUtils.h"

#include <kis_node.h>
#include <kis_group_layer.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <QtGlobal>
#include <qlist.h>
#include <qpointer.h>

#include "Document.h"
#include "Node.h"
#include "GroupLayer.h"
#include "CloneLayer.h"
#include "FilterLayer.h"
#include "FillLayer.h"
#include "FileLayer.h"
#include "VectorLayer.h"
#include "FilterMask.h"
#include "SelectionMask.h"
#include "TransparencyMask.h"
#include "TransformMask.h"
#include "kis_types.h"



QList<Node *> LibKisUtils::createNodeList(KisNodeList kisnodes, KisImageWSP image)
{
    QList <Node*> nodes;
    Q_FOREACH(KisNodeSP node, kisnodes) {
        nodes << Node::createNode(image, node);
    }
    return nodes;
}

Document* LibKisUtils::findNodeInDocuments(KisNodeSP kisnode) {
    foreach(QPointer<KisDocument> doc, KisPart::instance()->documents()) {
        if (kisnode->image()->rootLayer()->uuid() == doc->image()->rootLayer()->uuid()) return new Document(doc, false);
    }

    return 0;
}
