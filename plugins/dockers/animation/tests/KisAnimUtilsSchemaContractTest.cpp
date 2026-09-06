/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "KisAnimUtils.h"
#include "KisTimeBasedItemModel.h"

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

namespace
{

class ConcreteTimeBasedItemModel : public KisTimeBasedItemModel
{
public:
    using KisTimeBasedItemModel::KisTimeBasedItemModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

protected:
    KisNodeSP nodeAt(QModelIndex index) const override;
    QMap<QString, KisKeyframeChannel *> channelsAt(QModelIndex index) const override;
    KisKeyframeChannel *channelByID(QModelIndex index, const QString &id) const override;
};

} // namespace

class KisAnimUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void frameItemTypeAndDefaultStateRemainStable();
    void frameItemValueAndComparisonContractsRemainStable();
    void keyframeCreationAndMoveSignaturesRemainStable();
    void keyframeEditingAndQuerySignaturesRemainStable();
    void animationActionNameTypesRemainStable();
    void timeBasedModelTypeAndRoleSchemaRemainStable();
    void timeBasedModelOwnershipAndContextSignaturesRemainStable();
    void timeBasedModelTableAndHeaderSignaturesRemainStable();
    void timeBasedModelFrameEditingAndScrubSignaturesRemainStable();
    void timeBasedModelPlaybackSignaturesRemainStable();
};

void KisAnimUtilsSchemaContractTest::frameItemTypeAndDefaultStateRemainStable()
{
    using namespace KisAnimUtils;

    static_assert(std::is_class_v<FrameItem>);
    static_assert(std::is_default_constructible_v<FrameItem>);
    static_assert(std::is_same_v<decltype(FrameItem::node), KisNodeSP>);
    static_assert(std::is_same_v<decltype(FrameItem::channel), QString>);
    static_assert(std::is_same_v<decltype(FrameItem::time), int>);
    static_assert(std::is_same_v<FrameItemList, QVector<FrameItem>>);

    const FrameItem item;
    QVERIFY(!item.node);
    QVERIFY(item.channel.isEmpty());
    QCOMPARE(item.time, -1);
}

void KisAnimUtilsSchemaContractTest::frameItemValueAndComparisonContractsRemainStable()
{
    using namespace KisAnimUtils;
    using DebugFrame = QDebug (*)(QDebug, const FrameItem &);

    static_assert(std::is_constructible_v<FrameItem, KisNodeSP, const QString &, int>);
    static_assert(std::is_same_v<decltype(static_cast<DebugFrame>(&KisAnimUtils::operator<<)), DebugFrame>);
    static_assert(std::is_same_v<decltype(&FrameItem::operator==), bool (FrameItem::*)(const FrameItem &) const>);
    static_assert(std::is_same_v<decltype(&KisAnimUtils::qHash), uint (*)(const FrameItem &)>);
    static_assert(std::is_same_v<FrameMovePair, std::pair<FrameItem, FrameItem>>);
    static_assert(std::is_same_v<FrameMovePairList, QVector<FrameMovePair>>);

    const FrameItem item(KisNodeSP(), QStringLiteral("opacity"), 17);
    const FrameItem equal(KisNodeSP(), QStringLiteral("opacity"), 17);
    const FrameItem different(KisNodeSP(), QStringLiteral("opacity"), 18);
    QCOMPARE(item.channel, QStringLiteral("opacity"));
    QCOMPARE(item.time, 17);
    QVERIFY(item == equal);
    QVERIFY(!(item == different));
    QCOMPARE(KisAnimUtils::qHash(item), KisAnimUtils::qHash(equal));
}

void KisAnimUtilsSchemaContractTest::keyframeCreationAndMoveSignaturesRemainStable()
{
    using namespace KisAnimUtils;
    using CreateKeyframe = KUndo2Command *(*)(KisImageSP, KisNodeSP, const QString &, int, bool, KUndo2Command *);
    using CreateKeyframeLazy = void (*)(KisImageSP, KisNodeSP, const QString &, int, bool);
    using MoveFrameLists =
        KUndo2Command *(*)(const FrameItemList &, const FrameItemList &, bool, bool, KUndo2Command *);
    using MovePairs = KUndo2Command *(*)(const FrameMovePairList &, bool, bool, KUndo2Command *);
    using ClonePairs = KUndo2Command *(*)(const FrameMovePairList &, KUndo2Command *);

    static_assert(std::is_same_v<decltype(static_cast<CreateKeyframe>(&createKeyframeCommand)), CreateKeyframe>);
    static_assert(std::is_same_v<decltype(static_cast<CreateKeyframeLazy>(&createKeyframeLazy)), CreateKeyframeLazy>);
    static_assert(std::is_same_v<decltype(static_cast<MoveFrameLists>(&createMoveKeyframesCommand)), MoveFrameLists>);
    static_assert(std::is_same_v<decltype(static_cast<MovePairs>(&createMoveKeyframesCommand)), MovePairs>);
    static_assert(std::is_same_v<decltype(static_cast<ClonePairs>(&createCloneKeyframesCommand)), ClonePairs>);

    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::keyframeEditingAndQuerySignaturesRemainStable()
{
    using namespace KisAnimUtils;
    using RemoveFrames = void (*)(KisImageSP, const FrameItemList &);
    using RemoveFrame = void (*)(KisImageSP, KisNodeSP, const QString &, int);
    using ResetChannels = void (*)(KisImageSP, KisNodeSP, const QList<QString> &);
    using ResetChannel = void (*)(KisImageSP, KisNodeSP, const QString &);
    using SortPoints = void (*)(QModelIndexList *, const QPoint &);
    using MakeClonesUnique = void (*)(KisImageSP, const FrameItemList &);
    using SupportsContentFrames = bool (*)(KisNodeSP);

    static_assert(std::is_same_v<decltype(&removeKeyframes), RemoveFrames>);
    static_assert(std::is_same_v<decltype(&removeKeyframe), RemoveFrame>);
    static_assert(std::is_same_v<decltype(&resetChannels), ResetChannels>);
    static_assert(std::is_same_v<decltype(&resetChannel), ResetChannel>);
    static_assert(std::is_same_v<decltype(&sortPointsForSafeMove), SortPoints>);
    static_assert(std::is_same_v<decltype(&makeClonesUnique), MakeClonesUnique>);
    static_assert(std::is_same_v<decltype(&supportsContentFrames), SupportsContentFrames>);

    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::animationActionNameTypesRemainStable()
{
    using namespace KisAnimUtils;

    static_assert(std::is_same_v<decltype(lazyFrameCreationActionName), const QString>);
    static_assert(std::is_same_v<decltype(dropFramesActionName), const QString>);
    static_assert(std::is_same_v<decltype(newLayerActionName), const QString>);
    static_assert(std::is_same_v<decltype(pinExistingLayerActionName), const QString>);
    static_assert(std::is_same_v<decltype(removeLayerActionName), const QString>);
    static_assert(std::is_same_v<decltype(addOpacityKeyframeActionName), const QString>);
    static_assert(std::is_same_v<decltype(addTransformKeyframeActionName), const QString>);
    static_assert(std::is_same_v<decltype(removeOpacityKeyframeActionName), const QString>);
    static_assert(std::is_same_v<decltype(removeTransformKeyframeActionName), const QString>);

    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::timeBasedModelTypeAndRoleSchemaRemainStable()
{
    using Role = KisTimeBasedItemModel::ItemDataRole;

    static_assert(std::is_class_v<KisTimeBasedItemModel>);
    static_assert(std::is_base_of_v<QAbstractTableModel, KisTimeBasedItemModel>);
    static_assert(std::is_abstract_v<KisTimeBasedItemModel>);
    static_assert(std::is_enum_v<Role>);
    static_assert(KisTimeBasedItemModel::ActiveFrameRole == Qt::UserRole + 101);
    static_assert(KisTimeBasedItemModel::ScrubToRole == KisTimeBasedItemModel::ActiveFrameRole + 1);
    static_assert(KisTimeBasedItemModel::CloneOfActiveFrame == KisTimeBasedItemModel::ScrubToRole + 1);
    static_assert(KisTimeBasedItemModel::CloneCount == KisTimeBasedItemModel::CloneOfActiveFrame + 1);
    static_assert(KisTimeBasedItemModel::FrameExistsRole == KisTimeBasedItemModel::CloneCount + 1);
    static_assert(KisTimeBasedItemModel::SpecialKeyframeExists == KisTimeBasedItemModel::FrameExistsRole + 1);
    static_assert(KisTimeBasedItemModel::FrameCachedRole == KisTimeBasedItemModel::SpecialKeyframeExists + 1);
    static_assert(KisTimeBasedItemModel::FrameEditableRole == KisTimeBasedItemModel::FrameCachedRole + 1);
    static_assert(KisTimeBasedItemModel::FramesPerSecondRole == KisTimeBasedItemModel::FrameEditableRole + 1);
    static_assert(KisTimeBasedItemModel::FrameHasContent == KisTimeBasedItemModel::FramesPerSecondRole + 1);
    static_assert(KisTimeBasedItemModel::WithinClipRange == KisTimeBasedItemModel::FrameHasContent + 1);
    static_assert(KisTimeBasedItemModel::UserRole == KisTimeBasedItemModel::WithinClipRange + 1);

    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::timeBasedModelOwnershipAndContextSignaturesRemainStable()
{
    using SetImage = void (KisTimeBasedItemModel::*)(KisImageWSP);
    using SetFrameCache = void (KisTimeBasedItemModel::*)(KisAnimationFrameCacheSP);
    using IsFrameCached = bool (KisTimeBasedItemModel::*)(int);
    using SetAnimationPlayer = void (KisTimeBasedItemModel::*)(KisCanvasAnimationState *);
    using SetDocument = void (KisTimeBasedItemModel::*)(KisDocument *);
    using Document = KisDocument *(KisTimeBasedItemModel::*)() const;

    static_assert(std::is_constructible_v<ConcreteTimeBasedItemModel, QObject *>);
    static_assert(std::has_virtual_destructor_v<KisTimeBasedItemModel>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setImage), SetImage>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setFrameCache), SetFrameCache>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::isFrameCached), IsFrameCached>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setAnimationPlayer), SetAnimationPlayer>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setDocument), SetDocument>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::document), Document>);

    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::timeBasedModelTableAndHeaderSignaturesRemainStable()
{
    using SetLastVisibleFrame = void (KisTimeBasedItemModel::*)(int);
    using ColumnCount = int (KisTimeBasedItemModel::*)(const QModelIndex &) const;
    using Data = QVariant (KisTimeBasedItemModel::*)(const QModelIndex &, int) const;
    using SetData = bool (KisTimeBasedItemModel::*)(const QModelIndex &, const QVariant &, int);
    using HeaderData = QVariant (KisTimeBasedItemModel::*)(int, Qt::Orientation, int) const;
    using SetHeaderData = bool (KisTimeBasedItemModel::*)(int, Qt::Orientation, const QVariant &, int);
    using ScrubHeader = void (KisTimeBasedItemModel::*)(int);

    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setLastVisibleFrame), SetLastVisibleFrame>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::columnCount), ColumnCount>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::data), Data>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setData), SetData>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::headerData), HeaderData>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setHeaderData), SetHeaderData>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::scrubHorizontalHeaderUpdate), ScrubHeader>);

    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::timeBasedModelFrameEditingAndScrubSignaturesRemainStable()
{
    using RemoveFrames = bool (KisTimeBasedItemModel::*)(const QModelIndexList &);
    using RemoveFramesAndOffset = bool (KisTimeBasedItemModel::*)(QModelIndexList);
    using MirrorFrames = bool (KisTimeBasedItemModel::*)(QModelIndexList);
    using SetScrubState = void (KisTimeBasedItemModel::*)(bool);
    using IsScrubbing = bool (KisTimeBasedItemModel::*)();

    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::removeFrames), RemoveFrames>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::removeFramesAndOffset), RemoveFramesAndOffset>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::mirrorFrames), MirrorFrames>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setScrubState), SetScrubState>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::isScrubbing), IsScrubbing>);

    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::timeBasedModelPlaybackSignaturesRemainStable()
{
    using SetPlaybackRange = void (KisTimeBasedItemModel::*)(const KisTimeSpan &);
    using PlaybackQuery = bool (KisTimeBasedItemModel::*)() const;
    using StopPlayback = void (KisTimeBasedItemModel::*)() const;
    using CurrentTime = int (KisTimeBasedItemModel::*)() const;

    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::setPlaybackRange), SetPlaybackRange>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::isPlaybackActive), PlaybackQuery>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::isPlaybackPaused), PlaybackQuery>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::stopPlayback), StopPlayback>);
    static_assert(std::is_same_v<decltype(&KisTimeBasedItemModel::currentTime), CurrentTime>);

    QVERIFY(true);
}

QTEST_MAIN(KisAnimUtilsSchemaContractTest)

#include "KisAnimUtilsSchemaContractTest.moc"
