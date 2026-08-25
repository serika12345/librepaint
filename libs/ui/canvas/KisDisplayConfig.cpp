/*
 *  SPDX-FileCopyrightText: 2024 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "canvas/KisDisplayConfig.h"

#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>
#include <KoColorSpaceRegistry.h>
#include <kis_color_manager.h>
#include <application/kis_config.h>
//#include <opengl/KisOpenGLModeProber.h>

namespace {
KoColorConversionTransformation::Intent
renderingIntentFromConfig(const KisConfig &cfg)
{
    return (KoColorConversionTransformation::Intent)cfg.monitorRenderIntent();
}


KoColorConversionTransformation::ConversionFlags
conversionFlagsFromConfig(const KisConfig &cfg)
{
    KoColorConversionTransformation::ConversionFlags conversionFlags =
        KoColorConversionTransformation::HighQuality;

    if (cfg.useBlackPointCompensation()) conversionFlags |= KoColorConversionTransformation::BlackpointCompensation;
    if (!cfg.allowLCMSOptimization()) conversionFlags |= KoColorConversionTransformation::NoOptimization;

    return conversionFlags;
}

}

KisDisplayConfig::KisDisplayConfig()
    : profile(nullptr)
    , intent(KoColorConversionTransformation::internalRenderingIntent())
    , conversionFlags(KoColorConversionTransformation::internalConversionFlags())
    , isHDR(false)
{
}

KisDisplayConfig::KisDisplayConfig(const KoColorProfile *_profile,
                                   KoColorConversionTransformation::Intent _intent,
                                   KoColorConversionTransformation::ConversionFlags _conversionFlags,
                                   bool _isHDR)
    : profile(_profile)
    , intent(_intent)
    , conversionFlags(_conversionFlags)
    , isHDR(_isHDR)
{
}

KisDisplayConfig::Options KisDisplayConfig::optionsFromKisConfig(const KisConfig &cfg)
{
    return {renderingIntentFromConfig(cfg),
            conversionFlagsFromConfig(cfg)};
}

void KisDisplayConfig::initializeSystemColorManager()
{
    Q_UNUSED(KisColorManager::instance());
}

const KoColorProfile *KisDisplayConfig::profileForScreen(int screen)
{
    if (screen < 0) {
        return nullptr;
    }

    KisConfig config(true);
    const KoColorProfile *profile = nullptr;

    if (config.useSystemMonitorProfile() &&
        KisColorManager::instance()->devices().size() > screen) {
        const QString monitorId = config.monitorForScreen(
            screen, KisColorManager::instance()->devices()[screen]);
        const QByteArray profileData = KisColorManager::instance()->displayProfile(monitorId);
        if (!profileData.isEmpty()) {
            profile = KoColorSpaceRegistry::instance()->createColorProfile(
                RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), profileData);
        }
    }

    if (!profile || !profile->isSuitableForDisplay()) {
        const QString profileName = config.monitorProfile(screen);
        if (!profileName.isEmpty()) {
            profile = KoColorSpaceRegistry::instance()->profileByName(profileName);
        }
    }

    if (!profile || !profile->isSuitableForDisplay()) {
        profile = KoColorSpaceRegistry::instance()->profileByName("sRGB Built-in");
    }

    return profile;
}

bool KisDisplayConfig::operator==(const KisDisplayConfig &rhs) const
{
    return profile == rhs.profile &&
            intent == rhs.intent &&
            conversionFlags == rhs.conversionFlags && 
            isHDR == rhs.isHDR;
}

QDebug operator<<(QDebug debug, const KisDisplayConfig &value) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "KisDisplayConfig(";

    debug.nospace() << "profile: " << value.profile;

    if (value.profile) {
        debug.nospace() << " (" << value.profile->name() << ")";
    }
    debug.nospace() << ", ";
    debug.nospace() << "intent: " << value.intent << ", ";
    debug.nospace() << "conversionFlags: " << value.conversionFlags << ", ";
    debug.nospace() << "isHDR: " << value.isHDR;

    debug.nospace() << ")";
    return debug;
}


bool KisMultiSurfaceDisplayConfig::operator==(const KisMultiSurfaceDisplayConfig &rhs) const
{
    return
        uiProfile == rhs.uiProfile &&
        canvasProfile == rhs.canvasProfile &&
        intent == rhs.intent &&
        conversionFlags == rhs.conversionFlags &&
        isCanvasHDR == rhs.isCanvasHDR;
}
