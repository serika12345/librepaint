/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
struct Parameters {
    origin: vec2<i32>, center: vec2<i32>,
    clip: vec4<u32>, canvasSize: vec2<i32>,
    radiusSquared: u32, projection: u32,
    implicitPixel: u32, present: u32, radiusOne: u32, padding: u32,
}
@group(0) @binding(0) var<storage, read> pixels: array<u32>;
// Three exact 64-bit color sums, followed by total alpha and disk sample count.
@group(0) @binding(1) var<storage, read_write> summary: array<atomic<u32>, 8>;
@group(0) @binding(2) var<uniform> parameters: Parameters;
var<workgroup> colors: array<vec4<u32>, 256>;
var<workgroup> counts: array<u32, 256>;

fn addColor(index: u32, value: u32) {
    let previous = atomicAdd(&summary[index], value);
    if (previous + value < previous) { atomicAdd(&summary[index + 1u], 1u); }
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) id: vec3<u32>,
        @builtin(local_invocation_index) lane: u32) {
    let local = parameters.clip.xy + id.xy;
    let point = parameters.origin + vec2<i32>(local);
    let delta = point - parameters.center;
    let inDisk = select(u32(dot(delta, delta)) < parameters.radiusSquared,
                        all(delta == vec2<i32>(0)), parameters.radiusOne != 0u);
    var count = 0u;
    var rgba = vec4<u32>(0u);
    if (all(local < parameters.clip.zw) && inDisk) {
        count = 1u;
        var value = parameters.implicitPixel;
        if (parameters.present != 0u) { value = pixels[local.y * 256u + local.x]; }
        if (parameters.projection != 0u &&
            (any(point < vec2<i32>(0)) || any(point >= parameters.canvasSize))) { value = 0u; }
        let alpha = value >> 24u;
        rgba = vec4<u32>((value & 255u) * alpha, ((value >> 8u) & 255u) * alpha,
                         ((value >> 16u) & 255u) * alpha, alpha);
    }
    colors[lane] = rgba;
    counts[lane] = count;
    workgroupBarrier();
    for (var stride = 128u; stride != 0u; stride >>= 1u) {
        if (lane < stride) {
            colors[lane] += colors[lane + stride];
            counts[lane] += counts[lane + stride];
        }
        workgroupBarrier();
    }
    if (lane == 0u) {
        addColor(0u, colors[0].r);
        addColor(2u, colors[0].g);
        addColor(4u, colors[0].b);
        atomicAdd(&summary[6], colors[0].a);
        atomicAdd(&summary[7], counts[0]);
    }
}
