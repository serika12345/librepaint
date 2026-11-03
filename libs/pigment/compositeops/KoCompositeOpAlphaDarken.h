/*
 * SPDX-FileCopyrightText: 2006 Cyrille Berger <cberger@cberger.net>
 * SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KOCOMPOSITEOPALPHADARKEN_H_
#define KOCOMPOSITEOPALPHADARKEN_H_

#include "KoColorSpaceMaths.h"
#include "KoCompositeOpBase.h"
#include <KoCompositeOpRegistry.h>
#include <libs/global/KoAlwaysInline.h>
#include <cmath>
#include <type_traits>

struct KoBgrU8Traits;

template<bool haveMask, class ParamsWrapper>
ALWAYS_INLINE void compositeAlphaDarken32(const quint8 *src, quint8 *dst, const quint8 *mask,
                                         const ParamsWrapper &params)
{
    // Keep the SIMD byte-domain intermediates through the final nearest-even
    // quantization, including when a tile boundary leaves a scalar pixel.
    constexpr float unitRec = 1.0f / 255.0f;
    const float opacity = 255.0f * params.opacity;
    const float averageOpacity = 255.0f * params.averageOpacity;
    const float maskAlpha = haveMask ? float(src[3]) * float(*mask) * (1.0f / (255.0f * 255.0f))
                                    : float(src[3]) * unitRec;
    const float srcAlpha = maskAlpha * opacity;
    if (srcAlpha == 0.0f) return;

    const float dstAlpha = dst[3];
    const float blend = srcAlpha * unitRec;
    for (int i = 0; i < 3; ++i) {
        dst[i] = dstAlpha == 0.0f ? src[i]
                                 : quint8(std::nearbyint(blend * (float(src[i]) - float(dst[i])) + float(dst[i])));
    }

    const float fullFlowAlpha = params.averageOpacity > params.opacity
        ? (averageOpacity > dstAlpha ? (averageOpacity - srcAlpha) * (dstAlpha / averageOpacity) + srcAlpha : dstAlpha)
        : (opacity > dstAlpha ? (opacity - dstAlpha) * maskAlpha + dstAlpha : dstAlpha);
    const float alpha = [&]() {
        if (params.flow == 1.0f) return fullFlowAlpha;
        const float zeroFlowAlpha = ParamsWrapper::calculateZeroFlowAlpha(srcAlpha, dstAlpha, unitRec);
        return (fullFlowAlpha - zeroFlowAlpha) * params.flow + zeroFlowAlpha;
    }();
    dst[3] = quint8(std::nearbyint(alpha));
}

/**
 * A template version of the alphadarken composite operation to use in colorspaces
 */
template<class Traits, class ParamsWrapper>
class KoCompositeOpAlphaDarken: public KoCompositeOp
{
    typedef typename Traits::channels_type channels_type;

    static const qint32 channels_nb = Traits::channels_nb;
    static const qint32 alpha_pos   = Traits::alpha_pos;

public:
    KoCompositeOpAlphaDarken(const KoColorSpace* cs):
        KoCompositeOp(cs, COMPOSITE_ALPHA_DARKEN, KoCompositeOp::categoryMix()) { }

    using KoCompositeOp::composite;

    void composite(const KoCompositeOp::ParameterInfo& params) const override
    {
        if(params.maskRowStart != 0)
            genericComposite<true>(params);
        else
            genericComposite<false>(params);
    }

    template<bool useMask>
    void genericComposite(const KoCompositeOp::ParameterInfo& params) const
    {
        using namespace Arithmetic;

        ParamsWrapper paramsWrapper(params);

        qint32        srcInc       = (params.srcRowStride == 0) ? 0 : channels_nb;
        constexpr bool byteRgba = std::is_same_v<Traits, KoBgrU8Traits>;
        channels_type flow = 0;
        channels_type opacity = 0;
        if constexpr (!byteRgba) {
            flow = scale<channels_type>(paramsWrapper.flow);
            opacity = scale<channels_type>(paramsWrapper.opacity);
        }
        quint8*       dstRowStart  = params.dstRowStart;
        const quint8* srcRowStart  = params.srcRowStart;
        const quint8* maskRowStart = params.maskRowStart;

        for (qint32 r = params.rows; r > 0; --r) {
            const channels_type* src  = reinterpret_cast<const channels_type*>(srcRowStart);
            channels_type*       dst  = reinterpret_cast<channels_type*>(dstRowStart);
            const quint8*        mask = maskRowStart;

            for(qint32 c=params.cols; c>0; --c) {
                if constexpr (byteRgba) {
                    compositeAlphaDarken32<useMask>(src, dst, mask, paramsWrapper);
                } else {
                    channels_type srcAlpha = (alpha_pos == -1) ? unitValue<channels_type>() : src[alpha_pos];
                    channels_type dstAlpha = (alpha_pos == -1) ? unitValue<channels_type>() : dst[alpha_pos];
                    channels_type mskAlpha = useMask ? mul(scale<channels_type>(*mask), srcAlpha) : srcAlpha;

                    srcAlpha = mul(mskAlpha, opacity);

                    if(dstAlpha != zeroValue<channels_type>()) {
                        for(qint32 i=0; i <channels_nb; i++) {
                            if(i != alpha_pos)
                                dst[i] = lerp(dst[i], src[i], srcAlpha);
                        }
                    }
                    else {
                        for(qint32 i=0; i <channels_nb; i++) {
                            if(i != alpha_pos)
                                dst[i] = src[i];
                        }
                    }

                    if(alpha_pos != -1) {
                        dst[alpha_pos] = calculateAlpha(params, paramsWrapper, flow, opacity, srcAlpha, dstAlpha, mskAlpha);
                    }
                }

                src += srcInc;
                dst += channels_nb;

                if(useMask)
                    ++mask;
            }

            srcRowStart  += params.srcRowStride;
            dstRowStart  += params.dstRowStride;
            maskRowStart += params.maskRowStride;
        }
    }

    static channels_type calculateAlpha(const KoCompositeOp::ParameterInfo &params,
                                        const ParamsWrapper &paramsWrapper,
                                        channels_type flow,
                                        channels_type opacity,
                                        channels_type srcAlpha,
                                        channels_type dstAlpha,
                                        channels_type mskAlpha)
    {
        using namespace Arithmetic;

        channels_type fullFlowAlpha;
        channels_type averageOpacity = scale<channels_type>(paramsWrapper.averageOpacity);

        /**
         * Here we calculate fullFlowAlpha, which should strive either to
         * averageOpacity or opacity (whichever is the greater) or just keep old dstAlpha
         * value, if both opacity values are not bit enough
         */
        if (averageOpacity > opacity) {
            /**
             * This cryptic code is basically an optimized version of the following:
             * fullFlowAlpha = averageOpacity *
             *                     unionShapeOpacity(srcAlpha / averageOpacity,
             *                                       dstAlpha / averageOpacity);
             *
             * The main idea is: fullFlowAlpha should be as near to averageOpacity as
             * maximum of srcAlpha and dstAlpha and a bit more. So that in consequent
             * applications of the blending operation alpha channel would aim to
             * averageOpacity.
             */

            channels_type reverseBlend = KoColorSpaceMaths<channels_type>::divide(dstAlpha, averageOpacity);
            fullFlowAlpha = averageOpacity > dstAlpha ? lerp(srcAlpha, averageOpacity, reverseBlend) : dstAlpha;
        } else {
            fullFlowAlpha = opacity > dstAlpha ? lerp(dstAlpha, opacity, mskAlpha) : dstAlpha;
        }

        if (params.flow == 1.0f) {
            return fullFlowAlpha;
        } else {
            channels_type zeroFlowAlpha = ParamsWrapper::calculateZeroFlowAlphaLegacy(srcAlpha, dstAlpha);
            return lerp(zeroFlowAlpha, fullFlowAlpha, flow);
        }
    }
};

#endif // KOCOMPOSITEOPALPHADARKEN_H_
