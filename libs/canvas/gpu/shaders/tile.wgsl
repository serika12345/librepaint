// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters { matrix: mat4x4<f32>, clip: vec4<f32> }
@group(0) @binding(0) var<uniform> p: Parameters;
@group(0) @binding(1) var<storage, read> pixels: array<u32>;
struct Vertex { @builtin(position) position: vec4<f32>, @location(0) uv: vec2<f32> }
@vertex fn vertexMain(@builtin(vertex_index) index: u32) -> Vertex {
    let corners = array<vec2<f32>, 6>(vec2(0., 0.), vec2(1., 0.), vec2(0., 1.),
        vec2(0., 1.), vec2(1., 0.), vec2(1., 1.));
    var out: Vertex;
    out.uv = corners[index];
    out.position = p.matrix * vec4(out.uv, 0., 1.);
    return out;
}
@fragment fn fragmentMain(in: Vertex) -> @location(0) vec4<f32> {
    if (any(in.uv < p.clip.xy) || any(in.uv >= p.clip.zw)) { discard; }
    let pos = vec2<u32>(clamp(floor(in.uv * 256.), vec2(0.), vec2(255.)));
    let pixel = pixels[pos.y * 256u + pos.x];
    let straight = vec4<f32>(f32(pixel & 255u), f32((pixel >> 8u) & 255u),
        f32((pixel >> 16u) & 255u), f32(pixel >> 24u)) / 255.;
    return vec4(straight.rgb * straight.a, straight.a);
}
