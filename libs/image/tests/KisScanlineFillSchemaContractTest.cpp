/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "floodfill/kis_scanline_fill.h"

#include <QTest>

#include <type_traits>

class KisScanlineFillSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void colorFillSignaturesRemainStable();
    void selectionFillSignaturesRemainStable();
    void boundarySelectionFillSignaturesRemainStable();
    void operationConfigurationSignaturesRemainStable();
};

void KisScanlineFillSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisScanlineFill>);
    static_assert(std::is_constructible_v<KisScanlineFill, KisPaintDeviceSP, const QPoint &, const QRect &>);
    static_assert(std::is_destructible_v<KisScanlineFill>);
}

void KisScanlineFillSchemaContractTest::colorFillSignaturesRemainStable()
{
    using FillSignature = void (KisScanlineFill::*)(const KoColor &);
    using FillExternalSignature = void (KisScanlineFill::*)(const KoColor &, KisPaintDeviceSP);
    using FillUntilColorSignature = void (KisScanlineFill::*)(const KoColor &, const KoColor &);
    using FillUntilColorExternalSignature =
        void (KisScanlineFill::*)(const KoColor &, const KoColor &, KisPaintDeviceSP);

    static_assert(std::is_same_v<decltype(static_cast<FillSignature>(&KisScanlineFill::fill)), FillSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<FillExternalSignature>(&KisScanlineFill::fill)), FillExternalSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FillUntilColorSignature>(&KisScanlineFill::fillUntilColor)),
                                 FillUntilColorSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<FillUntilColorExternalSignature>(&KisScanlineFill::fillUntilColor)),
                       FillUntilColorExternalSignature>);
}

void KisScanlineFillSchemaContractTest::selectionFillSignaturesRemainStable()
{
    using FillSelectionSignature = void (KisScanlineFill::*)(KisPixelSelectionSP);
    using FillSelectionBoundarySignature = void (KisScanlineFill::*)(KisPixelSelectionSP, KisPaintDeviceSP);

    static_assert(std::is_same_v<decltype(static_cast<FillSelectionSignature>(&KisScanlineFill::fillSelection)),
                                 FillSelectionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FillSelectionBoundarySignature>(&KisScanlineFill::fillSelection)),
                                 FillSelectionBoundarySignature>);
}

void KisScanlineFillSchemaContractTest::boundarySelectionFillSignaturesRemainStable()
{
    using FillUntilColorSignature = void (KisScanlineFill::*)(KisPixelSelectionSP, const KoColor &);
    using FillUntilColorBoundarySignature =
        void (KisScanlineFill::*)(KisPixelSelectionSP, const KoColor &, KisPaintDeviceSP);

    static_assert(
        std::is_same_v<decltype(static_cast<FillUntilColorSignature>(&KisScanlineFill::fillSelectionUntilColor)),
                       FillUntilColorSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FillUntilColorBoundarySignature>(
                                     &KisScanlineFill::fillSelectionUntilColor)),
                                 FillUntilColorBoundarySignature>);
    static_assert(std::is_same_v<decltype(static_cast<FillUntilColorSignature>(
                                     &KisScanlineFill::fillSelectionUntilColorOrTransparent)),
                                 FillUntilColorSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FillUntilColorBoundarySignature>(
                                     &KisScanlineFill::fillSelectionUntilColorOrTransparent)),
                                 FillUntilColorBoundarySignature>);
}

void KisScanlineFillSchemaContractTest::operationConfigurationSignaturesRemainStable()
{
    using ClearSignature = void (KisScanlineFill::*)();
    using FillGroupSignature = void (KisScanlineFill::*)(KisPaintDeviceSP, qint32);
    using SetIntegerSignature = void (KisScanlineFill::*)(int);
    using FillExtentSignature = QRect (KisScanlineFill::*)() const;

    static_assert(std::is_same_v<decltype(&KisScanlineFill::clearNonZeroComponent), ClearSignature>);
    static_assert(std::is_same_v<decltype(&KisScanlineFill::fillContiguousGroup), FillGroupSignature>);
    static_assert(std::is_same_v<decltype(&KisScanlineFill::setThreshold), SetIntegerSignature>);
    static_assert(std::is_same_v<decltype(&KisScanlineFill::setOpacitySpread), SetIntegerSignature>);
    static_assert(std::is_same_v<decltype(&KisScanlineFill::setCloseGap), SetIntegerSignature>);
    static_assert(std::is_same_v<decltype(&KisScanlineFill::fillExtent), FillExtentSignature>);
}

QTEST_APPLESS_MAIN(KisScanlineFillSchemaContractTest)

#include "KisScanlineFillSchemaContractTest.moc"
