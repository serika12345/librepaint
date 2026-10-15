// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

@group(0) @binding(0) var<storage, read> pixels: array<u32>;
@group(0) @binding(1) var<storage, read_write> transparent: atomic<u32>;
@group(0) @binding(2) var<uniform> bounds: vec4<u32>;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) position: vec3<u32>) {
    if (position.x < bounds.x || position.y < bounds.y || position.x >= bounds.z || position.y >= bounds.w) {
        return;
    }
    if ((pixels[position.y * 256u + position.x] >> 24u) != 255u) {
        atomicOr(&transparent, 1u);
    }
}
