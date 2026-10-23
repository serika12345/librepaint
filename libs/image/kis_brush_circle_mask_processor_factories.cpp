/*
 *  SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <xsimd_extensions/xsimd.hpp>

#if !defined(XSIMD_NO_SUPPORTED_ARCHITECTURE) && XSIMD_UNIVERSAL_BUILD_PASS

#include "kis_circle_mask_generator.h"
#include "kis_circle_mask_generator_p.h"
#include "kis_brush_mask_vector_applicator.h"

template<>
template<>
void FastRowProcessor<KisCircleMaskGenerator>::process<xsimd::current_arch>(float *buffer,
                                                                            int width,
                                                                            float y,
                                                                            float cosa,
                                                                            float sina,
                                                                            float centerX,
                                                                            float centerY)
{
    using float_v = xsimd::batch<float, xsimd::current_arch>;
    using float_m = typename float_v::batch_bool_type;

    const bool useSmoothing = d->copyOfAntialiasEdges;

    const float y_ = y - centerY;
    const float sinay_ = sina * y_;
    const float cosay_ = cosa * y_;

    float *bufferPointer = buffer;

    float_v currentIndices =
        xsimd::detail::make_sequence_as_batch<float_v>();

    const float_v increment((float)float_v::size);
    const float_v vCenterX(centerX);

    const float_v vCosa(cosa);
    const float_v vSina(sina);
    const float_v vCosaY_(cosay_);
    const float_v vSinaY_(sinay_);

    const float_v vXCoeff(static_cast<float>(d->xcoef));
    const float_v vYCoeff(static_cast<float>(d->ycoef));

    const float_v vTransformedFadeX(static_cast<float>(d->transformedFadeX));
    const float_v vTransformedFadeY(static_cast<float>(d->transformedFadeY));

    const float_v vOne(1);

    for (size_t i = 0; i < static_cast<size_t>(width); i += float_v::size) {
        const float_v x_ = currentIndices - vCenterX;

        float_v xr = x_ * vCosa - vSinaY_;
        float_v yr = x_ * vSina + vCosaY_;

        const float_v n = xsimd::pow2(xr * vXCoeff) + xsimd::pow2(yr * vYCoeff);
        const float_m outsideMask = n > vOne;

        if (!xsimd::all(outsideMask)) {
            if (useSmoothing) {
                xr = xsimd::abs(xr) + vOne;
                yr = xsimd::abs(yr) + vOne;
            }
            float_v vNormFade = xsimd::pow2(xr * vTransformedFadeX)
                + xsimd::pow2(yr * vTransformedFadeY);
            const float_m vNormLowMask = vNormFade < vOne;
            vNormFade = xsimd::set_zero(vNormFade, vNormLowMask);

            // 255 * n * (normeFade - 1) / (normeFade - n)
            float_v vFade = n * (vNormFade - vOne) / (vNormFade - n);

            // Mask in the inner circle of the mask
            const float_m mask = vNormFade < vOne;
            vFade = xsimd::set_zero(vFade, mask);

            // Mask out the outer circle of the mask
            vFade = xsimd::set_one(vFade, outsideMask);

            vFade.store_aligned(bufferPointer);
        } else {
            // Mask out everything outside the circle
            vOne.store_aligned(bufferPointer);
        }

        currentIndices = currentIndices + increment;

        bufferPointer += float_v::size;
    }
}

#endif /* XSIMD_UNIVERSAL_BUILD_PASS */
