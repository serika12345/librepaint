/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
struct Parameters {
    origin: vec2<i32>,
    implicitPixel: u32,
    nondefault: u32,
    clip: vec4<u32>,
    native: vec4<u32>,
};
@group(0) @binding(0) var<storage, read> pixels: array<u32>;
@group(0) @binding(1) var<storage, read_write> bounds: array<atomic<i32>, 4>;
@group(0) @binding(2) var<uniform> parameters: Parameters;
var<workgroup> extrema: array<vec4<i32>, 256>;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) id: vec3<u32>,
        @builtin(local_invocation_index) lane: u32) {
    var value = vec4<i32>(2147483647, 2147483647, -2147483647 - 1, -2147483647 - 1);
    if (all(id.xy >= parameters.clip.xy) && all(id.xy < parameters.clip.zw)) {
        let wide = parameters.native.y != 0u;
        let stride = select(1u, 2u, wide);
        let word = (id.y * 256u + id.x) * stride;
        let pixel = nativePixel(pixels[word], pixels[word + stride - 1u], wide);
        let implicit = nativePixel(parameters.implicitPixel, parameters.native.x, wide);
        if (select(pixel.w != 0u, any(pixel != implicit), parameters.nondefault != 0u)) {
            let point = parameters.origin + vec2<i32>(id.xy);
            value = vec4<i32>(point, point);
        }
    }
    extrema[lane] = value;
    workgroupBarrier();
    for (var stride = 128u; stride > 0u; stride /= 2u) {
        if (lane < stride) {
            let other = extrema[lane + stride];
            extrema[lane] = vec4<i32>(min(extrema[lane].xy, other.xy), max(extrema[lane].zw, other.zw));
        }
        workgroupBarrier();
    }
    if (lane == 0u) {
        atomicMin(&bounds[0], extrema[0].x);
        atomicMin(&bounds[1], extrema[0].y);
        atomicMax(&bounds[2], extrema[0].z);
        atomicMax(&bounds[3], extrema[0].w);
    }
}
