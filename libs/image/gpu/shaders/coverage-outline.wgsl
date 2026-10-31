// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct OutlineParameters {
    size: vec2<u32>, origin: vec2<i32>, format: vec4<u32>
}
@group(0) @binding(0) var<uniform> parameters: OutlineParameters;
@group(0) @binding(1) var<storage, read> pixels: array<u32>;
@group(0) @binding(2) var<storage, read_write> rowData: array<u32>;
@group(0) @binding(3) var<storage, read_write> runs: array<vec4<i32>>;
@group(0) @binding(4) var<storage, read_write> bounds: array<atomic<u32>>;
fn alphaAt(x: u32, y: u32) -> u32 {
    let index = y * parameters.size.x + x;
    if (parameters.format.x == 1u) { return pixels[index] >> 24u; }
    if (parameters.format.x == 2u) { return (pixels[index >> 2u] >> ((index & 3u) * 8u)) & 255u; }
    return pixels[index];
}
@compute @workgroup_size(64) fn countRows(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let y = invocation.x;
    if (y >= parameters.size.y) { return; }
    var previous = false;
    var count = 0u;
    for (var x = 0u; x < parameters.size.x; x += 1u) {
        let alpha = alphaAt(x, y);
        if (alpha != parameters.format.y) {
            atomicMin(&bounds[1], x); atomicMin(&bounds[2], y);
            atomicMax(&bounds[3], x); atomicMax(&bounds[4], y);
        }
        let selected = (alpha >= 128u) != (parameters.format.y >= 128u);
        if (selected && !previous) { count += 1u; }
        previous = selected;
    }
    rowData[y] = count;
}
@compute @workgroup_size(64) fn writeRows(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let y = invocation.x;
    if (y >= parameters.size.y) { return; }
    var runIndex = rowData[y];
    var inside = false;
    var first = 0u;
    for (var x = 0u; x <= parameters.size.x; x += 1u) {
        var selected = false;
        if (x < parameters.size.x) { selected = (alphaAt(x, y) >= 128u) != (parameters.format.y >= 128u); }
        if (selected && !inside) { first = x; inside = true; }
        if (!selected && inside) {
            runs[runIndex] = vec4<i32>(parameters.origin + vec2<i32>(i32(first), i32(y)), i32(x - first), 1);
            runIndex += 1u; inside = false;
        }
    }
}
