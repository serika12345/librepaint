/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisOverlayPaintDeviceWrapper.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisOverlayPaintDeviceWrapperSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void overlayWrapperTypeModeAndLifetimeSchemaRemainStable();
    void overlayWrapperSourceAndOverlaySignaturesRemainStable();
    void overlayWrapperDestinationAndCompositionSignaturesRemainStable();
    void overlayWrapperRegionTransferSignaturesRemainStable();
    void overlayWrapperTransactionSignaturesRemainStable();
};

void KisOverlayPaintDeviceWrapperSchemaContractTest::overlayWrapperTypeModeAndLifetimeSchemaRemainStable()
{
    using Wrapper = KisOverlayPaintDeviceWrapper;
    using Mode = Wrapper::OverlayMode;

    static_assert(std::is_class_v<Wrapper>);
    static_assert(std::is_enum_v<Mode>);
    static_assert(Wrapper::NormalMode == 0);
    static_assert(Wrapper::PreciseMode == 1);
    static_assert(Wrapper::LazyPreciseMode == 2);
    static_assert(std::is_constructible_v<Wrapper, KisPaintDeviceSP, int, Mode, const KoColorSpace *>);
    static_assert(std::is_same_v<decltype(Wrapper(std::declval<KisPaintDeviceSP>())), Wrapper>);
    static_assert(std::is_destructible_v<Wrapper>);
}

void KisOverlayPaintDeviceWrapperSchemaContractTest::overlayWrapperSourceAndOverlaySignaturesRemainStable()
{
    using Wrapper = KisOverlayPaintDeviceWrapper;

    static_assert(std::is_same_v<decltype(&Wrapper::source), KisPaintDeviceSP (Wrapper::*)() const>);
    static_assert(std::is_same_v<decltype(&Wrapper::overlay), KisPaintDeviceSP (Wrapper::*)(int) const>);
    static_assert(std::is_same_v<decltype(std::declval<const Wrapper &>().overlay()), KisPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(&Wrapper::overlayColorSpace), const KoColorSpace *(Wrapper::*)() const>);
}

void KisOverlayPaintDeviceWrapperSchemaContractTest::overlayWrapperDestinationAndCompositionSignaturesRemainStable()
{
    using Wrapper = KisOverlayPaintDeviceWrapper;

    static_assert(std::is_same_v<decltype(&Wrapper::setExternalDestination), void (Wrapper::*)(KisPaintDeviceSP)>);
    static_assert(std::is_same_v<decltype(&Wrapper::externalDestination), KisPaintDeviceSP (Wrapper::*)() const>);
    static_assert(
        std::is_same_v<decltype(&Wrapper::createPreciseCompositionSourceDevice), KisPaintDeviceSP (Wrapper::*)()>);
}

void KisOverlayPaintDeviceWrapperSchemaContractTest::overlayWrapperRegionTransferSignaturesRemainStable()
{
    using Wrapper = KisOverlayPaintDeviceWrapper;

    static_assert(std::is_same_v<decltype(&Wrapper::readRect), void (Wrapper::*)(const QRect &)>);
    static_assert(std::is_same_v<decltype(&Wrapper::readRects), void (Wrapper::*)(const QVector<QRect> &)>);
    static_assert(std::is_same_v<decltype(&Wrapper::writeRect), void (Wrapper::*)(const QRect &, int)>);
    static_assert(std::is_same_v<decltype(&Wrapper::writeRects), void (Wrapper::*)(const QVector<QRect> &, int)>);
    static_assert(std::is_same_v<decltype(std::declval<Wrapper &>().writeRect(std::declval<const QRect &>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<Wrapper &>().writeRects(std::declval<const QVector<QRect> &>())), void>);
}

void KisOverlayPaintDeviceWrapperSchemaContractTest::overlayWrapperTransactionSignaturesRemainStable()
{
    using Wrapper = KisOverlayPaintDeviceWrapper;

    static_assert(std::is_same_v<decltype(&Wrapper::beginTransaction), void (Wrapper::*)(KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(std::declval<Wrapper &>().beginTransaction()), void>);
    static_assert(std::is_same_v<decltype(&Wrapper::endTransaction), KUndo2Command *(Wrapper::*)()>);
}

QTEST_APPLESS_MAIN(KisOverlayPaintDeviceWrapperSchemaContractTest)

#include "KisOverlayPaintDeviceWrapperSchemaContractTest.moc"
