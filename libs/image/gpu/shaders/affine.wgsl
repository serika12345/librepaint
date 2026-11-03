// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters {
    origin: vec2<i32>, format: u32, kind: u32,
    source: vec4<i32>, destination: vec4<i32>,
    matrix: vec4<i32>, placement: vec4<i32>,
    scale: f32, shear: f32, offset: f32, support: f32,
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

fn signedTruncate(value: f32) -> i32 { return i32(value); }
fn nativeRound(value: f32) -> i32 { return i32(sign(value) * floor(abs(value) + 0.5)); }
fn mappedSource(destination: i32, line: i32) -> f32 {
    let center = (f32(destination) + 0.5);
    let shifted = ((center - parameters.offset) - (f32(line) * parameters.shear));
    return (shifted / parameters.scale);
}
fn mappedDestination(source: i32, line: i32) -> f32 {
    return (((f32(source) * parameters.scale) + parameters.offset) + (f32(line) * parameters.shear));
}
fn quotientClamped(numerator: f32, denominator: f32, maximum: u32) -> u32 {
    return u32(clamp(floor(numerator / denominator + 0.5), 0.0, f32(maximum)));
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
    let positive = parameters.scale >= 0.0;
    let clampEdge = (parameters.shear == 0.0);
    var left = mappedDestination(select(end, start, positive), line);
    var right = mappedDestination(select(start, end, positive), line);
    if (!clampEdge) { left = (left - parameters.support); right = (right + parameters.support); }
    let begin = nativeRound(left);
    let finish = max(begin + 1, nativeRound(right));
    if (coordinate < begin || coordinate >= finish) {
        if (inside(position, parameters.source)) { return parameters.implicit; }
        return pixel(position);
    }
    let mapped = mappedSource(coordinate, line);
    let fixed = signedTruncate((mapped * 256.0));
    let lower = fixed - 128;
    let ceiling = select(-((-lower) / 256), (lower + 255) / 256, lower >= 0);
    let center = ceiling * 256 + 128;
    let phaseIndex = u32(((center - fixed) * i32(parameters.positionScale)) >> 8u);
    let phase = phases[phaseIndex];
    let first = floor256(center) - phase.z;
    var totals = array<f32, 3>(0.0, 0.0, 0.0);
    var alpha = 0.0;
    for (var i = 0; i < phase.y; i++) {
        var source = first + i;
        if (clampEdge) { source = clamp(source, start, end - 1); }
        var color = parameters.implicit;
        if (source >= start && source < end) {
            color = pixel(select(vec2<i32>(line, source), vec2<i32>(source, line), horizontal));
        }
        let weight = weights[u32(phase.x + i)];
        let amount = f32(color.w) * f32(weight);
        alpha += amount;
        for (var channel = 0u; channel < 3u; channel++) {
            totals[channel] += f32(color[channel]) * amount;
        }
    }
    if ((alpha == 0.0) || alpha < 0.0) { return vec4<u32>(0u); }
    let maximum = select(255u, 65535u, parameters.format != 0u);
    return vec4<u32>(quotientClamped(totals[0], alpha, maximum), quotientClamped(totals[1], alpha, maximum),
        quotientClamped(totals[2], alpha, maximum), quotientClamped(alpha, 255.0, maximum));
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
