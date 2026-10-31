// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters {
    tile: vec2<i32>, inverted: u32, selected: u32,
    copyRect: vec4<i32>, outputRect: vec4<i32>,
    defaultGray: u32, hasTransformed: u32, padding: vec2<u32>
}
@group(0) @binding(0) var<uniform> parameters: Parameters;
@group(0) @binding(1) var<storage, read> mask: array<u32>;
@group(0) @binding(2) var<storage, read> selection: array<u32>;
@group(0) @binding(3) var<storage, read_write> captured: array<u32>;
@group(0) @binding(4) var<storage, read> transformed: array<u32>;
@group(0) @binding(5) var<storage, read_write> output: array<u32>;
fn multiply(a: u32, b: u32) -> u32 {
    let product = a * b + 128u;
    return (product + (product >> 8u)) >> 8u;
}
fn selectedness(index: u32, point: vec2<i32>) -> u32 {
    if (parameters.selected == 0u) { return 255u; }
    if (any(point < parameters.copyRect.xy) || any(point >= parameters.copyRect.zw)) { return 0u; }
    let amount = (selection[index >> 2u] >> ((index & 3u) * 8u)) & 255u;
    return select(amount, 255u - amount, parameters.inverted != 0u);
}
@compute @workgroup_size(64) fn capture(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let index = invocation.x;
    if (index >= 65536u) { return; }
    let point = parameters.tile + vec2<i32>(i32(index % 256u), i32(index / 256u));
    let alpha = selectedness(index, point);
    let gray = select(0u, (mask[index >> 2u] >> ((index & 3u) * 8u)) & 255u, alpha != 0u);
    captured[index] = gray * 65793u + (alpha << 24u);
}
@compute @workgroup_size(64) fn merge(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let word = invocation.x;
    if (word >= 16384u) { return; }
    var packed = 0u;
    for (var component = 0u; component < 4u; component++) {
        let index = word * 4u + component;
        let point = parameters.tile + vec2<i32>(i32(index % 256u), i32(index / 256u));
        var base = parameters.defaultGray;
        if (parameters.selected != 0u) {
            base = (mask[word] >> (component * 8u)) & 255u;
            base = multiply(base, 255u - selectedness(index, point));
        }
        if (parameters.hasTransformed != 0u && all(point >= parameters.outputRect.xy) && all(point < parameters.outputRect.zw)) {
            let pixel = transformed[index];
            let alpha = pixel >> 24u;
            // Native GrayA Over onto the opaque gray representation of Alpha8.
            var blend = (i32(pixel & 255u) - i32(base)) * i32(alpha) + 128;
            blend = ((blend >> 8u) + blend) >> 8u;
            base = u32(blend + i32(base));
        }
        packed |= min(base, 255u) << (component * 8u);
    }
    output[word] = packed;
}
