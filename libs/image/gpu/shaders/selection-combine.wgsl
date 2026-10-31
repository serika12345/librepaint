// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct SelectionParameters {
    extent: vec4<i32>, tile: vec2<i32>, action: u32, inverted: u32
}
@group(0) @binding(0) var<uniform> parameters: SelectionParameters;
@group(0) @binding(1) var<storage, read> first: array<u32>;
@group(0) @binding(2) var<storage, read> second: array<u32>;
@group(0) @binding(3) var<storage, read_write> outputPixels: array<u32>;
@compute @workgroup_size(8, 8) fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= vec2<u32>(256u))) { return; }
    let point = parameters.tile + vec2<i32>(p);
    let local = point - parameters.extent.xy;
    if (any(local < vec2<i32>()) || any(local >= parameters.extent.zw)) { return; }
    let index = p.y * 256u + p.x;
    let shift = (index & 3u) * 8u;
    var a = (first[index >> 2u] >> shift) & 255u;
    let b = (second[index >> 2u] >> shift) & 255u;
    if (parameters.inverted != 0u) { a = 255u - a; }
    var value = b;
    switch parameters.action {
        case 1u: { value = min(a + b, 255u); }
        case 2u: { value = a - min(a, b); }
        case 3u: { value = min(a, b); }
        case 4u: { value = u32(abs(i32(a) - i32(b))); }
        default: {}
    }
    outputPixels[u32(local.y * parameters.extent.z + local.x)] = value;
}
