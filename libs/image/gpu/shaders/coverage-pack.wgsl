// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct CoverageParameters {
    source: vec4<i32>, tile: vec4<i32>, bounds: vec4<i32>
}
@group(0) @binding(0) var<uniform> parameters: CoverageParameters;
@group(0) @binding(1) var<storage, read> inputPixels: array<u32>;
@group(0) @binding(2) var<storage, read_write> outputPixels: array<u32>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let first = invocation.x * 4u;
    if (first >= 65536u) { return; }
    var packed = 0u;
    for (var component = 0u; component < 4u; component += 1u) {
        let index = first + component;
        let point = parameters.tile.xy + vec2<i32>(i32(index % 256u), i32(index / 256u));
        var alpha = 0u;
        if (all(point >= parameters.bounds.xy) && all(point < parameters.bounds.zw)) {
            if (parameters.tile.z <= 1) {
                let relative = point - parameters.source.xy;
                if (all(relative >= vec2<i32>()) && all(relative < parameters.source.zw)) {
                    alpha = inputPixels[u32(relative.y * parameters.source.z + relative.x)];
                    if (parameters.tile.z == 1) { alpha >>= 24u; }
                }
            } else {
                alpha = u32(parameters.tile.w);
            }
        }
        packed |= (alpha & 255u) << (component * 8u);
    }
    outputPixels[invocation.x] = packed;
}
