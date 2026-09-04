/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../KisAnimatedTransformMaskParamsHolder.h"
#include "../tool_transform_args.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TRANSFORM_ARGS_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&ToolTransformArgs::method)), signature>)
#define ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(method, signature)                                                  \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KisAnimatedTransformMaskParamsHolder::method)), signature>)
} // namespace

class ToolTransformStateSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void transformArgsTypeLifetimeAndConstructionSchemaRemainStable();
    void transformArgsExternalSourceFilterAndPersistenceSchemaRemainStable();
    void animatedTransformHolderTypeLifetimeAndConstructionSchemaRemainStable();
    void animatedTransformHolderStateBoundsAndKeyframeSchemaRemainStable();
    void animatedTransformHolderCloneBakeAndMutationSchemaRemainStable();
};

void ToolTransformStateSchemaContractTest::transformArgsTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_default_constructible_v<ToolTransformArgs>);
    static_assert(std::is_copy_constructible_v<ToolTransformArgs>);
    static_assert(std::is_constructible_v<ToolTransformArgs,
                                          ToolTransformArgs::TransformMode,
                                          QPointF,
                                          QPointF,
                                          QPointF,
                                          bool,
                                          double,
                                          double,
                                          double,
                                          double,
                                          double,
                                          double,
                                          double,
                                          double,
                                          KisWarpTransformWorker::WarpType,
                                          double,
                                          bool,
                                          const QString &,
                                          int,
                                          int,
                                          KisPaintDeviceSP>);
    static_assert(std::is_destructible_v<ToolTransformArgs>);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(operator=, ToolTransformArgs & (ToolTransformArgs::*)(const ToolTransformArgs &));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(clone, KisToolChangesTrackerData * (ToolTransformArgs::*)() const);
}

void ToolTransformStateSchemaContractTest::transformArgsExternalSourceFilterAndPersistenceSchemaRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(externalSource, KisPaintDeviceSP (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(filter, KisFilterStrategy * (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(filterId, QString (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(fromXML, ToolTransformArgs (*)(const QDomElement &));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setExternalSource, void (ToolTransformArgs::*)(KisPaintDeviceSP));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setFilterId, void (ToolTransformArgs::*)(const QString &));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(toXML, void (ToolTransformArgs::*)(QDomElement *) const);
}

void ToolTransformStateSchemaContractTest::animatedTransformHolderTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAnimatedTransformMaskParamsHolder>);
    static_assert(std::is_base_of_v<KisAnimatedTransformParamsHolderInterface, KisAnimatedTransformMaskParamsHolder>);
    static_assert(std::is_constructible_v<KisAnimatedTransformMaskParamsHolder, KisDefaultBoundsBaseSP>);
    static_assert(std::is_copy_constructible_v<KisAnimatedTransformMaskParamsHolder>);
    static_assert(std::has_virtual_destructor_v<KisAnimatedTransformMaskParamsHolder>);
}

void ToolTransformStateSchemaContractTest::animatedTransformHolderStateBoundsAndKeyframeSchemaRemainStable()
{
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(defaultBounds,
                                               KisDefaultBoundsBaseSP (KisAnimatedTransformMaskParamsHolder::*)()
                                                   const);
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(getKeyframeChannel,
                                               KisKeyframeChannel
                                                   * (KisAnimatedTransformMaskParamsHolder::*)(const QString &) const);
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(isAnimated, bool (KisAnimatedTransformMaskParamsHolder::*)() const);
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(requestKeyframeChannel,
                                               KisKeyframeChannel
                                                   * (KisAnimatedTransformMaskParamsHolder::*)(const QString &));
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(setDefaultBounds,
                                               void (KisAnimatedTransformMaskParamsHolder::*)(KisDefaultBoundsBaseSP));
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(
        transformArgs,
        const QSharedPointer<ToolTransformArgs> (KisAnimatedTransformMaskParamsHolder::*)() const);
}

void ToolTransformStateSchemaContractTest::animatedTransformHolderCloneBakeAndMutationSchemaRemainStable()
{
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(
        bakeIntoParams,
        KisTransformMaskParamsInterfaceSP (KisAnimatedTransformMaskParamsHolder::*)() const);
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(
        clone,
        KisAnimatedTransformParamsHolderInterfaceSP (KisAnimatedTransformMaskParamsHolder::*)() const);
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(
        setParamsAtCurrentPosition,
        void (KisAnimatedTransformMaskParamsHolder::*)(const KisTransformMaskParamsInterface *, KUndo2Command *));
    ASSERT_ANIMATED_TRANSFORM_HOLDER_SIGNATURE(syncLodCache, void (KisAnimatedTransformMaskParamsHolder::*)());
}

QTEST_MAIN(ToolTransformStateSchemaContractTest)

#include "ToolTransformStateSchemaContractTest.moc"
