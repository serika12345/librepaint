// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

@group(0) @binding(0) var<storage, read> pixels: array<u32>;
@group(0) @binding(1) var<storage, read_write> transparent: atomic<u32>;
struct Parameters { bounds: vec4<u32>, native: vec4<u32>, }
@group(0) @binding(2) var<uniform> parameters: Parameters;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) position: vec3<u32>) {
    let bounds = parameters.bounds;
    if (position.x < bounds.x || position.y < bounds.y || position.x >= bounds.z || position.y >= bounds.w) {
        return;
    }
    let wide = parameters.native.x != 0u;
    let stride = select(1u, 2u, wide);
    let word = (position.y * 256u + position.x) * stride;
    let pixel = nativePixel(pixels[word], pixels[word + stride - 1u], wide);
    if (pixel.w != select(255u, 65535u, wide)) {
        atomicOr(&transparent, 1u);
    }
}
