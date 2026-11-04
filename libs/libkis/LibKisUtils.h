/*
 * SPDX-FileCopyrightText: 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef LIBKISUTILS_H
#define LIBKISUTILS_H

class Node;
class Document;

#include <kis_types.h>
#include <QByteArrayView>

namespace LibKisUtils
{

QList<Node *> createNodeList(KisNodeList kisnodes, KisImageWSP image);

Document* findNodeInDocuments(KisNodeSP kisnode);

QByteArray readPixelData(KisPaintDeviceSP device, int x, int y, int width, int height);
bool writePixelData(KisPaintDeviceSP device, QByteArrayView bytes, int x, int y, int width, int height);

}

#endif // LIBKISUTILS_H
