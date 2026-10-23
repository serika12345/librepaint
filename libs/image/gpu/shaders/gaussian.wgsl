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
fn doubleAdd(a: Word64, b: Word64) -> Word64 {
    let aa = doubleMagnitude(a);
    let bb = doubleMagnitude(b);
    if (all(aa == Word64(0u))) { return b; }
    if (all(bb == Word64(0u))) { return a; }
    let reverse = !wordLessEqual(bb, aa);
    let larger = select(a, b, reverse);
    let smaller = select(b, a, reverse);
    let power = doublePower(larger);
    let first = wordLeft(doubleMantissa(larger), 3u);
    let second = wordJam(wordLeft(doubleMantissa(smaller), 3u), u32(power - doublePower(smaller)));
    var result: Word64;
    if (((larger.y ^ smaller.y) & 0x80000000u) == 0u) { result = wordAdd(first, second); }
    else { result = wordSubtract(first, second); }
    return doubleFloat(result, power - 3, larger.y & 0x80000000u);
}
fn doubleNegate(value: Word64) -> Word64 { return Word64(value.x, value.y ^ 0x80000000u); }
fn doubleSubtract(a: Word64, b: Word64) -> Word64 { return doubleAdd(a, doubleNegate(b)); }
fn doubleMultiply(a: Word64, b: Word64) -> Word64 {
    if (all(doubleMagnitude(a) == Word64(0u)) || all(doubleMagnitude(b) == Word64(0u))) { return Word64(0u); }
    let product = wideProduct(doubleMantissa(a), doubleMantissa(b));
    let leading = 127u - countLeadingZeros(product.w);
    let shift = leading - 52u;
    var mantissa = wideRight(product, shift);
    let guard = wideRight(product, shift - 1u).x & 1u;
    if (guard != 0u && (wideLow(product, shift - 1u) || (mantissa.x & 1u) != 0u)) {
        mantissa = wordAdd(mantissa, Word64(1u, 0u));
    }
    return doubleEncode(mantissa, doublePower(a) + doublePower(b) + i32(shift), (a.y ^ b.y) & 0x80000000u);
}
fn doubleSqrt(value: Word64) -> Word64 {
    if (all(doubleMagnitude(value) == Word64(0u))) { return Word64(0u); }
    let power = doublePower(value) + 52;
    let odd = u32(power & 1);
    let square = wideLeft(Word128(doubleMantissa(value), 0u, 0u), 52u + odd);
    var root = Word64(0u);
    var remainder = Word64(0u);
    for (var i = 52; i >= 0; i -= 1) {
        remainder = wordLeft(remainder, 2u);
        remainder.x |= wideRight(square, u32(i) * 2u).x & 3u;
        let trial = wordLeft(root, 2u) | Word64(1u, 0u);
        root = wordLeft(root, 1u);
        if (wordLessEqual(trial, remainder)) {
            remainder = wordSubtract(remainder, trial);
            root.x |= 1u;
        }
    }
    if (!wordLessEqual(remainder, root)) { root = wordAdd(root, Word64(1u, 0u)); }
    return doubleEncode(root, (power - i32(odd)) / 2 - 52, 0u);
}
fn doubleFromFloat(value: u32) -> Word64 {
    if ((value & 0x7fffffffu) == 0u) { return Word64(0u); }
    let mantissa = wordLeft(Word64(projectionMantissa(value), 0u), 29u);
    return Word64(mantissa.x, (value & 0x80000000u) |
        (u32(projectionPower(value) + 23 + 1023) << 20u) | (mantissa.y & 0xfffffu));
}
fn doubleInteger(value: u32) -> Word64 { return doubleFromFloat(bitcast<u32>(f32(value))); }
fn doubleTruncate(value: Word64) -> u32 {
    let exponent = doublePower(value) + 52;
    if (exponent < 0) { return 0u; }
    return wordRight(doubleMantissa(value), u32(52 - exponent)).x;
}
fn floatFromDouble(value: Word64) -> u32 {
    if (all(doubleMagnitude(value) == Word64(0u))) { return 0u; }
    return projectionFloat(doubleMantissa(value), doublePower(value)) | (value.y & 0x80000000u);
}
fn floatAdd(a: u32, b: u32) -> u32 {
    let aa = a & 0x7fffffffu;
    let bb = b & 0x7fffffffu;
    if (aa == 0u) { return b; }
    if (bb == 0u) { return a; }
    let larger = select(a, b, aa < bb);
    let smaller = select(b, a, aa < bb);
    let power = projectionPower(larger);
    let first = projectionMantissa(larger) << 3u;
    let second = wordJam(Word64(projectionMantissa(smaller) << 3u, 0u), u32(power - projectionPower(smaller))).x;
    var result: u32;
    if (((a ^ b) & 0x80000000u) == 0u) { result = first + second; }
    else { result = first - second; }
    return projectionFloat(Word64(result, 0u), power - 3) | (larger & 0x80000000u);
}
fn floatSubtract(a: u32, b: u32) -> u32 { return floatAdd(a, b ^ 0x80000000u); }
fn floatMultiply(a: u32, b: u32) -> u32 {
    if ((a & 0x7fffffffu) == 0u || (b & 0x7fffffffu) == 0u) { return 0u; }
    return projectionMultiply(a, b) | ((a ^ b) & 0x80000000u);
}
fn floatDivide(a: u32, b: u32) -> u32 {
    if ((a & 0x7fffffffu) == 0u) { return 0u; }
    return projectionDivide(a, b);
}
fn floatSqrt(value: u32) -> u32 {
    if (value == 0u) { return 0u; }
    let power = projectionPower(value) + 23;
    let odd = u32(power & 1);
    let square = wordLeft(Word64(projectionMantissa(value), 0u), 23u + odd);
    var root = 0u;
    var remainder = 0u;
    for (var i = 23; i >= 0; i -= 1) {
        remainder = (remainder << 2u) | (wordRight(square, u32(i) * 2u).x & 3u);
        let trial = (root << 2u) | 1u;
        root <<= 1u;
        if (trial <= remainder) { remainder -= trial; root |= 1u; }
    }
    if (remainder > root) { root += 1u; }
    var exponent = (power - i32(odd)) / 2;
    if (root == 0x1000000u) { root >>= 1u; exponent += 1; }
    return (u32(exponent + 127) << 23u) | (root & 0x7fffffu);
}

struct MaskDab {
    size: vec4<u32>,
    center: vec4<u32>,
    rotation: vec4<u32>,
    radius: vec4<u32>,
    coefficient: vec4<u32>,
    application: vec4<u32>,
}

fn smallCircleAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    let side = dab.size.z;
    var inverseStep = Word64(0u, 0x3ff00000u);
    if (side == 3u) { inverseStep = Word64(0x55555555u, 0x3fd55555u); }
    if (side == 6u) { inverseStep = Word64(0x55555555u, 0x3fc55555u); }
    var total = 0u;
    for (var y = 0u; y < side; y += 1u) {
        for (var x = 0u; x < side; x += 1u) {
            let dx = doubleSubtract(doubleAdd(doubleInteger(point.x), doubleMultiply(doubleInteger(x), inverseStep)), dab.center.xy);
            let dy = doubleSubtract(doubleAdd(doubleInteger(point.y), doubleMultiply(doubleInteger(y), inverseStep)), dab.center.zw);
            let xr = doubleSubtract(doubleMultiply(dab.rotation.xy, dx), doubleMultiply(dab.rotation.zw, dy));
            let yr = doubleAdd(doubleMultiply(dab.rotation.zw, dx), doubleMultiply(dab.rotation.xy, dy));
            let distance = doubleSqrt(doubleAdd(doubleMultiply(xr, xr), doubleMultiply(yr, yr)));
            if (!wordLessEqual(distance, dab.radius.xy)) { total += 255u; }
            else if (!wordLessEqual(distance, dab.radius.zw)) {
                total += min(255u, doubleTruncate(doubleMultiply(doubleSubtract(distance, dab.radius.zw), dab.coefficient.xy)));
            }
        }
    }
    return 255u - total / (side * side);
}
fn circleAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    let dx = floatSubtract(bitcast<u32>(f32(point.x)), floatFromDouble(dab.center.xy));
    let dy = floatSubtract(bitcast<u32>(f32(point.y)), floatFromDouble(dab.center.zw));
    let cosine = floatFromDouble(dab.rotation.xy);
    let sine = floatFromDouble(dab.rotation.zw);
    let xr = floatSubtract(floatMultiply(dx, cosine), floatMultiply(sine, dy));
    let yr = floatAdd(floatMultiply(dx, sine), floatMultiply(cosine, dy));
    let distance = floatSqrt(floatAdd(floatMultiply(xr, xr), floatMultiply(yr, yr)));
    let radius = floatFromDouble(dab.radius.xy);
    let start = floatFromDouble(dab.radius.zw);
    if (distance > radius) { return 0u; }
    if (distance <= start) { return 255u; }
    let inverse = floatDivide(floatMultiply(floatSubtract(distance, start), floatFromDouble(dab.coefficient.xy)), 0x437f0000u);
    return projectionByte(floatMultiply(0x437f0000u, floatSubtract(0x3f800000u, inverse)));
}
