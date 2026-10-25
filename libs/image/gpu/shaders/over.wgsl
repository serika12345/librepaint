// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// blendModeOpacity holds LayerBlendMode in the low byte and opacity in the next.
struct CompositeParameters { opacityBits: u32, normalizeBase: u32, channelMask: u32, blendModeOpacity: u32, }
@group(0) @binding(0) var<storage, read> basePixels: array<u32>;
@group(0) @binding(1) var<storage, read> layerPixels: array<u32>;
@group(0) @binding(2) var<storage, read_write> outputPixels: array<u32>;
@group(0) @binding(3) var<uniform> parameters: CompositeParameters;
struct CompositePairParameters { first: CompositeParameters, second: CompositeParameters, }
@group(0) @binding(4) var<storage, read> secondLayerPixels: array<u32>;
@group(0) @binding(5) var<uniform> pairParameters: CompositePairParameters;
// Four alpha bytes per word. Opaque coverage is bound for layers without a mask.
@group(0) @binding(6) var<storage, read> firstMaskCoverage: array<u32>;
@group(0) @binding(7) var<storage, read> secondMaskCoverage: array<u32>;

fn coverageAlpha(words: u32, index: u32) -> u32 {
    return (words >> ((index & 3u) * 8u)) & 255u;
}

fn multiplyByte(a: u32, b: u32) -> u32 {
    let product = a * b + 128u;
    return ((product >> 8u) + product) >> 8u;
}
fn multiplyThreeBytes(a: u32, b: u32, c: u32) -> u32 {
    let product = a * b * c + 0x7f5bu;
    return ((product >> 7u) + product) >> 16u;
}
fn blendByte(destination: u32, source: u32, alpha: u32) -> u32 {
    let weighted = (i32(source) - i32(destination)) * i32(alpha) + 128;
    return u32((((weighted >> 8u) + weighted) >> 8u) + i32(destination)) & 255u;
}
fn separableProjectionPixel(base: vec4<u32>, source: vec4<u32>, opacity: u32, mask: u32, mode: u32) -> u32 {
    var destination = base;
    if (mask != 15u && destination.w == 0u) { destination = vec4<u32>(0u); }
    let alpha = multiplyThreeBytes(source.w, 255u, opacity);
    if (alpha == 0u) { return packPixel(destination); }
    let locked = (mask & 8u) == 0u;
    let oldAlpha = destination.w;
    var nextAlpha = oldAlpha;
    if (!locked) { nextAlpha = alpha + oldAlpha - multiplyByte(alpha, oldAlpha); }
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((mask & (1u << channel)) == 0u) { continue; }
        let multiplied = multiplyByte(source[channel], destination[channel]);
        var product = multiplied;
        switch mode {
            case 2u: { product = source[channel] + destination[channel] - multiplied; }
            case 3u: { product = min(source[channel], destination[channel]); }
            case 4u: { product = max(source[channel], destination[channel]); }
            case 5u, 6u: { product = min(255u, source[channel] + destination[channel]); }
            default: {}
        }
        if (locked || oldAlpha == 255u) {
            if (oldAlpha != 0u) { destination[channel] = blendByte(destination[channel], product, alpha); }
        } else if (oldAlpha == 0u) { destination[channel] = source[channel]; }
        else if (alpha == 255u) { destination[channel] = blendByte(source[channel], product, oldAlpha); }
        else {
            let result = (multiplyThreeBytes(255u - alpha, oldAlpha, destination[channel]) +
                multiplyThreeBytes(255u - oldAlpha, alpha, source[channel]) + multiplyThreeBytes(oldAlpha, alpha, product)) & 255u;
            destination[channel] = ((result * 255u + nextAlpha / 2u) / nextAlpha) & 255u;
        }
    }
    destination.w = nextAlpha;
    return packPixel(destination);
}

fn projectionOver(base: u32, layer: u32, values: CompositeParameters, maskAlpha: u32) -> u32 {
    var destination = unpackPixel(base);
    if (values.normalizeBase != 0u && destination.w == 0u) { destination = vec4<u32>(0u); }
    var source = unpackPixel(layer);
    if (maskAlpha != 255u) { source.w = multiplyByte(source.w, maskAlpha); }
    if ((values.blendModeOpacity & 255u) != 0u) {
        return separableProjectionPixel(destination, source, values.blendModeOpacity >> 8u, values.channelMask, values.blendModeOpacity & 255u);
    }
    if (source.w == 0u) { return packPixel(destination); }
    if (values.channelMask == 15u && source.w == 255u && values.opacityBits == 0x3f800000u) {
        return packPixel(source);
    }
    // Full-opacity byte colors have binary32 error below 2^-14. The odd
    // divisor 255 of an opaque base stays farther from every half-byte.
    if (values.opacityBits == 0x3f800000u && destination.w == 255u) {
        for (var channel = 0u; channel < 3u; channel += 1u) {
            if ((values.channelMask & (1u << channel)) != 0u) {
                destination[channel] = (source[channel] * source.w + destination[channel] * (255u - source.w) + 127u) / 255u;
            }
        }
        return packPixel(destination);
    }
    if (values.opacityBits == 0x3f800000u && values.channelMask == 15u && destination.w != 0u) {
        let denominator = source.w * 255u + destination.w * (255u - source.w);
        let numerator = source.xyz * (source.w * 255u) + destination.xyz * (destination.w * (255u - source.w));
        let quotient = numerator / vec3<u32>(denominator);
        let doubledRemainder = (numerator - quotient * denominator) * 2u;
        let gap = abs(vec3<i32>(doubledRemainder) - vec3<i32>(i32(denominator)));
        // Rounding boundaries retain the precise calculation below.
        if (all(gap > vec3<i32>(16))) {
            // The distance exceeds 8/65025 in color units, beyond the error.
            let rounded = quotient + select(vec3<u32>(0u), vec3<u32>(1u), doubledRemainder > vec3<u32>(denominator));
            return packPixel(vec4<u32>(rounded, (denominator + 127u) / 255u));
        }
    }
    let sourceAlpha = projectionScaleByte(values.opacityBits, source.w);
    var alpha = destination.w;
    var blend: u32;
    if ((values.channelMask & 8u) == 0u || destination.w == 255u) {
        blend = projectionMultiply(sourceAlpha, 0x3b808081u);
    } else if (destination.w == 0u) {
        alpha = projectionByte(sourceAlpha);
        blend = 0x3f800000u;
        if (values.channelMask != 15u) { destination = vec4<u32>(0u); }
    } else {
        let value = projectionAlpha(sourceAlpha, destination.w);
        alpha = projectionByte(value);
        blend = projectionDivide(sourceAlpha, value);
    }
    var color = destination.xyz;
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((values.channelMask & (1u << channel)) == 0u) { continue; }
        if (blend == 0x3f800000u) { color[channel] = source[channel]; }
        else { color[channel] = projectionColor(destination[channel], source[channel], blend); }
    }
    return packPixel(vec4<u32>(color, alpha));
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let position = invocation.xy;
    if (any(position >= vec2<u32>(256u))) { return; }
    let index = position.y * 256u + position.x;
    let maskAlpha = coverageAlpha(firstMaskCoverage[index >> 2u], index);
    outputPixels[index] = projectionOver(basePixels[index], layerPixels[index], parameters, maskAlpha);
}

@compute @workgroup_size(8, 8)
fn mainPair(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let position = invocation.xy;
    if (any(position >= vec2<u32>(256u))) { return; }
    let index = position.y * 256u + position.x;
    let firstMask = coverageAlpha(firstMaskCoverage[index >> 2u], index);
    let secondMask = coverageAlpha(secondMaskCoverage[index >> 2u], index);
    let first = projectionOver(basePixels[index], layerPixels[index], pairParameters.first, firstMask);
    outputPixels[index] = projectionOver(first, secondLayerPixels[index], pairParameters.second, secondMask);
}
