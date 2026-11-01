/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
struct Parameters { pixelFormat: u32, rectangles: u32, padding: vec2<u32>, implicit: vec4<u32> }
@group(0) @binding(0) var<storage, read> source: array<u32>;
@group(0) @binding(1) var<storage, read_write> destination: array<u32>;
@group(0) @binding(2) var<uniform> parameters: Parameters;
@group(0) @binding(3) var<storage, read> rectangles: array<vec4<i32>>;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let position = vec2<i32>(id.xy);
    var allocated = false;
    for (var i = 0u; i < parameters.rectangles; i += 1u) {
        allocated = allocated || (all(position >= rectangles[i].xy) && all(position < rectangles[i].zw));
    }
    let wide = parameters.pixelFormat != 0u;
    let stride = select(1u, 2u, wide);
    let offset = (id.y * 256u + id.x) * stride;
    let words = nativePixelWords(parameters.implicit, wide);
    for (var word = 0u; word < stride; word += 1u) {
        destination[offset + word] = select(words[word], source[offset + word], allocated);
    }
}
