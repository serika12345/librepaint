// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later


fn copyInterpolate(destination: u32, source: u32, opacity: u32) -> u32 {
    let base = bitcast<f32>(destination);
    return bitcast<u32>(base + (bitcast<f32>(source) - base) * bitcast<f32>(opacity));
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
