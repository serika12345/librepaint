// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

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
    var leading = 31u - countLeadingZeros(value.x);
    if (value.y != 0u) { leading = 63u - countLeadingZeros(value.y); }
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
