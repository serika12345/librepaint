/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisEncloseAndFillPainter.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisEncloseAndFillPainter::method)), signature>)

} // namespace

class KisEncloseAndFillPainterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void encloseAndFillPainterTypeLifetimeAndConstructionSchemaRemainStable();
    void encloseAndFillRegionSelectionMethodSchemaRemainStable();
    void encloseAndFillOperationSignaturesRemainStable();
    void encloseAndFillRegionSelectionQuerySignaturesRemainStable();
    void encloseAndFillRegionSelectionMutationSignaturesRemainStable();
};

void KisEncloseAndFillPainterSchemaContractTest::encloseAndFillPainterTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisEncloseAndFillPainter>);
    static_assert(std::is_base_of_v<KisFillPainter, KisEncloseAndFillPainter>);
    static_assert(std::is_constructible_v<KisEncloseAndFillPainter, const QSize &>);
    static_assert(std::is_constructible_v<KisEncloseAndFillPainter, KisPaintDeviceSP, const QSize &>);
    static_assert(std::is_constructible_v<KisEncloseAndFillPainter, KisPaintDeviceSP, KisSelectionSP, const QSize &>);
    static_assert(std::has_virtual_destructor_v<KisEncloseAndFillPainter>);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::encloseAndFillRegionSelectionMethodSchemaRemainStable()
{
    using Method = KisEncloseAndFillPainter::RegionSelectionMethod;

    static_assert(std::is_enum_v<Method>);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectAllRegions) == 0);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectRegionsFilledWithSpecificColor) == 1);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectRegionsFilledWithTransparent) == 2);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectRegionsFilledWithSpecificColorOrTransparent) == 3);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectAllRegionsExceptFilledWithSpecificColor) == 4);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectAllRegionsExceptFilledWithTransparent) == 5);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectAllRegionsExceptFilledWithSpecificColorOrTransparent)
                  == 6);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectRegionsSurroundedBySpecificColor) == 7);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectRegionsSurroundedByTransparent) == 8);
    static_assert(static_cast<int>(KisEncloseAndFillPainter::SelectRegionsSurroundedBySpecificColorOrTransparent) == 9);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::encloseAndFillOperationSignaturesRemainStable()
{
    using FillSignature = void (KisEncloseAndFillPainter::*)(KisPixelSelectionSP, KisPaintDeviceSP);
    using PatternSignature = void (KisEncloseAndFillPainter::*)(KisPixelSelectionSP, KisPaintDeviceSP, QTransform);
    using SelectionSignature =
        KisPixelSelectionSP (KisEncloseAndFillPainter::*)(KisPixelSelectionSP, KisPaintDeviceSP, KisPixelSelectionSP);
    using SelectionIntoSignature = KisPixelSelectionSP (
        KisEncloseAndFillPainter::*)(KisPixelSelectionSP, KisPixelSelectionSP, KisPaintDeviceSP, KisPixelSelectionSP);

    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(encloseAndFillColor, FillSignature);
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(encloseAndFillPattern, PatternSignature);
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(createEncloseAndFillSelection, SelectionSignature);
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(createEncloseAndFillSelection, SelectionIntoSignature);
    static_assert(std::is_same_v<decltype(std::declval<KisEncloseAndFillPainter &>().encloseAndFillPattern(
                                     std::declval<KisPixelSelectionSP>(),
                                     std::declval<KisPaintDeviceSP>())),
                                 void>);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::encloseAndFillRegionSelectionQuerySignaturesRemainStable()
{
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(regionSelectionColor, KoColor (KisEncloseAndFillPainter::*)() const);
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(regionSelectionIncludeContourRegions,
                                              bool (KisEncloseAndFillPainter::*)() const);
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(regionSelectionIncludeSurroundingRegions,
                                              bool (KisEncloseAndFillPainter::*)() const);
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(regionSelectionInvert, bool (KisEncloseAndFillPainter::*)() const);
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(
        regionSelectionMethod,
        KisEncloseAndFillPainter::RegionSelectionMethod (KisEncloseAndFillPainter::*)() const);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::encloseAndFillRegionSelectionMutationSignaturesRemainStable()
{
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(setRegionSelectionColor,
                                              void (KisEncloseAndFillPainter::*)(const KoColor &));
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(setRegionSelectionIncludeContourRegions,
                                              void (KisEncloseAndFillPainter::*)(bool));
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(setRegionSelectionIncludeSurroundingRegions,
                                              void (KisEncloseAndFillPainter::*)(bool));
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(setRegionSelectionInvert, void (KisEncloseAndFillPainter::*)(bool));
    ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(
        setRegionSelectionMethod,
        void (KisEncloseAndFillPainter::*)(KisEncloseAndFillPainter::RegionSelectionMethod));

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisEncloseAndFillPainterSchemaContractTest)

#include "KisEncloseAndFillPainterSchemaContractTest.moc"
