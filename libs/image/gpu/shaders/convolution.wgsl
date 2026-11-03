// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct ConvolutionParameters {
    area: vec4<i32>,
    // Native filter kind, RGBA16 flag, enabled RGBA components, padded stride.
    control: vec4<u32>,
    destination: vec4<i32>,
}
@group(0) @binding(0) var<storage, read> neighbours: array<u32>;
@group(0) @binding(1) var<storage, read_write> convolved: array<u32>;
@group(0) @binding(2) var<uniform> convolution: ConvolutionParameters;

fn convolutionWeight(kind: u32, index: u32) -> i32 {
    if (kind == 7u) { return array<i32, 9>(0, -2, 0, -2, 11, -2, 0, -2, 0)[index]; }
    if (kind == 8u) { return array<i32, 9>(-1, -1, -1, -1, 9, -1, -1, -1, -1)[index]; }
    if (kind == 9u) { return array<i32, 9>(-1, 0, -1, 0, 4, 0, -1, 0, -1)[index]; }
    if (kind == 10u) { return array<i32, 9>(-1, -1, -1, -1, 8, -1, -1, -1, -1)[index]; }
    if (kind == 11u) { return array<i32, 9>(0, -1, 0, -1, 4, -1, 0, -1, 0)[index]; }
    if (kind == 12u) { return array<i32, 9>(0, -1, 0, 0, 2, 0, 0, -1, 0)[index]; }
    return array<i32, 9>(0, 0, 0, -1, 2, -1, 0, 0, 0)[index];
}
fn convolutionSigned(value: Word64) -> Word64 {
    let negative = (value.y & 0x80000000u) != 0u;
    let magnitude = select(value, wordSubtract(Word64(0u), value), negative);
    return doubleFloat(magnitude, 0, select(0u, 0x80000000u, negative));
}
fn convolutionClamp(value: Word64, maximum: Word64) -> Word64 {
    if ((value.y & 0x80000000u) != 0u) { return Word64(0u); }
    if (!wordLessEqual(value, maximum)) { return maximum; }
    return value;
}
fn convolutionRounded(value: Word64) -> u32 {
    return doubleTruncate(doubleAdd(value, Word64(0u, 0x3fe00000u)));
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let local = invocation.xy;
    if (any(local >= vec2<u32>(convolution.area.zw))) { return; }
    let wide = convolution.control.y != 0u;
    let enabled = convolution.control.z;
    let alphaEnabled = (enabled & 8u) != 0u;
    let stride = convolution.control.w;
    let words = select(1u, 2u, wide);
    var sums: array<Word64, 4>;
    for (var k = 0u; k < 9u; k += 1u) {
        let offset = ((local.y + k / 3u) * stride + local.x + k % 3u) * words;
        var high = 0u;
        if (wide) { high = neighbours[offset + 1u]; }
        let pixel = nativePixel(neighbours[offset], high, wide);
        let weight = convolutionWeight(convolution.control.x, 8u - k);
        for (var c = 0u; c < 4u; c += 1u) {
            var value = pixel[c];
            if (alphaEnabled && c != 3u) { value *= pixel.w; }
            let term = wordProduct(value, u32(abs(weight)));
            if (weight < 0) { sums[c] = wordSubtract(sums[c], term); }
            else { sums[c] = wordAdd(sums[c], term); }
        }
    }
    let middle = ((local.y + 1u) * stride + local.x + 1u) * words;
    var high = 0u;
    if (wide) { high = neighbours[middle + 1u]; }
    var result = nativePixel(neighbours[middle], high, wide);
    let maximum = doubleInteger(select(255u, 65535u, wide));
    let factor = select(Word64(0u, 0x3ff00000u), Word64(0x55555555u, 0x3fd55555u), convolution.control.x == 7u);
    var shift = Word64(0u);
    if (convolution.control.x >= 9u) { shift = doubleMultiply(maximum, Word64(0u, 0x3fe00000u)); }
    var alpha = Word64(0u);
    if (alphaEnabled) {
        alpha = convolutionClamp(doubleAdd(doubleMultiply(convolutionSigned(sums[3]), factor), shift), maximum);
        result.w = convolutionRounded(alpha);
    }
    for (var c = 0u; c < 3u; c += 1u) {
        if ((enabled & (1u << c)) == 0u) { continue; }
        var value = doubleMultiply(convolutionSigned(sums[c]), factor);
        if (alphaEnabled) {
            if (all(alpha == Word64(0u))) { result[c] = 0u; continue; }
            value = doubleMultiply(value, doubleDivide(Word64(0u, 0x3ff00000u), alpha));
        }
        result[c] = convolutionRounded(convolutionClamp(doubleAdd(value, shift), maximum));
    }
    let destination = vec2<u32>(convolution.destination.xy) + local;
    let offset = (destination.y * 256u + destination.x) * words;
    let packed = nativePixelWords(result, wide);
    convolved[offset] = packed.x;
    if (wide) { convolved[offset + 1u] = packed.y; }
}
