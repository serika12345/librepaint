// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct TileCommand {
    lower: vec2<u32>, upper: vec2<u32>, color: u32,
    operation: u32, opacity: u32, coverage: u32,
}
struct TileParameters {
    firstCommand: u32, commandCount: u32, selectionOffset: u32, reserved: u32,
    textureOrigin: vec2<u32>, textureSize: vec2<u32>,
}
@group(0) @binding(0) var<storage, read_write> pixels: array<u32>;
@group(0) @binding(1) var<storage, read> parameters: array<TileParameters>;
@group(0) @binding(2) var<storage, read> commands: array<TileCommand>;
@group(0) @binding(5) var<storage, read> maskPixels: array<u32>;
@group(0) @binding(6) var<storage, read> texturePixels: array<u32>;

fn dabAlpha(position: vec2<u32>, shape: TileCommand) -> u32 {
    let delta = vec2<f32>(position) - bitcast<vec2<f32>>(shape.lower);
    let normalized = delta * bitcast<vec2<f32>>(shape.upper);
    let n = dot(normalized, normalized);
    if (n > 1.0) { return 0u; }
    let faded = delta * bitcast<vec2<f32>>(vec2<u32>(shape.color, shape.operation));
    let nf = dot(faded, faded);
    if (nf <= 1.0) { return 255u; }
    // Scalar brush masks truncate the inverse coverage before subtracting it.
    return 255u - u32(clamp(255.0 * n * (nf - 1.0) / (nf - n), 0.0, 255.0));
}

fn textureMultiply8(alpha: u32, texture: u32, strength: u32) -> u32 {
    // Match the CPU texture option's three-factor approximation.
    let product = alpha * texture * strength + 0x7F5Bu;
    return ((product >> 7u) + product) >> 16u;
}

fn paintPixel(position: vec3<u32>, selection: u32, texture: u32, textured: bool) {
    let index = position.z * 4096u + position.y * 64u + position.x;
    let tile = parameters[position.z];
    var pixel = pixels[index];
    for (var i = tile.firstCommand; i < tile.firstCommand + tile.commandCount; i++) {
        let command = commands[i];
        var source = unpack(command.color);
        var operation = command.operation;
        if (operation >= 3u) {
            i++;
            source.a = multiply8(source.a, dabAlpha(position.xy, commands[i]));
            if (textured) { source.a = textureMultiply8(source.a, texture, commands[i].opacity); }
            operation = select(1u, 2u, operation == 4u);
        }
        if (all(position.xy >= command.lower) && all(position.xy < command.upper)) {
            if (command.operation == 0u) {
                pixel = command.color;
            } else {
                pixel = pack(blend(source, unpack(pixel), operation, command.opacity, multiply8(command.coverage, selection)));
            }
        }
    }
    pixels[index] = pixel;
}

@compute @workgroup_size(8, 8)
fn paint(@builtin(global_invocation_id) position: vec3<u32>) {
    paintPixel(position, 255u, 255u, false);
}

@compute @workgroup_size(8, 8)
fn paintSelected(@builtin(global_invocation_id) position: vec3<u32>) {
    let offset = parameters[position.z].selectionOffset + position.y * 64u + position.x;
    paintPixel(position, maskPixels[offset] >> 24u, 255u, false);
}

fn textureAlpha(position: vec3<u32>) -> u32 {
    let tile = parameters[position.z];
    let pixel = (tile.textureOrigin + position.xy) % tile.textureSize;
    let offset = pixel.y * tile.textureSize.x + pixel.x;
    return (texturePixels[offset / 4u] >> ((offset % 4u) * 8u)) & 255u;
}

@compute @workgroup_size(8, 8)
fn paintTextured(@builtin(global_invocation_id) position: vec3<u32>) {
    paintPixel(position, 255u, textureAlpha(position), true);
}

@compute @workgroup_size(8, 8)
fn paintSelectedTextured(@builtin(global_invocation_id) position: vec3<u32>) {
    let offset = parameters[position.z].selectionOffset + position.y * 64u + position.x;
    paintPixel(position, maskPixels[offset] >> 24u, textureAlpha(position), true);
}

struct CompositeParameters {
    sourceTile: u32, destinationTile: u32, lower: vec2<u32>, upper: vec2<u32>,
    operation: u32, opacity: u32, coverage: u32, maskTile: u32,
}
@group(0) @binding(3) var<storage, read> sourcePixels: array<u32>;
@group(0) @binding(4) var<storage, read> compositeParameters: array<CompositeParameters>;

@compute @workgroup_size(8, 8)
fn composite(@builtin(global_invocation_id) position: vec3<u32>) {
    let parameters = compositeParameters[position.z];
    if (all(position.xy >= parameters.lower) && all(position.xy < parameters.upper)) {
        let localIndex = position.y * 64u + position.x;
        let destinationIndex = parameters.destinationTile * 4096u + localIndex;
        let source = unpack(sourcePixels[parameters.sourceTile * 4096u + localIndex]);
        var coverage = parameters.coverage;
        if (parameters.maskTile != 0xFFFFFFFFu) {
            coverage = multiply8(coverage, maskPixels[parameters.maskTile * 4096u + localIndex] >> 24u);
        }
        pixels[destinationIndex] = pack(blend(source, unpack(pixels[destinationIndex]),
            parameters.operation, parameters.opacity, coverage));
    }
}
