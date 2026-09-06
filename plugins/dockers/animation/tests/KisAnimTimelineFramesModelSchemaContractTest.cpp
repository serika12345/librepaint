/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

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
    void modelCanvasAndNodeManipulationSchemaRemainStable();
    void frameAudioClipAndCacheSignaturesRemainStable();
    void tableMimeAndRowOperationSignaturesRemainStable();
    void publicSlotAndNotificationSignaturesRemainStable();
    void nodeListKeeperTypeAndLifetimeSchemaRemainStable();
    void nodeListKeeperRowQuerySignaturesRemainStable();
    void nodeListKeeperOtherLayerValueContractRemainsStable();
    void nodeListKeeperModelTypeAndConstructionSchemaRemainStable();
    void nodeListKeeperModelNotificationSignaturesRemainStable();
};

void KisAnimTimelineFramesModelSchemaContractTest::timelineSelectionAndModelRoleSchemaRemainStable()
{
    using EntryEqual = bool (*)(const TimelineSelectionEntry &, const TimelineSelectionEntry &);
    using EntryHash = uint (*)(const TimelineSelectionEntry &);
    using Model = KisAnimTimelineFramesModel;

    static_assert(std::is_class_v<TimelineSelectionEntry>);
    static_assert(std::is_same_v<decltype(TimelineSelectionEntry::channel), KisRasterKeyframeChannel *>);
    static_assert(std::is_same_v<decltype(TimelineSelectionEntry::time), int>);
    static_assert(std::is_same_v<decltype(TimelineSelectionEntry::keyframe), KisRasterKeyframeSP>);
    static_assert(std::is_same_v<decltype(static_cast<EntryEqual>(&operator==)), EntryEqual>);
    static_assert(std::is_same_v<decltype(static_cast<EntryHash>(&qHash)), EntryHash>);

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_base_of_v<TimelineNodeListKeeper::ModelWithExternalNotifications, Model>);
    static_assert(std::is_enum_v<Model::MimeCopyPolicy>);
    static_assert(Model::UndefinedPolicy == 0);
    static_assert(Model::MoveFramesPolicy == 1);
    static_assert(Model::CopyFramesPolicy == 2);
    static_assert(Model::CloneFramesPolicy == 3);
    static_assert(std::is_enum_v<Model::ItemDataRole>);
    static_assert(static_cast<int>(Model::ActiveLayerRole) == static_cast<int>(KisTimeBasedItemModel::UserRole));
    static_assert(Model::TimelinePropertiesRole == Model::ActiveLayerRole + 1);
    static_assert(Model::OtherLayersRole == Model::TimelinePropertiesRole + 1);
    static_assert(Model::PinnedToTimelineRole == Model::OtherLayersRole + 1);
    static_assert(Model::FrameColorLabelIndexRole == Model::PinnedToTimelineRole + 1);
    static_assert(std::is_same_v<Model::Property, KisBaseNode::Property>);
    static_assert(std::is_same_v<Model::PropertyList, KisBaseNode::PropertyList>);
    static_assert(std::is_same_v<Model::OtherLayer, TimelineNodeListKeeper::OtherLayer>);
    static_assert(std::is_same_v<Model::OtherLayersList, TimelineNodeListKeeper::OtherLayersList>);

    const TimelineSelectionEntry first{nullptr, 17, KisRasterKeyframeSP()};
    const TimelineSelectionEntry equal{nullptr, 17, KisRasterKeyframeSP()};
    const TimelineSelectionEntry different{nullptr, 18, KisRasterKeyframeSP()};
    QVERIFY(first == equal);
    QVERIFY(!(first == different));
    QCOMPARE(qHash(first), qHash(equal));
}

void KisAnimTimelineFramesModelSchemaContractTest::modelCanvasAndNodeManipulationSchemaRemainStable()
{
    using Model = KisAnimTimelineFramesModel;
    using Interface = Model::NodeManipulationInterface;
    using HasConnection = bool (Model::*)() const;
    using SetFacade = void (Model::*)(KisDummiesFacadeBase *, KisImageSP, KisNodeDisplayModeAdapter *);
    using CanDrop = bool (Model::*)(const QMimeData *, const QModelIndex &);
    using InsertOtherLayer = bool (Model::*)(int, int);
    using ActiveLayerRow = int (Model::*)() const;
    using SetInterface = void (Model::*)(Interface *);
    using NodeAt = KisNodeSP (Model::*)(QModelIndex) const;
    using AddPaintLayer = KisLayerSP (Interface::*)() const;
    using RemoveNode = void (Interface::*)(KisNodeSP) const;
    using SetNodeProperties = bool (Interface::*)(KisNodeSP, KisImageSP, KisBaseNode::PropertyList) const;

    static_assert(std::is_constructible_v<Model, QObject *>);
    static_assert(std::has_virtual_destructor_v<Model>);
    static_assert(std::is_same_v<decltype(&Model::hasConnectionToCanvas), HasConnection>);
    static_assert(std::is_same_v<decltype(&Model::setDummiesFacade), SetFacade>);
    static_assert(std::is_same_v<decltype(&Model::canDropFrameData), CanDrop>);
    static_assert(std::is_same_v<decltype(&Model::insertOtherLayer), InsertOtherLayer>);
    static_assert(std::is_same_v<decltype(&Model::activeLayerRow), ActiveLayerRow>);
    static_assert(std::is_same_v<decltype(&Model::setNodeManipulationInterface), SetInterface>);
    static_assert(std::is_same_v<decltype(&Model::nodeAt), NodeAt>);
    static_assert(std::is_class_v<Interface>);
    static_assert(std::is_abstract_v<Interface>);
    static_assert(std::has_virtual_destructor_v<Interface>);
    static_assert(std::is_same_v<decltype(&Interface::addPaintLayer), AddPaintLayer>);
    static_assert(std::is_same_v<decltype(&Interface::removeNode), RemoveNode>);
    static_assert(std::is_same_v<decltype(&Interface::setNodeProperties), SetNodeProperties>);

    QVERIFY(true);
}

void KisAnimTimelineFramesModelSchemaContractTest::frameAudioClipAndCacheSignaturesRemainStable()
{
    using Model = KisAnimTimelineFramesModel;
    using CreateFrame = bool (Model::*)(const QModelIndexList &);
    using CopyFrame = bool (Model::*)(const QModelIndex &);
    using MakeClonesUnique = void (Model::*)(const QModelIndexList &);
    using InsertFrames = bool (Model::*)(int, const QList<int> &, int, int);
    using InsertHoldFrames = bool (Model::*)(const QModelIndexList &, int);
    using AudioFileName = QString (Model::*)() const;
    using SetAudioFileName = void (Model::*)(const QFileInfo &);
    using AudioMuted = bool (Model::*)() const;
    using SetAudioMuted = void (Model::*)(bool);
    using AudioVolume = qreal (Model::*)() const;
    using SetAudioVolume = void (Model::*)(qreal);
    using SetClipRange = void (Model::*)(int);
    using ClearCache = void (Model::*)();
    using SetSelectedTimes = void (Model::*)(const QSet<int> &);
    using SetLastClicked = void (Model::*)(const QModelIndex &);

    static_assert(std::is_same_v<decltype(&Model::createFrame), CreateFrame>);
    static_assert(std::is_same_v<decltype(&Model::copyFrame), CopyFrame>);
    static_assert(std::is_same_v<decltype(&Model::makeClonesUnique), MakeClonesUnique>);
    static_assert(std::is_same_v<decltype(&Model::insertFrames), InsertFrames>);
    static_assert(std::is_same_v<decltype(&Model::insertHoldFrames), InsertHoldFrames>);
    static_assert(std::is_same_v<decltype(&Model::audioChannelFileName), AudioFileName>);
    static_assert(std::is_same_v<decltype(&Model::setAudioChannelFileName), SetAudioFileName>);
    static_assert(std::is_same_v<decltype(&Model::isAudioMuted), AudioMuted>);
    static_assert(std::is_same_v<decltype(&Model::setAudioMuted), SetAudioMuted>);
    static_assert(std::is_same_v<decltype(&Model::audioVolume), AudioVolume>);
    static_assert(std::is_same_v<decltype(&Model::setAudioVolume), SetAudioVolume>);
    static_assert(std::is_same_v<decltype(&Model::setDocumentClipRangeStart), SetClipRange>);
    static_assert(std::is_same_v<decltype(&Model::setDocumentClipRangeEnd), SetClipRange>);
    static_assert(std::is_same_v<decltype(&Model::clearEntireCache), ClearCache>);
    static_assert(std::is_same_v<decltype(&Model::setActiveLayerSelectedTimes), SetSelectedTimes>);
    static_assert(std::is_same_v<decltype(&Model::setLastClickedIndex), SetLastClicked>);

    QVERIFY(true);
}

void KisAnimTimelineFramesModelSchemaContractTest::tableMimeAndRowOperationSignaturesRemainStable()
{
    using Model = KisAnimTimelineFramesModel;
    using RowCount = int (Model::*)(const QModelIndex &) const;
    using Data = QVariant (Model::*)(const QModelIndex &, int) const;
    using SetData = bool (Model::*)(const QModelIndex &, const QVariant &, int);
    using HeaderData = QVariant (Model::*)(int, Qt::Orientation, int) const;
    using SetHeaderData = bool (Model::*)(int, Qt::Orientation, const QVariant &, int);
    using SupportedActions = Qt::DropActions (Model::*)() const;
    using MimeTypes = QStringList (Model::*)() const;
    using MimeData = QMimeData *(Model::*)(const QModelIndexList &) const;
    using MimeDataExtended =
        QMimeData *(Model::*)(const QModelIndexList &, const QModelIndex &, Model::MimeCopyPolicy) const;
    using DropMimeData = bool (Model::*)(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &);
    using DropMimeDataExtended = bool (Model::*)(const QMimeData *, Qt::DropAction, const QModelIndex &, bool *);
    using Flags = Qt::ItemFlags (Model::*)(const QModelIndex &) const;
    using Rows = bool (Model::*)(int, int, const QModelIndex &);

    static_assert(std::is_same_v<decltype(&Model::rowCount), RowCount>);
    static_assert(std::is_same_v<decltype(&Model::data), Data>);
    static_assert(std::is_same_v<decltype(&Model::setData), SetData>);
    static_assert(std::is_same_v<decltype(&Model::headerData), HeaderData>);
    static_assert(std::is_same_v<decltype(&Model::setHeaderData), SetHeaderData>);
    static_assert(std::is_same_v<decltype(&Model::supportedDragActions), SupportedActions>);
    static_assert(std::is_same_v<decltype(&Model::supportedDropActions), SupportedActions>);
    static_assert(std::is_same_v<decltype(&Model::mimeTypes), MimeTypes>);
    static_assert(std::is_same_v<decltype(&Model::mimeData), MimeData>);
    static_assert(std::is_same_v<decltype(&Model::mimeDataExtended), MimeDataExtended>);
    static_assert(std::is_same_v<decltype(&Model::dropMimeData), DropMimeData>);
    static_assert(std::is_same_v<decltype(&Model::dropMimeDataExtended), DropMimeDataExtended>);
    static_assert(std::is_same_v<decltype(&Model::flags), Flags>);
    static_assert(std::is_same_v<decltype(&Model::insertRows), Rows>);
    static_assert(std::is_same_v<decltype(&Model::removeRows), Rows>);

    QVERIFY(true);
}

void KisAnimTimelineFramesModelSchemaContractTest::publicSlotAndNotificationSignaturesRemainStable()
{
    using Model = KisAnimTimelineFramesModel;
    using NodeNotification = void (Model::*)(KisNodeSP);
    using NoArgumentNotification = void (Model::*)();
    using RowNotification = void (Model::*)(int);
    using RowTransferNotification = void (Model::*)(int, int);

    static_assert(std::is_same_v<decltype(&Model::slotCurrentNodeChanged), NodeNotification>);
    static_assert(std::is_same_v<decltype(&Model::requestCurrentNodeChanged), NodeNotification>);
    static_assert(std::is_same_v<decltype(&Model::sigInfiniteTimelineUpdateNeeded), NoArgumentNotification>);
    static_assert(std::is_same_v<decltype(&Model::sigEnsureRowVisible), RowNotification>);
    static_assert(std::is_same_v<decltype(&Model::requestTransferSelectionBetweenRows), RowTransferNotification>);
    static_assert(std::is_same_v<decltype(&Model::sigFullClipRangeChanged), NoArgumentNotification>);

    QVERIFY(true);
}

void KisAnimTimelineFramesModelSchemaContractTest::nodeListKeeperTypeAndLifetimeSchemaRemainStable()
{
    using Keeper = TimelineNodeListKeeper;
    using Model = Keeper::ModelWithExternalNotifications;

    static_assert(std::is_class_v<Keeper>);
    static_assert(std::is_base_of_v<QObject, Keeper>);
    static_assert(std::is_constructible_v<Keeper, Model *, KisDummiesFacadeBase *, KisNodeDisplayModeAdapter *>);
    static_assert(std::has_virtual_destructor_v<Keeper>);
    QVERIFY(true);
}

void KisAnimTimelineFramesModelSchemaContractTest::nodeListKeeperRowQuerySignaturesRemainStable()
{
    using Keeper = TimelineNodeListKeeper;

    static_assert(std::is_same_v<decltype(&Keeper::dummyFromRow), KisNodeDummy *(Keeper::*)(int)>);
    static_assert(std::is_same_v<decltype(&Keeper::rowForDummy), int (Keeper::*)(KisNodeDummy *)>);
    static_assert(std::is_same_v<decltype(&Keeper::rowCount), int (Keeper::*)()>);
    static_assert(std::is_same_v<decltype(&Keeper::otherLayersList), Keeper::OtherLayersList (Keeper::*)() const>);
    static_assert(std::is_same_v<decltype(&Keeper::updateActiveDummy), void (Keeper::*)(KisNodeDummy *)>);
    QVERIFY(true);
}

void KisAnimTimelineFramesModelSchemaContractTest::nodeListKeeperOtherLayerValueContractRemainsStable()
{
    using Keeper = TimelineNodeListKeeper;
    using OtherLayer = Keeper::OtherLayer;

    static_assert(std::is_class_v<OtherLayer>);
    static_assert(std::is_same_v<Keeper::OtherLayersList, QList<OtherLayer>>);
    static_assert(std::is_constructible_v<OtherLayer, const QString &, KisNodeDummy *>);
    static_assert(std::is_same_v<decltype(OtherLayer::name), QString>);
    static_assert(std::is_same_v<decltype(OtherLayer::dummy), KisNodeDummy *>);

    const OtherLayer layer(QStringLiteral("Paint Layer"), nullptr);
    QCOMPARE(layer.name, QStringLiteral("Paint Layer"));
    QVERIFY(!layer.dummy);
}

void KisAnimTimelineFramesModelSchemaContractTest::nodeListKeeperModelTypeAndConstructionSchemaRemainStable()
{
    using Model = TimelineNodeListKeeper::ModelWithExternalNotifications;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_base_of_v<KisTimeBasedItemModel, Model>);
    static_assert(std::is_abstract_v<Model>);
    static_assert(std::is_constructible_v<ExternalNotificationsProbe, QObject *>);
    QVERIFY(true);
}

void KisAnimTimelineFramesModelSchemaContractTest::nodeListKeeperModelNotificationSignaturesRemainStable()
{
    using Model = TimelineNodeListKeeper::ModelWithExternalNotifications;
    using NoArgument = void (Model::*)();
    using Rows = void (Model::*)(const QModelIndex &, int, int);

    static_assert(std::is_same_v<decltype(&Model::callBeginResetModel), NoArgument>);
    static_assert(std::is_same_v<decltype(&Model::callEndResetModel), NoArgument>);
    static_assert(std::is_same_v<decltype(&Model::callBeginInsertRows), Rows>);
    static_assert(std::is_same_v<decltype(&Model::callEndInsertRows), NoArgument>);
    static_assert(std::is_same_v<decltype(&Model::callBeginRemoveRows), Rows>);
    static_assert(std::is_same_v<decltype(&Model::callEndRemoveRows), NoArgument>);
    static_assert(
        std::is_same_v<decltype(&Model::callIndexChanged), void (Model::*)(const QModelIndex &, const QModelIndex &)>);
    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisAnimTimelineFramesModelSchemaContractTest)

#include "KisAnimTimelineFramesModelSchemaContractTest.moc"
