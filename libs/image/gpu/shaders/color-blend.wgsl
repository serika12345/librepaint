// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Coupled RGB operations retain each native binary32/binary64 rounding step.
// The source byte normalization and final half-up byte conversion are separate.
const LAMBERT_SCALE: Word64 = Word64(0x74b9bcbeu, 0x40128ba4u);
const LAMBERT_CURVE: Word64 = Word64(0xa1cac083u, 0x3f93b645u);
const LAMBERT_GAMMA_CURVE: Word64 = Word64(0x9999999au, 0x3fd99999u);
const COLOR_ONE: u32 = 0x3f800000u;
const COLOR_EPSILON: u32 = 0x34000000u;
fn colorIsCoupled(mode: u32) -> bool { return mode >= 88u && mode <= 126u; }
fn colorLess(a: u32, b: u32) -> bool {
    let aa = a & 0x7fffffffu;
    let bb = b & 0x7fffffffu;
    if (aa == 0u && bb == 0u) { return false; }
    let negative = (a & 0x80000000u) != 0u;
    if (((a ^ b) & 0x80000000u) != 0u) { return negative; }
    return select(aa < bb, aa > bb, negative);
}
fn colorMin(a: u32, b: u32) -> u32 { return select(b, a, colorLess(a, b)); }
fn colorMax(a: u32, b: u32) -> u32 { return select(b, a, colorLess(b, a)); }
fn colorMinimum(v: vec3<u32>) -> u32 { return colorMin(colorMin(v.x, v.y), v.z); }
fn colorMaximum(v: vec3<u32>) -> u32 { return colorMax(colorMax(v.x, v.y), v.z); }
fn colorDivide(a: u32, b: u32) -> u32 {
    if ((a & 0x7fffffffu) == 0u) { return 0u; }
    return projectionDivide(a & 0x7fffffffu, b & 0x7fffffffu) | ((a ^ b) & 0x80000000u);
}
fn colorLightness(model: u32, v: vec3<u32>) -> u32 {
    if (model == 0u) {
        return floatAdd(floatAdd(floatMultiply(bitcast<u32>(0.299f), v.x), floatMultiply(bitcast<u32>(0.587f), v.y)),
                        floatMultiply(bitcast<u32>(0.114f), v.z));
    }
    if (model == 1u) { return floatMultiply(floatAdd(floatAdd(v.x, v.y), v.z), 0x3eaaaaabu); }
    if (model == 2u) { return floatMultiply(floatAdd(colorMaximum(v), colorMinimum(v)), 0x3f000000u); }
    return colorMaximum(v);
}
fn colorSaturation(model: u32, v: vec3<u32>) -> u32 {
    let maximum = colorMaximum(v);
    let minimum = colorMinimum(v);
    let chroma = floatSubtract(maximum, minimum);
    if (model == 0u) { return chroma; }
    if (model == 1u) {
        if (!colorLess(COLOR_EPSILON, chroma)) { return 0u; }
        return floatSubtract(COLOR_ONE, colorDivide(minimum, colorLightness(model, v)));
    }
    if (model == 2u) {
        let light = floatMultiply(floatAdd(maximum, minimum), 0x3f000000u);
        let absolute = floatSubtract(floatMultiply(0x40000000u, light), COLOR_ONE) & 0x7fffffffu;
        let divisor = floatSubtract(COLOR_ONE, absolute);
        if (!colorLess(COLOR_EPSILON, divisor)) { return 0u; }
        return colorDivide(chroma, divisor);
    }
    if (!colorLess(COLOR_EPSILON, maximum)) { return 0u; }
    return colorDivide(chroma, maximum);
}
fn colorToneMapping(model: u32, input: vec3<u32>) -> vec3<u32> {
    var rgb = input;
    let light = colorLightness(model, rgb);
    let minimum = colorMinimum(rgb);
    let maximum = colorMaximum(rgb);
    if (colorLess(minimum, 0u)) {
        let stretch = floatSubtract(light, minimum);
        if (!colorLess(bitcast<u32>(0.00001f), light) || colorLess(stretch, COLOR_EPSILON)) { rgb = vec3<u32>(0u); }
        else {
            let inverse = colorDivide(COLOR_ONE, stretch);
            for (var i = 0u; i < 3u; i += 1u) {
                rgb[i] = floatAdd(light, floatMultiply(floatMultiply(floatSubtract(rgb[i], light), light), inverse));
            }
        }
    }
    if (colorLess(COLOR_ONE, maximum)) {
        let stretch = floatSubtract(maximum, light);
        if (colorLess(COLOR_ONE, light) || colorLess(stretch, COLOR_EPSILON)) {
            if (model != 3u) { rgb = vec3<u32>(COLOR_ONE); }
            else { for (var i = 0u; i < 3u; i += 1u) { rgb[i] = colorMin(rgb[i], COLOR_ONE); } }
        } else {
            let complement = floatSubtract(COLOR_ONE, light);
            let inverse = colorDivide(COLOR_ONE, stretch);
            for (var i = 0u; i < 3u; i += 1u) {
                rgb[i] = floatAdd(light, floatMultiply(floatMultiply(floatSubtract(rgb[i], light), complement), inverse));
            }
        }
    }
    return rgb;
}
fn colorAddLightness(model: u32, input: vec3<u32>, light: u32) -> vec3<u32> {
    return colorToneMapping(model, vec3<u32>(floatAdd(input.x, light), floatAdd(input.y, light), floatAdd(input.z, light)));
}
fn colorSetLightness(model: u32, input: vec3<u32>, light: u32) -> vec3<u32> {
    return colorAddLightness(model, input, floatSubtract(light, colorLightness(model, input)));
}
fn colorSetSaturation(input: vec3<u32>, saturation: u32) -> vec3<u32> {
    var rgb = input;
    var low = 0u;
    var mid = 1u;
    var high = 2u;
    if (colorLess(rgb[mid], rgb[low])) { let temporary = low; low = mid; mid = temporary; }
    if (colorLess(rgb[high], rgb[mid])) { let temporary = mid; mid = high; high = temporary; }
    if (colorLess(rgb[mid], rgb[low])) { let temporary = low; low = mid; mid = temporary; }
    let chroma = floatSubtract(rgb[high], rgb[low]);
    if (!colorLess(COLOR_EPSILON, chroma)) { return vec3<u32>(0u); }
    rgb[mid] = colorDivide(floatMultiply(floatSubtract(rgb[mid], rgb[low]), saturation), chroma);
    rgb[high] = saturation;
    rgb[low] = 0u;
    return rgb;
}
fn colorRecombineMode(mode: u32, source: vec3<u32>, input: vec3<u32>) -> vec3<u32> {
    var destination = input;
    if (mode == 121u || mode == 122u) {
        let old = colorLightness(0u, destination);
        let replacement = colorLightness(0u, source);
        let replace = select(colorLess(replacement, old), colorLess(old, replacement), mode == 122u);
        return select(destination, source, replace);
    }
    if (mode == 123u) {
        return vec3<u32>(floatAdd(source.x, floatSubtract(destination.x, 0x3f000000u)),
                         floatAdd(source.y, floatSubtract(destination.y, 0x3f000000u)),
                         floatAdd(source.z, floatSubtract(destination.z, COLOR_ONE)));
    }
    if (mode == 124u) {
        if (!colorLess(bitcast<u32>(0.00001f), source.z)) { return destination; }
        let t = vec3<u32>(floatSubtract(floatMultiply(0x40000000u, source.x), COLOR_ONE),
                          floatSubtract(floatMultiply(0x40000000u, source.y), COLOR_ONE),
                          floatMultiply(0x40000000u, source.z));
        let u = vec3<u32>(floatAdd(floatMultiply(0xc0000000u, destination.x), COLOR_ONE),
                          floatAdd(floatMultiply(0xc0000000u, destination.y), COLOR_ONE),
                          floatSubtract(floatMultiply(0x40000000u, destination.z), COLOR_ONE));
        var factor = colorDivide(floatAdd(floatAdd(floatMultiply(t.x, u.x), floatMultiply(t.y, u.y)), floatMultiply(t.z, u.z)), t.z);
        var r = vec3<u32>(floatSubtract(floatMultiply(t.x, factor), u.x),
                          floatSubtract(floatMultiply(t.y, factor), u.y),
                          floatSubtract(floatMultiply(t.z, factor), u.z));
        let length = floatAdd(floatAdd(floatMultiply(r.x, r.x), floatMultiply(r.y, r.y)), floatMultiply(r.z, r.z));
        if ((length & 0x7fffffffu) == 0u) { return destination; }
        factor = colorDivide(COLOR_ONE, floatSqrt(length));
        for (var i = 0u; i < 3u; i += 1u) {
            let normalized = floatMultiply(r[i], factor);
            let scaled = doubleMultiply(doubleFromFloat(normalized), Word64(0u, 0x3fe00000u));
            destination[i] = floatFromDouble(doubleAdd(scaled, Word64(0u, 0x3fe00000u)));
        }
        return destination;
    }
    if (mode == 125u || mode == 126u) {
        let gamma = mode == 126u;
        let scale = select(LAMBERT_SCALE, Word64(0u, 0x40000000u), gamma);
        let coefficient = select(LAMBERT_CURVE, LAMBERT_GAMMA_CURVE, gamma);
        for (var i = 0u; i < 3u; i += 1u) {
            let product = floatMultiply(source[i], destination[i]);
            let value = floatFromDouble(doubleMultiply(doubleFromFloat(product), scale));
            if (colorLess(COLOR_ONE, value)) {
                let shifted = doubleSubtract(doubleFromFloat(value), Word64(0u, 0x3ff00000u));
                let squared = doubleMultiply(shifted, shifted);
                let weighted = doubleMultiply(squared, coefficient);
                destination[i] = floatFromDouble(doubleAdd(Word64(0u, 0x3ff00000u), weighted));
            } else { destination[i] = value; }
        }
        return colorToneMapping(1u, destination);
    }
    var model: u32;
    var operation: u32;
    if (mode <= 96u) {
        model = 0u;
        operation = mode - 88u;
        if (operation == 2u) { operation = 8u; }
        else if (operation > 2u) { operation -= 1u; }
    } else {
        let offset = mode - 97u;
        model = 1u + offset / 8u;
        operation = offset % 8u;
    }
    let light = colorLightness(model, destination);
    switch operation {
        case 0u: { return colorSetLightness(model, source, light); }
        case 1u: { return colorSetLightness(model, colorSetSaturation(source, colorSaturation(model, destination)), light); }
        case 2u: { return colorSetLightness(model, colorSetSaturation(destination, colorSaturation(model, source)), light); }
        case 3u: {
            let old = colorSaturation(model, destination);
            let weight = colorSaturation(model, source);
            let saturation = floatAdd(floatMultiply(floatSubtract(COLOR_ONE, old), weight), old);
            return colorSetLightness(model, colorSetSaturation(destination, saturation), light);
        }
        case 4u: {
            let saturation = floatMultiply(colorSaturation(model, destination), colorSaturation(model, source));
            return colorSetLightness(model, colorSetSaturation(destination, saturation), light);
        }
        case 5u: { return colorSetLightness(model, destination, colorLightness(model, source)); }
        case 6u: { return colorAddLightness(model, destination, colorLightness(model, source)); }
        case 7u: { return colorAddLightness(model, destination, floatSubtract(colorLightness(model, source), COLOR_ONE)); }
        default: {
            let complement = floatSubtract(COLOR_ONE, light);
            return vec3<u32>(floatAdd(light, floatMultiply(source.x, complement)),
                             floatAdd(light, floatMultiply(source.y, complement)),
                             floatAdd(light, floatMultiply(source.z, complement)));
        }
    }
}
// Each production program supplies only its one or two current operations.
// Constant modes remove unrelated arithmetic before the driver compiles it.
fn colorRecombine(mode: u32, source: vec3<u32>, destination: vec3<u32>) -> vec3<u32> {
    if (COLOR_BLEND_MODE_A != 0u && mode == COLOR_BLEND_MODE_A) {
        return colorRecombineMode(COLOR_BLEND_MODE_A, source, destination);
    }
    if (COLOR_BLEND_MODE_B != 0u && mode == COLOR_BLEND_MODE_B) {
        return colorRecombineMode(COLOR_BLEND_MODE_B, source, destination);
    }
    return destination;
}
fn colorComponentByte(value: u32) -> u32 {
    if (colorLess(value, 0u)) { return 0u; }
    if (!colorLess(value, COLOR_ONE)) { return 255u; }
    let scaled = floatMultiply(value, 0x437f0000u);
    let rounded = floatAdd(scaled, 0x3f000000u);
    return projectionMantissa(rounded) >> u32(-projectionPower(rounded));
}
fn coupledRgba8Pixel(base: vec4<u32>, source: vec4<u32>, alpha: u32, mask: u32, mode: u32) -> u32 {
    var destination = base;
    if (mask != 15u && destination.w == 0u) { destination = vec4<u32>(0u); }
    let oldAlpha = destination.w;
    let locked = (mask & 8u) == 0u;
    let nextAlpha = select(alpha + oldAlpha - multiplyByte(alpha, oldAlpha), oldAlpha, locked);
    var mixed = vec3<u32>(0u);
    if (alpha != 0u && oldAlpha != 0u && (mask & 7u) != 0u) {
        var s: vec3<u32>;
        var d: vec3<u32>;
        for (var i = 0u; i < 3u; i += 1u) {
            s[i] = colorDivide(bitcast<u32>(f32(source[i])), 0x437f0000u);
            d[i] = colorDivide(bitcast<u32>(f32(destination[i])), 0x437f0000u);
        }
        mixed = colorRecombine(mode, s, d);
    }
    for (var i = 0u; i < 3u; i += 1u) {
        if ((mask & (1u << i)) == 0u) { continue; }
        let product = colorComponentByte(mixed[i]);
        if (locked) {
            if (oldAlpha != 0u) { destination[i] = blendByte(destination[i], product, alpha); }
        } else if (nextAlpha != 0u) {
            let weighted = (multiplyThreeBytes(255u - alpha, oldAlpha, destination[i]) +
                multiplyThreeBytes(255u - oldAlpha, alpha, source[i]) + multiplyThreeBytes(oldAlpha, alpha, product)) & 255u;
            destination[i] = ((weighted * 255u + nextAlpha / 2u) / nextAlpha) & 255u;
        }
    }
    destination.w = nextAlpha;
    return packPixel(destination);
}
