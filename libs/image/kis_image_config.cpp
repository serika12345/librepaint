/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_config.h"

#include <ksharedconfig.h>

#include <KoConfig.h>
#include <KoColorProfile.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorConversionTransformation.h>
#include <kis_properties_configuration.h>

#include <KisImageConfigNotifier.h>
#include <KisTemporaryFileConfiguration.h>
#include "kis_debug.h"

#include <QThread>
#include <QApplication>
#include <QColor>

#include "kis_global.h"
#include <cmath>

KisImageConfig::KisImageConfig(bool readOnly)
    : m_config(KSharedConfig::openConfig()->group(QString()))
    , m_readOnly(readOnly)
{
    if (!readOnly) {
        KIS_SAFE_ASSERT_RECOVER_RETURN(qApp->thread() == QThread::currentThread());
    }
    KritaUtils::normalizeSwapFileLocation(m_config);
}

KisImageConfig::~KisImageConfig()
{
    if (m_readOnly) return;

    if (qApp->thread() != QThread::currentThread()) {
        dbgKrita << "KisImageConfig: requested config synchronization from nonGUI thread! Called from" << kisBacktrace();
        return;
    }

    m_config.sync();
}

bool KisImageConfig::enableProgressReporting(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("enableProgressReporting", true) : true;
}

void KisImageConfig::setEnableProgressReporting(bool value)
{
    m_config.writeEntry("enableProgressReporting", value);
}

bool KisImageConfig::enablePerfLog(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("enablePerfLog", false) :false;
}

void KisImageConfig::setEnablePerfLog(bool value)
{
    m_config.writeEntry("enablePerfLog", value);
}

qreal KisImageConfig::transformMaskOffBoundsReadArea() const
{
    return m_config.readEntry("transformMaskOffBoundsReadArea", 0.5);
}

int KisImageConfig::updatePatchHeight() const
{
    int patchHeight = m_config.readEntry("updatePatchHeight", 512);
    if (patchHeight <= 0) return 512;
    return patchHeight;
}

void KisImageConfig::setUpdatePatchHeight(int value)
{
    m_config.writeEntry("updatePatchHeight", value);
}

int KisImageConfig::updatePatchWidth() const
{
    int patchWidth = m_config.readEntry("updatePatchWidth", 512);
    if (patchWidth <= 0) return 512;
    return patchWidth;
}

void KisImageConfig::setUpdatePatchWidth(int value)
{
    m_config.writeEntry("updatePatchWidth", value);
}

qreal KisImageConfig::maxCollectAlpha() const
{
    return m_config.readEntry("maxCollectAlpha", 2.5);
}

qreal KisImageConfig::maxMergeAlpha() const
{
    return m_config.readEntry("maxMergeAlpha", 1.);
}

qreal KisImageConfig::maxMergeCollectAlpha() const
{
    return m_config.readEntry("maxMergeCollectAlpha", 1.5);
}

qreal KisImageConfig::schedulerBalancingRatio() const
{
    /**
     * updates-queue-size / strokes-queue-size
     */
    return m_config.readEntry("schedulerBalancingRatio", 100.);
}

void KisImageConfig::setSchedulerBalancingRatio(qreal value)
{
    m_config.writeEntry("schedulerBalancingRatio", value);
}

int KisImageConfig::maxSwapSize(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("maxSwapSize", 4096) : 4096; // in MiB
}

void KisImageConfig::setMaxSwapSize(int value)
{
    m_config.writeEntry("maxSwapSize", value);
}

int KisImageConfig::swapSlabSize() const
{
    return m_config.readEntry("swapSlabSize", 64); // in MiB
}

void KisImageConfig::setSwapSlabSize(int value)
{
    m_config.writeEntry("swapSlabSize", value);
}

int KisImageConfig::swapWindowSize() const
{
    return m_config.readEntry("swapWindowSize", 16); // in MiB
}

void KisImageConfig::setSwapWindowSize(int value)
{
    m_config.writeEntry("swapWindowSize", value);
}

int KisImageConfig::tilesHardLimit() const
{
    qreal hp = qreal(memoryHardLimitPercent()) / 100.0;
    qreal pp = qreal(memoryPoolLimitPercent()) / 100.0;

    return totalRAM() * hp * (1 - pp);
}

int KisImageConfig::tilesSoftLimit() const
{
    qreal sp = qreal(memorySoftLimitPercent()) / 100.0;

    return tilesHardLimit() * sp;
}

int KisImageConfig::poolLimit() const
{
    qreal hp = qreal(memoryHardLimitPercent()) / 100.0;
    qreal pp = qreal(memoryPoolLimitPercent()) / 100.0;

    return totalRAM() * hp * pp;
}

qreal KisImageConfig::memoryHardLimitPercent(bool requestDefault) const
{
    const qreal defaultValue = defaultMemoryHardLimitPercent();
    const qreal value = !requestDefault ?
        m_config.readEntry("memoryHardLimitPercent", defaultValue) : defaultValue;

    return qBound(1.0, value, maximumMemoryHardLimitPercent());
}

void KisImageConfig::setMemoryHardLimitPercent(qreal value)
{
    m_config.writeEntry("memoryHardLimitPercent",
                        qBound(1.0, value, maximumMemoryHardLimitPercent()));
}

qreal KisImageConfig::memorySoftLimitPercent(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("memorySoftLimitPercent", 2.) : 2.;
}

void KisImageConfig::setMemorySoftLimitPercent(qreal value)
{
    m_config.writeEntry("memorySoftLimitPercent", value);
}

qreal KisImageConfig::memoryPoolLimitPercent(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("memoryPoolLimitPercent", 0.0) : 0.0;
}

void KisImageConfig::setMemoryPoolLimitPercent(qreal value)
{
    m_config.writeEntry("memoryPoolLimitPercent", value);
}

QString KisImageConfig::swapDir(bool requestDefault)
{
    return KritaUtils::writableSwapFileLocation(m_config, requestDefault);
}

void KisImageConfig::setSwapDir(const QString &swapDir)
{
    m_config.writeEntry("swaplocation", swapDir);
}

int KisImageConfig::numberOfOnionSkins() const
{
    return m_config.readEntry("numberOfOnionSkins", 10);
}

void KisImageConfig::setNumberOfOnionSkins(int value)
{
    m_config.writeEntry("numberOfOnionSkins", value);
}

int KisImageConfig::onionSkinTintFactor() const
{
    return m_config.readEntry("onionSkinTintFactor", 192);
}

void KisImageConfig::setOnionSkinTintFactor(int value)
{
    m_config.writeEntry("onionSkinTintFactor", value);
}

int KisImageConfig::onionSkinOpacity(int offset, bool requestDefault) const
{
    int value = m_config.readEntry("onionSkinOpacity_" + QString::number(offset), -1);

    if (value < 0 || requestDefault) {
        const int num = numberOfOnionSkins();
        if (num > 0) {
            const qreal dx = qreal(qAbs(offset)) / num;
            value = 0.7 * exp(-pow2(dx) / 0.5) * 255;
        }
    }

    return value;
}

void KisImageConfig::setOnionSkinOpacity(int offset, int value)
{
    m_config.writeEntry("onionSkinOpacity_" + QString::number(offset), value);
}

bool KisImageConfig::onionSkinState(int offset) const
{
    bool enableByDefault = (qAbs(offset) <= 2);
    return m_config.readEntry("onionSkinState_" + QString::number(offset), enableByDefault);
}

void KisImageConfig::setOnionSkinState(int offset, bool value)
{
    m_config.writeEntry("onionSkinState_" + QString::number(offset), value);
}

QColor KisImageConfig::onionSkinTintColorBackward() const
{
    return m_config.readEntry("onionSkinTintColorBackward", QColor(Qt::red));
}

void KisImageConfig::setOnionSkinTintColorBackward(const QColor &value)
{
    m_config.writeEntry("onionSkinTintColorBackward", value);
}

QColor KisImageConfig::onionSkinTintColorForward() const
{
    return m_config.readEntry("oninSkinTintColorForward", QColor(Qt::green));
}

void KisImageConfig::setOnionSkinTintColorForward(const QColor &value)
{
    m_config.writeEntry("oninSkinTintColorForward", value);
}

bool KisImageConfig::autoKeyEnabled(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("lazyFrameCreationEnabled", true) : true;
}

void KisImageConfig::setAutoKeyEnabled(bool value)
{
    m_config.writeEntry("lazyFrameCreationEnabled", value);
}

bool KisImageConfig::autoKeyModeDuplicate(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("lazyFrameModeDuplicate", true) : true;
}

void KisImageConfig::setAutoKeyModeDuplicate(bool value)
{
    m_config.writeEntry("lazyFrameModeDuplicate", value);
}

#if defined Q_OS_LINUX
#include <sys/sysinfo.h>
#elif defined Q_OS_HAIKU
#include <OS.h>
#elif defined Q_OS_FREEBSD || defined Q_OS_NETBSD || defined Q_OS_OPENBSD
#include <sys/sysctl.h>
#elif defined Q_OS_WIN
#include <windows.h>
#elif defined Q_OS_MACOS || defined Q_OS_IOS
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

int KisImageConfig::totalRAM()
{
    // let's think that default memory size is 1000MiB
    int totalMemory = 1000; // MiB
    int error = 1;

#if defined Q_OS_LINUX
    struct sysinfo info;

    error = sysinfo(&info);
    if(!error) {
        totalMemory = info.totalram * info.mem_unit / (1UL << 20);
    }
#elif defined Q_OS_HAIKU
	system_info info;
	error = get_system_info(&info) == B_OK ? 0 : 1;
	if (!error) {
		uint64_t size = (info.max_pages * B_PAGE_SIZE);
	totalMemory = size >> 20;
	}
#elif defined Q_OS_FREEBSD || defined Q_OS_NETBSD || defined Q_OS_OPENBSD
    u_long physmem;
#   if defined HW_PHYSMEM64 // NetBSD only
    int mib[] = {CTL_HW, HW_PHYSMEM64};
#   else
    int mib[] = {CTL_HW, HW_PHYSMEM};
#   endif
    size_t len = sizeof(physmem);

    error = sysctl(mib, 2, &physmem, &len, 0, 0);
    if(!error) {
        totalMemory = physmem >> 20;
    }
#elif defined Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    error  = !GlobalMemoryStatusEx(&status);

    if (!error) {
        totalMemory = status.ullTotalPhys >> 20;
    }

    // For 32 bit windows, the total memory available is at max the 2GB per process memory limit.
#   if defined ENV32BIT
    totalMemory = qMin(totalMemory, 2000);
#   endif
#elif defined Q_OS_MACOS || defined Q_OS_IOS
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    u_int namelen = sizeof(mib) / sizeof(mib[0]);
    uint64_t size;
    size_t len = sizeof(size);

    errno = 0;
    if (sysctl(mib, namelen, &size, &len, 0, 0) >= 0) {
        totalMemory = size >> 20;
        error = 0;
    }
    else {
        dbgKrita << "sysctl(\"hw.memsize\") raised error" << strerror(errno);
    }
#endif

    if (error) {
        warnKrita << "Cannot get the size of your RAM. Using 1 GiB by default.";
    }

    return totalMemory;
}

qreal KisImageConfig::defaultMemoryHardLimitPercent()
{
#ifdef Q_OS_IOS
    const qreal totalMemory = totalRAM();
    const qreal defaultLimitMiB = qMin(totalMemory * 0.25, 1024.0);
    return qBound(1.0, defaultLimitMiB * 100.0 / totalMemory, 100.0);
#else
    return 50.0;
#endif
}

qreal KisImageConfig::maximumMemoryHardLimitPercent()
{
#ifdef Q_OS_IOS
    const qreal totalMemory = totalRAM();
    const qreal maximumLimitMiB = qMin(totalMemory * 0.375, 1536.0);
    return qBound(1.0, maximumLimitMiB * 100.0 / totalMemory, 100.0);
#else
    return 100.0;
#endif
}

bool KisImageConfig::showAdditionalOnionSkinsSettings(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("showAdditionalOnionSkinsSettings", true) : true;
}

void KisImageConfig::setShowAdditionalOnionSkinsSettings(bool value)
{
    m_config.writeEntry("showAdditionalOnionSkinsSettings", value);
}

int KisImageConfig::defaultFrameColorLabel() const
{
    return m_config.readEntry("defaultFrameColorLabel", 0);
}

void KisImageConfig::setDefaultFrameColorLabel(int label)
{
    m_config.writeEntry("defaultFrameColorLabel", label);
}

KisProofingConfigurationSP KisImageConfig::defaultProofingconfiguration(bool requestDefault)
{
    KisProofingConfiguration *proofingConfig= new KisProofingConfiguration();
    if (!requestDefault) {
        proofingConfig->proofingProfile = m_config.readEntry("defaultProofingProfileName", "Chemical proof");
        proofingConfig->proofingModel = m_config.readEntry("defaultProofingProfileModel", "CMYKA");
        proofingConfig->proofingDepth = m_config.readEntry("defaultProofingProfileDepth", "U8");
        proofingConfig->displayIntent = KoColorConversionTransformation::Intent(m_config.readEntry("defaultProofingProfileIntent", int(INTENT_ABSOLUTE_COLORIMETRIC)));
        proofingConfig->conversionIntent = KoColorConversionTransformation::Intent(m_config.readEntry("defaultProofingConversionIntent", int(INTENT_RELATIVE_COLORIMETRIC)));
        proofingConfig->useBlackPointCompensationFirstTransform = m_config.readEntry("defaultProofingBlackpointCompensation", true);

        proofingConfig->displayFlags.setFlag(KoColorConversionTransformation::BlackpointCompensation, m_config.readEntry("defaultProofingDisplayBlackpointCompensation", true));
        proofingConfig->displayMode = KisProofingConfiguration::DisplayTransformState(m_config.readEntry("defaultProofingDisplayMode", int(KisProofingConfiguration::Paper)));
        QColor def(Qt::green);
        def = m_config.readEntry("defaultProofingGamutwarning", def);
        KoColor col(KoColorSpaceRegistry::instance()->rgb8());
        col.fromQColor(def);
        col.setOpacity(1.0);
        proofingConfig->warningColor = col;
        proofingConfig->setLegacyAdaptationState(m_config.readEntry("defaultProofingAdaptationState", 1.0));
    }
    return toQShared(proofingConfig);
}

void KisImageConfig::setDefaultProofingConfig(const KisProofingConfiguration &config)
{
    if (*defaultProofingconfiguration() == config) return;

    m_config.writeEntry("defaultProofingProfileName", config.proofingProfile);
    m_config.writeEntry("defaultProofingProfileModel", config.proofingModel);
    m_config.writeEntry("defaultProofingProfileDepth", config.proofingDepth);
    m_config.writeEntry("defaultProofingConversionIntent", int(config.conversionIntent));
    m_config.writeEntry("defaultProofingBlackpointCompensation", config.useBlackPointCompensationFirstTransform);
    QColor c;
    c = config.warningColor.toQColor();
    m_config.writeEntry("defaultProofingGamutwarning", c);
    m_config.writeEntry("defaultProofingAdaptationState", config.legacyAdaptationState());
    m_config.writeEntry("defaultProofingDisplayBlackpointCompensation", config.displayFlags.testFlag(KoColorConversionTransformation::BlackpointCompensation));
    m_config.writeEntry("defaultProofingProfileIntent", int(config.displayIntent));
    m_config.writeEntry("defaultProofingDisplayMode", int(config.displayMode));

    KisImageConfigNotifier::instance()->notifyGlobalProofingConfigChanged();
}

bool KisImageConfig::useLodForColorizeMask(bool requestDefault) const
{
    return !requestDefault ?
        m_config.readEntry("useLodForColorizeMask", false) : false;
}

void KisImageConfig::setUseLodForColorizeMask(bool value)
{
    m_config.writeEntry("useLodForColorizeMask", value);
}

int KisImageConfig::maxNumberOfThreads(bool defaultValue) const
{
    return (defaultValue ? QThread::idealThreadCount() : m_config.readEntry("maxNumberOfThreads", QThread::idealThreadCount()));
}

void KisImageConfig::setMaxNumberOfThreads(int value)
{
    if (value == QThread::idealThreadCount()) {
        m_config.deleteEntry("maxNumberOfThreads");
    } else {
        m_config.writeEntry("maxNumberOfThreads", value);
    }
}

int KisImageConfig::frameRenderingClones(bool defaultValue) const
{
    const int defaultClonesCount = qMax(1, maxNumberOfThreads(defaultValue) / 2);
    return defaultValue ? defaultClonesCount : m_config.readEntry("frameRenderingClones", defaultClonesCount);
}

void KisImageConfig::setFrameRenderingClones(int value)
{
    m_config.writeEntry("frameRenderingClones", value);
}

int KisImageConfig::frameRenderingTimeout(bool defaultValue) const
{
    const int defaultFrameRenderingTimeout = 30000; // 30 ms
    return defaultValue ? defaultFrameRenderingTimeout : m_config.readEntry("frameRenderingTimeout", defaultFrameRenderingTimeout);
}

void KisImageConfig::setFrameRenderingTimeout(int value)
{
    m_config.writeEntry("frameRenderingTimeout", value);
}

int KisImageConfig::fpsLimit(bool defaultValue) const
{
    int limit = defaultValue ? 100 : m_config.readEntry("fpsLimit", 100);
    return limit > 0 ? limit : 1;
}

void KisImageConfig::setFpsLimit(int value)
{
    m_config.writeEntry("fpsLimit", value);
}

bool KisImageConfig::detectFpsLimit(bool defaultValue) const
{
    return defaultValue ? true : m_config.readEntry("detectFpsLimit", true);
}

void KisImageConfig::setDetectFpsLimit(bool value)
{
    m_config.writeEntry("detectFpsLimit", value);
}

bool KisImageConfig::useOnDiskAnimationCacheSwapping(bool defaultValue) const
{
    return defaultValue ? true : m_config.readEntry("useOnDiskAnimationCacheSwapping", true);
}

void KisImageConfig::setUseOnDiskAnimationCacheSwapping(bool value)
{
    m_config.writeEntry("useOnDiskAnimationCacheSwapping", value);
}

QString KisImageConfig::animationCacheDir(bool defaultValue) const
{
    return KritaUtils::writableTemporaryFileLocation(m_config, "animation_cache", "animationCacheDir", defaultValue);
}

void KisImageConfig::setAnimationCacheDir(const QString &value)
{
    m_config.writeEntry("animationCacheDir", value);
}

bool KisImageConfig::useAnimationCacheFrameSizeLimit(bool defaultValue) const
{
    return defaultValue ? true : m_config.readEntry("useAnimationCacheFrameSizeLimit", true);
}

void KisImageConfig::setUseAnimationCacheFrameSizeLimit(bool value)
{
    m_config.writeEntry("useAnimationCacheFrameSizeLimit", value);
}

int KisImageConfig::animationCacheFrameSizeLimit(bool defaultValue) const
{
    return defaultValue ? 2500 : m_config.readEntry("animationCacheFrameSizeLimit", 2500);
}

void KisImageConfig::setAnimationCacheFrameSizeLimit(int value)
{
    m_config.writeEntry("animationCacheFrameSizeLimit", value);
}

bool KisImageConfig::useAnimationCacheRegionOfInterest(bool defaultValue) const
{
    return defaultValue ? true : m_config.readEntry("useAnimationCacheRegionOfInterest", true);
}

void KisImageConfig::setUseAnimationCacheRegionOfInterest(bool value)
{
    m_config.writeEntry("useAnimationCacheRegionOfInterest", value);
}

qreal KisImageConfig::animationCacheRegionOfInterestMargin(bool defaultValue) const
{
    return defaultValue ? 0.25 : m_config.readEntry("animationCacheRegionOfInterestMargin", 0.25);
}

void KisImageConfig::setAnimationCacheRegionOfInterestMargin(qreal value)
{
    m_config.writeEntry("animationCacheRegionOfInterestMargin", value);
}

qreal KisImageConfig::selectionOutlineOpacity(bool defaultValue) const
{
    return defaultValue ? 1.0 : m_config.readEntry("selectionOutlineOpacity", 1.0);
}

void KisImageConfig::setSelectionOutlineOpacity(qreal value)
{
    m_config.writeEntry("selectionOutlineOpacity", value);
}

QColor KisImageConfig::selectionOverlayMaskColor(bool defaultValue) const
{
    QColor def(255, 0, 0, 128);
    return (defaultValue ? def : m_config.readEntry("selectionOverlayMaskColor", def));
}

void KisImageConfig::setSelectionOverlayMaskColor(const QColor &color)
{
    m_config.writeEntry("selectionOverlayMaskColor", color);
}

int KisImageConfig::maxBrushSize(bool defaultValue) const
{
    return !defaultValue ? m_config.readEntry("maximumBrushSize", 1000) : 1000;
}

void KisImageConfig::setMaxBrushSize(int value)
{
    m_config.writeEntry("maximumBrushSize", value);
}

int KisImageConfig::maxMaskingBrushSize() const
{
    return qMin(15000, 3 * maxBrushSize());
}

bool KisImageConfig::renameMergedLayers(bool defaultValue) const
{
    return defaultValue ? true : m_config.readEntry("renameMergedLayers", true);
}

void KisImageConfig::setRenameMergedLayers(bool value)
{
    m_config.writeEntry("renameMergedLayers", value);
}

bool KisImageConfig::renameDuplicatedLayers(bool defaultValue) const
{
    return defaultValue ? true : m_config.readEntry("renameDuplicatedLayers", true);
}

void KisImageConfig::setRenameDuplicatedLayers(bool value)
{
    m_config.writeEntry("renameDuplicatedLayers", value);
}

QString KisImageConfig::exportConfigurationXML(const QString &exportConfigId, bool defaultValue) const
{
    return (defaultValue ? QString() : m_config.readEntry("ExportConfiguration-" + exportConfigId, QString()));
}

bool KisImageConfig::hasExportConfiguration(const QString &exportConfigID)
{
    return m_config.hasKey("ExportConfiguration-" + exportConfigID);
}

KisPropertiesConfigurationSP KisImageConfig::exportConfiguration(const QString &exportConfigId, bool defaultValue) const
{
    KisPropertiesConfigurationSP cfg = new KisPropertiesConfiguration();
    const QString xmlData = exportConfigurationXML(exportConfigId, defaultValue);
    cfg->fromXML(xmlData);
    return cfg;
}

void KisImageConfig::setExportConfiguration(const QString &exportConfigId, KisPropertiesConfigurationSP properties)
{
    const QString exportConfig = properties->toXML();
    QString configId = "ExportConfiguration-" + exportConfigId;
    m_config.writeEntry(configId, exportConfig);
}

QString KisImageConfig::importConfiguration(const QString &filterId, bool defaultValue) const
{
    return defaultValue ? QString() : m_config.readEntry("ImportConfiguration-" + filterId, QString());
}

void KisImageConfig::setImportConfiguration(const QString &filterId, KisPropertiesConfigurationSP properties)
{
    m_config.writeEntry("ImportConfiguration-" + filterId, properties->toXML());
}

void KisImageConfig::resetConfig()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteGroup();
}
