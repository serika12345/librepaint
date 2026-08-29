/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_transform_mask_params_interface.h"

#include <QDomDocument>
#include <QTest>
#include <QTransform>
#include <QWeakPointer>

#include <type_traits>

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

void kisSharedPtrAddReference(KisPaintDevice *)
{
}

bool kisSharedPtrRelease(KisPaintDevice *)
{
    return true;
}

KisDefaultBoundsBase::~KisDefaultBoundsBase() = default;

QRect KisDefaultBoundsBase::imageBorderRect() const
{
    return bounds();
}

namespace
{
class DefaultBoundsProbe final : public KisDefaultBoundsBase
{
public:
    explicit DefaultBoundsProbe(bool *destroyed = nullptr)
        : m_destroyed(destroyed)
    {
    }

    ~DefaultBoundsProbe() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    QRect bounds() const override
    {
        return QRect(2, 3, 40, 50);
    }

    bool wrapAroundMode() const override
    {
        return false;
    }

    WrapAroundAxis wrapAroundModeAxis() const override
    {
        return WRAPAROUND_BOTH;
    }

    int currentLevelOfDetail() const override
    {
        return 0;
    }

    int currentTime() const override
    {
        return 17;
    }

    bool externalFrameActive() const override
    {
        return false;
    }

    void *sourceCookie() const override
    {
        return nullptr;
    }

private:
    bool *m_destroyed;
};

class TransformParamsProbe final : public KisTransformMaskParamsInterface
{
public:
    explicit TransformParamsProbe(bool *destroyed = nullptr)
        : destroyed(destroyed)
    {
    }

    ~TransformParamsProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    QTransform finalAffineTransform() const override
    {
        ++finalAffineTransformCalls;
        return affineTransformResult;
    }

    bool isAffine() const override
    {
        ++isAffineCalls;
        return affineResult;
    }

    bool isHidden() const override
    {
        ++isHiddenCalls;
        return hidden;
    }

    void setHidden(bool value) override
    {
        ++setHiddenCalls;
        hidden = value;
    }

    void transformDevice(KisNodeSP node,
                         KisPaintDeviceSP src,
                         KisPaintDeviceSP dst,
                         bool forceSubPixelTranslation) const override
    {
        ++transformDeviceCalls;
        transformedNode = node.data();
        transformedSource = src.data();
        transformedDestination = dst.data();
        transformedWithSubPixelTranslation = forceSubPixelTranslation;
    }

    QString id() const override
    {
        ++idCalls;
        return idResult;
    }

    void toXML(QDomElement *element) const override
    {
        ++toXmlCalls;
        receivedElement = element;
    }

    void translateSrcAndDst(const QPointF &offset) override
    {
        ++translateSourceAndDestinationCalls;
        sourceAndDestinationOffset = offset;
    }

    void transformSrcAndDst(const QTransform &transform) override
    {
        ++transformSourceAndDestinationCalls;
        sourceAndDestinationTransform = transform;
    }

    void translateDstSpace(const QPointF &offset) override
    {
        ++translateDestinationCalls;
        destinationOffset = offset;
    }

    QRect nonAffineChangeRect(const QRect &rect) override
    {
        ++nonAffineChangeRectCalls;
        changeRectArgument = rect;
        return changeRectResult;
    }

    QRect nonAffineNeedRect(const QRect &rect, const QRect &sourceBounds) override
    {
        ++nonAffineNeedRectCalls;
        needRectArgument = rect;
        needRectSourceBounds = sourceBounds;
        return needRectResult;
    }

    bool compareTransform(KisTransformMaskParamsInterfaceSP rhs) const override
    {
        ++compareTransformCalls;
        comparedTransform = rhs.data();
        return compareResult;
    }

    KisTransformMaskParamsInterfaceSP clone() const override
    {
        ++cloneCalls;
        return cloneResult;
    }

    bool *destroyed = nullptr;
    QTransform affineTransformResult;
    bool affineResult = false;
    bool hidden = false;
    QString idResult;
    QRect changeRectResult;
    QRect needRectResult;
    bool compareResult = false;
    KisTransformMaskParamsInterfaceSP cloneResult;

    mutable int finalAffineTransformCalls = 0;
    mutable int isAffineCalls = 0;
    mutable int isHiddenCalls = 0;
    int setHiddenCalls = 0;
    mutable int transformDeviceCalls = 0;
    mutable int idCalls = 0;
    mutable int toXmlCalls = 0;
    int translateSourceAndDestinationCalls = 0;
    int transformSourceAndDestinationCalls = 0;
    int translateDestinationCalls = 0;
    int nonAffineChangeRectCalls = 0;
    int nonAffineNeedRectCalls = 0;
    mutable int compareTransformCalls = 0;
    mutable int cloneCalls = 0;

    mutable KisNode *transformedNode = nullptr;
    mutable KisPaintDevice *transformedSource = nullptr;
    mutable KisPaintDevice *transformedDestination = nullptr;
    mutable bool transformedWithSubPixelTranslation = false;
    mutable QDomElement *receivedElement = nullptr;
    QPointF sourceAndDestinationOffset;
    QTransform sourceAndDestinationTransform;
    QPointF destinationOffset;
    QRect changeRectArgument;
    QRect needRectArgument;
    QRect needRectSourceBounds;
    mutable KisTransformMaskParamsInterface *comparedTransform = nullptr;
};

class AnimatedParamsHolderProbe final : public KisAnimatedTransformParamsHolderInterface
{
public:
    explicit AnimatedParamsHolderProbe(bool *destroyed = nullptr)
        : destroyed(destroyed)
    {
    }

    ~AnimatedParamsHolderProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    bool isAnimated() const override
    {
        ++isAnimatedCalls;
        return animatedResult;
    }

    KisKeyframeChannel *requestKeyframeChannel(const QString &id) override
    {
        ++requestKeyframeChannelCalls;
        requestedChannelId = id;
        return requestedChannelResult;
    }

    KisKeyframeChannel *getKeyframeChannel(const QString &id) const override
    {
        ++getKeyframeChannelCalls;
        fetchedChannelId = id;
        return fetchedChannelResult;
    }

    KisTransformMaskParamsInterfaceSP bakeIntoParams() const override
    {
        ++bakeIntoParamsCalls;
        return bakedParamsResult;
    }

    void setParamsAtCurrentPosition(const KisTransformMaskParamsInterface *params,
                                    KUndo2Command *parentCommand) override
    {
        ++setParamsAtCurrentPositionCalls;
        receivedParams = params;
        receivedParentCommand = parentCommand;
    }

    KisAnimatedTransformParamsHolderInterfaceSP clone() const override
    {
        ++cloneCalls;
        return cloneResult;
    }

    void setDefaultBounds(KisDefaultBoundsBaseSP bounds) override
    {
        ++setDefaultBoundsCalls;
        defaultBoundsValue = bounds;
    }

    KisDefaultBoundsBaseSP defaultBounds() const override
    {
        ++defaultBoundsCalls;
        return defaultBoundsValue;
    }

    void syncLodCache() override
    {
        ++syncLodCacheCalls;
    }

    bool *destroyed = nullptr;
    bool animatedResult = false;
    KisKeyframeChannel *requestedChannelResult = nullptr;
    KisKeyframeChannel *fetchedChannelResult = nullptr;
    KisTransformMaskParamsInterfaceSP bakedParamsResult;
    KisAnimatedTransformParamsHolderInterfaceSP cloneResult;
    KisDefaultBoundsBaseSP defaultBoundsValue;

    mutable int isAnimatedCalls = 0;
    int requestKeyframeChannelCalls = 0;
    mutable int getKeyframeChannelCalls = 0;
    mutable int bakeIntoParamsCalls = 0;
    int setParamsAtCurrentPositionCalls = 0;
    mutable int cloneCalls = 0;
    int setDefaultBoundsCalls = 0;
    mutable int defaultBoundsCalls = 0;
    int syncLodCacheCalls = 0;

    QString requestedChannelId;
    mutable QString fetchedChannelId;
    const KisTransformMaskParamsInterface *receivedParams = nullptr;
    KUndo2Command *receivedParentCommand = nullptr;
};
} // namespace

class KisTransformMaskParamsInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aliasesAndBaseOwnershipArePreserved();
    void transformStateAndValuesAreDispatched();
    void transformOperationsPreserveArgumentsAndResults();
    void animatedHolderPreservesArgumentsResultsAndBounds();
};

void KisTransformMaskParamsInterfaceContractTest::aliasesAndBaseOwnershipArePreserved()
{
    static_assert(std::is_same_v<KisTransformMaskParamsInterfaceSP, QSharedPointer<KisTransformMaskParamsInterface>>);
    static_assert(std::is_same_v<KisTransformMaskParamsInterfaceWSP, QWeakPointer<KisTransformMaskParamsInterface>>);
    static_assert(std::is_same_v<KisAnimatedTransformParamsHolderInterfaceSP,
                                 QSharedPointer<KisAnimatedTransformParamsHolderInterface>>);
    static_assert(std::is_same_v<KisAnimatedTransformParamsHolderInterfaceWSP,
                                 QWeakPointer<KisAnimatedTransformParamsHolderInterface>>);

    bool transformDestroyed = false;
    KisTransformMaskParamsInterface *transform = new TransformParamsProbe(&transformDestroyed);
    delete transform;
    QVERIFY(transformDestroyed);

    bool holderDestroyed = false;
    KisAnimatedTransformParamsHolderInterface *holder = new AnimatedParamsHolderProbe(&holderDestroyed);
    delete holder;
    QVERIFY(holderDestroyed);

    const KisTransformMaskParamsInterfaceSP transformStrong(new TransformParamsProbe);
    const KisTransformMaskParamsInterfaceWSP transformWeak(transformStrong);
    QCOMPARE(transformWeak.toStrongRef().data(), transformStrong.data());

    const KisAnimatedTransformParamsHolderInterfaceSP holderStrong(new AnimatedParamsHolderProbe);
    const KisAnimatedTransformParamsHolderInterfaceWSP holderWeak(holderStrong);
    QCOMPARE(holderWeak.toStrongRef().data(), holderStrong.data());
}

void KisTransformMaskParamsInterfaceContractTest::transformStateAndValuesAreDispatched()
{
    TransformParamsProbe probe;
    probe.affineTransformResult = QTransform(1.5, 0.25, -0.5, 2.0, 7.0, -3.0);
    probe.affineResult = true;
    probe.hidden = false;
    probe.idResult = QStringLiteral("probe-transform");

    KisTransformMaskParamsInterface *params = &probe;
    QCOMPARE(params->finalAffineTransform(), probe.affineTransformResult);
    QCOMPARE(probe.finalAffineTransformCalls, 1);
    QVERIFY(params->isAffine());
    QCOMPARE(probe.isAffineCalls, 1);
    QVERIFY(!params->isHidden());
    QCOMPARE(probe.isHiddenCalls, 1);

    params->setHidden(true);
    QCOMPARE(probe.setHiddenCalls, 1);
    QVERIFY(params->isHidden());
    QCOMPARE(probe.isHiddenCalls, 2);

    QCOMPARE(params->id(), QStringLiteral("probe-transform"));
    QCOMPARE(probe.idCalls, 1);
}

void KisTransformMaskParamsInterfaceContractTest::transformOperationsPreserveArgumentsAndResults()
{
    TransformParamsProbe probe;
    TransformParamsProbe comparisonProbe;
    TransformParamsProbe cloneProbe;
    const KisTransformMaskParamsInterfaceSP comparison(&comparisonProbe, [](KisTransformMaskParamsInterface *) { });
    const KisTransformMaskParamsInterfaceSP clone(&cloneProbe, [](KisTransformMaskParamsInterface *) { });
    probe.compareResult = true;
    probe.cloneResult = clone;
    probe.changeRectResult = QRect(11, 13, 17, 19);
    probe.needRectResult = QRect(-7, 5, 31, 37);

    alignas(void *) unsigned char nodeStorage = 0;
    alignas(void *) unsigned char sourceStorage = 0;
    alignas(void *) unsigned char destinationStorage = 0;
    const KisNodeSP node(reinterpret_cast<KisNode *>(&nodeStorage));
    const KisPaintDeviceSP source(reinterpret_cast<KisPaintDevice *>(&sourceStorage));
    const KisPaintDeviceSP destination(reinterpret_cast<KisPaintDevice *>(&destinationStorage));

    KisTransformMaskParamsInterface *params = &probe;
    params->transformDevice(node, source, destination, true);
    QCOMPARE(probe.transformDeviceCalls, 1);
    QCOMPARE(probe.transformedNode, node.data());
    QCOMPARE(probe.transformedSource, source.data());
    QCOMPARE(probe.transformedDestination, destination.data());
    QVERIFY(probe.transformedWithSubPixelTranslation);

    QDomDocument document;
    QDomElement element = document.createElement(QStringLiteral("transform"));
    params->toXML(&element);
    QCOMPARE(probe.toXmlCalls, 1);
    QCOMPARE(probe.receivedElement, &element);

    const QPointF sourceAndDestinationOffset(4.25, -8.5);
    params->translateSrcAndDst(sourceAndDestinationOffset);
    QCOMPARE(probe.translateSourceAndDestinationCalls, 1);
    QCOMPARE(probe.sourceAndDestinationOffset, sourceAndDestinationOffset);

    const QTransform sourceAndDestinationTransform(0.5, 1.0, -1.5, 2.5, 9.0, 12.0);
    params->transformSrcAndDst(sourceAndDestinationTransform);
    QCOMPARE(probe.transformSourceAndDestinationCalls, 1);
    QCOMPARE(probe.sourceAndDestinationTransform, sourceAndDestinationTransform);

    const QPointF destinationOffset(-3.75, 6.125);
    params->translateDstSpace(destinationOffset);
    QCOMPARE(probe.translateDestinationCalls, 1);
    QCOMPARE(probe.destinationOffset, destinationOffset);

    const QRect changedRect(2, 3, 5, 7);
    QCOMPARE(params->nonAffineChangeRect(changedRect), probe.changeRectResult);
    QCOMPARE(probe.nonAffineChangeRectCalls, 1);
    QCOMPARE(probe.changeRectArgument, changedRect);

    const QRect neededRect(-2, 4, 23, 29);
    const QRect sourceBounds(-50, -40, 100, 80);
    QCOMPARE(params->nonAffineNeedRect(neededRect, sourceBounds), probe.needRectResult);
    QCOMPARE(probe.nonAffineNeedRectCalls, 1);
    QCOMPARE(probe.needRectArgument, neededRect);
    QCOMPARE(probe.needRectSourceBounds, sourceBounds);

    QVERIFY(params->compareTransform(comparison));
    QCOMPARE(probe.compareTransformCalls, 1);
    QCOMPARE(probe.comparedTransform, comparison.data());
    QCOMPARE(params->clone().data(), clone.data());
    QCOMPARE(probe.cloneCalls, 1);
}

void KisTransformMaskParamsInterfaceContractTest::animatedHolderPreservesArgumentsResultsAndBounds()
{
    AnimatedParamsHolderProbe probe;
    TransformParamsProbe paramsProbe;
    TransformParamsProbe bakedProbe;
    AnimatedParamsHolderProbe clonedHolderProbe;
    const KisTransformMaskParamsInterfaceSP bakedParams(&bakedProbe, [](KisTransformMaskParamsInterface *) { });
    const KisAnimatedTransformParamsHolderInterfaceSP clonedHolder(&clonedHolderProbe,
                                                                   [](KisAnimatedTransformParamsHolderInterface *) { });
    probe.animatedResult = true;
    probe.bakedParamsResult = bakedParams;
    probe.cloneResult = clonedHolder;

    alignas(void *) unsigned char requestedChannelStorage = 0;
    alignas(void *) unsigned char fetchedChannelStorage = 0;
    alignas(void *) unsigned char commandStorage = 0;
    probe.requestedChannelResult = reinterpret_cast<KisKeyframeChannel *>(&requestedChannelStorage);
    probe.fetchedChannelResult = reinterpret_cast<KisKeyframeChannel *>(&fetchedChannelStorage);
    KUndo2Command *parentCommand = reinterpret_cast<KUndo2Command *>(&commandStorage);

    KisAnimatedTransformParamsHolderInterface *holder = &probe;
    QVERIFY(holder->isAnimated());
    QCOMPARE(probe.isAnimatedCalls, 1);

    QCOMPARE(holder->requestKeyframeChannel(QStringLiteral("requested-channel")), probe.requestedChannelResult);
    QCOMPARE(probe.requestKeyframeChannelCalls, 1);
    QCOMPARE(probe.requestedChannelId, QStringLiteral("requested-channel"));

    QCOMPARE(holder->getKeyframeChannel(QStringLiteral("fetched-channel")), probe.fetchedChannelResult);
    QCOMPARE(probe.getKeyframeChannelCalls, 1);
    QCOMPARE(probe.fetchedChannelId, QStringLiteral("fetched-channel"));

    QCOMPARE(holder->bakeIntoParams().data(), bakedParams.data());
    QCOMPARE(probe.bakeIntoParamsCalls, 1);

    holder->setParamsAtCurrentPosition(&paramsProbe, parentCommand);
    QCOMPARE(probe.setParamsAtCurrentPositionCalls, 1);
    QCOMPARE(probe.receivedParams, &paramsProbe);
    QCOMPARE(probe.receivedParentCommand, parentCommand);

    QCOMPARE(holder->clone().data(), clonedHolder.data());
    QCOMPARE(probe.cloneCalls, 1);

    bool boundsDestroyed = false;
    KisDefaultBoundsBaseSP bounds = new DefaultBoundsProbe(&boundsDestroyed);
    KisDefaultBoundsBase *boundsPointer = bounds.data();
    holder->setDefaultBounds(bounds);
    QCOMPARE(probe.setDefaultBoundsCalls, 1);
    bounds.clear();
    QVERIFY(!boundsDestroyed);

    KisDefaultBoundsBaseSP returnedBounds = holder->defaultBounds();
    QCOMPARE(probe.defaultBoundsCalls, 1);
    QCOMPARE(returnedBounds.data(), boundsPointer);
    returnedBounds.clear();
    QVERIFY(!boundsDestroyed);

    holder->syncLodCache();
    holder->syncLodCache();
    QCOMPARE(probe.syncLodCacheCalls, 2);

    probe.defaultBoundsValue.clear();
    QVERIFY(boundsDestroyed);
}

QTEST_GUILESS_MAIN(KisTransformMaskParamsInterfaceContractTest)

#include "KisTransformMaskParamsInterfaceContractTest.moc"
