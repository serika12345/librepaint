/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LCMSRGBP2020PQCOLORSPACETRANSFORMATION_H
#define LCMSRGBP2020PQCOLORSPACETRANSFORMATION_H

#include <libs/global/KoAlwaysInline.h>
#include "KoColorModelStandardIds.h"
#include "KoColorSpaceMaths.h"
#include "KoColorModelStandardIdsUtils.h"
#include "KoColorConversionTransformationFactory.h"
#include "KoColorTransferFunctions.h"

#include <colorspaces/rgb_u8/RgbU8ColorSpace.h>
#include <colorspaces/rgb_u16/RgbU16ColorSpace.h>
#ifdef HAVE_OPENEXR
#include <colorspaces/rgb_f16/RgbF16ColorSpace.h>
#endif
#include <colorspaces/rgb_f32/RgbF32ColorSpace.h>


namespace
{

template <class T>
struct DstTraitsForSource {
    typedef KoRgbF32Traits result;
};

/**
 * If half format is present, we use it instead
 */
#ifdef HAVE_OPENEXR
template <>
struct DstTraitsForSource<KoBgrU16Traits> {
    typedef KoRgbF16Traits result;
};

template <>
struct DstTraitsForSource<KoBgrU8Traits> {
    typedef KoRgbF16Traits result;
};
#endif

template <typename src_channel_type,
          typename dst_channel_type>
struct RemoveSmpte2048Policy {
    static ALWAYS_INLINE dst_channel_type process(src_channel_type value, float refWhite) {
        return
            KoColorSpaceMaths<float, dst_channel_type>::scaleToA(
            removeSmpte2048Curve(
            KoColorSpaceMaths<src_channel_type, float>::scaleToA(
            value), refWhite));
    }

    static ALWAYS_INLINE double getRefWhite(const KoColorSpace *src, const KoColorSpace *target) {
        Q_UNUSED(target)
        if (!src || !src->profile()) return 203.0;
        return src->profile()->hdrReferenceWhite().value_or(203.0);
    }
};

template <typename src_channel_type,
          typename dst_channel_type>
struct ApplySmpte2048Policy {
    static ALWAYS_INLINE dst_channel_type process(src_channel_type value, float refWhite) {
        return
            KoColorSpaceMaths<float, dst_channel_type>::scaleToA(
            applySmpte2048Curve(
            KoColorSpaceMaths<src_channel_type, float>::scaleToA(
            value), refWhite));
    }

    static ALWAYS_INLINE double getRefWhite(const KoColorSpace *src, const KoColorSpace *target) {
        Q_UNUSED(src)
        if (!target || !target->profile()) return 203.0;
        return target->profile()->hdrReferenceWhite().value_or(203.0);
    }
};

template <typename src_channel_type,
          typename dst_channel_type>
struct NoopPolicy {
    static ALWAYS_INLINE dst_channel_type process(src_channel_type value, float refWhite) {
        Q_UNUSED(refWhite)
        return KoColorSpaceMaths<src_channel_type, dst_channel_type>::scaleToA(value);
    }

    static ALWAYS_INLINE double getRefWhite(const KoColorSpace *src, const KoColorSpace *target) {
        Q_UNUSED(src)
        Q_UNUSED(target)
        return 203.0;
    }
};

}

template<typename SrcCSTraits,
         typename DstCSTraits,
         template<typename, typename> class Policy>
struct ApplyRgbShaper : public KoColorConversionTransformation
{
    ApplyRgbShaper(const KoColorSpace* srcCs,
                   const KoColorSpace* dstCs,
                   Intent renderingIntent,
                   ConversionFlags conversionFlags)
        : KoColorConversionTransformation(srcCs,
                                          dstCs,
                                          renderingIntent,
                                          conversionFlags)
    {
    }

    void transform(const quint8 *src, quint8 *dst, qint32 nPixels) const override {
        KIS_ASSERT(src != dst);

        const typename SrcCSTraits::Pixel *srcPixel = reinterpret_cast<const typename SrcCSTraits::Pixel*>(src);
        typename DstCSTraits::Pixel *dstPixel = reinterpret_cast<typename DstCSTraits::Pixel*>(dst);

        typedef typename SrcCSTraits::channels_type src_channel_type;
        typedef typename DstCSTraits::channels_type dst_channel_type;
        typedef Policy<src_channel_type, dst_channel_type> ConcretePolicy;

        const double refWhite = ConcretePolicy::getRefWhite(srcColorSpace(), dstColorSpace());

        for (int i = 0; i < nPixels; i++) {
            dstPixel->red = ConcretePolicy::process(srcPixel->red, refWhite);
            dstPixel->green = ConcretePolicy::process(srcPixel->green, refWhite);
            dstPixel->blue = ConcretePolicy::process(srcPixel->blue, refWhite);
            dstPixel->alpha =
                KoColorSpaceMaths<src_channel_type, dst_channel_type>::scaleToA(
                srcPixel->alpha);

            srcPixel++;
            dstPixel++;
        }
    }

};

template<class ParentColorSpace, class DstColorSpaceTraits = typename DstTraitsForSource<typename ParentColorSpace::ColorSpaceTraits>::result>
class LcmsFromRGBP2020PQTransformationFactory : public KoColorConversionTransformationFactory
{
public:
    LcmsFromRGBP2020PQTransformationFactory(const QString &targetProfileName, const QString &linearProfileName)
        : KoColorConversionTransformationFactory(RGBAColorModelID.id(),
                                                 colorDepthIdForChannelType<typename ParentColorSpace::ColorSpaceTraits::channels_type>().id(),
                                                 targetProfileName,
                                                 RGBAColorModelID.id(),
                                                 colorDepthIdForChannelType<typename DstColorSpaceTraits::channels_type>().id(),
                                                 linearProfileName)
    {
    }

    KoColorConversionTransformation* createColorTransformation(const KoColorSpace* srcColorSpace,
                                                               const KoColorSpace* dstColorSpace,
                                                               KoColorConversionTransformation::Intent renderingIntent,
                                                               KoColorConversionTransformation::ConversionFlags conversionFlags) const override
    {
        return new ApplyRgbShaper<
                typename ParentColorSpace::ColorSpaceTraits,
                DstColorSpaceTraits,
                RemoveSmpte2048Policy>(srcColorSpace,
                                       dstColorSpace,
                                       renderingIntent,
                                       conversionFlags);
    }
};

template<class ParentColorSpace, class DstColorSpaceTraits = typename DstTraitsForSource<typename ParentColorSpace::ColorSpaceTraits>::result>
class LcmsToRGBP2020PQTransformationFactory : public KoColorConversionTransformationFactory
{
public:
    LcmsToRGBP2020PQTransformationFactory(const QString &targetProfileName, const QString &linearProfileName)
        : KoColorConversionTransformationFactory(RGBAColorModelID.id(),
                                                 colorDepthIdForChannelType<typename DstColorSpaceTraits::channels_type>().id(),
                                                 linearProfileName,
                                                 RGBAColorModelID.id(),
                                                 colorDepthIdForChannelType<typename ParentColorSpace::ColorSpaceTraits::channels_type>().id(),
                                                 targetProfileName)
    {
    }

    KoColorConversionTransformation* createColorTransformation(const KoColorSpace* srcColorSpace,
                                                               const KoColorSpace* dstColorSpace,
                                                               KoColorConversionTransformation::Intent renderingIntent,
                                                               KoColorConversionTransformation::ConversionFlags conversionFlags) const override
    {
        return new ApplyRgbShaper<
                DstColorSpaceTraits,
                typename ParentColorSpace::ColorSpaceTraits,
                ApplySmpte2048Policy>(srcColorSpace,
                                      dstColorSpace,
                                      renderingIntent,
                                      conversionFlags);
    }
};

template<class ParentColorSpace, class DstColorSpaceTraits>
class LcmsScaleRGBP2020PQTransformationFactory : public KoColorConversionTransformationFactory
{
public:
    LcmsScaleRGBP2020PQTransformationFactory(const QString &targetProfileName)
        : KoColorConversionTransformationFactory(RGBAColorModelID.id(),
                                                 colorDepthIdForChannelType<typename ParentColorSpace::ColorSpaceTraits::channels_type>().id(),
                                                 targetProfileName,
                                                 RGBAColorModelID.id(),
                                                 colorDepthIdForChannelType<typename DstColorSpaceTraits::channels_type>().id(),
                                                 targetProfileName)
    {
        KIS_SAFE_ASSERT_RECOVER_NOOP(srcColorDepthId() != dstColorDepthId());
    }

    KoColorConversionTransformation* createColorTransformation(const KoColorSpace* srcColorSpace,
                                                               const KoColorSpace* dstColorSpace,
                                                               KoColorConversionTransformation::Intent renderingIntent,
                                                               KoColorConversionTransformation::ConversionFlags conversionFlags) const override
    {
        return new ApplyRgbShaper<
                typename ParentColorSpace::ColorSpaceTraits,
                DstColorSpaceTraits,
                NoopPolicy>(srcColorSpace,
                            dstColorSpace,
                            renderingIntent,
                            conversionFlags);
    }
};

#endif // LCMSRGBP2020PQCOLORSPACETRANSFORMATION_H
