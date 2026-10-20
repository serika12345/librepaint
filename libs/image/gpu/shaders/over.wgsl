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

// Positive finite binary32 values for normal RGBA8 projection. Integer
// evaluation fixes normalization, single-rounded multiply-add and quotient
// rounding before the final nearest-even byte conversion.
fn projectionAdd(a: vec2<u32>, b: vec2<u32>) -> vec2<u32> {
    let low = a.x + b.x;
    return vec2<u32>(low, a.y + b.y + select(0u, 1u, low < a.x));
}
fn projectionSubtract(a: vec2<u32>, b: vec2<u32>) -> vec2<u32> {
    return vec2<u32>(a.x - b.x, a.y - b.y - select(0u, 1u, a.x < b.x));
}
fn projectionLeft(value: vec2<u32>, count: u32) -> vec2<u32> {
    if (count == 0u) { return value; }
    if (count >= 32u) { return vec2<u32>(0u, value.x << (count - 32u)); }
    return vec2<u32>(value.x << count, (value.y << count) | (value.x >> (32u - count)));
}
fn projectionRight(value: vec2<u32>, count: u32) -> vec2<u32> {
    if (count == 0u) { return value; }
    if (count >= 32u) { return vec2<u32>(value.y >> (count - 32u), 0u); }
    return vec2<u32>((value.x >> count) | (value.y << (32u - count)), value.y >> count);
}
fn projectionLowBits(value: vec2<u32>, count: u32) -> bool {
    if (count >= 32u) {
        return value.x != 0u || (value.y & ((1u << (count - 32u)) - 1u)) != 0u;
    }
    return (value.x & ((1u << count) - 1u)) != 0u;
}
fn projectionRoundedShift(value: vec2<u32>, count: u32) -> u32 {
    let quotient = projectionRight(value, count).x;
    let guardBit = projectionRight(value, count - 1u).x & 1u;
    return quotient + select(0u, 1u, guardBit != 0u &&
        (projectionLowBits(value, count - 1u) || (quotient & 1u) != 0u));
}
fn projectionFloat(value: vec2<u32>, scale: i32) -> u32 {
    if (all(value == vec2<u32>(0u))) { return 0u; }
    var leading = firstLeadingBit(value.x);
    if (value.y != 0u) { leading = 32u + firstLeadingBit(value.y); }
    let shift = i32(leading) - 23;
    var mantissa: u32;
    var power = scale + shift;
    if (shift > 0) { mantissa = projectionRoundedShift(value, u32(shift)); }
    else { mantissa = value.x << u32(-shift); }
    if (mantissa == 0x1000000u) { mantissa >>= 1u; power += 1; }
    return (u32(power + 23 + 127) << 23u) | (mantissa & 0x7fffffu);
}
fn projectionMantissa(value: u32) -> u32 { return (value & 0x7fffffu) | 0x800000u; }
fn projectionPower(value: u32) -> i32 { return i32((value >> 23u) & 255u) - 127 - 23; }
fn projectionScaleByte(value: u32, factor: u32) -> u32 {
    return projectionFloat(vec2<u32>(projectionMantissa(value) * factor, 0u), projectionPower(value));
}
fn projectionProduct(a: u32, b: u32) -> vec2<u32> {
    // Each operand has 24 bits; the two cross products fit in 25 bits.
    let low = (a & 65535u) * (b & 65535u);
    let cross = (a >> 16u) * (b & 65535u) + (b >> 16u) * (a & 65535u);
    let sum = low + (cross << 16u);
    return vec2<u32>(sum, (a >> 16u) * (b >> 16u) + (cross >> 16u) + select(0u, 1u, sum < low));
}
fn projectionMultiply(a: u32, b: u32) -> u32 {
    return projectionFloat(projectionProduct(projectionMantissa(a), projectionMantissa(b)),
                           projectionPower(a) + projectionPower(b));
}
fn projectionAlpha(sourceAlpha: u32, destinationAlpha: u32) -> u32 {
    let weighted = projectionScaleByte(sourceAlpha, 255u - destinationAlpha);
    let product = projectionProduct(projectionMantissa(weighted), 0x808081u);
    let productPower = projectionPower(weighted) - 31;
    // Keep 56 significant accumulator bits. Discarded product bits become a
    // sticky bit, below every guard bit used by binary32 normalization.
    let commonPower = max(productPower, i32(firstLeadingBit(destinationAlpha)) + 1 - 56);
    let shift = u32(commonPower - productPower);
    var retained = projectionRight(product, shift);
    if (projectionLowBits(product, shift)) { retained.x |= 1u; }
    let destination = projectionLeft(vec2<u32>(destinationAlpha, 0u), u32(-commonPower));
    return projectionFloat(projectionAdd(retained, destination), commonPower);
}
fn projectionDivide(a: u32, b: u32) -> u32 {
    let denominator = projectionMantissa(b);
    var remainder = projectionMantissa(a);
    var exponent = projectionPower(a) - projectionPower(b);
    if (remainder < denominator) { remainder <<= 1u; exponent -= 1; }
    remainder -= denominator;
    var quotient = 1u;
    for (var bit = 0u; bit < 23u; bit += 1u) {
        remainder <<= 1u;
        quotient <<= 1u;
        if (remainder >= denominator) { remainder -= denominator; quotient |= 1u; }
    }
    if (remainder * 2u > denominator || (remainder * 2u == denominator && (quotient & 1u) != 0u)) {
        quotient += 1u;
    }
    if (quotient == 0x1000000u) { quotient >>= 1u; exponent += 1; }
    return (u32(exponent + 127) << 23u) | (quotient & 0x7fffffu);
}
fn projectionByte(value: u32) -> u32 {
    if (value == 0u) { return 0u; }
    let shift = -projectionPower(value);
    if (shift > 24) { return 0u; }
    return projectionRoundedShift(vec2<u32>(projectionMantissa(value), 0u), u32(shift));
}
fn projectionColor(destination: u32, source: u32, blend: u32) -> u32 {
    let difference = i32(source) - i32(destination);
    let product = vec2<u32>(u32(abs(difference)) * projectionMantissa(blend), 0u);
    let base = projectionLeft(vec2<u32>(destination, 0u), u32(-projectionPower(blend)));
    var value: vec2<u32>;
    if (difference < 0) { value = projectionSubtract(base, product); }
    else { value = projectionAdd(base, product); }
    return projectionByte(projectionFloat(value, projectionPower(blend)));
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
        let product = select(multiplied, source[channel] + destination[channel] - multiplied, mode == 2u);
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

fn projectionOver(base: u32, layer: u32, values: CompositeParameters) -> u32 {
    var destination = unpackPixel(base);
    if (values.normalizeBase != 0u && destination.w == 0u) { destination = vec4<u32>(0u); }
    let source = unpackPixel(layer);
    if ((values.blendModeOpacity & 255u) != 0u) {
        return separableProjectionPixel(destination, source, values.blendModeOpacity >> 8u, values.channelMask, values.blendModeOpacity & 255u);
    }
    if (source.w == 0u) { return packPixel(destination); }
    if (values.channelMask == 15u && source.w == 255u && values.opacityBits == 0x3f800000u) {
        return packPixel(source);
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
    outputPixels[index] = projectionOver(basePixels[index], layerPixels[index], parameters);
}

@compute @workgroup_size(8, 8)
fn mainPair(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let position = invocation.xy;
    if (any(position >= vec2<u32>(256u))) { return; }
    let index = position.y * 256u + position.x;
    let first = projectionOver(basePixels[index], layerPixels[index], pairParameters.first);
    outputPixels[index] = projectionOver(first, secondLayerPixels[index], pairParameters.second);
}
