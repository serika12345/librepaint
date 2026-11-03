// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct Parameters {
    origin: vec2<i32>, format: u32, flags: u32,
    copyRect: vec4<i32>, clearRect: vec4<i32>,
    implicit: vec4<u32>, defaultPixel: vec4<u32>
}
@group(0) @binding(0) var<storage, read> source: array<u32>;
@group(0) @binding(1) var<storage, read> base: array<u32>;
@group(0) @binding(2) var<storage, read_write> destination: array<u32>;
@group(0) @binding(3) var<storage, read_write> cleared: array<u32>;
@group(0) @binding(4) var<uniform> parameters: Parameters;
@group(0) @binding(5) var<storage, read> coverage: array<u32>;

fn inside(position: vec2<i32>, bounds: vec4<i32>) -> bool {
    return all(position >= bounds.xy) && all(position < bounds.zw);
}
fn composeNormal(background: vec4<u32>, foreground: vec4<u32>, amount: u32, wide: bool) -> vec4<u32> {
    var alpha = f32(foreground.w);
    if (wide) { alpha = nativeNormalizedAlpha(foreground.w); }
    if (amount != 255u) { alpha = (alpha * (0.00392156886 * f32(amount))); }
    if (wide) { return normalRgba16(background, foreground.xyz, alpha, 15u); }
    return unpackPixel(normalRgba8(background, foreground.xyz, alpha, 15u));
}

@compute @workgroup_size(8, 8)
fn split(@builtin(global_invocation_id) id: vec3<u32>) {
    let index = id.y * 256u + id.x;
    let wide = parameters.format != 0u;
    let stride = select(1u, 2u, wide);
    let offset = index * stride;
    let position = parameters.origin + vec2<i32>(id.xy);
    var pixel = parameters.implicit;
    if ((parameters.flags & 2u) == 0u) {
        pixel = nativePixel(source[offset], source[offset + stride - 1u], wide);
    }
    var amount = (coverage[index >> 2u] >> ((index & 3u) * 8u)) & 255u;
    if ((parameters.flags & 1u) != 0u) { amount = 255u - amount; }
    var captured = vec4<u32>(0u);
    if (inside(position, parameters.copyRect)) { captured = composeNormal(captured, pixel, amount, wide); }
    var remainder = pixel;
    if (inside(position, parameters.clearRect)) {
        var defaultAlpha = parameters.defaultPixel.w;
        var resultAlpha: u32;
        if (wide) {
            remainder.w = mul65535(pixel.w, (255u - amount) * 257u);
            resultAlpha = (remainder.w - (remainder.w >> 8u) + 128u) >> 8u;
            defaultAlpha = (defaultAlpha - (defaultAlpha >> 8u) + 128u) >> 8u;
        } else {
            remainder.w = mul255(pixel.w, 255u - amount);
            resultAlpha = remainder.w;
        }
        if (defaultAlpha == 0u && resultAlpha == 0u) { remainder = parameters.defaultPixel; }
    }
    let capturedWords = nativePixelWords(captured, wide);
    let remainderWords = nativePixelWords(remainder, wide);
    for (var word = 0u; word < stride; word += 1u) {
        destination[offset + word] = capturedWords[word];
        cleared[offset + word] = remainderWords[word];
    }
}

@compute @workgroup_size(8, 8)
fn merge(@builtin(global_invocation_id) id: vec3<u32>) {
    let wide = parameters.format != 0u;
    let stride = select(1u, 2u, wide);
    let offset = (id.y * 256u + id.x) * stride;
    var foreground = vec4<u32>(0u);
    if ((parameters.flags & 2u) == 0u) {
        foreground = nativePixel(source[offset], source[offset + stride - 1u], wide);
    }
    var background = parameters.implicit;
    if ((parameters.flags & 4u) == 0u) {
        background = nativePixel(base[offset], base[offset + stride - 1u], wide);
    }
    let words = nativePixelWords(composeNormal(background, foreground, 255u, wide), wide);
    for (var word = 0u; word < stride; word += 1u) { destination[offset + word] = words[word]; }
}
