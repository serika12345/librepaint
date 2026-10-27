// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Native U16 normal composition follows binary32 normalization and division,
// single-rounded alpha/color multiply-add, then nearest-even U16 conversion.
fn nativeNormalizedAlpha(component: u32) -> u32 {
    return projectionMultiply(bitcast<u32>(f32(component)), 0x37800080u);
}
fn nativeCombinedAlpha(destination: u32, source: u32) -> u32 {
    let power = projectionPower(destination);
    let one = projectionLeft(vec2<u32>(1u, 0u), u32(-power));
    let complement = projectionFloat(projectionSubtract(one, vec2<u32>(projectionMantissa(destination), 0u)), power);
    let product = projectionProduct(projectionMantissa(complement), projectionMantissa(source));
    let productPower = projectionPower(complement) + projectionPower(source);
    // A nonzero native destination is at least 1/65535. Its mantissa and the
    // product share a 61-bit accumulator; discarded product bits become sticky
    // below every binary32 guard bit, including a halfway result.
    let commonPower = max(productPower, power - 37);
    let shift = u32(commonPower - productPower);
    var retained = projectionRight(product, shift);
    if (projectionLowBits(product, shift)) { retained.x |= 1u; }
    let base = projectionLeft(vec2<u32>(projectionMantissa(destination), 0u), u32(power - commonPower));
    return projectionFloat(projectionAdd(base, retained), commonPower);
}
fn nativeColor(destination: u32, source: u32, blend: u32) -> u32 {
    // Below 2^-18 a U16 color changes by less than a quarter component,
    // including binary32 rounding before nearest-even integer conversion.
    if (blend < 0x36800000u) { return destination; }
    if (blend == 0x3f800000u) { return source; }
    let difference = i32(source) - i32(destination);
    let product = projectionProduct(u32(abs(difference)), projectionMantissa(blend));
    let power = projectionPower(blend);
    let base = projectionLeft(vec2<u32>(destination, 0u), u32(-power));
    var value: vec2<u32>;
    if (difference < 0) { value = projectionSubtract(base, product); }
    else { value = projectionAdd(base, product); }
    return projectionComponent(projectionFloat(value, power));
}
fn normalRgba16(base: vec4<u32>, source: vec3<u32>, sourceAlpha: u32, writeMask: u32) -> vec4<u32> {
    if (sourceAlpha == 0u) { return base; }
    var destination = base;
    var alpha = nativeNormalizedAlpha(base.w);
    var blend: u32;
    if ((writeMask & 8u) == 0u || base.w == 65535u) {
        blend = sourceAlpha;
    } else if (base.w == 0u) {
        alpha = sourceAlpha;
        blend = 0x3f800000u;
        if (writeMask != 15u) { destination = vec4<u32>(0u); }
    } else {
        alpha = nativeCombinedAlpha(alpha, sourceAlpha);
        blend = projectionDivide(sourceAlpha, alpha);
    }
    var color = destination.xyz;
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((writeMask & (1u << channel)) != 0u) {
            color[channel] = nativeColor(destination[channel], source[channel], blend);
        }
    }
    var outputAlpha = base.w;
    if ((writeMask & 8u) != 0u) {
        outputAlpha = projectionComponent(projectionMultiply(alpha, 0x477fff00u));
    }
    return vec4<u32>(color, outputAlpha);
}
