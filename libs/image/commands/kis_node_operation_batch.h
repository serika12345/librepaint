/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_NODE_OPERATION_BATCH_H
#define KIS_NODE_OPERATION_BATCH_H

#include <QObject>
#include <QScopedPointer>

#include <kritaimage_export.h>
#include <kundo2command.h>
#include "kis_types.h"


/**
 * Batches consecutive image node operations into one asynchronous undo entry.
 *
 * The batch owns the operation lifetime and receives the active node as a value
 * from its caller when selection restoration is part of the operation.
 */
class KRITAIMAGE_EXPORT KisNodeOperationBatch : public QObject
{
    Q_OBJECT
public:
    KisNodeOperationBatch(const KUndo2MagicString &actionName, KisImageSP image, int timeout);
    ~KisNodeOperationBatch() override;

    void moveNode(KisNodeSP node, KisNodeSP parent, KisNodeSP above);
    void setAutoDelete(bool value);

    bool isEnded() const;

    void lowerNode(const KisNodeList &nodes, KisNodeSP activeNode);
    void raiseNode(const KisNodeList &nodes, KisNodeSP activeNode);
    void removeNode(const KisNodeList &nodes, KisNodeSP activeNode);
    void duplicateNode(const KisNodeList &nodes, KisNodeSP activeNode);

    void copyNode(const KisNodeList &nodes, KisNodeSP dstParent, KisNodeSP dstAbove, KisNodeSP activeNode);
    void moveNode(const KisNodeList &nodes, KisNodeSP dstParent, KisNodeSP dstAbove, KisNodeSP activeNode);
    void addNode(const KisNodeList &nodes, KisNodeSP dstParent, KisNodeSP dstAbove, KisNodeSP activeNode);

    bool canMergeAction(const KUndo2MagicString &actionName) const;

public Q_SLOTS:
    void end();

private Q_SLOTS:
    void startTimers();
    void slotUpdateTimeout();
    void slotEndStrokeRequested();
    void slotUndoDuringStrokeRequested();
    void slotImageAboutToBeDeleted();

Q_SIGNALS:
    void requestUpdateAsyncFromCommand();

private:
    void cleanup();

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* KIS_NODE_OPERATION_BATCH_H */
