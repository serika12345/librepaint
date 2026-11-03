// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Opacity, alpha and native color components travel as binary32 bit patterns.
// The 64-bit integer helpers are exact; floating-point helpers use native f32
// (#107: saved pixels stay within one 8-bit step across hosts).
fn projectionAdd(a: vec2<u32>, b: vec2<u32>) -> vec2<u32> {
    let low = a.x + b.x;
    return vec2<u32>(low, a.y + b.y + select(0u, 1u, low < a.x));
}
fn projectionSubtract(a: vec2<u32>, b: vec2<u32>) -> vec2<u32> {
    return vec2<u32>(a.x - b.x, a.y - b.y - select(0u, 1u, a.x < b.x));
}
fn projectionLeft(value: vec2<u32>, count: u32) -> vec2<u32> {
    if (count == 0u) { return value; }
    if (count >= 64u) { return vec2<u32>(0u); }
    if (count >= 32u) { return vec2<u32>(0u, value.x << (count - 32u)); }
    return vec2<u32>(value.x << count, (value.y << count) | (value.x >> (32u - count)));
}
fn projectionRight(value: vec2<u32>, count: u32) -> vec2<u32> {
    if (count == 0u) { return value; }
    if (count >= 64u) { return vec2<u32>(0u); }
    if (count >= 32u) { return vec2<u32>(value.y >> (count - 32u), 0u); }
    return vec2<u32>((value.x >> count) | (value.y << (32u - count)), value.y >> count);
}
fn projectionLowBits(value: vec2<u32>, count: u32) -> bool {
    if (count >= 64u) { return any(value != vec2<u32>(0u)); }
    if (count >= 32u) {
        return value.x != 0u || (value.y & ((1u << (count - 32u)) - 1u)) != 0u;
    }
    return (value.x & ((1u << count) - 1u)) != 0u;
}
fn projectionRoundedShift(value: vec2<u32>, count: u32) -> u32 {
    if (count == 0u) { return value.x; }
    if (count > 64u) { return 0u; }
    let quotient = projectionRight(value, count).x;
    let guardBit = projectionRight(value, count - 1u).x & 1u;
    return quotient + select(0u, 1u, guardBit != 0u &&
        (projectionLowBits(value, count - 1u) || (quotient & 1u) != 0u));
}
fn projectionFloat(value: vec2<u32>, scale: i32) -> u32 {
    return bitcast<u32>(ldexp(f32(value.y) * 4294967296.0 + f32(value.x), scale));
}
fn projectionMantissa(value: u32) -> u32 {
    let fraction = value & 0x7fffffu;
    if ((value & 0x7f800000u) != 0u) { return fraction | 0x800000u; }
    if (fraction == 0u) { return 0u; }
    return fraction << (countLeadingZeros(fraction) - 8u);
}
fn projectionPower(value: u32) -> i32 {
    let exponent = (value >> 23u) & 255u;
    if (exponent != 0u) { return i32(exponent) - 127 - 23; }
    let fraction = value & 0x7fffffu;
    if (fraction == 0u) { return -149; }
    return -149 - i32(countLeadingZeros(fraction) - 8u);
}
fn projectionScaleByte(value: u32, factor: u32) -> u32 {
    return bitcast<u32>(bitcast<f32>(value) * f32(factor));
}
fn projectionProduct(a: u32, b: u32) -> vec2<u32> {
    // Normalized mantissas have 24 bits. U16 triples also multiply a 32-bit
    // product by a 16-bit component; its sole cross product fits 32 bits.
    let low = (a & 65535u) * (b & 65535u);
    let cross = (a >> 16u) * (b & 65535u) + (b >> 16u) * (a & 65535u);
    let sum = low + (cross << 16u);
    return vec2<u32>(sum, (a >> 16u) * (b >> 16u) + (cross >> 16u) + select(0u, 1u, sum < low));
}
fn projectionMultiply(a: u32, b: u32) -> u32 {
    return bitcast<u32>(bitcast<f32>(a) * bitcast<f32>(b));
}
fn projectionAlpha(sourceAlpha: u32, destinationAlpha: u32) -> u32 {
    let destination = f32(destinationAlpha);
    return bitcast<u32>(destination + bitcast<f32>(sourceAlpha) * (255.0 - destination) / 255.0);
}
fn projectionDivide(a: u32, b: u32) -> u32 {
    if (a == 0u) { return 0u; }
    return bitcast<u32>(bitcast<f32>(a) / bitcast<f32>(b));
}
fn projectionComponent(value: u32) -> u32 {
    return u32(round(max(bitcast<f32>(value), 0.0)));
}
fn projectionColor(destination: u32, source: u32, blend: u32) -> u32 {
    // Below 2^-9 the change is less than half a byte.
    if (blend < 0x3b000000u) { return destination; }
    let difference = f32(i32(source) - i32(destination));
    return u32(round(max(f32(destination) + difference * bitcast<f32>(blend), 0.0)));
}
