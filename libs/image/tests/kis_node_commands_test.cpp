/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_commands_test.h"

#include <QTest>

#include <commands/kis_node_command.h>

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return false;
}

namespace
{
class InspectableNodeCommand : public KisNodeCommand
{
public:
    using KisNodeCommand::KisNodeCommand;

    bool hasNode() const
    {
        return !m_node.isNull();
    }
};
}

void KisNodeCommandsTest::constructorPreservesTextAndNullNode()
{
    InspectableNodeCommand command(kundo2_noi18n(QStringLiteral("Node Contract")), KisNodeSP());

    QCOMPARE(command.text().toString(), QStringLiteral("Node Contract"));
    QVERIFY(!command.hasNode());
}

QTEST_GUILESS_MAIN(KisNodeCommandsTest)
