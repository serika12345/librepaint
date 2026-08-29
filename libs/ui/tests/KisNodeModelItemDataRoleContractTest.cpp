/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_model.h"
#pragma pop_macro("Q_OBJECT")

class KisNodeModelItemDataRoleContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void standardRolesRemainSequential();
    void thumbnailRolesKeepReservedRange();
};

void KisNodeModelItemDataRoleContractTest::standardRolesRemainSequential()
{
    const QList<int> roles{
        KisNodeModel::ActiveRole,
        KisNodeModel::PropertiesRole,
        KisNodeModel::AspectRatioRole,
        KisNodeModel::ProgressRole,
        KisNodeModel::AlternateActiveRole,
        KisNodeModel::ShouldGrayOutRole,
        KisNodeModel::ColorLabelIndexRole,
        KisNodeModel::DropEnabled,
        KisNodeModel::SelectOpaqueRole,
        KisNodeModel::DropReasonRole,
        KisNodeModel::IsAnimatedRole,
        KisNodeModel::InfoTextRole,
        KisNodeModel::FilterMaskColorRole,
        KisNodeModel::FilterMaskPropertiesRole,
        KisNodeModel::LayerColorOverlayColorRole,
        KisNodeModel::LayerColorOverlayPropertiesRole,
    };

    for (int index = 0; index < roles.size(); ++index) {
        QCOMPARE(roles.at(index), Qt::UserRole + index + 1);
    }
}

void KisNodeModelItemDataRoleContractTest::thumbnailRolesKeepReservedRange()
{
    QCOMPARE(int(KisNodeModel::ReservedRole), int(Qt::UserRole + 99));
    QCOMPARE(int(KisNodeModel::BeginThumbnailRole), int(Qt::UserRole + 100));

    constexpr int requestedSize = 64;
    const int thumbnailRole = KisNodeModel::BeginThumbnailRole + requestedSize;
    QCOMPARE(thumbnailRole - KisNodeModel::BeginThumbnailRole, requestedSize);
}

QTEST_MAIN(KisNodeModelItemDataRoleContractTest)

#include "KisNodeModelItemDataRoleContractTest.moc"
