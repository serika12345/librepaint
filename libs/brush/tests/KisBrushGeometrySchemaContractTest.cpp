/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_boundary.h>
#include <kis_scaling_size_brush.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_BOUNDARY_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBoundary::method)), signature>)
#define ASSERT_SCALING_BRUSH_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisScalingSizeBrush::method)), signature>)
} // namespace

class ScalingSizeBrushConstructionProbe final : public KisScalingSizeBrush
{
public:
    ScalingSizeBrushConstructionProbe()
        : KisScalingSizeBrush()
    {
    }

    explicit ScalingSizeBrushConstructionProbe(const QString &filename)
        : KisScalingSizeBrush(filename)
    {
    }

    ScalingSizeBrushConstructionProbe(const ScalingSizeBrushConstructionProbe &rhs)
        : KisScalingSizeBrush(rhs)
    {
    }

    KoResourceSP clone() const override;
    bool loadFromDevice(QIODevice *, KisResourcesInterfaceSP) override;
};

class KisBrushGeometrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void boundaryTypeAndLifetimeSchemaRemainStable();
    void boundaryGenerationAndRenderingSchemaRemainStable();
    void scalingBrushConstructionSchemaRemainsStable();
    void scalingBrushEffectiveSizeSchemaRemainsStable();
};

void KisBrushGeometrySchemaContractTest::boundaryTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisBoundary>);
    static_assert(std::is_constructible_v<KisBoundary, KisFixedPaintDeviceSP>);
    static_assert(std::is_destructible_v<KisBoundary>);
}

void KisBrushGeometrySchemaContractTest::boundaryGenerationAndRenderingSchemaRemainStable()
{
    ASSERT_BOUNDARY_SIGNATURE(generateBoundary, void (KisBoundary::*)());
    ASSERT_BOUNDARY_SIGNATURE(paint, void (KisBoundary::*)(QPainter &) const);
    ASSERT_BOUNDARY_SIGNATURE(path, QPainterPath (KisBoundary::*)() const);
}

void KisBrushGeometrySchemaContractTest::scalingBrushConstructionSchemaRemainsStable()
{
    using Probe = ScalingSizeBrushConstructionProbe;

    static_assert(std::is_class_v<KisScalingSizeBrush>);
    static_assert(std::is_default_constructible_v<Probe>);
    static_assert(std::is_constructible_v<Probe, const QString &>);
    static_assert(std::is_copy_constructible_v<Probe>);
}

void KisBrushGeometrySchemaContractTest::scalingBrushEffectiveSizeSchemaRemainsStable()
{
    ASSERT_SCALING_BRUSH_SIGNATURE(userEffectiveSize, qreal (KisScalingSizeBrush::*)() const);
    ASSERT_SCALING_BRUSH_SIGNATURE(setUserEffectiveSize, void (KisScalingSizeBrush::*)(qreal));
}

QTEST_GUILESS_MAIN(KisBrushGeometrySchemaContractTest)

#include "KisBrushGeometrySchemaContractTest.moc"
