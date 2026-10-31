// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct TranslationParameters {
    source: vec4<i32>, extent: vec4<i32>, bounds: vec4<i32>, offset: vec4<i32>
}
@group(0) @binding(0) var<uniform> parameters: TranslationParameters;
@group(0) @binding(1) var<storage, read> sourcePixels: array<u32>;
@group(0) @binding(2) var<storage, read_write> outputPixels: array<u32>;
@compute @workgroup_size(8, 8) fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let local = vec2<i32>(invocation.xy);
    if (any(local >= parameters.extent.zw)) { return; }
    let point = parameters.extent.xy + local - parameters.offset.xy;
    let relative = point - parameters.source.xy;
    var alpha = 0u;
    if (all(point >= parameters.bounds.xy) && all(point < parameters.bounds.zw) &&
        all(relative >= vec2<i32>()) && all(relative < parameters.source.zw)) {
        alpha = sourcePixels[u32(relative.y * parameters.source.z + relative.x)];
    }
    outputPixels[u32(local.y * parameters.extent.z + local.x)] = alpha;
}
