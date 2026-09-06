/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <compositeops/KoCompositeOpAlphaBase.h>
#include <dithering/KisCmykDitherOpFactory.h>

#include <QTest>

#include <type_traits>

namespace
{
struct DummyCompositeOperation;
using AlphaComposite = KoCompositeOpAlphaBase<KoCmykU8Traits, DummyCompositeOperation, false>;
using CmykDither = KisCmykDitherOpImpl<KoCmykU8Traits, KoCmykU16Traits, DITHER_NONE>;
} // namespace

class KisCmykDitherAndCompositeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void alphaCompositeTypeAndConstructionSchemaRemainStable();
    void alphaCompositeDispatchSchemaRemainsStable();
    void cmykDitherTypeAndConstructionSchemaRemainStable();
    void cmykDitherDispatchAndRegistrationSchemaRemainStable();
};

void KisCmykDitherAndCompositeSchemaContractTest::alphaCompositeTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<AlphaComposite>);
    static_assert(std::is_constructible_v<AlphaComposite, const KoColorSpace *, const QString &, const QString &>);
}

void KisCmykDitherAndCompositeSchemaContractTest::alphaCompositeDispatchSchemaRemainsStable()
{
    using CompositeSignature = void (AlphaComposite::*)(const KoCompositeOp::ParameterInfo &) const;

    static_assert(
        std::is_same_v<decltype(static_cast<CompositeSignature>(&AlphaComposite::composite)), CompositeSignature>);
}

void KisCmykDitherAndCompositeSchemaContractTest::cmykDitherTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<CmykDither>);
    static_assert(std::is_constructible_v<CmykDither, const KoID &, const KoID &>);
}

void KisCmykDitherAndCompositeSchemaContractTest::cmykDitherDispatchAndRegistrationSchemaRemainStable()
{
    using PixelDitherSignature = void (CmykDither::*)(const quint8 *, quint8 *, int, int) const;
    using BlockDitherSignature = void (CmykDither::*)(const quint8 *, int, quint8 *, int, int, int, int, int) const;
    using AddByDepthSignature = void(KoColorSpace *, const KoID &);
    using AddStandardSignature = void(KoColorSpace *);

    static_assert(
        std::is_same_v<decltype(static_cast<PixelDitherSignature>(&CmykDither::dither)), PixelDitherSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<BlockDitherSignature>(&CmykDither::dither)), BlockDitherSignature>);
    static_assert(
        std::is_same_v<decltype(addCmykDitherOpsByDepth<KoCmykU8Traits, KoCmykU16Traits>), AddByDepthSignature>);
    static_assert(std::is_same_v<decltype(addStandardDitherOps<KoCmykU8Traits>), AddStandardSignature>);
}

QTEST_GUILESS_MAIN(KisCmykDitherAndCompositeSchemaContractTest)

#include "KisCmykDitherAndCompositeSchemaContractTest.moc"
