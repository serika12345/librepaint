/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
struct Parameters {
    tile: vec2<i32>, first: vec2<i32>, after: vec2<i32>, pixelFormat: u32, padding: u32,
    implicit: vec4<u32>
}
@group(0) @binding(0) var<storage, read> source: array<u32>;
@group(0) @binding(1) var<storage, read_write> destination: array<u32>;
@group(0) @binding(2) var<uniform> parameters: Parameters;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let position = parameters.tile + vec2<i32>(id.xy);
    let wide = parameters.pixelFormat != 0u;
    let stride = select(1u, 2u, wide);
    let offset = (id.y * 256u + id.x) * stride;
    let words = nativePixelWords(parameters.implicit, wide);
    let inside = all(position >= parameters.first) && all(position < parameters.after);
    for (var word = 0u; word < stride; word += 1u) {
        destination[offset + word] = select(words[word], source[offset + word], inside);
    }
}
