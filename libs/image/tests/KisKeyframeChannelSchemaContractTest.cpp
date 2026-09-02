/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_keyframe_channel.h"
#include "kis_scalar_keyframe_channel.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace KisKeyframeChannelContractTypes
{

class KeyframeChannelConstructionProbe final : public KisKeyframeChannel
{
public:
    using KisKeyframeChannel::KisKeyframeChannel;

    explicit KeyframeChannelConstructionProbe(const KisKeyframeChannel &rhs)
        : KisKeyframeChannel(rhs)
    {
    }

    QRect affectedRect(int) const override;

private:
    KisKeyframeSP createKeyframe() override;
    QPair<int, KisKeyframeSP> loadKeyframe(const QDomElement &) override;
    void saveKeyframe(KisKeyframeSP, QDomElement, const QString &) override;
};

} // namespace KisKeyframeChannelContractTypes

class KisKeyframeChannelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keyframeChannelTypeLifetimeAndIdentifierSchemaRemainsStable();
    void keyframeChannelLookupAndTimelineSchemaRemainsStable();
    void keyframeChannelMutationSchemaRemainsStable();
    void keyframeChannelOwnershipFrameAndSerializationSchemaRemainsStable();
    void keyframeChannelNotificationSchemaRemainsStable();
    void scalarKeyframeLimitsValueSchemaRemainsStable();
    void scalarKeyframeTypeAndInterpolationSchemaRemainsStable();
    void scalarKeyframeValueAndTangentSchemaRemainsStable();
    void scalarKeyframeChannelTypeAndPolicySchemaRemainsStable();
    void scalarKeyframeChannelEvaluationAndMutationSchemaRemainsStable();
};

void KisKeyframeChannelSchemaContractTest::keyframeChannelTypeLifetimeAndIdentifierSchemaRemainsStable()
{
    using Channel = KisKeyframeChannel;

    static_assert(std::is_class_v<Channel>);
    static_assert(std::is_abstract_v<Channel>);
    static_assert(std::has_virtual_destructor_v<Channel>);
    using ConstructionProbe = KisKeyframeChannelContractTypes::KeyframeChannelConstructionProbe;
    static_assert(std::is_constructible_v<ConstructionProbe, const KoID &, KisDefaultBoundsBaseSP>);
    static_assert(std::is_constructible_v<ConstructionProbe, const Channel &>);
    static_assert(std::is_same_v<decltype(Channel::Raster), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::Opacity), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::TransformArguments), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::PositionX), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::PositionY), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::ScaleX), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::ScaleY), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::ShearX), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::ShearY), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::RotationX), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::RotationY), const KoID>);
    static_assert(std::is_same_v<decltype(Channel::RotationZ), const KoID>);
}

void KisKeyframeChannelSchemaContractTest::keyframeChannelLookupAndTimelineSchemaRemainsStable()
{
    using Channel = KisKeyframeChannel;
    using ConstIntegerSignature = int (Channel::*)() const;

    static_assert(
        std::is_same_v<decltype(static_cast<KisKeyframeSP (Channel::*)(int) const>(&Channel::activeKeyframeAt)),
                       KisKeyframeSP (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(static_cast<ConstIntegerSignature>(&Channel::activeKeyframeTime)),
                                 ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(static_cast<int (Channel::*)(int) const>(&Channel::activeKeyframeTime)),
                                 int (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(static_cast<KisKeyframeSP (Channel::*)(int) const>(&Channel::keyframeAt)),
                                 KisKeyframeSP (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::keyframeCount), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&Channel::lookupKeyframeTime), int (Channel::*)(KisKeyframeSP)>);
    static_assert(std::is_same_v<decltype(&Channel::firstKeyframeTime), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&Channel::previousKeyframeTime), int (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::nextKeyframeTime), int (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::lastKeyframeTime), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&Channel::allKeyframeTimes), QSet<int> (Channel::*)() const>);
    static_assert(std::is_same_v<decltype(&Channel::id), QString (Channel::*)() const>);
    static_assert(std::is_same_v<decltype(&Channel::name), QString (Channel::*)() const>);
    static_assert(std::is_same_v<decltype(&Channel::channelHash), ConstIntegerSignature>);
}

void KisKeyframeChannelSchemaContractTest::keyframeChannelMutationSchemaRemainsStable()
{
    using Channel = KisKeyframeChannel;
    using LocalSignature = void (Channel::*)(int, int, KUndo2Command *);
    using MoveSignature = void (*)(Channel *, int, Channel *, int, KUndo2Command *);
    using CopySignature = void (*)(const Channel *, int, Channel *, int, KUndo2Command *);

    static_assert(std::is_same_v<decltype(&Channel::addKeyframe), void (Channel::*)(int, KUndo2Command *)>);
    static_assert(
        std::is_same_v<decltype(&Channel::insertKeyframe), void (Channel::*)(int, KisKeyframeSP, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Channel::removeKeyframe), void (Channel::*)(int, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(static_cast<LocalSignature>(&Channel::moveKeyframe)), LocalSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MoveSignature>(&Channel::moveKeyframe)), MoveSignature>);
    static_assert(std::is_same_v<decltype(static_cast<LocalSignature>(&Channel::copyKeyframe)), LocalSignature>);
    static_assert(std::is_same_v<decltype(static_cast<CopySignature>(&Channel::copyKeyframe)), CopySignature>);
    static_assert(std::is_same_v<decltype(static_cast<LocalSignature>(&Channel::swapKeyframes)), LocalSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MoveSignature>(&Channel::swapKeyframes)), MoveSignature>);

    static_assert(std::is_same_v<decltype(std::declval<Channel &>().addKeyframe(0)), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<Channel &>().insertKeyframe(0, std::declval<KisKeyframeSP>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Channel &>().removeKeyframe(0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<Channel &>().moveKeyframe(0, 0)), void>);
    static_assert(std::is_same_v<decltype(Channel::moveKeyframe(nullptr, 0, nullptr, 0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<Channel &>().copyKeyframe(0, 0)), void>);
    static_assert(std::is_same_v<decltype(Channel::copyKeyframe(nullptr, 0, nullptr, 0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<Channel &>().swapKeyframes(0, 0)), void>);
    static_assert(std::is_same_v<decltype(Channel::swapKeyframes(nullptr, 0, nullptr, 0)), void>);
}

void KisKeyframeChannelSchemaContractTest::keyframeChannelOwnershipFrameAndSerializationSchemaRemainsStable()
{
    using Channel = KisKeyframeChannel;

    static_assert(std::is_same_v<decltype(&Channel::setNode), void (Channel::*)(KisNodeWSP)>);
    static_assert(std::is_same_v<decltype(&Channel::node), KisNodeWSP (Channel::*)() const>);
    static_assert(std::is_same_v<decltype(&Channel::setDefaultBounds), void (Channel::*)(KisDefaultBoundsBaseSP)>);
    static_assert(std::is_same_v<decltype(&Channel::affectedFrames), KisTimeSpan (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::identicalFrames), KisTimeSpan (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::affectedRect), QRect (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::toXML), QDomElement (Channel::*)(QDomDocument, const QString &)>);
    static_assert(std::is_same_v<decltype(&Channel::loadXML), void (Channel::*)(const QDomElement &)>);
    static_assert(std::is_same_v<decltype(&Channel::channelIdToKoId), KoID (*)(const QString &)>);
}

void KisKeyframeChannelSchemaContractTest::keyframeChannelNotificationSchemaRemainsStable()
{
    using Channel = KisKeyframeChannel;
    using NotificationSignature = void (Channel::*)(const Channel *, int);

    static_assert(std::is_same_v<decltype(&Channel::sigAddedKeyframe), NotificationSignature>);
    static_assert(std::is_same_v<decltype(&Channel::sigKeyframeAboutToBeRemoved), NotificationSignature>);
    static_assert(std::is_same_v<decltype(&Channel::sigKeyframeHasBeenRemoved), NotificationSignature>);
    static_assert(std::is_same_v<decltype(&Channel::sigKeyframeChanged), NotificationSignature>);
    static_assert(std::is_same_v<decltype(&Channel::sigAnyKeyframeChange), void (Channel::*)()>);
}

void KisKeyframeChannelSchemaContractTest::scalarKeyframeLimitsValueSchemaRemainsStable()
{
    static_assert(std::is_class_v<ScalarKeyframeLimits>);
    static_assert(std::is_same_v<decltype(ScalarKeyframeLimits::lower), qreal>);
    static_assert(std::is_same_v<decltype(ScalarKeyframeLimits::upper), qreal>);
    static_assert(std::is_constructible_v<ScalarKeyframeLimits, qreal, qreal>);
    static_assert(std::is_same_v<decltype(&ScalarKeyframeLimits::clamp), qreal (ScalarKeyframeLimits::*)(qreal)>);

    ScalarKeyframeLimits limits(5.0, 1.0);
    QCOMPARE(limits.lower, 1.0);
    QCOMPARE(limits.upper, 5.0);
    QCOMPARE(limits.clamp(-1.0), 1.0);
    QCOMPARE(limits.clamp(3.0), 3.0);
    QCOMPARE(limits.clamp(8.0), 5.0);
}

void KisKeyframeChannelSchemaContractTest::scalarKeyframeTypeAndInterpolationSchemaRemainsStable()
{
    using Keyframe = KisScalarKeyframe;
    using LimitsSP = QSharedPointer<ScalarKeyframeLimits>;

    static_assert(std::is_class_v<Keyframe>);
    static_assert(std::is_enum_v<Keyframe::InterpolationMode>);
    static_assert(int(Keyframe::Constant) == 0);
    static_assert(int(Keyframe::Linear) == 1);
    static_assert(int(Keyframe::Bezier) == 2);
    static_assert(std::is_enum_v<Keyframe::TangentsMode>);
    static_assert(int(Keyframe::Sharp) == 0);
    static_assert(int(Keyframe::Smooth) == 1);
    static_assert(std::is_constructible_v<Keyframe, qreal, LimitsSP>);
    static_assert(std::is_constructible_v<Keyframe,
                                          qreal,
                                          Keyframe::InterpolationMode,
                                          Keyframe::TangentsMode,
                                          QPointF,
                                          QPointF,
                                          LimitsSP>);
    static_assert(std::is_same_v<decltype(Keyframe::valueChangedChannelConnection), QMetaObject::Connection>);
}

void KisKeyframeChannelSchemaContractTest::scalarKeyframeValueAndTangentSchemaRemainsStable()
{
    using Keyframe = KisScalarKeyframe;
    using LimitsSP = QSharedPointer<ScalarKeyframeLimits>;

    static_assert(std::is_same_v<decltype(&Keyframe::duplicate), KisKeyframeSP (Keyframe::*)(KisKeyframeChannel *)>);
    static_assert(std::is_same_v<decltype(&Keyframe::setValue), void (Keyframe::*)(qreal, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Keyframe::value), qreal (Keyframe::*)() const>);
    static_assert(std::is_same_v<decltype(&Keyframe::setInterpolationMode),
                                 void (Keyframe::*)(Keyframe::InterpolationMode, KUndo2Command *)>);
    static_assert(
        std::is_same_v<decltype(&Keyframe::interpolationMode), Keyframe::InterpolationMode (Keyframe::*)() const>);
    static_assert(std::is_same_v<decltype(&Keyframe::setTangentsMode),
                                 void (Keyframe::*)(Keyframe::TangentsMode, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Keyframe::tangentsMode), Keyframe::TangentsMode (Keyframe::*)() const>);
    static_assert(std::is_same_v<decltype(&Keyframe::setInterpolationTangents),
                                 void (Keyframe::*)(QPointF, QPointF, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Keyframe::leftTangent), QPointF (Keyframe::*)() const>);
    static_assert(std::is_same_v<decltype(&Keyframe::rightTangent), QPointF (Keyframe::*)() const>);
    static_assert(std::is_same_v<decltype(&Keyframe::setLimits), void (Keyframe::*)(LimitsSP)>);
    static_assert(std::is_same_v<decltype(&Keyframe::sigChanged), void (Keyframe::*)(const Keyframe *)>);

    static_assert(std::is_same_v<decltype(std::declval<Keyframe &>().duplicate()), KisKeyframeSP>);
    static_assert(std::is_same_v<decltype(std::declval<Keyframe &>().setValue(0.0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<Keyframe &>().setInterpolationMode(Keyframe::Constant)), void>);
    static_assert(std::is_same_v<decltype(std::declval<Keyframe &>().setTangentsMode(Keyframe::Sharp)), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<Keyframe &>().setInterpolationTangents(QPointF(), QPointF())), void>);
}

void KisKeyframeChannelSchemaContractTest::scalarKeyframeChannelTypeAndPolicySchemaRemainsStable()
{
    using Channel = KisScalarKeyframeChannel;
    using LimitsSP = QSharedPointer<ScalarKeyframeLimits>;

    static_assert(std::is_class_v<Channel>);
    static_assert(std::is_constructible_v<Channel, const KoID &, KisDefaultBoundsBaseSP>);
    static_assert(std::is_constructible_v<Channel, const Channel &>);
    static_assert(std::has_virtual_destructor_v<Channel>);
    static_assert(std::is_same_v<decltype(&Channel::limits), LimitsSP (Channel::*)() const>);
    static_assert(std::is_same_v<decltype(&Channel::setLimits), void (Channel::*)(qreal, qreal)>);
    static_assert(std::is_same_v<decltype(&Channel::removeLimits), void (Channel::*)()>);
    static_assert(std::is_same_v<decltype(&Channel::setDefaultValue), void (Channel::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Channel::setDefaultInterpolationMode),
                                 void (Channel::*)(KisScalarKeyframe::InterpolationMode)>);
    static_assert(std::is_same_v<decltype(&Channel::currentValue), qreal (Channel::*)()>);
    static_assert(std::is_same_v<decltype(&Channel::isCurrentTimeAffectedBy), bool (Channel::*)(int)>);
}

void KisKeyframeChannelSchemaContractTest::scalarKeyframeChannelEvaluationAndMutationSchemaRemainsStable()
{
    using Channel = KisScalarKeyframeChannel;

    static_assert(
        std::is_same_v<decltype(&Channel::addScalarKeyframe), void (Channel::*)(int, qreal, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Channel::valueAt), qreal (Channel::*)(int) const>);
    static_assert(
        std::is_same_v<decltype(&Channel::interpolate), QPointF (*)(QPointF, QPointF, QPointF, QPointF, qreal)>);
    static_assert(
        std::is_same_v<decltype(&Channel::insertKeyframe), void (Channel::*)(int, KisKeyframeSP, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Channel::removeKeyframe), void (Channel::*)(int, KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Channel::affectedFrames), KisTimeSpan (Channel::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Channel::identicalFrames), KisTimeSpan (Channel::*)(int) const>);

    static_assert(std::is_same_v<decltype(std::declval<Channel &>().addScalarKeyframe(0, 0.0)), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<Channel &>().insertKeyframe(0, std::declval<KisKeyframeSP>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Channel &>().removeKeyframe(0)), void>);
}

QTEST_GUILESS_MAIN(KisKeyframeChannelSchemaContractTest)

#include "KisKeyframeChannelSchemaContractTest.moc"
