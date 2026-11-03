// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Single-round binary32 multiply-add for the native Copy opacity interpolation.
// The 61-bit accumulator keeps a sticky tail below the final rounding bits.
fn copyAligned(value: vec2<u32>, power: i32, commonPower: i32) -> vec2<u32> {
    if (power >= commonPower) { return projectionLeft(value, u32(power - commonPower)); }
    let shift = u32(commonPower - power);
    var result = projectionRight(value, shift);
    if (projectionLowBits(value, shift)) { result.x |= 1u; }
    return result;
}
fn copyInterpolate(destination: u32, source: u32, opacity: u32) -> u32 {
    let difference = floatSubtract(source, destination);
    if ((difference & 0x7fffffffu) == 0u) { return destination; }
    let product = projectionProduct(projectionMantissa(difference), projectionMantissa(opacity));
    let productPower = projectionPower(difference) + projectionPower(opacity);
    let basePower = projectionPower(destination);
    let leading = select(31u - countLeadingZeros(product.x), 63u - countLeadingZeros(product.y), product.y != 0u);
    let highest = max(productPower + i32(leading), basePower + 23);
    let commonPower = max(min(productPower, basePower), highest - 60);
    let term = copyAligned(product, productPower, commonPower);
    let base = copyAligned(vec2<u32>(projectionMantissa(destination), 0u), basePower, commonPower);
    var result: vec2<u32>;
    if ((difference & 0x80000000u) != 0u) { result = projectionSubtract(base, term); }
    else { result = projectionAdd(base, term); }
    return projectionFloat(result, commonPower);
}
fn copyPixel(base: vec4<u32>, source: vec4<u32>, opacity: u32, writeMask: u32, wide: bool) -> vec4<u32> {
    let locked = (writeMask & 8u) == 0u;
    if ((locked && source.w == 0u) || opacity == 0u) { return base; }
    var result = base;
    if (opacity == 0x3f800000u) {
        if (writeMask == 15u) { return source; }
        if (!locked && base.w == 0u) { result = vec4<u32>(0u); }
        for (var channel = 0u; channel < 4u; channel += 1u) {
            if ((writeMask & (1u << channel)) != 0u) { result[channel] = source[channel]; }
        }
        return result;
    }
    let maximum = select(0x437f0000u, 0x477fff00u, wide);
    let reciprocal = select(0x3b808081u, 0x37800080u, wide);
    let sourceAlpha = projectionMultiply(bitcast<u32>(f32(source.w)), reciprocal);
    let destinationAlpha = projectionMultiply(bitcast<u32>(f32(base.w)), reciprocal);
    let alpha = copyInterpolate(destinationAlpha, sourceAlpha, opacity);
    if (alpha == 0u) {
        if (writeMask == 15u || destinationAlpha == 0u) { return vec4<u32>(0u); }
        for (var channel = 0u; channel < 4u; channel += 1u) {
            if ((writeMask & (1u << channel)) != 0u) { result[channel] = 0u; }
        }
        return result;
    }
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((writeMask & (1u << channel)) == 0u) {
            if (!locked && destinationAlpha == 0u) { result[channel] = 0u; }
            continue;
        }
        let sourceColor = projectionMultiply(bitcast<u32>(f32(source[channel])), sourceAlpha);
        let destinationColor = projectionMultiply(bitcast<u32>(f32(base[channel])), destinationAlpha);
        var color = copyInterpolate(destinationColor, sourceColor, opacity);
        if (alpha != 0x3f800000u) { color = min(projectionDivide(color, alpha), maximum); }
        if (!locked && destinationAlpha == 0u && (writeMask & 7u) != 7u) {
            result[channel] = u32(bitcast<f32>(color));
        } else { result[channel] = projectionComponent(color); }
    }
    if (!locked) { result.w = projectionComponent(projectionMultiply(alpha, maximum)); }
    return result;
}
