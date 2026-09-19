/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>


#include "KisAnimTimelineFramesModel.h"
#include "timeline_node_list_keeper.h"

namespace
{
class ExternalNotificationsProbe final : public TimelineNodeListKeeper::ModelWithExternalNotifications
{
public:
    using ModelWithExternalNotifications::ModelWithExternalNotifications;
    int rowCount(const QModelIndex &) const override
    {
        return 0;
    }

protected:
    KisNodeSP nodeAt(QModelIndex) const override
    {
        return {};
    }
    QMap<QString, KisKeyframeChannel *> channelsAt(QModelIndex) const override
    {
        return {};
    }
    KisKeyframeChannel *channelByID(QModelIndex, const QString &) const override
    {
        return nullptr;
    }
};
} // namespace

class KisAnimTimelineFramesModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void timelineSelectionAndModelRoleSchemaRemainStable();
    void nodeListKeeperOtherLayerValueContractRemainsStable();
};

void KisAnimTimelineFramesModelSchemaContractTest::timelineSelectionAndModelRoleSchemaRemainStable()
{



    const TimelineSelectionEntry first{nullptr, 17, KisRasterKeyframeSP()};
    const TimelineSelectionEntry equal{nullptr, 17, KisRasterKeyframeSP()};
    const TimelineSelectionEntry different{nullptr, 18, KisRasterKeyframeSP()};
    QVERIFY(first == equal);
    QVERIFY(!(first == different));
    QCOMPARE(qHash(first), qHash(equal));
}

void KisAnimTimelineFramesModelSchemaContractTest::nodeListKeeperOtherLayerValueContractRemainsStable()
{
    using Keeper = TimelineNodeListKeeper;
    using OtherLayer = Keeper::OtherLayer;


    const OtherLayer layer(QStringLiteral("Paint Layer"), nullptr);
    QCOMPARE(layer.name, QStringLiteral("Paint Layer"));
    QVERIFY(!layer.dummy);
}

QTEST_GUILESS_MAIN(KisAnimTimelineFramesModelSchemaContractTest)

#include "KisAnimTimelineFramesModelSchemaContractTest.moc"
