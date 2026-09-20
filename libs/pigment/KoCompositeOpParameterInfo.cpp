/*
 * SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoCompositeOp.h"

KoCompositeOp::ParameterInfo::ParameterInfo()
    : opacity(1.0f)
    , flow(1.0f)
    , lastOpacity(&opacity)
{
}

KoCompositeOp::ParameterInfo::ParameterInfo(const ParameterInfo &rhs)
{
    copy(rhs);
}

KoCompositeOp::ParameterInfo &KoCompositeOp::ParameterInfo::operator=(const ParameterInfo &rhs)
{
    copy(rhs);
    return *this;
}

void KoCompositeOp::ParameterInfo::setOpacityAndAverage(float _opacity, float _averageOpacity)
{
    if (qFuzzyCompare(_opacity, _averageOpacity)) {
        opacity = _opacity;
        lastOpacity = &opacity;
    } else {
        opacity = _opacity;
        _lastOpacityData = _averageOpacity;
        lastOpacity = &_lastOpacityData;
    }
}

void KoCompositeOp::ParameterInfo::copy(const ParameterInfo &rhs)
{
    dstRowStart = rhs.dstRowStart;
    dstRowStride = rhs.dstRowStride;
    srcRowStart = rhs.srcRowStart;
    srcRowStride = rhs.srcRowStride;
    maskRowStart = rhs.maskRowStart;
    maskRowStride = rhs.maskRowStride;
    rows = rhs.rows;
    cols = rhs.cols;
    opacity = rhs.opacity;
    flow = rhs.flow;
    _lastOpacityData = rhs._lastOpacityData;
    channelFlags = rhs.channelFlags;

    lastOpacity = rhs.lastOpacity == &rhs.opacity ? &opacity : &_lastOpacityData;
}

void KoCompositeOp::ParameterInfo::updateOpacityAndAverage(float value)
{
    const float exponent = 0.1;

    opacity = value;

    if (*lastOpacity < opacity) {
        lastOpacity = &opacity;
    } else {
        _lastOpacityData = exponent * opacity + (1.0 - exponent) * (*lastOpacity);
        lastOpacity = &_lastOpacityData;
    }
}
