// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct SelectionParameters {
    extent: vec4<i32>, tile: vec2<i32>, action: u32, inverted: u32, region: vec4<i32>
}
@group(0) @binding(0) var<uniform> parameters: SelectionParameters;
@group(0) @binding(1) var<storage, read> first: array<u32>;
@group(0) @binding(2) var<storage, read> second: array<u32>;
@group(0) @binding(3) var<storage, read_write> outputPixels: array<u32>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let word = invocation.x;
    if (word >= 16384u) { return; }
    var packed = 0u;
    for (var component = 0u; component < 4u; component++) {
        let index = word * 4u + component;
        let point = parameters.tile + vec2<i32>(i32(index % 256u), i32(index / 256u));
        let local = point - parameters.extent.xy;
        let shift = component * 8u;
        if (any(local < vec2<i32>()) || any(local >= parameters.extent.zw)) {
            packed |= (parameters.inverted >> 8u) << shift;
            continue;
        }
        var a = (first[word] >> shift) & 255u;
        let b = (second[word] >> shift) & 255u;
        if (parameters.action == 7u) {
            packed |= (255u - a) << shift;
            continue;
        }
        if ((parameters.inverted & 1u) != 0u) { amount = 255u - amount; }
            let inside = all(point >= parameters.region.xy) && all(point < parameters.region.zw);
            var value = a;
            if (parameters.action == 5u) { value = select(0u, multiplyAlpha(a, amount), inside); }
            else {
                if (inside) { value = multiplyAlpha(a, 255u - amount); }
                let foreground = (replacement[word] >> shift) & 255u;
                value = foreground + multiplyAlpha(value, 255u - foreground);
            }
            packed |= value << shift;
            continue;
        }
        if ((parameters.inverted & 1u) != 0u) { a = 255u - a; }
        var value = b;
        switch parameters.action {
            case 1u: { value = min(a + b, 255u); }
            case 2u: { value = a - min(a, b); }
            case 3u: { value = min(a, b); }
            case 4u: { value = u32(abs(i32(a) - i32(b))); }
            default: {}
        }
        packed |= value << shift;
    }
    outputPixels[word] = packed;
}
