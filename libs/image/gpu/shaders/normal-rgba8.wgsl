// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The caller computes source alpha in binary32 after opacity and selection.
// writeMask is the RGBA channel mask, including alpha protection.
fn normalRgba8(base: vec4<u32>, source: vec3<u32>, sourceAlpha: f32, writeMask: u32) -> u32 {
    if (sourceAlpha == 0.0) { return packPixel(base); }
    var destination = base;
    var alpha = destination.w;
    var blend: f32;
    if ((writeMask & 8u) == 0u || destination.w == 255u) {
        blend = (sourceAlpha * 0.00392156886);
    } else if (destination.w == 0u) {
        alpha = roundComponent(sourceAlpha);
        blend = 1.0;
        if (writeMask != 15u) { destination = vec4<u32>(0u); }
    } else {
        let value = alphaOverByte(sourceAlpha, destination.w);
        alpha = roundComponent(value);
        blend = safeRatio(sourceAlpha, value);
    }
    var color = destination.xyz;
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((writeMask & (1u << channel)) == 0u) { continue; }
        if (blend == 1.0) { color[channel] = source[channel]; }
        else { color[channel] = mixByteFloat(destination[channel], source[channel], blend); }
    }
    return packPixel(vec4<u32>(color, alpha));
}
