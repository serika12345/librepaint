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
    // Keep the x product and the y fused multiply-add in the same rounding order on each backend.
    let n = fma(normalized.y, normalized.y, normalized.x * normalized.x);
    if (n > 1.0) { return 0u; }
    let faded = delta * bitcast<vec2<f32>>(vec2<u32>(shape.color, shape.operation));
    let nf = fma(faded.y, faded.y, faded.x * faded.x);
    if (nf <= 1.0) { return 255u; }
    // Scalar brush masks truncate the inverse coverage before subtracting it.
    let numerator = 255.0 * n * (nf - 1.0);
    let denominator = nf - n;
    var inverse = u32(clamp(numerator / denominator, 0.0, 255.0));
    let residual = fma(-f32(inverse), denominator, numerator);
    if (residual < 0.0 && inverse > 0u) { inverse--; }
    if (residual >= denominator && inverse < 255u) { inverse++; }
    return 255u - inverse;
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

@group(0) @binding(7) var<storage, read> pickupPixel: array<u32>;

fn dulling(source: vec4<u32>, destination: vec4<u32>, opacity: f32) -> vec4<u32> {
    let alpha = f32(source.a) * opacity;
    if (alpha == 0.0) { return destination; }
    var resultAlpha = f32(destination.a);
    var blend = alpha * (1.0 / 255.0);
    if (destination.a == 0u) {
        resultAlpha = alpha;
        blend = 1.0;
    } else if (destination.a != 255u) {
        resultAlpha += (255.0 - resultAlpha) * alpha * (1.0 / 255.0);
        blend = alpha / resultAlpha;
    }
    let color = fma(vec3<f32>(blend), vec3<f32>(source.rgb) - vec3<f32>(destination.rgb), vec3<f32>(destination.rgb));
    return vec4<u32>(quantize(color.r), quantize(color.g), quantize(color.b), quantize(resultAlpha));
}

fn lerp8(destination: u32, source: u32, opacity: u32) -> u32 {
    let product = (i32(source) - i32(destination)) * i32(opacity) + 128;
    return u32(i32(destination) + (((product >> 8u) + product) >> 8u));
}

fn maskedCopy(source: vec4<u32>, destination: vec4<u32>, coverage: u32) -> vec4<u32> {
    if (coverage == 0u) { return destination; }
    if (coverage == 255u) { return source; }
    let alpha = lerp8(destination.a, source.a, coverage);
    if (alpha == 0u) { return vec4<u32>(destination.rgb, 0u); }
    var color: vec3<u32>;
    for (var channel = 0u; channel < 3u; channel++) {
        let premultiplied = lerp8(multiply8(destination[channel], destination.a), multiply8(source[channel], source.a), coverage);
        color[channel] = min(255u, (premultiplied * 255u + alpha / 2u) / alpha);
    }
    return vec4<u32>(color, alpha);
}

fn paintMixedPixel(position: vec3<u32>, selection: u32) {
    let tile = parameters[0];
    let command = commands[tile.firstCommand];
    if (command.operation != 5u) { paintPixel(position, selection, 255u, false); return; }
    if (all(position.xy >= command.lower) && all(position.xy < command.upper)) {
        let index = position.y * 64u + position.x;
        let destination = unpack(pixels[index]);
        let source = dulling(unpack(pickupPixel[0]), destination, bitcast<f32>(tile.reserved));
        let mask = multiply8(multiply8(dabAlpha(position.xy, commands[tile.firstCommand + 1u]), command.coverage), selection);
        pixels[index] = pack(maskedCopy(source, destination, mask));
    }
}

@compute @workgroup_size(8, 8)
fn paintMixed(@builtin(global_invocation_id) position: vec3<u32>) {
    paintMixedPixel(position, 255u);
}

@compute @workgroup_size(8, 8)
fn paintSelectedMixed(@builtin(global_invocation_id) position: vec3<u32>) {
    let offset = parameters[0].selectionOffset + position.y * 64u + position.x;
    paintMixedPixel(position, maskPixels[offset] >> 24u);
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
