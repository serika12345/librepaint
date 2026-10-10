// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct LayerParameters {
    sourceTile: u32, maskTile: u32, operation: u32, opacity: u32,
}
struct ProjectionParameters {
    lower: vec2<u32>, upper: vec2<u32>, destinationTile: u32, reserved: u32,
    layers: array<LayerParameters, 3>,
}
@group(0) @binding(0) var<storage, read_write> pixels: array<u32>;
@group(0) @binding(1) var<storage, read> parameters: array<ProjectionParameters>;
@group(0) @binding(2) var<storage, read> source0: array<u32>;
@group(0) @binding(3) var<storage, read> source1: array<u32>;
@group(0) @binding(4) var<storage, read> source2: array<u32>;
@group(0) @binding(5) var<storage, read> mask0: array<u32>;
@group(0) @binding(6) var<storage, read> mask1: array<u32>;
@group(0) @binding(7) var<storage, read> mask2: array<u32>;

@compute @workgroup_size(8, 8)
fn project(@builtin(global_invocation_id) position: vec3<u32>) {
    let tile = parameters[position.z];
    if (all(position.xy >= tile.lower) && all(position.xy < tile.upper)) {
        let localIndex = position.y * 64u + position.x;
        let destinationIndex = tile.destinationTile * 4096u + localIndex;
        var pixel = unpack(pixels[destinationIndex]);
        // Each layer produces RGBA8 before the next layer consumes it.
        let layer0 = tile.layers[0];
        if (layer0.sourceTile != 0xFFFFFFFFu) {
            var coverage = 255u;
            if (layer0.maskTile != 0xFFFFFFFFu) {
                coverage = mask0[layer0.maskTile * 4096u + localIndex] >> 24u;
            }
            let source = unpack(source0[layer0.sourceTile * 4096u + localIndex]);
            pixel = blend(source, pixel, layer0.operation, layer0.opacity, coverage);
        }
        let layer1 = tile.layers[1];
        if (layer1.sourceTile != 0xFFFFFFFFu) {
            var coverage = 255u;
            if (layer1.maskTile != 0xFFFFFFFFu) {
                coverage = mask1[layer1.maskTile * 4096u + localIndex] >> 24u;
            }
            let source = unpack(source1[layer1.sourceTile * 4096u + localIndex]);
            pixel = blend(source, pixel, layer1.operation, layer1.opacity, coverage);
        }
        let layer2 = tile.layers[2];
        if (layer2.sourceTile != 0xFFFFFFFFu) {
            var coverage = 255u;
            if (layer2.maskTile != 0xFFFFFFFFu) {
                coverage = mask2[layer2.maskTile * 4096u + localIndex] >> 24u;
            }
            let source = unpack(source2[layer2.sourceTile * 4096u + localIndex]);
            pixel = blend(source, pixel, layer2.operation, layer2.opacity, coverage);
        }
        pixels[destinationIndex] = pack(pixel);
    }
}
