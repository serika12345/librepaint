/*
 *  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "KoColorConversionTransformationFactory.h"

#include <QString>
#include <QtGlobal>

#include "KoColorProfile.h"
#include "KoColorSpace.h"
#include "DebugPigment.h"
#include "KoColorSpaceRegistry.h"

struct Q_DECL_HIDDEN KoColorConversionTransformationFactory::Private {
    QString srcModelId;
    QString srcDepthId;
    QString dstModelId;
    QString dstDepthId;
    QString srcProfile;
    QString dstProfile;
    QByteArray srcProfileData;
    QByteArray dstProfileData;
};

KoColorConversionTransformationFactory::KoColorConversionTransformationFactory(const QString &_srcModelId, const QString &_srcDepthId, const QString &_srcProfile, const QString &_dstModelId, const QString &_dstDepthId, const QString &_dstProfile) : d(new Private)
{
    d->srcModelId = _srcModelId;
    d->srcDepthId = _srcDepthId;
    d->dstModelId = _dstModelId;
    d->dstDepthId = _dstDepthId;
    d->srcProfile = KoColorSpaceRegistry::instance()->profileAlias(_srcProfile);
    d->dstProfile = KoColorSpaceRegistry::instance()->profileAlias(_dstProfile);
}

KoColorConversionTransformationFactory::KoColorConversionTransformationFactory(const QString &srcModelId, const QString &srcDepthId, const KoColorProfile *srcProfile,
                                                                               const QString &dstModelId, const QString &dstDepthId, const KoColorProfile *dstProfile)
    : KoColorConversionTransformationFactory(srcModelId, srcDepthId, QString(), dstModelId, dstDepthId, QString())
{
    if (srcProfile) {
        d->srcProfile = srcProfile->name();
        d->srcProfileData = srcProfile->rawData();
    }
    if (dstProfile) {
        d->dstProfile = dstProfile->name();
        d->dstProfileData = dstProfile->rawData();
    }
}

KoColorConversionTransformationFactory::~KoColorConversionTransformationFactory()
{
    delete d;
}

bool KoColorConversionTransformationFactory::canBeSource(const KoColorSpace* srcCS) const
{
    return ((srcCS->colorModelId().id() == d->srcModelId)
            && (srcCS->colorDepthId().id() == d->srcDepthId)
            && (d->srcProfile == "" || srcCS->profile()->name() == d->srcProfile)
            && (d->srcProfileData.isEmpty() || srcCS->profile()->rawData() == d->srcProfileData));
}

bool KoColorConversionTransformationFactory::canBeDestination(const KoColorSpace* dstCS) const
{
    dbgPigment << dstCS->colorModelId().id() << " " << d->dstModelId << " " << dstCS->colorDepthId().id() << " " <<  d->dstDepthId << " " << d->dstProfile << " " << (dstCS->profile() ? dstCS->profile()->name() : "noprofile")  << " " << d->dstProfile;
    return ((dstCS->colorModelId().id() == d->dstModelId)
            && (dstCS->colorDepthId().id() == d->dstDepthId)
            && (d->dstProfile == "" || dstCS->profile()->name() == d->dstProfile)
            && (d->dstProfileData.isEmpty() || dstCS->profile()->rawData() == d->dstProfileData));
}

QString KoColorConversionTransformationFactory::srcColorModelId() const
{
    return d->srcModelId;
}
QString KoColorConversionTransformationFactory::srcColorDepthId() const
{
    return d->srcDepthId;
}

QString KoColorConversionTransformationFactory::srcProfile() const
{
    return d->srcProfile;
}

QString KoColorConversionTransformationFactory::dstColorModelId() const
{
    return d->dstModelId;
}
QString KoColorConversionTransformationFactory::dstColorDepthId() const
{
    return d->dstDepthId;
}

QString KoColorConversionTransformationFactory::dstProfile() const
{
    return d->dstProfile;
}

QByteArray KoColorConversionTransformationFactory::srcProfileData() const
{
    return d->srcProfileData;
}

QByteArray KoColorConversionTransformationFactory::dstProfileData() const
{
    return d->dstProfileData;
}

