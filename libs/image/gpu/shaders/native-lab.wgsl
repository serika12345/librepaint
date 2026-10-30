// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Integer tetrahedral interpolation of the native converter's original Lab table.
// Two 16-bit words per conversion word, low half first.
fn clutWord(table: u32, index: u32) -> u32 {
    let word = conversion[table + (index >> 1u)];
    return (word >> (16u * (index & 1u))) & 65535u;
}
// LCMS TetrahedralInterp16: fixed-domain nodes, signed modular accumulation and
// the same six >= branches that select the cumulative corner order.
fn nativeLabClut(input: vec3<u32>) -> vec3<u32> {
    let grid = vec3<u32>(conversion[2u], conversion[3u], conversion[4u]);
    let table = 6u;
    let domain = grid - vec3(1u);
    let product = input * domain;
    let fixed = product + (product + vec3(32767u)) / vec3(65535u);
    let cell = fixed >> vec3(16u);
    let rest = fixed & vec3(65535u);
    let strides = vec3<u32>(grid.y * grid.z * 3u, grid.z * 3u, 3u);
    let base = cell.x * strides.x + cell.y * strides.y + cell.z * strides.z;
    // The step along an axis is zero at the top node, as in LCMS.
    let steps = vec3<u32>(
        select(strides.x, 0u, input.x == 65535u),
        select(strides.y, 0u, input.y == 65535u),
        select(strides.z, 0u, input.z == 65535u));
    var order: array<u32, 3>;
    order[0u] = 0u; order[1u] = 1u; order[2u] = 2u;
    if (rest.x >= rest.y) {
        if (rest.y >= rest.z) { }
        else if (rest.z >= rest.x) { order[0u] = 2u; order[1u] = 0u; order[2u] = 1u; }
        else { order[0u] = 0u; order[1u] = 2u; order[2u] = 1u; }
    } else {
        if (rest.x >= rest.z) { order[0u] = 1u; order[1u] = 0u; order[2u] = 2u; }
        else if (rest.y >= rest.z) { order[0u] = 1u; order[1u] = 2u; order[2u] = 0u; }
        else { order[0u] = 2u; order[1u] = 1u; order[2u] = 0u; }
    }
    var positions: array<u32, 4>;
    positions[0u] = base; positions[1u] = 0u; positions[2u] = 0u; positions[3u] = 0u;
    var remainders: array<u32, 3>;
    remainders[0u] = 0u; remainders[1u] = 0u; remainders[2u] = 0u;
    for (var step = 0u; step < 3u; step++) {
        let axis = order[step];
        remainders[step] = rest[axis];
        positions[step + 1u] = positions[step] + steps[axis];
    }
    var output = vec3<u32>();
    for (var channel = 0u; channel < 3u; channel++) {
        let c0 = i32(clutWord(table, positions[0u] + channel));
        let c1 = i32(clutWord(table, positions[1u] + channel)) - c0;
        let c2 = i32(clutWord(table, positions[2u] + channel)) - i32(clutWord(table, positions[1u] + channel));
        let c3 = i32(clutWord(table, positions[3u] + channel)) - i32(clutWord(table, positions[2u] + channel));
        var sum = c1 * i32(remainders[0u]) + c2 * i32(remainders[1u]) + c3 * i32(remainders[2u]) + 0x8001;
        sum = sum + (sum >> 16u);
        output[channel] = (u32(c0 + (sum >> 16u))) & 65535u;
    }
    return output;
}
const LAB_LIMIT = Word64(0xd28f7cd6u, 0x3f822354u);
const LAB_SLOPE = Word64(0x097b425fu, 0x401f25edu);
const LAB_INTERCEPT = Word64(0x611a7b96u, 0x3fc1a7b9u);
const LAB_THIRD = Word64(0x55555555u, 0x3fd55555u);
const LAB_XYZ_SCALE = Word64(0x00000000u, 0x3fffffe0u);
const LAB_D50_X = Word64(0xf559b3d0u, 0x3feedab9u);
const LAB_D50_Z = Word64(0xaf4f0d84u, 0x3fea6594u);
const LAB_116 = Word64(0x00000000u, 0x405d0000u);
const LAB_16 = Word64(0x00000000u, 0x40300000u);
const LAB_100 = Word64(0x00000000u, 0x40590000u);
const LAB_500 = Word64(0x00000000u, 0x407f4000u);
const LAB_200 = Word64(0x00000000u, 0x40690000u);
const LAB_128 = Word64(0x00000000u, 0x40600000u);
const LAB_255 = Word64(0x00000000u, 0x406fe000u);
const LAB_65535 = Word64(0x00000000u, 0x40efffe0u);
const LAB_L_SCALE = Word64(0xcccccccdu, 0x40847accu);
const LAB_AB_SCALE = Word64(0x00000000u, 0x40701000u);
const LAB_ALPHA_SCALE = Word64(0x00190019u, 0x3f590019u);
fn labDouble(index: u32) -> Word64 { return Word64(conversion[6u + index * 2u], conversion[7u + index * 2u]); }
fn labFunction(value: Word64) -> Word64 {
    // LCMS cmspcs.c f(): the native double constants and branch order.
    if ((value.y & 0x80000000u) != 0u || wordLessEqual(value, LAB_LIMIT)) {
        return doubleAdd(doubleMultiply(LAB_SLOPE, value), LAB_INTERCEPT);
    }
    return wordColorPower(value, LAB_THIRD);
}
fn nativeLabLinearComponent(input: vec3<u32>, row: u32) -> Word64 {
    var sum = Word64(0u);
    for (var column = 0u; column < 3u; column += 1u) {
        // cmslut.c From16ToFloat rounds the division in binary32.
        let normalized = doubleFromFloat(floatDivide(bitcast<u32>(f32(input[column])), 0x477fff00u));
        sum = doubleAdd(sum, doubleMultiply(normalized, labDouble(row * 3u + column)));
    }
    sum = doubleAdd(sum, labDouble(9u + row));
    // The matrix writes a float before XYZ2Lab expands it in double.
    var xyz = doubleMultiply(doubleFromFloat(floatFromDouble(sum)), LAB_XYZ_SCALE);
    if (row == 0u) { xyz = doubleDivide(xyz, LAB_D50_X); }
    if (row == 2u) { xyz = doubleDivide(xyz, LAB_D50_Z); }
    return labFunction(xyz);
}
fn nativeLabEncode(fx: Word64, fy: Word64, fz: Word64) -> vec3<u32> {
    var lab: array<Word64, 3>;
    lab[0] = doubleDivide(doubleSubtract(doubleMultiply(LAB_116, fy), LAB_16), LAB_100);
    lab[1] = doubleDivide(doubleAdd(doubleMultiply(LAB_500, doubleSubtract(fx, fy)), LAB_128), LAB_255);
    lab[2] = doubleDivide(doubleAdd(doubleMultiply(LAB_200, doubleSubtract(fy, fz)), LAB_128), LAB_255);
    var result = vec3<u32>();
    for (var c = 0u; c < 3u; c += 1u) {
        let value = doubleFromFloat(floatFromDouble(lab[c]));
        if ((value.y & 0x80000000u) != 0u) { result[c] = 0u; }
        else if (!wordLessEqual(value, Word64(0u, 0x3ff00000u))) { result[c] = 65535u; }
        else { result[c] = doubleTruncate(doubleAdd(doubleMultiply(value, LAB_65535), Word64(0u, 0x3fe00000u))); }
    }
    return result;
}
fn nativeLabDifference(a: vec3<u32>, b: vec3<u32>, firstAlpha: u32, secondAlpha: u32, words: bool) -> u32 {
    let multiplier = select(257u, 1u, words);
    // Keep each native LabEncoded2Float division/subtraction before DeltaE.
    let dl = doubleMagnitude(doubleSubtract(doubleDivide(doubleInteger(a.x), LAB_L_SCALE), doubleDivide(doubleInteger(b.x), LAB_L_SCALE)));
    let da = doubleMagnitude(doubleSubtract(
        doubleSubtract(doubleDivide(doubleInteger(a.y), LAB_AB_SCALE), LAB_128),
        doubleSubtract(doubleDivide(doubleInteger(b.y), LAB_AB_SCALE), LAB_128)));
    let db = doubleMagnitude(doubleSubtract(
        doubleSubtract(doubleDivide(doubleInteger(a.z), LAB_AB_SCALE), LAB_128),
        doubleSubtract(doubleDivide(doubleInteger(b.z), LAB_AB_SCALE), LAB_128)));
    let alpha = doubleMultiply(doubleInteger(u32(abs(i32(firstAlpha) - i32(secondAlpha))) * multiplier), LAB_ALPHA_SCALE);
    let sum = doubleAdd(doubleAdd(doubleAdd(doubleMultiply(dl, dl), doubleMultiply(da, da)), doubleMultiply(db, db)), doubleMultiply(alpha, alpha));
    let difference = doubleSqrt(sum);
    return min(255u, doubleTruncate(difference));
}
