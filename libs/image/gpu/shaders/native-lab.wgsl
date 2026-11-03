// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Tetrahedral interpolation of the profile Lab table in native f32.
// Two 16-bit words per conversion word, low half first.
fn clutWord(table: u32, index: u32) -> u32 {
    let word = conversion[table + (index >> 1u)];
    return (word >> (16u * (index & 1u))) & 65535u;
}
// Sort the fractional coordinates to select the tetrahedron corners.
fn nativeLabClut(input: vec3<u32>) -> vec3<u32> {
    let grid = vec3<u32>(conversion[2u], conversion[3u], conversion[4u]);
    let table = 6u;
    let domain = grid - vec3(1u);
    let coordinate = vec3<f32>(input) * vec3<f32>(domain) / 65535.0;
    let cell = min(vec3<u32>(floor(coordinate)), domain);
    let rest = coordinate - vec3<f32>(cell);
    let strides = vec3<u32>(grid.y * grid.z * 3u, grid.z * 3u, 3u);
    let base = cell.x * strides.x + cell.y * strides.y + cell.z * strides.z;
    // At the top node no neighbour exists along that axis.
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
    var remainders: array<f32, 3>;
    remainders[0u] = 0.0; remainders[1u] = 0.0; remainders[2u] = 0.0;
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
        let interpolated = f32(c0) + f32(c1) * remainders[0u] +
            f32(c2) * remainders[1u] + f32(c3) * remainders[2u];
        output[channel] = u32(clamp(floor(interpolated + 0.5), 0.0, 65535.0));
    }
    return output;
}
const LAB_LIMIT = 0.008856452070176601;
const LAB_SLOPE = 7.787036895751953;
const LAB_INTERCEPT = 0.13793103396892548;
const LAB_THIRD = 0.3333333432674408;
const LAB_XYZ_SCALE = 1.999969482421875;
const LAB_D50_X = 0.9642000198364258;
const LAB_D50_Z = 0.8248999714851379;
const LAB_116 = 116.0;
const LAB_16 = 16.0;
const LAB_100 = 100.0;
const LAB_500 = 500.0;
const LAB_200 = 200.0;
const LAB_128 = 128.0;
const LAB_255 = 255.0;
const LAB_65535 = 65535.0;
const LAB_L_SCALE = 655.3499755859375;
const LAB_AB_SCALE = 257.0;
const LAB_ALPHA_SCALE = 0.0015259021893143654;
fn labCoefficient(index: u32) -> f32 { return bitcast<f32>(conversion[6u + index]); }
fn labFunction(value: f32) -> f32 {
    // CIE Lab uses a linear segment near zero and a cube root above it.
    if ((value < 0.0) || (value <= LAB_LIMIT)) {
        return ((LAB_SLOPE * value) + LAB_INTERCEPT);
    }
    return pow(value, LAB_THIRD);
}
fn nativeLabLinearComponent(input: vec3<u32>, row: u32) -> f32 {
    var sum = 0.0;
    for (var column = 0u; column < 3u; column += 1u) {
        let normalized = safeRatio(f32(input[column]), 65535.0);
        sum = (sum + (normalized * labCoefficient(row * 3u + column)));
    }
    sum = (sum + labCoefficient(9u + row));
    var xyz = (sum * LAB_XYZ_SCALE);
    if (row == 0u) { xyz = (xyz / LAB_D50_X); }
    if (row == 2u) { xyz = (xyz / LAB_D50_Z); }
    return labFunction(xyz);
}
fn nativeLabEncode(fx: f32, fy: f32, fz: f32) -> vec3<u32> {
    var lab: array<f32, 3>;
    lab[0] = (((LAB_116 * fy) - LAB_16) / LAB_100);
    lab[1] = (((LAB_500 * (fx - fy)) + LAB_128) / LAB_255);
    lab[2] = (((LAB_200 * (fy - fz)) + LAB_128) / LAB_255);
    var result = vec3<u32>();
    for (var c = 0u; c < 3u; c += 1u) {
        let value = lab[c];
        if ((value < 0.0)) { result[c] = 0u; }
        else if (!(value <= 1.0)) { result[c] = 65535u; }
        else { result[c] = u32(max(((value * LAB_65535) + 0.5), 0.0)); }
    }
    return result;
}
fn nativeLabDifference(a: vec3<u32>, b: vec3<u32>, firstAlpha: u32, secondAlpha: u32, words: bool) -> u32 {
    let multiplier = select(257u, 1u, words);
    // Keep each native LabEncoded2Float division/subtraction before DeltaE.
    let dl = abs(((f32(a.x) / LAB_L_SCALE) - (f32(b.x) / LAB_L_SCALE)));
    let da = abs((((f32(a.y) / LAB_AB_SCALE) - LAB_128) - ((f32(b.y) / LAB_AB_SCALE) - LAB_128)));
    let db = abs((((f32(a.z) / LAB_AB_SCALE) - LAB_128) - ((f32(b.z) / LAB_AB_SCALE) - LAB_128)));
    let alpha = (f32(u32(abs(i32(firstAlpha) - i32(secondAlpha))) * multiplier) * LAB_ALPHA_SCALE);
    let sum = ((((dl * dl) + (da * da)) + (db * db)) + (alpha * alpha));
    let difference = sqrt(max(sum, 0.0));
    return min(255u, u32(max(difference, 0.0)));
}
