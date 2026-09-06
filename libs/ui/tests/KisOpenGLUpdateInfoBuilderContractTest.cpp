/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "opengl/KisOpenGLUpdateInfoBuilder.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_UPDATE_INFO_BUILDER_MEMBER(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisOpenGLUpdateInfoBuilder::method)), signature>)

template<typename T>
QSharedPointer<T> nonOwningSharedPointer(T *pointer)
{
    return QSharedPointer<T>(pointer, [](T *) { });
}

} // namespace

class KisOpenGLUpdateInfoBuilderContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void builderTypeConstructionAndLifetimeRemainStable();
    void textureGeometryRemainsStable();
    void tileIndexMappingRemainsStable();
    void conversionAndChannelConfigurationRemainStable();
    void sharedConfigurationAndBuildSignaturesRemainStable();
};

void KisOpenGLUpdateInfoBuilderContractTest::builderTypeConstructionAndLifetimeRemainStable()
{
    using Builder = KisOpenGLUpdateInfoBuilder;

    static_assert(std::is_class_v<Builder>);
    static_assert(std::is_default_constructible_v<Builder>);
    static_assert(std::is_destructible_v<Builder>);
    static_assert(!std::is_copy_constructible_v<Builder>);
    static_assert(!std::is_copy_assignable_v<Builder>);

    Builder builder;
    QVERIFY(builder.destinationColorSpace() == nullptr);
    QVERIFY(builder.tileDataPool().isNull());
    QVERIFY(builder.proofingConfig().isNull());
}

void KisOpenGLUpdateInfoBuilderContractTest::textureGeometryRemainsStable()
{
    using Builder = KisOpenGLUpdateInfoBuilder;

    ASSERT_UPDATE_INFO_BUILDER_MEMBER(setEffectiveTextureSize, void (Builder::*)(const QSize &));
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(setTextureBorder, void (Builder::*)(int));
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(calculateEffectiveTileRect, QRect (Builder::*)(int, int, const QRect &) const);
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(calculatePhysicalTileRect,
                                      QRect (Builder::*)(int, int, const QRect &, int) const);

    Builder builder;
    builder.setEffectiveTextureSize(QSize(32, 24));
    builder.setTextureBorder(2);

    const QRect imageBounds(10, 6, 50, 42);
    QCOMPARE(builder.calculateEffectiveTileRect(0, 0, imageBounds), QRect(10, 6, 22, 18));
    QCOMPARE(builder.calculateEffectiveTileRect(1, 1, imageBounds), QRect(32, 24, 28, 24));
    QCOMPARE(builder.calculatePhysicalTileRect(1, 1, imageBounds, 0), QRect(30, 22, 32, 28));
    QCOMPARE(builder.calculatePhysicalTileRect(1, 1, imageBounds, 2), QRect(28, 20, 36, 32));
}

void KisOpenGLUpdateInfoBuilderContractTest::tileIndexMappingRemainsStable()
{
    using Builder = KisOpenGLUpdateInfoBuilder;

    ASSERT_UPDATE_INFO_BUILDER_MEMBER(xToCol, int (Builder::*)(int) const);
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(yToRow, int (Builder::*)(int) const);

    Builder builder;
    builder.setEffectiveTextureSize(QSize(32, 24));

    QCOMPARE(builder.xToCol(0), 0);
    QCOMPARE(builder.xToCol(31), 0);
    QCOMPARE(builder.xToCol(32), 1);
    QCOMPARE(builder.xToCol(95), 2);
    QCOMPARE(builder.xToCol(-31), 0);
    QCOMPARE(builder.xToCol(-32), -1);

    QCOMPARE(builder.yToRow(0), 0);
    QCOMPARE(builder.yToRow(23), 0);
    QCOMPARE(builder.yToRow(24), 1);
    QCOMPARE(builder.yToRow(71), 2);
    QCOMPARE(builder.yToRow(-23), 0);
    QCOMPARE(builder.yToRow(-24), -1);
}

void KisOpenGLUpdateInfoBuilderContractTest::conversionAndChannelConfigurationRemainStable()
{
    using Builder = KisOpenGLUpdateInfoBuilder;

    ASSERT_UPDATE_INFO_BUILDER_MEMBER(destinationColorSpace, const KoColorSpace *(Builder::*)() const);
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(setConversionOptions, void (Builder::*)(const ConversionOptions &));
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(setChannelFlags, void (Builder::*)(const QBitArray &, bool, int));

    Builder builder;
    const auto *const colorSpace = reinterpret_cast<const KoColorSpace *>(quintptr(1));
    builder.setConversionOptions(ConversionOptions(colorSpace,
                                                   KoColorConversionTransformation::IntentAbsoluteColorimetric,
                                                   KoColorConversionTransformation::BlackpointCompensation));
    QCOMPARE(builder.destinationColorSpace(), colorSpace);

    QBitArray channelFlags(3);
    channelFlags.setBit(2);
    builder.setChannelFlags(channelFlags, true, 2);
    QCOMPARE(builder.destinationColorSpace(), colorSpace);
}

void KisOpenGLUpdateInfoBuilderContractTest::sharedConfigurationAndBuildSignaturesRemainStable()
{
    using Builder = KisOpenGLUpdateInfoBuilder;

    ASSERT_UPDATE_INFO_BUILDER_MEMBER(setTileDataPool, void (Builder::*)(KisTileDataPoolSP));
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(tileDataPool, KisTileDataPoolSP (Builder::*)() const);
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(setProofingConfig, void (Builder::*)(KisProofingConfigurationSP));
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(proofingConfig, KisProofingConfigurationSP (Builder::*)() const);
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(buildUpdateInfo,
                                      KisOpenGLUpdateInfoSP (Builder::*)(const QRect &, KisImageSP, bool));
    ASSERT_UPDATE_INFO_BUILDER_MEMBER(
        buildUpdateInfo,
        KisOpenGLUpdateInfoSP (Builder::*)(const QRect &, KisPaintDeviceSP, const QRect &, int, bool));

    Builder builder;
    auto *const poolPointer = reinterpret_cast<KisTileDataPool *>(quintptr(1));
    const KisTileDataPoolSP pool = nonOwningSharedPointer(poolPointer);
    builder.setTileDataPool(pool);
    QCOMPARE(builder.tileDataPool().data(), poolPointer);

    auto *const proofingPointer = reinterpret_cast<KisProofingConfiguration *>(quintptr(2));
    const KisProofingConfigurationSP proofing = nonOwningSharedPointer(proofingPointer);
    builder.setProofingConfig(proofing);
    QCOMPARE(builder.proofingConfig().data(), proofingPointer);

    builder.setTileDataPool({});
    builder.setProofingConfig({});
    QVERIFY(builder.tileDataPool().isNull());
    QVERIFY(builder.proofingConfig().isNull());
}

#undef ASSERT_UPDATE_INFO_BUILDER_MEMBER

QTEST_APPLESS_MAIN(KisOpenGLUpdateInfoBuilderContractTest)

#include "KisOpenGLUpdateInfoBuilderContractTest.moc"
