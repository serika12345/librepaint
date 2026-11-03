// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

alias Word64 = vec2<u32>;
alias Word128 = vec4<u32>;

fn wordLessEqual(a: Word64, b: Word64) -> bool {
    return a.y < b.y || (a.y == b.y && a.x <= b.x);
}
fn wordAdd(a: Word64, b: Word64) -> Word64 { return projectionAdd(a, b); }
fn wordSubtract(a: Word64, b: Word64) -> Word64 { return projectionSubtract(a, b); }
fn wordLeft(value: Word64, count: u32) -> Word64 {
    if (count >= 64u) { return Word64(0u); }
    return projectionLeft(value, count);
}
fn wordRight(value: Word64, count: u32) -> Word64 {
    if (count >= 64u) { return Word64(0u); }
    return projectionRight(value, count);
}
fn wordLow(value: Word64, count: u32) -> bool {
    if (count >= 64u) { return any(value != Word64(0u)); }
    return projectionLowBits(value, count);
}
fn wordJam(value: Word64, count: u32) -> Word64 {
    var result = wordRight(value, count);
    if (wordLow(value, count)) { result.x |= 1u; }
    return result;
}
fn wordRound(value: Word64, count: u32) -> Word64 {
    let quotient = wordRight(value, count);
    let guard = wordRight(value, count - 1u).x & 1u;
    let up = guard != 0u && (wordLow(value, count - 1u) || (quotient.x & 1u) != 0u);
    return wordAdd(quotient, Word64(select(0u, 1u, up), 0u));
}
fn wideWord(value: Word128, index: i32) -> u32 {
    if (index < 0 || index >= 4) { return 0u; }
    return value[u32(index)];
}
fn wideAdd(a: Word128, b: Word128) -> Word128 {
    var result = Word128(0u);
    var carry = 0u;
    for (var i = 0u; i < 4u; i += 1u) {
        let first = a[i] + b[i];
        let second = first + carry;
        carry = select(0u, 1u, first < a[i] || second < first);
        result[i] = second;
    }
    return result;
}
fn wideLeft(value: Word128, count: u32) -> Word128 {
    var result = Word128(0u);
    let words = i32(count / 32u);
    let bits = count % 32u;
    for (var i = 0; i < 4; i += 1) {
        result[u32(i)] = wideWord(value, i - words) << bits;
        if (bits != 0u) { result[u32(i)] |= wideWord(value, i - words - 1) >> (32u - bits); }
    }
    return result;
}
fn wideRight(value: Word128, count: u32) -> Word64 {
    let words = i32(count / 32u);
    let bits = count % 32u;
    var result = Word64(wideWord(value, words), wideWord(value, words + 1));
    if (bits != 0u) {
        result.x = (result.x >> bits) | (result.y << (32u - bits));
        result.y = (result.y >> bits) | (wideWord(value, words + 2) << (32u - bits));
    }
    return result;
}
fn wideLow(value: Word128, count: u32) -> bool {
    let words = count / 32u;
    for (var i = 0u; i < min(words, 4u); i += 1u) {
        if (value[i] != 0u) { return true; }
    }
    return (wideWord(value, i32(words)) & ((1u << (count % 32u)) - 1u)) != 0u;
}
fn wordProduct(a: u32, b: u32) -> Word64 {
    let low = (a & 65535u) * (b & 65535u);
    let first = (a >> 16u) * (b & 65535u);
    let second = (b >> 16u) * (a & 65535u);
    let middle = (low >> 16u) + (first & 65535u) + (second & 65535u);
    return Word64((low & 65535u) | (middle << 16u),
        (a >> 16u) * (b >> 16u) + (first >> 16u) + (second >> 16u) + (middle >> 16u));
}
fn wideProduct(a: Word64, b: Word64) -> Word128 {
    let low = wordProduct(a.x, b.x);
    let first = wordProduct(a.x, b.y);
    let second = wordProduct(a.y, b.x);
    let high = wordProduct(a.y, b.y);
    return wideAdd(wideAdd(Word128(low, 0u, 0u), Word128(0u, first, 0u)),
                   wideAdd(Word128(0u, second, 0u), Word128(0u, 0u, high)));
}
fn doubleMagnitude(value: Word64) -> Word64 { return Word64(value.x, value.y & 0x7fffffffu); }
fn doubleMantissa(value: Word64) -> Word64 { return Word64(value.x, (value.y & 0xfffffu) | 0x100000u); }
fn doublePower(value: Word64) -> i32 { return i32((value.y >> 20u) & 2047u) - 1023 - 52; }
fn doubleEncode(value: Word64, scale: i32, sign: u32) -> Word64 {
    var mantissa = value;
    var power = scale;
    if ((mantissa.y & 0x200000u) != 0u) { mantissa = wordRight(mantissa, 1u); power += 1; }
    return Word64(mantissa.x, sign | (u32(power + 52 + 1023) << 20u) | (mantissa.y & 0xfffffu));
}
fn doubleFloat(value: Word64, scale: i32, sign: u32) -> Word64 {
    if (all(value == Word64(0u))) { return Word64(0u); }
    var leading = 31u - countLeadingZeros(value.x);
    if (value.y != 0u) { leading = 63u - countLeadingZeros(value.y); }
    let shift = i32(leading) - 52;
    var mantissa: Word64;
    if (shift > 0) { mantissa = wordRound(value, u32(shift)); }
    else { mantissa = wordLeft(value, u32(-shift)); }
    return doubleEncode(mantissa, scale + shift, sign);
}
// Values keep the binary64 and binary32 bit layouts used by buffers and callers,
// but arithmetic runs in native f32. Saved pixels only need to stay within one
// 8-bit step across hosts (#107), which f32 meets for every color depth.
fn doubleToNative(value: Word64) -> f32 {
    let sign = value.y & 0x80000000u;
    let exponent = (value.y >> 20u) & 2047u;
    if (exponent == 0u) { return bitcast<f32>(sign); }
    if (exponent == 2047u) {
        let nan = select(0u, 0x400000u, ((value.y & 0xfffffu) | value.x) != 0u);
        return bitcast<f32>(sign | 0x7f800000u | nan);
    }
    let power = i32(exponent) - 1023;
    if (power < -126) { return bitcast<f32>(sign); }
    if (power > 127) { return bitcast<f32>(sign | 0x7f800000u); }
    // The 23-bit fraction takes 20 high and 3 low mantissa bits; a carry from
    // rounding propagates into the exponent.
    let fraction = ((value.y & 0xfffffu) << 3u) | (value.x >> 29u);
    return bitcast<f32>((sign | (u32(power + 127) << 23u) | fraction) + ((value.x >> 28u) & 1u));
}
fn doubleFromNative(value: f32) -> Word64 {
    let bits = bitcast<u32>(value);
    let sign = bits & 0x80000000u;
    let exponent = (bits >> 23u) & 255u;
    let fraction = bits & 0x7fffffu;
    if (exponent == 0u) { return Word64(0u, 0u); }
    if (exponent == 255u) { return Word64(select(0u, 1u, fraction != 0u), sign | 0x7ff00000u); }
    return Word64(fraction << 29u, sign | ((exponent + 896u) << 20u) | (fraction >> 3u));
}
fn doubleAdd(a: Word64, b: Word64) -> Word64 { return doubleFromNative(doubleToNative(a) + doubleToNative(b)); }
fn doubleNegate(value: Word64) -> Word64 { return Word64(value.x, value.y ^ 0x80000000u); }
fn doubleSubtract(a: Word64, b: Word64) -> Word64 { return doubleFromNative(doubleToNative(a) - doubleToNative(b)); }
fn doubleMultiply(a: Word64, b: Word64) -> Word64 { return doubleFromNative(doubleToNative(a) * doubleToNative(b)); }
fn doubleDivide(a: Word64, b: Word64) -> Word64 {
    let numerator = doubleToNative(a);
    if (numerator == 0.0) { return Word64(0u); }
    return doubleFromNative(numerator / doubleToNative(b));
}
fn doubleFloor(value: Word64) -> Word64 { return doubleFromNative(floor(doubleToNative(value))); }
fn doubleSqrt(value: Word64) -> Word64 { return doubleFromNative(sqrt(max(doubleToNative(value), 0.0))); }
fn doubleFromFloat(value: u32) -> Word64 { return doubleFromNative(bitcast<f32>(value)); }
fn doubleInteger(value: u32) -> Word64 { return doubleFromNative(f32(value)); }
fn doubleTruncate(value: Word64) -> u32 { return u32(max(doubleToNative(value), 0.0)); }
fn floatFromDouble(value: Word64) -> u32 { return bitcast<u32>(doubleToNative(value)); }
fn floatAdd(a: u32, b: u32) -> u32 { return bitcast<u32>(bitcast<f32>(a) + bitcast<f32>(b)); }
fn floatSubtract(a: u32, b: u32) -> u32 { return bitcast<u32>(bitcast<f32>(a) - bitcast<f32>(b)); }
fn floatMultiply(a: u32, b: u32) -> u32 { return bitcast<u32>(bitcast<f32>(a) * bitcast<f32>(b)); }
fn floatMultiplyAdd(a: u32, b: u32, c: u32) -> u32 {
    return bitcast<u32>(bitcast<f32>(a) * bitcast<f32>(b) + bitcast<f32>(c));
}
fn floatDivide(a: u32, b: u32) -> u32 {
    if ((a & 0x7fffffffu) == 0u) { return 0u; }
    return bitcast<u32>(bitcast<f32>(a) / bitcast<f32>(b));
}
fn floatSqrt(value: u32) -> u32 { return bitcast<u32>(sqrt(max(bitcast<f32>(value), 0.0))); }
fn finiteColorExp(value: Word64) -> Word64 { return doubleFromNative(exp(doubleToNative(value))); }
fn positiveColorLog(value: Word64) -> Word64 { return doubleFromNative(log(doubleToNative(value))); }
fn wordColorPower(base: Word64, exponent: Word64) -> Word64 {
    let x = doubleToNative(base);
    let y = doubleToNative(exponent);
    if (y == 0.0 || x == 1.0) { return Word64(0u, 0x3ff00000u); }
    if (x <= 0.0) { return Word64(0u); }
    return doubleFromNative(pow(x, y));
}
fn positiveColorAtan(value: Word64) -> Word64 { return doubleFromNative(atan(doubleToNative(value))); }
