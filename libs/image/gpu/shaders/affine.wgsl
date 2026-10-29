// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters {
    origin: vec2<i32>, format: u32, kind: u32,
    source: vec4<i32>, destination: vec4<i32>,
    matrix: vec4<i32>, placement: vec4<i32>,
    scale: vec2<u32>, shear: vec2<u32>, offset: vec2<u32>, support: vec2<u32>,
    implicit: vec4<u32>, count: u32, positionScale: u32, reserved0: u32, reserved1: u32
}
@group(0) @binding(0) var<storage, read> atlas: array<u32>;
@group(0) @binding(1) var<storage, read_write> destination: array<u32>;
@group(0) @binding(2) var<storage, read> tiles: array<vec4<i32>>;
@group(0) @binding(3) var<storage, read> phases: array<vec4<i32>>;
@group(0) @binding(4) var<storage, read> weights: array<i32>;
@group(0) @binding(5) var<uniform> parameters: Parameters;

fn floor256(value: i32) -> i32 { return select(-((-value + 255) / 256), value / 256, value >= 0); }
fn inside(point: vec2<i32>, rect: vec4<i32>) -> bool { return all(point >= rect.xy) && all(point < rect.zw); }
fn pixel(point: vec2<i32>) -> vec4<u32> {
    let coordinate = vec2<i32>(floor256(point.x), floor256(point.y));
    var first = 0u; var end = parameters.count;
    loop {
        if (first >= end) { break; }
        let mid = first + (end - first) / 2u;
        let value = tiles[mid];
        if (value.y < coordinate.y || (value.y == coordinate.y && value.x < coordinate.x)) { first = mid + 1u; }
        else { end = mid; }
    }
    if (first >= parameters.count || any(tiles[first].xy != coordinate)) { return parameters.implicit; }
    let local = point - coordinate * 256;
    let stride = select(1u, 2u, parameters.format != 0u);
    let word = u32(tiles[first].z) + u32(local.y * 256 + local.x) * stride;
    return nativePixel(atlas[word], atlas[word + stride - 1u], parameters.format != 0u);
}
fn signedDouble(value: i32) -> Word64 {
    let result = doubleInteger(u32(abs(value)));
    return select(result, doubleNegate(result), value < 0);
}
fn signedTruncate(value: Word64) -> i32 {
    let result = i32(doubleTruncate(doubleMagnitude(value)));
    return select(result, -result, (value.y & 0x80000000u) != 0u);
}
fn nativeRound(value: Word64) -> i32 {
    let result = i32(doubleTruncate(doubleAdd(doubleMagnitude(value), Word64(0u, 0x3fe00000u))));
    return select(result, -result, (value.y & 0x80000000u) != 0u);
}
fn mappedSource(destination: i32, line: i32) -> Word64 {
    let center = doubleAdd(signedDouble(destination), Word64(0u, 0x3fe00000u));
    let shifted = doubleSubtract(doubleSubtract(center, parameters.offset), doubleMultiply(signedDouble(line), parameters.shear));
    return doubleDivide(shifted, parameters.scale);
}
fn mappedDestination(source: i32, line: i32) -> Word64 {
    return doubleAdd(doubleAdd(doubleMultiply(signedDouble(source), parameters.scale), parameters.offset),
                     doubleMultiply(signedDouble(line), parameters.shear));
}
fn quotientClamped(numerator: Word64, denominator: Word64, maximum: u32) -> u32 {
    if ((numerator.y & 0x80000000u) != 0u) { return 0u; }
    let rounded = wordAdd(numerator, wordRight(denominator, 1u));
    var result = 0u;
    var bit = select(128u, 32768u, maximum == 65535u);
    loop {
        if (bit == 0u) { break; }
        let candidate = result | bit;
        let product = wideProduct(denominator, Word64(candidate, 0u));
        if (all(product.zw == vec2<u32>(0u)) && wordLessEqual(product.xy, rounded)) { result = candidate; }
        bit >>= 1u;
    }
    return result;
}
fn resampled(position: vec2<i32>) -> vec4<u32> {
    if (!inside(position, parameters.destination)) {
        if (inside(position, parameters.source)) { return parameters.implicit; }
        return pixel(position);
    }
    let horizontal = parameters.kind == 0u;
    let line = select(position.x, position.y, horizontal);
    let coordinate = select(position.y, position.x, horizontal);
    let start = select(parameters.source.y, parameters.source.x, horizontal);
    let end = select(parameters.source.w, parameters.source.z, horizontal);
    let positive = (parameters.scale.y & 0x80000000u) == 0u;
    let clampEdge = all(doubleMagnitude(parameters.shear) == Word64(0u));
    var left = mappedDestination(select(end, start, positive), line);
    var right = mappedDestination(select(start, end, positive), line);
    if (!clampEdge) { left = doubleSubtract(left, parameters.support); right = doubleAdd(right, parameters.support); }
    let begin = nativeRound(left);
    let finish = max(begin + 1, nativeRound(right));
    if (coordinate < begin || coordinate >= finish) {
        if (inside(position, parameters.source)) { return parameters.implicit; }
        return pixel(position);
    }
    let mapped = mappedSource(coordinate, line);
    let fixed = signedTruncate(doubleMultiply(mapped, Word64(0u, 0x40700000u)));
    let lower = fixed - 128;
    let ceiling = select(-((-lower) / 256), (lower + 255) / 256, lower >= 0);
    let center = ceiling * 256 + 128;
    let phaseIndex = u32(((center - fixed) * i32(parameters.positionScale)) >> 8u);
    let phase = phases[phaseIndex];
    let first = floor256(center) - phase.z;
    var totals = array<Word64, 3>(Word64(0u), Word64(0u), Word64(0u));
    var alpha = Word64(0u);
    for (var i = 0; i < phase.y; i++) {
        var source = first + i;
        if (clampEdge) { source = clamp(source, start, end - 1); }
        var color = parameters.implicit;
        if (source >= start && source < end) {
            color = pixel(select(vec2<i32>(line, source), vec2<i32>(source, line), horizontal));
        }
        let weight = weights[u32(phase.x + i)];
        let weightedAlpha = color.w * u32(abs(weight));
        var amount = Word64(weightedAlpha, 0u);
        if (weight < 0) { amount = wordSubtract(Word64(0u), amount); }
        alpha = wordAdd(alpha, amount);
        for (var channel = 0u; channel < 3u; channel++) {
            var value = wordProduct(color[channel], weightedAlpha);
            if (weight < 0) { value = wordSubtract(Word64(0u), value); }
            totals[channel] = wordAdd(totals[channel], value);
        }
    }
    if (all(alpha == Word64(0u)) || (alpha.y & 0x80000000u) != 0u) { return vec4<u32>(0u); }
    let maximum = select(255u, 65535u, parameters.format != 0u);
    return vec4<u32>(quotientClamped(totals[0], alpha, maximum), quotientClamped(totals[1], alpha, maximum),
        quotientClamped(totals[2], alpha, maximum), quotientClamped(alpha, Word64(255u, 0u), maximum));
}
@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {
    let position = parameters.origin + vec2<i32>(id.xy);
    var color: vec4<u32>;
    if (parameters.kind != 2u) { color = resampled(position); }
    else if (parameters.placement.w != 0 || inside(position, parameters.destination)) {
        let point = position - parameters.placement.xy;
        let source = vec2<i32>(parameters.matrix.x * point.x + parameters.matrix.z * point.y,
                                parameters.matrix.y * point.x + parameters.matrix.w * point.y);
        color = pixel(source);
    } else if (parameters.placement.z != 0) { color = parameters.implicit; }
    else { color = pixel(position); }
    let wide = parameters.format != 0u;
    let words = nativePixelWords(color, wide);
    let stride = select(1u, 2u, wide);
    let index = (id.y * 256u + id.x) * stride;
    for (var word = 0u; word < stride; word++) { destination[index + word] = words[word]; }
}
