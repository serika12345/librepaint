/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_config.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImageConfig::method)), signature>)
} // namespace

class KisImageConfigAnimationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void onionSkinAppearanceSignaturesRemainStable();
    void animationFrameWorkflowSignaturesRemainStable();
    void animationRenderingCadenceSignaturesRemainStable();
    void animationCacheStorageSignaturesRemainStable();
    void animationCacheLimitSignaturesRemainStable();
};

void KisImageConfigAnimationSchemaContractTest::onionSkinAppearanceSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(numberOfOnionSkins, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setNumberOfOnionSkins, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(onionSkinTintFactor, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setOnionSkinTintFactor, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(onionSkinOpacity, int (KisImageConfig::*)(int, bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setOnionSkinOpacity, void (KisImageConfig::*)(int, int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(onionSkinState, bool (KisImageConfig::*)(int) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setOnionSkinState, void (KisImageConfig::*)(int, bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(onionSkinTintColorBackward, QColor (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setOnionSkinTintColorBackward, void (KisImageConfig::*)(const QColor &));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(onionSkinTintColorForward, QColor (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setOnionSkinTintColorForward, void (KisImageConfig::*)(const QColor &));

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().onionSkinOpacity(0)), int>);

    QVERIFY(true);
}

void KisImageConfigAnimationSchemaContractTest::animationFrameWorkflowSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(autoKeyEnabled, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setAutoKeyEnabled, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(autoKeyModeDuplicate, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setAutoKeyModeDuplicate, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(showAdditionalOnionSkinsSettings, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setShowAdditionalOnionSkinsSettings, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(defaultFrameColorLabel, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setDefaultFrameColorLabel, void (KisImageConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().autoKeyEnabled()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().autoKeyModeDuplicate()), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisImageConfig &>().showAdditionalOnionSkinsSettings()), bool>);

    QVERIFY(true);
}

void KisImageConfigAnimationSchemaContractTest::animationRenderingCadenceSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(frameRenderingClones, int (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setFrameRenderingClones, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(frameRenderingTimeout, int (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setFrameRenderingTimeout, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(fpsLimit, int (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setFpsLimit, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(detectFpsLimit, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setDetectFpsLimit, void (KisImageConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().frameRenderingClones()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().frameRenderingTimeout()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().fpsLimit()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().detectFpsLimit()), bool>);

    QVERIFY(true);
}

void KisImageConfigAnimationSchemaContractTest::animationCacheStorageSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(useOnDiskAnimationCacheSwapping, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setUseOnDiskAnimationCacheSwapping, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(animationCacheDir, QString (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setAnimationCacheDir, void (KisImageConfig::*)(const QString &));

    static_assert(
        std::is_same_v<decltype(std::declval<const KisImageConfig &>().useOnDiskAnimationCacheSwapping()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().animationCacheDir()), QString>);

    QVERIFY(true);
}

void KisImageConfigAnimationSchemaContractTest::animationCacheLimitSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(useAnimationCacheFrameSizeLimit, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setUseAnimationCacheFrameSizeLimit, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(animationCacheFrameSizeLimit, int (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setAnimationCacheFrameSizeLimit, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(useAnimationCacheRegionOfInterest, bool (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setUseAnimationCacheRegionOfInterest, void (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(animationCacheRegionOfInterestMargin, qreal (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setAnimationCacheRegionOfInterestMargin, void (KisImageConfig::*)(qreal));

    static_assert(
        std::is_same_v<decltype(std::declval<const KisImageConfig &>().useAnimationCacheFrameSizeLimit()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().animationCacheFrameSizeLimit()), int>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisImageConfig &>().useAnimationCacheRegionOfInterest()), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisImageConfig &>().animationCacheRegionOfInterestMargin()), qreal>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisImageConfigAnimationSchemaContractTest)

#include "KisImageConfigAnimationSchemaContractTest.moc"
