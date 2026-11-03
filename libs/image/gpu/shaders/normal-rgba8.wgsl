// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// The caller computes source alpha in binary32 after opacity and selection.
// writeMask is the RGBA channel mask, including alpha protection.
fn normalRgba8(base: vec4<u32>, source: vec3<u32>, sourceAlpha: u32, writeMask: u32) -> u32 {
    if (sourceAlpha == 0u) { return packPixel(base); }
    var destination = base;
    var alpha = destination.w;
    var blend: u32;
    if ((writeMask & 8u) == 0u || destination.w == 255u) {
        blend = projectionMultiply(sourceAlpha, 0x3b808081u);
    } else if (destination.w == 0u) {
        alpha = projectionComponent(sourceAlpha);
        blend = 0x3f800000u;
        if (writeMask != 15u) { destination = vec4<u32>(0u); }
    } else {
        let value = projectionAlpha(sourceAlpha, destination.w);
        alpha = projectionComponent(value);
        blend = projectionDivide(sourceAlpha, value);
    }
    var color = destination.xyz;
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((writeMask & (1u << channel)) == 0u) { continue; }
        if (blend == 0x3f800000u) { color[channel] = source[channel]; }
        else { color[channel] = projectionColor(destination[channel], source[channel], blend); }
    }
    return packPixel(vec4<u32>(color, alpha));
}
