/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
// flags holds source presence in bit0 and RGBA16 precision in bit1.
struct Parameters { flags: u32, implicit: u32, count: u32, implicitHigh: u32 }
@group(0) @binding(0) var<storage, read> source: array<u32>;
@group(0) @binding(1) var<storage, read_write> output: array<u32>;
@group(0) @binding(2) var<storage, read> samples: array<vec2<u32>>;
@group(0) @binding(3) var<uniform> parameters: Parameters;

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= parameters.count) { return; }
    let sample = samples[id.x];
    let wide = (parameters.flags & 2u) != 0u;
    let stride = select(1u, 2u, wide);
    var value = vec2<u32>(parameters.implicit, parameters.implicitHigh);
    if ((parameters.flags & 1u) != 0u && sample.y < 65536u) {
        value.x = source[sample.y * stride];
        if (wide) { value.y = source[sample.y * stride + 1u]; }
    }
    output[sample.x * stride] = value.x;
    if (wide) { output[sample.x * stride + 1u] = value.y; }
}
