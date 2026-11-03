/*
 *  SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoOptimizedCompositeOpFactoryPerArch.h"
#include "KoBgrColorSpaceTraits.h"
#include "KoCompositeOpAlphaDarken.h"
#include "KoAlphaDarkenParamsWrapper.h"

template<>
template<>
KoCompositeOp *
KoOptimizedCompositeOpFactoryPerArch<KoOptimizedCompositeOpAlphaDarkenHard32>::
    create<xsimd::generic>(const KoColorSpace *param)
{
    return new KoCompositeOpAlphaDarken<KoBgrU8Traits, KoAlphaDarkenParamsWrapperHard>(param);
}

template<>
template<>
KoCompositeOp *KoOptimizedCompositeOpFactoryPerArch<
    KoOptimizedCompositeOpAlphaDarkenCreamy32>::
    create<xsimd::generic>(const KoColorSpace *param)
{
    return new KoCompositeOpAlphaDarken<KoBgrU8Traits, KoAlphaDarkenParamsWrapperCreamy>(param);
}
