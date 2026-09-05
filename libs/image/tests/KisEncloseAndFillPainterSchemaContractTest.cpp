/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_fill_painter.h"

#include "KisEncloseAndFillPainter.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_ENCLOSE_AND_FILL_PAINTER_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisEncloseAndFillPainter::method)), signature>)
#define ASSERT_FILL_PAINTER_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisFillPainter::method)), signature>)

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
    void fillPainterTypeConstructionAndRegionModeSchemaRemainStable();
    void fillPainterSelectionGenerationSignaturesRemainStable();
    void fillPainterFillOperationSignaturesRemainStable();
    void fillPainterOptionQuerySignaturesRemainStable();
    void fillPainterOptionMutationSignaturesRemainStable();
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

void KisEncloseAndFillPainterSchemaContractTest::fillPainterTypeConstructionAndRegionModeSchemaRemainStable()
{
    using Painter = KisFillPainter;
    using Mode = Painter::RegionFillingMode;

    static_assert(std::is_class_v<Painter>);
    static_assert(std::is_base_of_v<KisPainter, Painter>);
    static_assert(std::is_enum_v<Mode>);
    static_assert(Painter::RegionFillingMode_FloodFill == 0);
    static_assert(Painter::RegionFillingMode_BoundaryFill == 1);
    static_assert(std::is_default_constructible_v<Painter>);
    static_assert(std::is_constructible_v<Painter, KisPaintDeviceSP>);
    static_assert(std::is_constructible_v<Painter, KisPaintDeviceSP, KisSelectionSP>);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::fillPainterSelectionGenerationSignaturesRemainStable()
{
    using Painter = KisFillPainter;
    using Flood = KisPixelSelectionSP (Painter::*)(int, int, KisPaintDeviceSP, KisPaintDeviceSP);
    using FloodInto =
        KisPixelSelectionSP (Painter::*)(KisPixelSelectionSP, int, int, KisPaintDeviceSP, KisPaintDeviceSP);
    using Similar =
        void (Painter::*)(KisPixelSelectionSP, const KoColor &, KisPaintDeviceSP, const QRect &, KisPixelSelectionSP);
    using SimilarJobs = QVector<KisStrokeJobData *> (Painter::*)(KisPixelSelectionSP,
                                                                 const QSharedPointer<KoColor>,
                                                                 KisPaintDeviceSP,
                                                                 const QRect &,
                                                                 KisPixelSelectionSP,
                                                                 QSharedPointer<KisProcessingVisitor::ProgressHelper>);

    ASSERT_FILL_PAINTER_SIGNATURE(createFloodSelection, Flood);
    ASSERT_FILL_PAINTER_SIGNATURE(createFloodSelection, FloodInto);
    ASSERT_FILL_PAINTER_SIGNATURE(createSimilarColorsSelection, Similar);
    ASSERT_FILL_PAINTER_SIGNATURE(createSimilarColorsSelectionJobs, SimilarJobs);
    static_assert(std::is_same_v<decltype(std::declval<Painter &>().createSimilarColorsSelectionJobs(
                                     std::declval<KisPixelSelectionSP>(),
                                     std::declval<QSharedPointer<KoColor>>(),
                                     std::declval<KisPaintDeviceSP>(),
                                     std::declval<const QRect &>(),
                                     std::declval<KisPixelSelectionSP>())),
                                 QVector<KisStrokeJobData *>>);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::fillPainterFillOperationSignaturesRemainStable()
{
    using Painter = KisFillPainter;
    using EraseRect = void (Painter::*)(const QRect &);
    using EraseCoords = void (Painter::*)(qint32, qint32, qint32, qint32);
    using FillColor = void (Painter::*)(int, int, KisPaintDeviceSP);
    using FillPattern = void (Painter::*)(int, int, KisPaintDeviceSP, QTransform);
    using RectColor = void (Painter::*)(const QRect &, const KoColor &);
    using RectColorOpacity = void (Painter::*)(const QRect &, const KoColor &, quint8);
    using CoordsColor = void (Painter::*)(qint32, qint32, qint32, qint32, const KoColor &);
    using CoordsColorOpacity = void (Painter::*)(qint32, qint32, qint32, qint32, const KoColor &, quint8);
    using RectPattern = void (Painter::*)(const QRect &, const KoPatternSP, const QPoint &);
    using CoordsPattern = void (Painter::*)(qint32, qint32, qint32, qint32, const KoPatternSP, const QPoint &);
    using RectDevice = void (Painter::*)(const QRect &, const KisPaintDeviceSP, const QRect &);
    using CoordsDevice = void (Painter::*)(qint32, qint32, qint32, qint32, const KisPaintDeviceSP, const QRect &);
    using Generator = void (Painter::*)(qint32, qint32, qint32, qint32, const KisFilterConfigurationSP);
    using RectNoCompose = void (Painter::*)(const QRect &, const KoPatternSP, const QTransform);
    using CoordsNoCompose =
        void (Painter::*)(qint32, qint32, qint32, qint32, const KisPaintDeviceSP, const QRect &, const QTransform);

    ASSERT_FILL_PAINTER_SIGNATURE(eraseRect, EraseRect);
    ASSERT_FILL_PAINTER_SIGNATURE(eraseRect, EraseCoords);
    ASSERT_FILL_PAINTER_SIGNATURE(fillColor, FillColor);
    ASSERT_FILL_PAINTER_SIGNATURE(fillPattern, FillPattern);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, RectColor);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, RectColorOpacity);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, CoordsColor);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, CoordsColorOpacity);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, RectPattern);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, CoordsPattern);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, RectDevice);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, CoordsDevice);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRect, Generator);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRectNoCompose, RectNoCompose);
    ASSERT_FILL_PAINTER_SIGNATURE(fillRectNoCompose, CoordsNoCompose);
    ASSERT_FILL_PAINTER_SIGNATURE(fillSelection, void (Painter::*)(const QRect &, const KoColor &));
    static_assert(
        std::is_same_v<decltype(std::declval<Painter &>().fillPattern(0, 0, std::declval<KisPaintDeviceSP>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<Painter &>().fillRect(0, 0, 0, 0, std::declval<KoPatternSP>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Painter &>().fillRect(std::declval<const QRect &>(),
                                                                             std::declval<KoPatternSP>())),
                                 void>);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::fillPainterOptionQuerySignaturesRemainStable()
{
    using Painter = KisFillPainter;

    ASSERT_FILL_PAINTER_SIGNATURE(antiAlias, bool (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(careForSelection, bool (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(closeGap, uint (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(feather, uint (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(fillThreshold, int (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(opacitySpread, int (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(regionFillingBoundaryColor, KoColor (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(regionFillingMode, Painter::RegionFillingMode (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(sizemod, int (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(stopGrowingAtDarkestPixel, bool (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(useCompositing, bool (Painter::*)() const);
    ASSERT_FILL_PAINTER_SIGNATURE(useSelectionAsBoundary, uint (Painter::*)() const);

    QVERIFY(true);
}

void KisEncloseAndFillPainterSchemaContractTest::fillPainterOptionMutationSignaturesRemainStable()
{
    using Painter = KisFillPainter;
    using BoolSetter = void (Painter::*)(bool);
    using IntSetter = void (Painter::*)(int);

    ASSERT_FILL_PAINTER_SIGNATURE(setAntiAlias, BoolSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setCareForSelection, BoolSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setCloseGap, IntSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setFeather, IntSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setFillThreshold, IntSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setHeight, IntSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setOpacitySpread, IntSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setRegionFillingBoundaryColor, void (Painter::*)(const KoColor &));
    ASSERT_FILL_PAINTER_SIGNATURE(setRegionFillingMode, void (Painter::*)(Painter::RegionFillingMode));
    ASSERT_FILL_PAINTER_SIGNATURE(setSizemod, IntSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setStopGrowingAtDarkestPixel, BoolSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setUseCompositing, BoolSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setUseSelectionAsBoundary, BoolSetter);
    ASSERT_FILL_PAINTER_SIGNATURE(setWidth, IntSetter);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisEncloseAndFillPainterSchemaContractTest)

#include "KisEncloseAndFillPainterSchemaContractTest.moc"
