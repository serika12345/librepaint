/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_paint_device.h"

#include "kis_base_rects_walker.h"

#include <QTest>

class KisBaseRectsWalkerPolicyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodePositionsMapToInvalidationReasons();
};

void KisBaseRectsWalkerPolicyContractTest::nodePositionsMapToInvalidationReasons()
{
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_ABOVE_FILTHY), KisNode::N_ABOVE_FILTHY);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_FILTHY_PROJECTION),
             KisNode::N_FILTHY_PROJECTION);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_FILTHY), KisNode::N_FILTHY);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_BELOW_FILTHY), KisNode::N_BELOW_FILTHY);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_EXTRA), KisNode::N_FILTHY);
}

QTEST_GUILESS_MAIN(KisBaseRectsWalkerPolicyContractTest)

#include "KisBaseRectsWalkerPolicyContractTest.moc"
