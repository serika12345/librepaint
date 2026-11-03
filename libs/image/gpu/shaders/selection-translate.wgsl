// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct TranslationParameters {
    source: vec4<i32>, extent: vec4<i32>, bounds: vec4<i32>, destination: vec4<i32>
}
@group(0) @binding(0) var<uniform> parameters: TranslationParameters;
@group(0) @binding(1) var<storage, read> topLeft: array<u32>;
@group(0) @binding(2) var<storage, read> topRight: array<u32>;
@group(0) @binding(3) var<storage, read> bottomLeft: array<u32>;
@group(0) @binding(4) var<storage, read> bottomRight: array<u32>;
@group(0) @binding(5) var<storage, read_write> outputPixels: array<u32>;
@compute @workgroup_size(64) fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let word = invocation.x;
    if (word >= 16384u) { return; }
    var packed = 0u;
    for (var component = 0u; component < 4u; component++) {
        let index = word * 4u + component;
        let local = vec2<i32>(i32(index % 256u), i32(index / 256u));
        let destination = parameters.destination.xy + local;
        let point = parameters.source.xy + local;
        if (any(destination < parameters.extent.xy) || any(destination >= parameters.extent.zw) ||
            any(point < parameters.bounds.xy) || any(point >= parameters.bounds.zw)) {
            packed |= u32(parameters.destination.z) << (component * 8u);
            continue;
        }
        let relative = point - parameters.source.zw;
        let tile = relative / 256;
        let offset = relative % 256;
        let sourceIndex = u32(offset.y * 256 + offset.x);
        var value = 0u;
        switch tile.y * 2 + tile.x {
            case 0: { value = topLeft[sourceIndex >> 2u]; }
            case 1: { value = topRight[sourceIndex >> 2u]; }
            case 2: { value = bottomLeft[sourceIndex >> 2u]; }
            case 3: { value = bottomRight[sourceIndex >> 2u]; }
            default: {}
        }
        packed |= ((value >> ((sourceIndex & 3u) * 8u)) & 255u) << (component * 8u);
    }
    outputPixels[word] = packed;
}
