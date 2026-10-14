// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct CompositeParameters { opacity: u32, normalizeBase: u32, padding: vec2<u32>, }
@group(0) @binding(0) var<storage, read> basePixels: array<u32>;
@group(0) @binding(1) var<storage, read> layerPixels: array<u32>;
@group(0) @binding(2) var<storage, read_write> outputPixels: array<u32>;
@group(0) @binding(3) var<uniform> parameters: CompositeParameters;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let position = invocation.xy;
    if (any(position >= vec2<u32>(256u))) { return; }
    let index = position.y * 256u + position.x;
    var destination = unpackPixel(basePixels[index]);
    if (parameters.normalizeBase != 0u && destination.w == 0u) { destination = vec4<u32>(0u); }
    var source = unpackPixel(layerPixels[index]);
    source.w = mul255(source.w, parameters.opacity);
    if (source.w == 0u) { outputPixels[index] = packPixel(destination); }
    else { outputPixels[index] = packPixel(over(destination, source)); }
}
