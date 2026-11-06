// SPDX-License-Identifier: GPL-2.0-or-later
@group(0) @binding(0) var<storage, read> projection: array<u32>;
@group(0) @binding(1) var<uniform> grid: vec4u;

@vertex fn vertexMain(@builtin(vertex_index) index: u32) -> @builtin(position) vec4f {
    let p = vec2f(f32((index << 1u) & 2u), f32(index & 2u));
    return vec4f(p * 2.0 - 1.0, 0.0, 1.0);
}
@fragment fn fragmentMain(@builtin(position) position: vec4f) -> @location(0) vec4f {
    let x = u32(position.x);
    let y = u32(position.y);
    let logical = (y / 64u) * grid.x + x / 64u;
    let p = projection[logical * 4096u + (y % 64u) * 64u + x % 64u];
    return vec4f(vec4u(p, p >> 8u, p >> 16u, p >> 24u) & vec4u(255u)) / 255.0;
}
