/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

struct Translation { x: u32, y: u32, padding: vec2<u32> };
@group(0) @binding(0) var<storage, read> topLeft: array<u32>;
@group(0) @binding(1) var<storage, read> topRight: array<u32>;
@group(0) @binding(2) var<storage, read> bottomLeft: array<u32>;
@group(0) @binding(3) var<storage, read> bottomRight: array<u32>;
@group(0) @binding(4) var<storage, read_write> output: array<u32>;
@group(0) @binding(5) var<uniform> shift: Translation;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let x = id.x + shift.x;
    let y = id.y + shift.y;
    let index = (y & 255u) * 256u + (x & 255u);
    var pixel: u32;
    switch ((x >> 8u) + 2u * (y >> 8u)) {
        case 0u: { pixel = topLeft[index]; }
        case 1u: { pixel = topRight[index]; }
        case 2u: { pixel = bottomLeft[index]; }
        default: { pixel = bottomRight[index]; }
    }
    output[id.y * 256u + id.x] = pixel;
}
