/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
struct Parameters {
    origin: vec2<i32>, center: vec2<i32>,
    clip: vec4<u32>, canvasSize: vec2<i32>,
    radiusSquared: u32, projection: u32,
    implicitPixel: vec2<u32>, flags: u32, padding: u32,
}
@group(0) @binding(0) var<storage, read> pixels: array<u32>;
// Three exact 64-bit RGB sums. U8 uses U32 alpha/count (32 bytes);
// U16 uses U64 alpha and U32 count (36 bytes).
@group(0) @binding(1) var<storage, read_write> summary: array<atomic<u32>>;
@group(0) @binding(2) var<uniform> parameters: Parameters;
var<workgroup> colors: array<vec4<u32>, 256>;
var<workgroup> counts: array<u32, 256>;
var<workgroup> colorHigh: array<vec3<u32>, 256>;

fn addColor(index: u32, low: u32, high: u32) {
    let previous = atomicAdd(&summary[index], low);
    let carry = high + select(0u, 1u, previous + low < previous);
    if (carry != 0u) { atomicAdd(&summary[index + 1u], carry); }
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) id: vec3<u32>,
        @builtin(local_invocation_index) lane: u32) {
    let wide = (parameters.flags & 2u) != 0u;
    let local = parameters.clip.xy + id.xy;
    let point = parameters.origin + vec2<i32>(local);
    let delta = point - parameters.center;
    let inDisk = select(u32(dot(delta, delta)) < parameters.radiusSquared,
                        all(delta == vec2<i32>(0)), parameters.radiusSquared == 0u);
    var count = 0u;
    var rgba = vec4<u32>(0u);
    if (all(local < parameters.clip.zw) && inDisk) {
        count = 1u;
        var value = nativePixel(parameters.implicitPixel.x, parameters.implicitPixel.y, wide);
        if ((parameters.flags & 1u) != 0u) {
            let stride = select(1u, 2u, wide);
            let word = (local.y * 256u + local.x) * stride;
            value = nativePixel(pixels[word], pixels[word + stride - 1u], wide);
        }
        if (parameters.projection != 0u &&
            (any(point < vec2<i32>(0)) || any(point >= parameters.canvasSize))) { value = vec4<u32>(0u); }
        rgba = vec4<u32>(value.xyz * value.w, value.w);
    }
    colors[lane] = rgba;
    colorHigh[lane] = vec3<u32>(0u);
    counts[lane] = count;
    workgroupBarrier();
    for (var stride = 128u; stride != 0u; stride >>= 1u) {
        if (lane < stride) {
            let previous = colors[lane];
            let sum = previous + colors[lane + stride];
            colorHigh[lane] += colorHigh[lane + stride] + select(vec3<u32>(0u), vec3<u32>(1u), sum.xyz < previous.xyz);
            colors[lane] = sum;
            counts[lane] += counts[lane + stride];
        }
        workgroupBarrier();
    }
    if (lane == 0u) {
        addColor(0u, colors[0].r, colorHigh[0].r);
        addColor(2u, colors[0].g, colorHigh[0].g);
        addColor(4u, colors[0].b, colorHigh[0].b);
        if (wide) {
            addColor(6u, colors[0].a, 0u);
            atomicAdd(&summary[8], counts[0]);
        } else {
            atomicAdd(&summary[6], colors[0].a);
            atomicAdd(&summary[7], counts[0]);
        }
    }
}
