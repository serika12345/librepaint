/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_manager.h"
#pragma pop_macro("Q_OBJECT")

namespace
{

enum Direction {
    Next,
    Previous,
};

QList<Direction> directions;
QList<bool> siblingsOnlyValues;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

void KisNodeManager::NavigationAccess::activateNextNode(KisNodeManager *, bool siblingsOnly)
{
    directions.append(Next);
    siblingsOnlyValues.append(siblingsOnly);
}

void KisNodeManager::NavigationAccess::activatePreviousNode(KisNodeManager *, bool siblingsOnly)
{
    directions.append(Previous);
    siblingsOnlyValues.append(siblingsOnly);
}

class KisNodeManagerNavigationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void siblingNavigationKeepsDirectionAndScope();
};

void KisNodeManagerNavigationContractTest::init()
{
    directions.clear();
    siblingsOnlyValues.clear();
}

void KisNodeManagerNavigationContractTest::siblingNavigationKeepsDirectionAndScope()
{
    KisNodeManager manager(nullptr);

    manager.activateNextSiblingNode();
    manager.activatePreviousSiblingNode();

    QCOMPARE(directions, (QList<Direction>{Next, Previous}));
    QCOMPARE(siblingsOnlyValues, (QList<bool>{true, true}));
}

QTEST_GUILESS_MAIN(KisNodeManagerNavigationContractTest)

#include "KisNodeManagerNavigationContractTest.moc"
