/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/KisDisplayConfig.h>

#include <QDebug>
#include <QTest>

#include <type_traits>

class KisDisplayConfigSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayConfigTypeAndConstructionSchemaRemainsStable();
    void displayConfigValueSchemaRemainsStable();
    void displayConfigExternalPolicySignaturesRemainStable();
    void multiSurfaceDisplayConfigValueSchemaRemainsStable();
    void multiSurfaceDisplayConfigProjectionSchemaRemainsStable();
};

void KisDisplayConfigSchemaContractTest::displayConfigTypeAndConstructionSchemaRemainsStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using ConversionFlags = KoColorConversionTransformation::ConversionFlags;
    using Options = std::pair<Intent, ConversionFlags>;

    static_assert(std::is_class_v<KisDisplayConfig>);
    static_assert(std::is_same_v<KisDisplayConfig::Options, Options>);
    static_assert(std::is_default_constructible_v<KisDisplayConfig>);
    static_assert(std::is_constructible_v<KisDisplayConfig, const KoColorProfile *, Intent, ConversionFlags, bool>);
    static_assert(std::is_same_v<decltype(KisDisplayConfig(nullptr, Intent::IntentPerceptual, ConversionFlags())),
                                 KisDisplayConfig>);

    static_assert(std::is_class_v<KisMultiSurfaceDisplayConfig>);
    static_assert(std::is_same_v<KisMultiSurfaceDisplayConfig::Options, Options>);
    static_assert(std::is_default_constructible_v<KisMultiSurfaceDisplayConfig>);
}

void KisDisplayConfigSchemaContractTest::displayConfigValueSchemaRemainsStable()
{
    using Config = KisDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::profile), const KoColorProfile * Config::*>);
    static_assert(std::is_same_v<decltype(&Config::intent), KoColorConversionTransformation::Intent Config::*>);
    static_assert(
        std::is_same_v<decltype(&Config::conversionFlags), KoColorConversionTransformation::ConversionFlags Config::*>);
    static_assert(std::is_same_v<decltype(&Config::isHDR), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::operator==), bool (Config::*)(const Config &) const>);
    static_assert(std::is_same_v<decltype(&Config::options), Config::Options (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::setOptions), void (Config::*)(const Config::Options &)>);
}

void KisDisplayConfigSchemaContractTest::displayConfigExternalPolicySignaturesRemainStable()
{
    using Config = KisDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::optionsFromKisConfig), Config::Options (*)(const KisConfig &)>);
    static_assert(std::is_same_v<decltype(&Config::initializeSystemColorManager), void (*)()>);
    static_assert(std::is_same_v<decltype(&Config::profileForScreen), const KoColorProfile *(*)(int)>);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const Config &)>(&operator<<)),
                                 QDebug (*)(QDebug, const Config &)>);
}

void KisDisplayConfigSchemaContractTest::multiSurfaceDisplayConfigValueSchemaRemainsStable()
{
    using Config = KisMultiSurfaceDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::uiProfile), const KoColorProfile * Config::*>);
    static_assert(std::is_same_v<decltype(&Config::canvasProfile), const KoColorProfile * Config::*>);
    static_assert(std::is_same_v<decltype(&Config::intent), KoColorConversionTransformation::Intent Config::*>);
    static_assert(
        std::is_same_v<decltype(&Config::conversionFlags), KoColorConversionTransformation::ConversionFlags Config::*>);
    static_assert(std::is_same_v<decltype(&Config::isCanvasHDR), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::operator==), bool (Config::*)(const Config &) const>);
}

void KisDisplayConfigSchemaContractTest::multiSurfaceDisplayConfigProjectionSchemaRemainsStable()
{
    using Config = KisMultiSurfaceDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::uiDisplayConfig), KisDisplayConfig (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::canvasDisplayConfig), KisDisplayConfig (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::options), Config::Options (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::setOptions), void (Config::*)(const Config::Options &)>);
}

QTEST_GUILESS_MAIN(KisDisplayConfigSchemaContractTest)

#include "KisDisplayConfigSchemaContractTest.moc"
