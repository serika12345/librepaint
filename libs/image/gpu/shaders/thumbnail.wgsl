/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
struct Parameters { present: u32, implicit: u32, count: u32, padding: u32 }
@group(0) @binding(0) var<storage, read> source: array<u32>;
@group(0) @binding(1) var<storage, read_write> output: array<u32>;
@group(0) @binding(2) var<storage, read> samples: array<vec2<u32>>;
@group(0) @binding(3) var<uniform> parameters: Parameters;

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= parameters.count) { return; }
    let sample = samples[id.x];
    var value = parameters.implicit;
    if (parameters.present != 0u && sample.y < 65536u) { value = source[sample.y]; }
    output[sample.x] = value;
}
