/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "KisAnimUtils.h"

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

class KisAnimUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void frameItemTypeAndDefaultStateRemainStable();
    void frameItemValueAndComparisonContractsRemainStable();
    void keyframeCreationAndMoveSignaturesRemainStable();
    void keyframeEditingAndQuerySignaturesRemainStable();
    void animationActionNameTypesRemainStable();
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

QTEST_MAIN(KisAnimUtilsSchemaContractTest)

#include "KisAnimUtilsSchemaContractTest.moc"
