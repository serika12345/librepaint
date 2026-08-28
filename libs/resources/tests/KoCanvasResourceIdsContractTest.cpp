/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoCanvasResourcesIds.h"

#include <QTest>

class KoCanvasResourceIdsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceIdsPreserveValues_data();
    void resourceIdsPreserveValues();
};

void KoCanvasResourceIdsContractTest::resourceIdsPreserveValues_data()
{
    QTest::addColumn<int>("actual");
    QTest::addColumn<int>("expected");

    using namespace KoCanvasResource;

    QTest::newRow("ForegroundColor") << static_cast<int>(ForegroundColor) << 0;
    QTest::newRow("BackgroundColor") << static_cast<int>(BackgroundColor) << 1;
    QTest::newRow("PageSize") << static_cast<int>(PageSize) << 2;
    QTest::newRow("Unit") << static_cast<int>(Unit) << 3;
    QTest::newRow("CurrentPage") << static_cast<int>(CurrentPage) << 4;
    QTest::newRow("ActiveStyleType") << static_cast<int>(ActiveStyleType) << 5;
    QTest::newRow("ActiveRange") << static_cast<int>(ActiveRange) << 6;
    QTest::newRow("ShowTextShapeOutlines") << static_cast<int>(ShowTextShapeOutlines) << 7;
    QTest::newRow("ShowFormattingCharacters") << static_cast<int>(ShowFormattingCharacters) << 8;
    QTest::newRow("ShowTableBorders") << static_cast<int>(ShowTableBorders) << 9;
    QTest::newRow("ShowSectionBounds") << static_cast<int>(ShowSectionBounds) << 10;
    QTest::newRow("ShowInlineObjectVisualization") << static_cast<int>(ShowInlineObjectVisualization) << 11;
    QTest::newRow("ApplicationSpeciality") << static_cast<int>(ApplicationSpeciality) << 12;

    QTest::newRow("KritaStart") << static_cast<int>(KritaStart) << 6000;
    QTest::newRow("HdrExposure") << static_cast<int>(HdrExposure) << 6001;
    QTest::newRow("CurrentPattern") << static_cast<int>(CurrentPattern) << 6002;
    QTest::newRow("CurrentGamutMask") << static_cast<int>(CurrentGamutMask) << 6003;
    QTest::newRow("GamutMaskActive") << static_cast<int>(GamutMaskActive) << 6004;
    QTest::newRow("CurrentGradient") << static_cast<int>(CurrentGradient) << 6005;
    QTest::newRow("CurrentDisplayProfile") << static_cast<int>(CurrentDisplayProfile) << 6006;
    QTest::newRow("CurrentKritaNode") << static_cast<int>(CurrentKritaNode) << 6007;
    QTest::newRow("CurrentPaintOpPreset") << static_cast<int>(CurrentPaintOpPreset) << 6008;
    QTest::newRow("CurrentPaintOpPresetCache") << static_cast<int>(CurrentPaintOpPresetCache) << 6009;
    QTest::newRow("CurrentPaintOpPresetName") << static_cast<int>(CurrentPaintOpPresetName) << 6010;
    QTest::newRow("CurrentGeneratorConfiguration") << static_cast<int>(CurrentGeneratorConfiguration) << 6011;
    QTest::newRow("CurrentCompositeOp") << static_cast<int>(CurrentCompositeOp) << 6012;
    QTest::newRow("CurrentEffectiveCompositeOp") << static_cast<int>(CurrentEffectiveCompositeOp) << 6013;
    QTest::newRow("LodAvailability") << static_cast<int>(LodAvailability) << 6014;
    QTest::newRow("LodSizeThreshold") << static_cast<int>(LodSizeThreshold) << 6015;
    QTest::newRow("LodSizeThresholdSupported") << static_cast<int>(LodSizeThresholdSupported) << 6016;
    QTest::newRow("EffectiveLodAvailability") << static_cast<int>(EffectiveLodAvailability) << 6017;
    QTest::newRow("EraserMode") << static_cast<int>(EraserMode) << 6018;
    QTest::newRow("MirrorHorizontal") << static_cast<int>(MirrorHorizontal) << 6019;
    QTest::newRow("MirrorVertical") << static_cast<int>(MirrorVertical) << 6020;
    QTest::newRow("MirrorHorizontalLock") << static_cast<int>(MirrorHorizontalLock) << 6021;
    QTest::newRow("MirrorVerticalLock") << static_cast<int>(MirrorVerticalLock) << 6022;
    QTest::newRow("MirrorVerticalHideDecorations") << static_cast<int>(MirrorVerticalHideDecorations) << 6023;
    QTest::newRow("MirrorHorizontalHideDecorations") << static_cast<int>(MirrorHorizontalHideDecorations) << 6024;
    QTest::newRow("Opacity") << static_cast<int>(Opacity) << 6025;
    QTest::newRow("Flow") << static_cast<int>(Flow) << 6026;
    QTest::newRow("Size") << static_cast<int>(Size) << 6027;
    QTest::newRow("Fade") << static_cast<int>(Fade) << 6028;
    QTest::newRow("Scatter") << static_cast<int>(Scatter) << 6029;
    QTest::newRow("PatternSize") << static_cast<int>(PatternSize) << 6030;
    QTest::newRow("HdrGamma") << static_cast<int>(HdrGamma) << 6031;
    QTest::newRow("GlobalAlphaLock") << static_cast<int>(GlobalAlphaLock) << 6032;
    QTest::newRow("DisablePressure") << static_cast<int>(DisablePressure) << 6033;
    QTest::newRow("PreviousPaintOpPreset") << static_cast<int>(PreviousPaintOpPreset) << 6034;
    QTest::newRow("EffectiveZoom") << static_cast<int>(EffectiveZoom) << 6035;
    QTest::newRow("EffectivePhysicalZoom") << static_cast<int>(EffectivePhysicalZoom) << 6036;
    QTest::newRow("BrushRotation") << static_cast<int>(BrushRotation) << 6037;
    QTest::newRow("HandleRadius") << static_cast<int>(HandleRadius) << 6038;
    QTest::newRow("DecorationThickness") << static_cast<int>(DecorationThickness) << 6039;
    QTest::newRow("UsingOtherColor") << static_cast<int>(UsingOtherColor) << 6040;
    QTest::newRow("ColorHistoryModel") << static_cast<int>(ColorHistoryModel) << 6041;
    QTest::newRow("SvgTextPropertyData") << static_cast<int>(SvgTextPropertyData) << 6042;
    QTest::newRow("SvgCharacterTextPropertyData") << static_cast<int>(SvgCharacterTextPropertyData) << 6043;
}

void KoCanvasResourceIdsContractTest::resourceIdsPreserveValues()
{
    QFETCH(int, actual);
    QFETCH(int, expected);

    QCOMPARE(actual, expected);
}

QTEST_GUILESS_MAIN(KoCanvasResourceIdsContractTest)

#include "KoCanvasResourceIdsContractTest.moc"
