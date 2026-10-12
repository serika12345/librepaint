// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

uint mul255(uint a, uint b) { return (a * b + 127u) / 255u; }
uvec4 unpackPixel(uint pixel) { return (uvec4(pixel) >> uvec4(0, 8, 16, 24)) & 255u; }
uint packPixel(uvec4 pixel) { return pixel.x | (pixel.y << 8u) | (pixel.z << 16u) | (pixel.w << 24u); }

uvec4 over(uvec4 destination, uvec4 source) {
    uint denominator = source.w * 255u + destination.w * (255u - source.w);
    uvec3 color = uvec3(0);
    if (denominator != 0u) {
        color = (source.xyz * source.w * 255u + destination.xyz * destination.w * (255u - source.w) + denominator / 2u) / denominator;
    }
    return uvec4(color, (denominator + 127u) / 255u);
}
