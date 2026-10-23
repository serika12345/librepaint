/*
 *  SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoOptimizedCompositeOpFactoryPerArch.h"

#if XSIMD_UNIVERSAL_BUILD_PASS
#include "KoOptimizedCompositeOpAlphaDarken32.h"

template<>
template<>
KoCompositeOp *
KoOptimizedCompositeOpFactoryPerArch<KoOptimizedCompositeOpAlphaDarkenHard32>::
    create<xsimd::current_arch>(const KoColorSpace *param)
{
    return new KoOptimizedCompositeOpAlphaDarkenHard32<xsimd::current_arch>(param);
}

template<>
template<>
KoCompositeOp *KoOptimizedCompositeOpFactoryPerArch<
    KoOptimizedCompositeOpAlphaDarkenCreamy32>::
    create<xsimd::current_arch>(const KoColorSpace *param)
{
    return new KoOptimizedCompositeOpAlphaDarkenCreamy32<xsimd::current_arch>(param);
}

#endif // XSIMD_UNIVERSAL_BUILD_PASS
