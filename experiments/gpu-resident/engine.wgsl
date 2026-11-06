// SPDX-License-Identifier: GPL-2.0-or-later
@group(0) @binding(0) var<storage, read_write> document: array<u32>;
@group(0) @binding(1) var<storage, read_write> projection: array<u32>;
@group(0) @binding(2) var<storage, read> commands: array<u32>;
@group(0) @binding(3) var<storage, read> mapping: array<u32>;

fn unpack(p: u32) -> vec4u {
    return vec4u(p, p >> 8u, p >> 16u, p >> 24u) & vec4u(255u);
}
fn pack(p: vec4u) -> u32 {
    return p.x | (p.y << 8u) | (p.z << 16u) | (p.w << 24u);
}
fn over(dst: u32, src: vec4u) -> u32 {
    return pack(src + (unpack(dst) * (255u - src.w) + vec4u(127u)) / 255u);
}

@compute @workgroup_size(64)
fn paint(@builtin(global_invocation_id) id: vec3u) {
    if (id.x >= 4096u || id.y >= commands[0]) { return; }
    let job = 8u + id.y * 4u;
    let logical = commands[job];
    let count = commands[job + 3u];
    if (count == 0u) { return; }
    let pixel = commands[job + 1u] * 4096u + id.x;
    let x = i32((logical % commands[2]) * 64u + id.x % 64u);
    let y = i32((logical / commands[2]) * 64u + id.x / 64u);
    var value = document[pixel];
    for (var i = 0u; i < count; i++) {
        let index = commands[commands[4] + commands[job + 2u] + i];
        let dab = commands[3] + index * 8u;
        let dx = x - i32(commands[dab]);
        let dy = y - i32(commands[dab + 1u]);
        let distance = u32(dx * dx + dy * dy);
        let radius = commands[dab + 2u];
        let r2 = radius * radius;
        if (distance < r2) {
            let alpha = (r2 - distance) * commands[dab + 3u] / r2;
            let rgb = (unpack(commands[dab + 4u]).xyz * alpha + vec3u(127u)) / 255u;
            value = over(value, vec4u(rgb, alpha));
        }
    }
    document[pixel] = value;
}

@compute @workgroup_size(64)
fn compose(@builtin(global_invocation_id) id: vec3u) {
    if (id.x >= 4096u || id.y >= commands[0]) { return; }
    let logical = commands[8u + id.y * 4u];
    let gridTiles = commands[2] * commands[2];
    var value = 0u;
    for (var layer = 0u; layer < commands[1]; layer++) {
        let slot = mapping[layer * gridTiles + logical];
        if (slot != 0xffffffffu) { value = over(value, unpack(document[slot * 4096u + id.x])); }
    }
    projection[logical * 4096u + id.x] = value;
}
