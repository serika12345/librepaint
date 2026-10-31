/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTextureMaskInfo.h"
#include <KisBrushTextureMask.h>

#include <kis_paintop_settings.h>
#include <qimage.h>
#include <QDebug>
#include <qlogging.h>
#include <QtGlobal>
#include <qnamespace.h>
#include <qnumeric.h>
#include <qrgb.h>
#include <resources/KoPattern.h>
#include "KisTextureOptionData.h"
#include "KoColorSpaceConstants.h"
#include "kis_paint_device.h"
#include "kis_properties_configuration.h"
#include "kis_types.h"
#include <KoResourceLoadResult.h>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include <kis_algebra_2d.h>
#include <kis_lod_transform.h>
#include <kis_iterator_ng.h>

#include <QGlobalStatic>
#include <QMutexLocker>

/**********************************************************************/
/*       KisTextureMaskInfo                                           */
/**********************************************************************/


KisTextureMaskInfo::KisTextureMaskInfo(int levelOfDetail, bool preserveAlpha)
    : m_levelOfDetail(levelOfDetail)
    , m_preserveAlpha(preserveAlpha)
{
}

KisTextureMaskInfo::KisTextureMaskInfo(const KisTextureMaskInfo &rhs)
    : m_levelOfDetail(rhs.m_levelOfDetail)
    , m_preserveAlpha(rhs.m_preserveAlpha)
    , m_pattern(rhs.m_pattern)
    , m_scale(rhs.m_scale)
    , m_brightness(rhs.m_brightness)
    , m_contrast(rhs.m_contrast)
    , m_neutralPoint(rhs.m_neutralPoint)
    , m_invert(rhs.m_invert)
    , m_cutoffLeft(rhs.m_cutoffLeft)
    , m_cutoffRight(rhs.m_cutoffRight)
    , m_cutoffPolicy(rhs.m_cutoffPolicy)

{
}

KisTextureMaskInfo::~KisTextureMaskInfo()
{
}

bool operator==(const KisTextureMaskInfo &lhs, const KisTextureMaskInfo &rhs) {
    return
            lhs.m_levelOfDetail == rhs.m_levelOfDetail &&
            (lhs.m_pattern == rhs.m_pattern ||
             (lhs.m_pattern &&
              rhs.m_pattern &&
              lhs.m_pattern->md5Sum() == rhs.m_pattern->md5Sum())) &&
            qFuzzyCompare(lhs.m_scale, rhs.m_scale) &&
            qFuzzyCompare(lhs.m_brightness, rhs.m_brightness) &&
            qFuzzyCompare(lhs.m_contrast, rhs.m_contrast) &&
            qFuzzyCompare(lhs.m_neutralPoint, rhs.m_neutralPoint) &&
            lhs.m_invert == rhs.m_invert &&
            lhs.m_cutoffLeft == rhs.m_cutoffLeft &&
            lhs.m_cutoffRight == rhs.m_cutoffRight &&
            lhs.m_cutoffPolicy == rhs.m_cutoffPolicy &&
            lhs.m_preserveAlpha == rhs.m_preserveAlpha;
}

KisTextureMaskInfo &KisTextureMaskInfo::operator=(const KisTextureMaskInfo &rhs)
{
    m_levelOfDetail = rhs.m_levelOfDetail;
    m_pattern = rhs.m_pattern;
    m_scale = rhs.m_scale;
    m_brightness = rhs.m_brightness;
    m_contrast = rhs.m_contrast;
    m_neutralPoint = rhs.m_neutralPoint;
    m_invert = rhs.m_invert;
    m_cutoffLeft = rhs.m_cutoffLeft;
    m_cutoffRight = rhs.m_cutoffRight;
    m_cutoffPolicy = rhs.m_cutoffPolicy;
    m_preserveAlpha = rhs.m_preserveAlpha;

    return *this;
}

bool KisTextureMaskInfo::isValid() const
{
    return (m_mask && m_maskBounds.isValid());
}

int KisTextureMaskInfo::levelOfDetail() const {
    return m_levelOfDetail;
}

bool KisTextureMaskInfo::hasMask() const {
    return m_mask;
}

KisPaintDeviceSP KisTextureMaskInfo::mask() {
    return m_mask;
}

QRect KisTextureMaskInfo::maskBounds() const {
    return m_maskBounds;
}

bool KisTextureMaskInfo::fillProperties(const KisPropertiesConfiguration *setting, KisResourcesInterfaceSP resourcesInterface, bool invertAdditionally)
{
    KisTextureOptionData data;
    data.read(setting);

    if (!data.isEnabled || data.textureData.isNull()) {
        return false;
    }

    m_pattern = data.textureData.loadLinkedPattern(resourcesInterface).resource<KoPattern>();

    if (!m_pattern) {
        qWarning() << "WARNING: Couldn't load the pattern for a stroke (KisTextureMaskInfo)";
        return false;
    }

    m_scale = data.scale;
    m_brightness = data.brightness;
    m_contrast = data.contrast;
    m_neutralPoint = data.neutralPoint;
    m_invert = data.invert != invertAdditionally;
    m_cutoffLeft = data.cutOffLeft;
    m_cutoffRight = data.cutOffRight;
    m_cutoffPolicy = data.cutOffPolicy;

    return true;
}

void KisTextureMaskInfo::recalculateMask()
{
    if (!m_pattern) return;

    const KoColorSpace* cs;
    const bool useAlpha = m_pattern->hasAlpha() && m_preserveAlpha;

    if (useAlpha) {
        cs = KoColorSpaceRegistry::instance()->rgb8();
    } else {
        cs = KoColorSpaceRegistry::instance()->alpha8();
    }
    if (!m_mask) {
        m_mask = new KisPaintDevice(cs);
    }

    KisBrushTextureMaskSettings settings;
    settings.scale = m_scale * KisLodTransform::lodToScale(m_levelOfDetail);
    settings.brightness = m_brightness; settings.contrast = m_contrast;
    settings.neutralPoint = m_neutralPoint; settings.invert = m_invert;
    settings.cutoffLeft = m_cutoffLeft; settings.cutoffRight = m_cutoffRight;
    settings.cutoffPolicy = m_cutoffPolicy; settings.preserveAlpha = useAlpha;
    const auto mask = prepareBrushTextureMask(m_pattern->pattern(), settings);
    if (mask.isNull()) { m_mask.clear(); m_maskBounds = {}; return; }
    if (useAlpha) m_mask->convertFromQImage(mask, 0);
    else for (int row = 0; row < mask.height(); ++row)
        m_mask->writeBytes(mask.constScanLine(row), 0, row, mask.width(), 1);
    m_maskBounds = mask.rect();
}

bool KisTextureMaskInfo::hasAlpha() {
    return m_pattern->hasAlpha();
}

/**********************************************************************/
/*       KisTextureMaskInfoCache                                      */
/**********************************************************************/

Q_GLOBAL_STATIC(KisTextureMaskInfoCache, s_instance)

KisTextureMaskInfoCache *KisTextureMaskInfoCache::instance()
{
    return s_instance;
}

KisTextureMaskInfoSP KisTextureMaskInfoCache::fetchCachedTextureInfo(KisTextureMaskInfoSP info) {
    QMutexLocker locker(&m_mutex);

    KisTextureMaskInfoSP &cachedInfo =
            info->levelOfDetail() > 0 ? m_lodInfo : m_mainInfo;

    if (!cachedInfo || *cachedInfo != *info) {
        cachedInfo = info;
        cachedInfo->recalculateMask();
    }

    return cachedInfo;
}
