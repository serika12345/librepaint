// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters { matrix: mat4x4<f32>, clip: vec4<f32>, channels: vec4<u32>, background: vec4<u32> }
@group(0) @binding(0) var<uniform> p: Parameters;
@group(0) @binding(1) var<storage, read> pixels: array<u32>;
struct Vertex { @builtin(position) position: vec4<f32>, @location(0) uv: vec2<f32> }
@vertex fn vertexMain(@builtin(vertex_index) index: u32) -> Vertex {
    // Two triangles: (0,0), (1,0), (0,1), (0,1), (1,0), (1,1).
    let uv = vec2<f32>(select(0., 1., index == 1u || index >= 4u),
                       select(0., 1., index == 2u || index == 3u || index == 5u));
    return Vertex(p.matrix * vec4(uv, 0., 1.), uv);
}
// Match the established Qt 8-bit channel multiplication before composition.
fn scaleDisplayBytes(pixel: vec4<u32>, opacity: u32) -> vec4<u32> {
    let product = pixel * opacity;
    return (product + (product >> vec4(8u)) + vec4(128u)) >> vec4(8u);
}
@fragment fn fragmentMain(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    if (any(uv < p.clip.xy) || any(uv >= p.clip.zw)) { discard; }
    let pos = vec2<u32>(clamp(floor(uv * 256.), vec2(0.), vec2(255.)));
    let pixel = pixels[pos.y * 256u + pos.x];
    let straight = vec4<u32>(pixel & 255u, (pixel >> 8u) & 255u,
        (pixel >> 16u) & 255u, pixel >> 24u);
    let mask = p.channels.x;
    let colors = mask & 7u;
    var visual: vec4<u32>;
    if (p.channels.y == 0u && (colors == 1u || colors == 2u || colors == 4u)) {
        let index = select(select(2u, 1u, colors == 2u), 0u, colors == 1u);
        visual = vec4(vec3(straight[index]), straight.a);
    } else {
        let enabled = vec4<bool>((mask & 1u) != 0u, (mask & 2u) != 0u,
                                  (mask & 4u) != 0u, (mask & 8u) != 0u);
        visual = select(vec4(0u), straight, enabled);
    }
    let source = scaleDisplayBytes(vec4<u32>(visual.rgb, 255u), visual.a);
    let destination = scaleDisplayBytes(p.background, 255u - visual.a);
    return vec4<f32>(source + destination) / 255.;
}
