/*
 *  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoCopyColorConversionTransformation.h"
#include "KoColorConversionTransformation.h"
#include "KoColorConversionTransformationFactory.h"

#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>
#include <cstring>
#include <QtGlobal>
#include <qhashfunctions.h>

// --- KoCopyColorConversionTransformation ---
KoCopyColorConversionTransformation::KoCopyColorConversionTransformation(const KoColorSpace* cs)
        : KoColorConversionTransformation(cs, cs, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags())
{
}
void KoCopyColorConversionTransformation::transform(const quint8 *srcU8, quint8 *dstU8, qint32 nPixels) const
{
    memcpy(dstU8, srcU8, nPixels * srcColorSpace()->pixelSize());
}

// --- KoCopyColorConversionTransformationFactory ---
KoCopyColorConversionTransformationFactory::KoCopyColorConversionTransformationFactory(const QString& _colorModelId, const QString& _depthId, const QString& _profileName) : KoColorConversionTransformationFactory(_colorModelId, _depthId, _profileName, _colorModelId, _depthId, _profileName)
{}
KoColorConversionTransformation* KoCopyColorConversionTransformationFactory::createColorTransformation(const KoColorSpace* srcColorSpace, const KoColorSpace* dstColorSpace, KoColorConversionTransformation::Intent renderingIntent, KoColorConversionTransformation::ConversionFlags conversionFlags) const
{
    Q_UNUSED(renderingIntent);
    Q_UNUSED(conversionFlags);
#ifdef QT_NO_DEBUG
    Q_UNUSED(dstColorSpace);
#endif
    Q_UNUSED(dstColorSpace);
    Q_ASSERT(canBeSource(srcColorSpace));
    Q_ASSERT(canBeDestination(dstColorSpace));
    Q_ASSERT(srcColorSpace->id() == dstColorSpace->id());
    return new KoCopyColorConversionTransformation(srcColorSpace);
}

KoColorConversionStagesResult KoCopyColorConversionTransformation::displayStages() const
{
    const KoColorSpace *cs = srcColorSpace();
    if (cs->colorModelId() != RGBAColorModelID ||
        (cs->colorDepthId() != Integer8BitsColorDepthID &&
         cs->colorDepthId() != Integer16BitsColorDepthID) || !cs->profile()) {
        return {KoColorConversionStagesResult::Unsupported, {},
                QStringLiteral("GPU display stages require integer RGB")};
    }
    KoColorConversionStages value;
    value.input.depth = cs->colorDepthId() == Integer16BitsColorDepthID
        ? KoColorConversionStages::Encoding::U16 : KoColorConversionStages::Encoding::U8;
    value.input.iccProfile = cs->profile()->rawData();
    value.output = value.input;
    return {KoColorConversionStagesResult::Success, std::move(value), {}};
}
