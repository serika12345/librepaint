/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
struct Parameters {
    stage: u32, mode: u32, width: u32, rows: u32,
    rowStart: i32, tileX: i32, tileY: i32, firstRow: i32,
    dispatchRows: u32, present: u32, implicit: u32, left: i32,
    right: i32, top: i32, bottom: i32, yFirst: i32,
    yOffset: u32, yCount: u32, padding0: u32, padding1: u32
}
@group(0) @binding(0) var<storage, read> source: array<u32>;
@group(0) @binding(1) var<storage, read_write> accumulated: array<atomic<i32>>;
@group(0) @binding(2) var<storage, read> spans: array<vec4<i32>>;
@group(0) @binding(3) var<storage, read> weights: array<i32>;
@group(0) @binding(4) var<storage, read_write> output: array<u32>;
@group(0) @binding(5) var<uniform> parameters: Parameters;

fn unpack(pixel: u32) -> vec4<i32> {
    return vec4<i32>(i32(pixel & 255u), i32((pixel >> 8u) & 255u),
                     i32((pixel >> 16u) & 255u), i32(pixel >> 24u));
}
fn pack(pixel: vec4<u32>) -> u32 {
    return pixel.r | (pixel.g << 8u) | (pixel.b << 16u) | (pixel.a << 24u);
}
fn mixed(sum: vec4<i32>) -> u32 {
    if (sum.a <= 0) { return 0u; }
    let rgb = clamp((sum.rgb + vec3<i32>(sum.a / 2)) / vec3<i32>(sum.a), vec3<i32>(0), vec3<i32>(255));
    return pack(vec4<u32>(vec3<u32>(rgb), u32(clamp((sum.a + 127) / 255, 0, 255))));
}
fn premultiplied(pixel: vec4<i32>) -> vec4<i32> {
    let product = pixel.rgb * vec3<i32>(pixel.a) + vec3<i32>(128);
    return vec4<i32>((product + (product >> vec3<u32>(8))) >> vec3<u32>(8), pixel.a);
}

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let p = parameters;
    if (p.stage == 0u) {
        if (id.x >= p.width * p.dispatchRows) { return; }
        let x = id.x % p.width;
        let y = p.firstRow + i32(id.x / p.width);
        let span = spans[x];
        var sum = vec4<i32>(0);
        for (var i = 0; i < span.z; i++) {
            let sx = clamp(span.x + i, p.left, p.right);
            if (sx < p.tileX || sx >= p.tileX + 256) { continue; }
            var pixel = p.implicit;
            if (p.present != 0u) {
                pixel = source[u32((y - p.tileY) * 256 + sx - p.tileX)];
            }
            let color = unpack(pixel);
            let weight = weights[u32(span.y + i)];
            if (p.mode == 0u) { sum += vec4<i32>(color.rgb * vec3<i32>(color.a), color.a) * weight; }
            else { sum += premultiplied(color) * weight; }
        }
        let destination = (u32(y - p.rowStart) * p.width + x) * 4u;
        atomicAdd(&accumulated[destination], sum.r);
        atomicAdd(&accumulated[destination + 1u], sum.g);
        atomicAdd(&accumulated[destination + 2u], sum.b);
        atomicAdd(&accumulated[destination + 3u], sum.a);
    } else if (p.stage == 1u) {
        if (id.x >= p.width * p.rows) { return; }
        let offset = id.x * 4u;
        let sum = vec4<i32>(atomicLoad(&accumulated[offset]), atomicLoad(&accumulated[offset + 1u]),
                            atomicLoad(&accumulated[offset + 2u]), atomicLoad(&accumulated[offset + 3u]));
        if (p.mode == 0u) { atomicStore(&accumulated[offset], bitcast<i32>(mixed(sum))); }
        else {
            var shift = 0u;
            if (p.mode == 2u) { shift = 4u; }
            let value = sum >> vec4<u32>(shift);
            atomicStore(&accumulated[offset], value.r);
            atomicStore(&accumulated[offset + 1u], value.g);
            atomicStore(&accumulated[offset + 2u], value.b);
            atomicStore(&accumulated[offset + 3u], value.a);
        }
    } else {
        if (id.x >= p.width) { return; }
        var sum = vec4<i32>(0);
        var area = vec4<u32>(0u);
        for (var i = 0u; i < p.yCount; i++) {
            let sy = clamp(p.yFirst + i32(i), p.top, p.bottom);
            let offset = (u32(sy - p.rowStart) * p.width + id.x) * 4u;
            let weight = weights[p.yOffset + i];
            if (p.mode == 0u) {
                let color = unpack(bitcast<u32>(atomicLoad(&accumulated[offset])));
                sum += vec4<i32>(color.rgb * vec3<i32>(color.a), color.a) * weight;
            } else {
                let color = vec4<u32>(u32(atomicLoad(&accumulated[offset])), u32(atomicLoad(&accumulated[offset + 1u])),
                                      u32(atomicLoad(&accumulated[offset + 2u])), u32(atomicLoad(&accumulated[offset + 3u])));
                area += color * u32(weight);
            }
        }
        if (p.mode == 0u) { output[id.x] = mixed(sum); }
        else {
            var shift = 22u;
            if (p.mode == 2u) { shift = 24u; }
            output[id.x] = pack((area >> vec4<u32>(shift)) & vec4<u32>(255u));
        }
    }
}
