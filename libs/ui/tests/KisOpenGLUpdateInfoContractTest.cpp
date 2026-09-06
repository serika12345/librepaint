/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "opengl/kis_opengl_update_info.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_OPENGL_UPDATE_INFO_MEMBER(type, method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

} // namespace

class KisOpenGLUpdateInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void conversionOptionsDefaultsAndExplicitValuesRemainStable();
    void openGLUpdateInfoTypeConstructionAndTileStorageRemainStable();
    void dirtyImageRectangleMutationRemainsStable();
    void levelOfDetailMutationRemainsStable();
    void mergeAndViewportContractRemainStable();
};

void KisOpenGLUpdateInfoContractTest::conversionOptionsDefaultsAndExplicitValuesRemainStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using Flags = KoColorConversionTransformation::ConversionFlags;

    static_assert(std::is_class_v<ConversionOptions>);
    static_assert(std::is_default_constructible_v<ConversionOptions>);
    static_assert(std::is_constructible_v<ConversionOptions, const KoColorSpace *, Intent, Flags>);
    static_assert(std::is_same_v<decltype(&ConversionOptions::m_needsConversion), bool ConversionOptions::*>);
    static_assert(std::is_same_v<decltype(&ConversionOptions::m_destinationColorSpace),
                                 const KoColorSpace * ConversionOptions::*>);
    static_assert(std::is_same_v<decltype(&ConversionOptions::m_renderingIntent), Intent ConversionOptions::*>);
    static_assert(std::is_same_v<decltype(&ConversionOptions::m_conversionFlags), Flags ConversionOptions::*>);

    const ConversionOptions defaults;
    QVERIFY(!defaults.m_needsConversion);
    QCOMPARE(defaults.m_destinationColorSpace, nullptr);
    QCOMPARE(defaults.m_renderingIntent, KoColorConversionTransformation::IntentPerceptual);
    QCOMPARE(defaults.m_conversionFlags, KoColorConversionTransformation::Empty);

    const auto *const colorSpace = reinterpret_cast<const KoColorSpace *>(quintptr(1));
    const Flags flags =
        KoColorConversionTransformation::GamutCheck | KoColorConversionTransformation::BlackpointCompensation;
    const ConversionOptions explicitOptions(colorSpace,
                                            KoColorConversionTransformation::IntentRelativeColorimetric,
                                            flags);

    QVERIFY(explicitOptions.m_needsConversion);
    QCOMPARE(explicitOptions.m_destinationColorSpace, colorSpace);
    QCOMPARE(explicitOptions.m_renderingIntent, KoColorConversionTransformation::IntentRelativeColorimetric);
    QCOMPARE(explicitOptions.m_conversionFlags, flags);
}

void KisOpenGLUpdateInfoContractTest::openGLUpdateInfoTypeConstructionAndTileStorageRemainStable()
{
    using Info = KisOpenGLUpdateInfo;

    static_assert(std::is_class_v<Info>);
    static_assert(std::is_base_of_v<KisUpdateInfo, Info>);
    static_assert(std::is_default_constructible_v<Info>);
    static_assert(std::is_same_v<decltype(&Info::tileList), KisTextureTileUpdateInfoSPList Info::*>);

    Info info;
    QVERIFY(info.tileList.isEmpty());

    info.tileList.append(KisTextureTileUpdateInfoSP());
    QCOMPARE(info.tileList.size(), 1);
    QVERIFY(info.tileList.constFirst().isNull());
}

void KisOpenGLUpdateInfoContractTest::dirtyImageRectangleMutationRemainsStable()
{
    using Info = KisOpenGLUpdateInfo;

    ASSERT_OPENGL_UPDATE_INFO_MEMBER(Info, assignDirtyImageRect, void (Info::*)(const QRect &));
    ASSERT_OPENGL_UPDATE_INFO_MEMBER(Info, dirtyImageRect, QRect (Info::*)() const);

    Info info;
    QCOMPARE(info.dirtyImageRect(), QRect());

    const QRect dirtyRect(3, 5, 11, 13);
    info.assignDirtyImageRect(dirtyRect);
    QCOMPARE(info.dirtyImageRect(), dirtyRect);
}

void KisOpenGLUpdateInfoContractTest::levelOfDetailMutationRemainsStable()
{
    using Info = KisOpenGLUpdateInfo;

    ASSERT_OPENGL_UPDATE_INFO_MEMBER(Info, assignLevelOfDetail, void (Info::*)(int));
    ASSERT_OPENGL_UPDATE_INFO_MEMBER(Info, levelOfDetail, int (Info::*)() const);

    Info info;
    QCOMPARE(info.levelOfDetail(), 0);

    info.assignLevelOfDetail(4);
    QCOMPARE(info.levelOfDetail(), 4);
}

void KisOpenGLUpdateInfoContractTest::mergeAndViewportContractRemainStable()
{
    using Info = KisOpenGLUpdateInfo;

    ASSERT_OPENGL_UPDATE_INFO_MEMBER(Info, dirtyViewportRect, QRect (Info::*)());
    ASSERT_OPENGL_UPDATE_INFO_MEMBER(Info, tryMergeWith, bool (Info::*)(const Info &));

    Info accumulated;
    accumulated.assignLevelOfDetail(2);
    accumulated.assignDirtyImageRect(QRect(0, 0, 5, 5));

    Info compatible;
    compatible.assignLevelOfDetail(2);
    compatible.assignDirtyImageRect(QRect(4, 4, 5, 5));
    compatible.tileList.append(KisTextureTileUpdateInfoSP());

    QVERIFY(accumulated.tryMergeWith(compatible));
    QCOMPARE(accumulated.dirtyImageRect(), QRect(0, 0, 9, 9));
    QCOMPARE(accumulated.tileList.size(), 1);

    Info incompatible;
    incompatible.assignLevelOfDetail(3);
    incompatible.assignDirtyImageRect(QRect(100, 100, 7, 7));
    incompatible.tileList.append(KisTextureTileUpdateInfoSP());

    QVERIFY(!accumulated.tryMergeWith(incompatible));
    QCOMPARE(accumulated.dirtyImageRect(), QRect(0, 0, 9, 9));
    QCOMPARE(accumulated.tileList.size(), 1);
}

#undef ASSERT_OPENGL_UPDATE_INFO_MEMBER

QTEST_APPLESS_MAIN(KisOpenGLUpdateInfoContractTest)

#include "KisOpenGLUpdateInfoContractTest.moc"
