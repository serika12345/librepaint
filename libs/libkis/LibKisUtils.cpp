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
#include <KisGpuPaintDeviceTransfer_p.h>
#include <kis_paint_device.h>
#include <limits>
#include <stdexcept>

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

namespace {
bool validPixelRectangle(int x, int y, int width, int height)
{
    return width >= 0 && height >= 0 &&
        qint64(x) + width - 1 <= std::numeric_limits<int>::max() &&
        qint64(y) + height - 1 <= std::numeric_limits<int>::max();
}
}

QByteArray LibKisUtils::readPixelData(KisPaintDeviceSP device, int x, int y, int width, int height)
{
    if (!device || !validPixelRectangle(x, y, width, height) || !width || !height) return {};
    qsizetype count;
    try {
        count = kisNativePixelByteCount(QRect(x, y, width, height), device->pixelSize());
    } catch (const std::length_error &) {
        return {};
    }
    QByteArray bytes(count, Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), x, y, width, height);
    return bytes;
}

bool LibKisUtils::writePixelData(KisPaintDeviceSP device, QByteArrayView bytes,
    int x, int y, int width, int height)
{
    if (!device || !validPixelRectangle(x, y, width, height)) return false;
    if (!width || !height) return true;
    try {
        kisNativePixelByteCount(QRect(x, y, width, height), device->pixelSize(), bytes.size());
    } catch (const std::length_error &) {
        return false;
    }
    device->writeBytes(reinterpret_cast<const quint8 *>(bytes.data()), x, y, width, height);
    return true;
}


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
