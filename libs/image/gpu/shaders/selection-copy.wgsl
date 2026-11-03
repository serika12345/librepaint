// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct Parameters {
    format: u32, inverted: u32, implicitSource: u32, projection: u32,
    implicit: vec4<u32>, extent: vec4<i32>,
}
@group(0) @binding(0) var<storage, read> source: array<u32>;
@group(0) @binding(1) var<storage, read_write> destination: array<u32>;
@group(0) @binding(2) var<uniform> parameters: Parameters;
@group(0) @binding(3) var<storage, read> coverage: array<u32>;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let index = id.y * 256u + id.x;
    let wide = parameters.format != 0u;
    let stride = select(1u, 2u, wide);
    let offset = index * stride;
    var pixel = parameters.implicit;
    if (parameters.implicitSource == 0u) {
        pixel = nativePixel(source[offset], source[offset + stride - 1u], wide);
    }
    var amount = (coverage[index >> 2u] >> ((index & 3u) * 8u)) & 255u;
    if (parameters.inverted != 0u) { amount = 255u - amount; }
    if (parameters.projection != 0u) {
        let point = vec2<i32>(id.xy);
        if (any(point < parameters.extent.xy) || any(point >= parameters.extent.zw)) { amount = 255u; }
    }
    if (wide) { pixel.w = mul65535(pixel.w, amount * 257u); }
    else { pixel.w = mul255(pixel.w, amount); }
    if (parameters.projection != 0u) {
        if (amount != 255u && pixel.w == 0u) { pixel = vec4<u32>(0u); }
    }
    let words = nativePixelWords(pixel, wide);
    for (var word = 0u; word < stride; word += 1u) { destination[offset + word] = words[word]; }
}
