/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "layerstyles/kis_layer_style_projection_plane.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(method, signature)                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerStyleProjectionPlane::method)), signature>)
} // namespace

class KisLayerStyleProjectionPlaneSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void recalculationApplicationAndFactorySignaturesRemainStable();
    void dependencyRectangleSignaturesRemainStable();
    void visibleBoundsSignaturesRemainStable();
    void lodDeviceSignatureRemainsStable();
};

void KisLayerStyleProjectionPlaneSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Plane = KisLayerStyleProjectionPlane;

    static_assert(std::is_class_v<Plane>);
    static_assert(std::is_base_of_v<KisAbstractProjectionPlane, Plane>);
    static_assert(std::is_same_v<KisLayerStyleProjectionPlaneSP, QSharedPointer<Plane>>);
    static_assert(std::is_same_v<KisLayerStyleProjectionPlaneWSP, QWeakPointer<Plane>>);
    static_assert(std::is_constructible_v<Plane, KisLayer *>);
    static_assert(std::is_constructible_v<Plane, const Plane &, KisLayer *, KisPSDLayerStyleSP>);
    static_assert(std::has_virtual_destructor_v<Plane>);

    QVERIFY(true);
}

void KisLayerStyleProjectionPlaneSchemaContractTest::recalculationApplicationAndFactorySignaturesRemainStable()
{
    using Plane = KisLayerStyleProjectionPlane;

    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(recalculate,
                                            QRect (Plane::*)(const QRect &, KisNodeSP, KisRenderPassFlags));
    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(apply, void (Plane::*)(KisPainter *, const QRect &));
    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(factoryObject, KisAbstractProjectionPlaneSP (*)(KisLayer *));
}

void KisLayerStyleProjectionPlaneSchemaContractTest::dependencyRectangleSignaturesRemainStable()
{
    using Plane = KisLayerStyleProjectionPlane;
    using Signature = QRect (Plane::*)(const QRect &, KisLayer::PositionToFilthy) const;

    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(accessRect, Signature);
    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(changeRect, Signature);
    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(needRect, Signature);
}

void KisLayerStyleProjectionPlaneSchemaContractTest::visibleBoundsSignaturesRemainStable()
{
    using Plane = KisLayerStyleProjectionPlane;

    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(needRectForOriginal, QRect (Plane::*)(const QRect &) const);
    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(looseUserVisibleBounds, QRect (Plane::*)() const);
    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(tightUserVisibleBounds, QRect (Plane::*)() const);
}

void KisLayerStyleProjectionPlaneSchemaContractTest::lodDeviceSignatureRemainsStable()
{
    using Plane = KisLayerStyleProjectionPlane;

    ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE(getLodCapableDevices, KisPaintDeviceList (Plane::*)() const);
}

#undef ASSERT_LAYER_STYLE_PROJECTION_SIGNATURE

QTEST_GUILESS_MAIN(KisLayerStyleProjectionPlaneSchemaContractTest)

#include "KisLayerStyleProjectionPlaneSchemaContractTest.moc"
