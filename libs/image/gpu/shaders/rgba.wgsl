// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

fn mul255(a: u32, b: u32) -> u32 { return (a * b + 127u) / 255u; }
fn unpackPixel(pixel: u32) -> vec4<u32> {
    return (vec4<u32>(pixel) >> vec4<u32>(0u, 8u, 16u, 24u)) & vec4<u32>(255u);
}
fn packPixel(pixel: vec4<u32>) -> u32 {
    return pixel.x | (pixel.y << 8u) | (pixel.z << 16u) | (pixel.w << 24u);
}
fn over(destination: vec4<u32>, source: vec4<u32>) -> vec4<u32> {
    let denominator = source.w * 255u + destination.w * (255u - source.w);
    var color = vec3<u32>(0u);
    if (denominator != 0u) {
        color = (source.xyz * source.w * 255u + destination.xyz * destination.w * (255u - source.w) + vec3<u32>(denominator / 2u)) / denominator;
    }
    return vec4<u32>(color, (denominator + 127u) / 255u);
}
