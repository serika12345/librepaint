// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later


fn copyInterpolate(destination: f32, source: f32, opacity: f32) -> f32 {
    return destination + (source - destination) * opacity;
}
fn copyPixel(base: vec4<u32>, source: vec4<u32>, opacity: f32, writeMask: u32, wide: bool) -> vec4<u32> {
    let locked = (writeMask & 8u) == 0u;
    if ((locked && source.w == 0u) || opacity == 0.0) { return base; }
    var result = base;
    if (opacity == 1.0) {
        if (writeMask == 15u) { return source; }
        if (!locked && base.w == 0u) { result = vec4<u32>(0u); }
        for (var channel = 0u; channel < 4u; channel += 1u) {
            if ((writeMask & (1u << channel)) != 0u) { result[channel] = source[channel]; }
        }
        return result;
    }
    let maximum = select(255.0, 65535.0, wide);
    let reciprocal = select(0.00392156886, 1.52590219e-05, wide);
    let sourceAlpha = (f32(source.w) * reciprocal);
    let destinationAlpha = (f32(base.w) * reciprocal);
    let alpha = copyInterpolate(destinationAlpha, sourceAlpha, opacity);
    if (alpha == 0.0) {
        if (writeMask == 15u || destinationAlpha == 0.0) { return vec4<u32>(0u); }
        for (var channel = 0u; channel < 4u; channel += 1u) {
            if ((writeMask & (1u << channel)) != 0u) { result[channel] = 0u; }
        }
        return result;
    }
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((writeMask & (1u << channel)) == 0u) {
            if (!locked && destinationAlpha == 0.0) { result[channel] = 0u; }
            continue;
        }
        let sourceColor = (f32(source[channel]) * sourceAlpha);
        let destinationColor = (f32(base[channel]) * destinationAlpha);
        var color = copyInterpolate(destinationColor, sourceColor, opacity);
        if (alpha != 1.0) { color = min(safeRatio(color, alpha), maximum); }
        if (!locked && destinationAlpha == 0.0 && (writeMask & 7u) != 7u) {
            result[channel] = u32(color);
        } else { result[channel] = roundComponent(color); }
    }
    if (!locked) { result.w = roundComponent((alpha * maximum)); }
    return result;
}
