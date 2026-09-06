/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <compositeops/KoCompositeOps.h>

#include <QTest>

#include <type_traits>

namespace
{
using Traits = KoBgrU8Traits;
using GeneralOps = _Private::AddGeneralOps<Traits, true>;
using AlphaOps = _Private::AddGeneralAlphaOps<Traits, true>;
using RgbOps = _Private::AddRGBOps<Traits, true>;
using OptimizedOps = _Private::OptimizedOpsSelector<Traits>;
using RegistrationFunction = void (*)(KoColorSpace *);
using NamedRegistrationFunction = void (*)(KoColorSpace *, const QString &, const QString &);
using OptimizedFunction = KoCompositeOp *(*)(const KoColorSpace *);
} // namespace

class KoCompositeOpsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void generalRegistrationSchemaRemainsStable();
    void alphaRegistrationSchemaRemainsStable();
    void rgbRegistrationSchemaRemainsStable();
    void optimizedSelectionSchemaRemainsStable();
    void standardEntryPointSchemaRemainsStable();
};

void KoCompositeOpsSchemaContractTest::generalRegistrationSchemaRemainsStable()
{
    static_assert(std::is_class_v<GeneralOps>);
    static_assert(std::is_same_v<GeneralOps::Arg, quint8>);
    static_assert(std::is_same_v<GeneralOps::CompositeFunc, quint8 (*)(quint8, quint8)>);
    static_assert(std::is_same_v<decltype(GeneralOps::alpha_pos), const qint32>);
    static_assert(std::is_same_v<decltype(GeneralOps::IsIntegerSpace), const bool>);
    static_assert(std::is_same_v<decltype(static_cast<RegistrationFunction>(&GeneralOps::add)), RegistrationFunction>);
    static_assert(std::is_same_v<decltype(&GeneralOps::template add<CFOverlay<quint8>>), NamedRegistrationFunction>);
}

void KoCompositeOpsSchemaContractTest::alphaRegistrationSchemaRemainsStable()
{
    static_assert(std::is_class_v<AlphaOps>);
    static_assert(std::is_same_v<AlphaOps::Arg, float>);
    static_assert(std::is_same_v<decltype(AlphaOps::alpha_pos), const qint32>);
    static_assert(std::is_same_v<decltype(static_cast<RegistrationFunction>(&AlphaOps::add)), RegistrationFunction>);
    static_assert(
        std::is_same_v<decltype(&AlphaOps::template add<&cfAdditionSAI<HSVType, float>>), NamedRegistrationFunction>);
}

void KoCompositeOpsSchemaContractTest::rgbRegistrationSchemaRemainsStable()
{
    static_assert(std::is_class_v<RgbOps>);
    static_assert(std::is_same_v<RgbOps::channels_type, quint8>);
    static_assert(std::is_same_v<decltype(RgbOps::red_pos), const qint32>);
    static_assert(std::is_same_v<decltype(RgbOps::green_pos), const qint32>);
    static_assert(std::is_same_v<decltype(RgbOps::blue_pos), const qint32>);
    static_assert(std::is_same_v<decltype(static_cast<RegistrationFunction>(&RgbOps::add)), RegistrationFunction>);
    static_assert(std::is_same_v<decltype(&RgbOps::template add<CFTint<HSYType, quint8>>), NamedRegistrationFunction>);
}

void KoCompositeOpsSchemaContractTest::optimizedSelectionSchemaRemainsStable()
{
    static_assert(std::is_class_v<OptimizedOps>);
    static_assert(std::is_same_v<decltype(&OptimizedOps::createAlphaDarkenOp), OptimizedFunction>);
    static_assert(std::is_same_v<decltype(&OptimizedOps::createOverOp), OptimizedFunction>);
    static_assert(std::is_same_v<decltype(&OptimizedOps::createCopyOp), OptimizedFunction>);
}

void KoCompositeOpsSchemaContractTest::standardEntryPointSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&addStandardCompositeOps<Traits>), RegistrationFunction>);
    static_assert(std::is_same_v<decltype(&createAlphaDarkenCompositeOp<Traits>), OptimizedFunction>);
}

QTEST_GUILESS_MAIN(KoCompositeOpsSchemaContractTest)

#include "KoCompositeOpsSchemaContractTest.moc"
