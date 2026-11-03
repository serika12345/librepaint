// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// U16 normal composition evaluates normalized alpha and color in native f32.
fn nativeNormalizedAlpha(component: u32) -> u32 {
    return projectionMultiply(bitcast<u32>(f32(component)), 0x37800080u);
}
fn nativeCombinedAlpha(destination: u32, source: u32) -> u32 {
    let alpha = bitcast<f32>(destination);
    return bitcast<u32>(alpha + bitcast<f32>(source) * (1.0 - alpha));
}
fn nativeColor(destination: u32, source: u32, blend: u32) -> u32 {
    let value = f32(destination) + f32(i32(source) - i32(destination)) * bitcast<f32>(blend);
    return u32(round(clamp(value, 0.0, 65535.0)));
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
