/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>
#include <utility>

#include "kis_raster_keyframe_channel.h"

class KisRasterKeyframeChannelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rasterKeyframeTypeLifetimeAndContentSchemaRemainStable();
    void rasterKeyframeChannelTypeLifetimeAndConstructionSchemaRemainStable();
    void rasterKeyframeChannelCloneAndFrameMappingSignaturesRemainStable();
    void rasterKeyframeChannelPersistenceAndTransferSignaturesRemainStable();
    void rasterKeyframeChannelPresentationAndMutationSignaturesRemainStable();
};

void KisRasterKeyframeChannelSchemaContractTest::rasterKeyframeTypeLifetimeAndContentSchemaRemainStable()
{
    using Keyframe = KisRasterKeyframe;

    static_assert(std::is_class_v<Keyframe>);
    static_assert(std::is_base_of_v<KisKeyframe, Keyframe>);
    static_assert(std::is_constructible_v<Keyframe, KisPaintDeviceWSP>);
    static_assert(std::is_constructible_v<Keyframe, KisPaintDeviceWSP, const int &>);
    static_assert(std::is_constructible_v<Keyframe, KisPaintDeviceWSP, const int &, const int &>);
    static_assert(std::has_virtual_destructor_v<Keyframe>);
    static_assert(std::is_same_v<decltype(&Keyframe::frameID), int (Keyframe::*)() const>);
    static_assert(std::is_same_v<decltype(&Keyframe::hasContent), bool (Keyframe::*)()>);
    static_assert(std::is_same_v<decltype(&Keyframe::contentBounds), QRect (Keyframe::*)()>);
    static_assert(std::is_same_v<decltype(&Keyframe::writeFrameToDevice), void (Keyframe::*)(KisPaintDeviceSP)>);
    static_assert(std::is_same_v<decltype(&Keyframe::duplicate), KisKeyframeSP (Keyframe::*)(KisKeyframeChannel *)>);
    static_assert(std::is_same_v<decltype(std::declval<Keyframe &>().duplicate()), KisKeyframeSP>);
}

void KisRasterKeyframeChannelSchemaContractTest::rasterKeyframeChannelTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Channel = KisRasterKeyframeChannel;

    static_assert(std::is_class_v<Channel>);
    static_assert(std::is_base_of_v<KisKeyframeChannel, Channel>);
    static_assert(
        std::is_constructible_v<Channel, const KoID &, const KisPaintDeviceWSP, const KisDefaultBoundsBaseSP>);
    static_assert(std::is_constructible_v<Channel, const Channel &, const KisPaintDeviceWSP>);
    static_assert(std::has_virtual_destructor_v<Channel>);
}

void KisRasterKeyframeChannelSchemaContractTest::rasterKeyframeChannelCloneAndFrameMappingSignaturesRemainStable()
{
    using Channel = KisRasterKeyframeChannel;
    using CloneSignature = void (Channel::*)(int, int, KUndo2Command *);
    using MemberClonesSignature = QSet<int> (Channel::*)(int);
    using StaticClonesSignature = QSet<int> (*)(const KisNode *, int);

    static_assert(std::is_same_v<decltype(&Channel::frameExtents), QRect (Channel::*)(KisKeyframeSP)>);
    static_assert(std::is_same_v<decltype(&Channel::cloneKeyframe), CloneSignature>);
    static_assert(std::is_same_v<decltype(&Channel::areClones), bool (Channel::*)(int, int)>);
    static_assert(
        std::is_same_v<decltype(static_cast<MemberClonesSignature>(&Channel::clonesOf)), MemberClonesSignature>);
    static_assert(std::is_same_v<decltype(&Channel::timesForFrameID), QSet<int> (Channel::*)(int) const>);
    static_assert(
        std::is_same_v<decltype(static_cast<StaticClonesSignature>(&Channel::clonesOf)), StaticClonesSignature>);
    static_assert(std::is_same_v<decltype(&Channel::makeUnique), void (Channel::*)(int, KUndo2Command *)>);

    static_assert(std::is_same_v<decltype(std::declval<Channel &>().cloneKeyframe(0, 0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<Channel &>().makeUnique(0)), void>);
}

void KisRasterKeyframeChannelSchemaContractTest::rasterKeyframeChannelPersistenceAndTransferSignaturesRemainStable()
{
    using Channel = KisRasterKeyframeChannel;

    static_assert(std::is_same_v<decltype(&Channel::writeToDevice), void (Channel::*)(int, KisPaintDeviceSP)>);
    static_assert(
        std::is_same_v<decltype(&Channel::importFrame), void (Channel::*)(int, KisPaintDeviceSP, KUndo2Command *)>);
    static_assert(
        std::is_same_v<decltype(&Channel::insertKeyframe), void (Channel::*)(int, KisKeyframeSP, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Channel::removeKeyframe), void (Channel::*)(int, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Channel::toXML), QDomElement (Channel::*)(QDomDocument, const QString &)>);
    static_assert(std::is_same_v<decltype(&Channel::loadXML), void (Channel::*)(const QDomElement &)>);

    static_assert(
        std::is_same_v<decltype(std::declval<Channel &>().insertKeyframe(0, std::declval<KisKeyframeSP>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Channel &>().removeKeyframe(0)), void>);
}

void KisRasterKeyframeChannelSchemaContractTest::rasterKeyframeChannelPresentationAndMutationSignaturesRemainStable()
{
    using Channel = KisRasterKeyframeChannel;

    static_assert(std::is_same_v<decltype(&Channel::frameFilename), QString (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::setFilenameSuffix), void (Channel::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Channel::setOnionSkinsEnabled), void (Channel::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Channel::onionSkinsEnabled), bool (Channel::*)() const>);
    static_assert(std::is_same_v<decltype(&Channel::paintDevice), KisPaintDeviceWSP (Channel::*)()>);
}

QTEST_GUILESS_MAIN(KisRasterKeyframeChannelSchemaContractTest)

#include "KisRasterKeyframeChannelSchemaContractTest.moc"
