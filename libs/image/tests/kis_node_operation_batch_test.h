/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_NODE_OPERATION_BATCH_TEST_H
#define KIS_NODE_OPERATION_BATCH_TEST_H

#include <simpletest.h>
#include <testutil.h>
#include "kis_group_layer.h"

class KisNodeOperationBatchTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void testApplyUndo();
    void testEndBeforeUpdate();

    void testDuplicate();
    void testCopyLayers();
    void testMoveLayers();
    void testActiveNodeIsRestoredByUndo();

private:
    void testMove(int delayBeforeEnd);
    void testDuplicateImpl(bool externalParent, bool useMove);
private:
    QScopedPointer<TestUtil::MaskParent> p;
    KisPaintLayerSP layer1;
    KisPaintLayerSP layer2;
    KisPaintLayerSP layer3;
    KisGroupLayerSP group4;
    KisPaintLayerSP layer5;
    KisPaintLayerSP layer6;
};

#endif /* KIS_NODE_OPERATION_BATCH_TEST_H */
