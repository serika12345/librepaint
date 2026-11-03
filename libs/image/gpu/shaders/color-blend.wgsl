// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Coupled RGB composition uses native f32 channel arithmetic.
const LAMBERT_SCALE: f32 = 4.636369705200195;
const LAMBERT_CURVE: f32 = 0.01924999989569187;
const LAMBERT_GAMMA_CURVE: f32 = 0.4000000059604645;
const COLOR_ONE: f32 = 1.0;
const COLOR_EPSILON: f32 = 1.1920929e-07;
fn colorIsCoupled(mode: u32) -> bool { return mode >= 88u && mode <= 126u; }



fn colorMinimum(v: vec3<f32>) -> f32 { return min(min(v.x, v.y), v.z); }
fn colorMaximum(v: vec3<f32>) -> f32 { return max(max(v.x, v.y), v.z); }

fn colorLightness(model: u32, v: vec3<f32>) -> f32 {
    if (model == 0u) {
        return (((0.299f * v.x) + (0.587f * v.y)) + (0.114f * v.z));
    }
    if (model == 1u) { return (((v.x + v.y) + v.z) * 0.333333343); }
    if (model == 2u) { return ((colorMaximum(v) + colorMinimum(v)) * 0.5); }
    return colorMaximum(v);
}
fn colorSaturation(model: u32, v: vec3<f32>) -> f32 {
    let maximum = colorMaximum(v);
    let minimum = colorMinimum(v);
    let chroma = (maximum - minimum);
    if (model == 0u) { return chroma; }
    if (model == 1u) {
        if (!(COLOR_EPSILON < chroma)) { return 0.0; }
        return (COLOR_ONE - safeRatio(minimum, colorLightness(model, v)));
    }
    if (model == 2u) {
        let light = ((maximum + minimum) * 0.5);
        let absolute = abs(((2.0 * light) - COLOR_ONE));
        let divisor = (COLOR_ONE - absolute);
        if (!(COLOR_EPSILON < divisor)) { return 0.0; }
        return safeRatio(chroma, divisor);
    }
    if (!(COLOR_EPSILON < maximum)) { return 0.0; }
    return safeRatio(chroma, maximum);
}
fn colorToneMapping(model: u32, input: vec3<f32>) -> vec3<f32> {
    var rgb = input;
    let light = colorLightness(model, rgb);
    let minimum = colorMinimum(rgb);
    let maximum = colorMaximum(rgb);
    if ((minimum < 0.0)) {
        let stretch = (light - minimum);
        if (!(0.00001f < light) || (stretch < COLOR_EPSILON)) { rgb = vec3<f32>(0.0); }
        else {
            let inverse = safeRatio(COLOR_ONE, stretch);
            for (var i = 0u; i < 3u; i += 1u) {
                rgb[i] = (light + (((rgb[i] - light) * light) * inverse));
            }
        }
    }
    if ((COLOR_ONE < maximum)) {
        let stretch = (maximum - light);
        if ((COLOR_ONE < light) || (stretch < COLOR_EPSILON)) {
            if (model != 3u) { rgb = vec3<f32>(COLOR_ONE); }
            else { for (var i = 0u; i < 3u; i += 1u) { rgb[i] = min(rgb[i], COLOR_ONE); } }
        } else {
            let complement = (COLOR_ONE - light);
            let inverse = safeRatio(COLOR_ONE, stretch);
            for (var i = 0u; i < 3u; i += 1u) {
                rgb[i] = (light + (((rgb[i] - light) * complement) * inverse));
            }
        }
    }
    return rgb;
}
fn colorAddLightness(model: u32, input: vec3<f32>, light: f32) -> vec3<f32> {
    return colorToneMapping(model, vec3<f32>((input.x + light), (input.y + light), (input.z + light)));
}
fn colorSetLightness(model: u32, input: vec3<f32>, light: f32) -> vec3<f32> {
    return colorAddLightness(model, input, (light - colorLightness(model, input)));
}
fn colorSetSaturation(input: vec3<f32>, saturation: f32) -> vec3<f32> {
    var rgb = input;
    var low = 0u;
    var mid = 1u;
    var high = 2u;
    if ((rgb[mid] < rgb[low])) { let temporary = low; low = mid; mid = temporary; }
    if ((rgb[high] < rgb[mid])) { let temporary = mid; mid = high; high = temporary; }
    if ((rgb[mid] < rgb[low])) { let temporary = low; low = mid; mid = temporary; }
    let chroma = (rgb[high] - rgb[low]);
    if (!(COLOR_EPSILON < chroma)) { return vec3<f32>(0.0); }
    rgb[mid] = safeRatio(((rgb[mid] - rgb[low]) * saturation), chroma);
    rgb[high] = saturation;
    rgb[low] = 0.0;
    return rgb;
}
fn colorRecombineMode(mode: u32, source: vec3<f32>, input: vec3<f32>) -> vec3<f32> {
    var destination = input;
    if (mode == 121u || mode == 122u) {
        let old = colorLightness(0u, destination);
        let replacement = colorLightness(0u, source);
        let replace = select((replacement < old), (old < replacement), mode == 122u);
        return select(destination, source, replace);
    }
    if (mode == 123u) {
        return vec3<f32>((source.x + (destination.x - 0.5)),
                         (source.y + (destination.y - 0.5)),
                         (source.z + (destination.z - COLOR_ONE)));
    }
    if (mode == 124u) {
        if (!(0.00001f < source.z)) { return destination; }
        let t = vec3<f32>(((2.0 * source.x) - COLOR_ONE),
                          ((2.0 * source.y) - COLOR_ONE),
                          (2.0 * source.z));
        let u = vec3<f32>(((-2.0 * destination.x) + COLOR_ONE),
                          ((-2.0 * destination.y) + COLOR_ONE),
                          ((2.0 * destination.z) - COLOR_ONE));
        var factor = safeRatio((((t.x * u.x) + (t.y * u.y)) + (t.z * u.z)), t.z);
        var r = vec3<f32>(((t.x * factor) - u.x),
                          ((t.y * factor) - u.y),
                          ((t.z * factor) - u.z));
        let length = (((r.x * r.x) + (r.y * r.y)) + (r.z * r.z));
        if (length == 0.0) { return destination; }
        factor = safeRatio(COLOR_ONE, sqrt(max(length, 0.0)));
        for (var i = 0u; i < 3u; i += 1u) {
            let normalized = (r[i] * factor);
            let scaled = (normalized * 0.5);
            destination[i] = (scaled + 0.5);
        }
        return destination;
    }
    if (mode == 125u || mode == 126u) {
        let gamma = mode == 126u;
        let scale = select(LAMBERT_SCALE, 2.0, gamma);
        let coefficient = select(LAMBERT_CURVE, LAMBERT_GAMMA_CURVE, gamma);
        for (var i = 0u; i < 3u; i += 1u) {
            let product = (source[i] * destination[i]);
            let value = (product * scale);
            if ((COLOR_ONE < value)) {
                let shifted = (value - 1.0);
                let squared = (shifted * shifted);
                let weighted = (squared * coefficient);
                destination[i] = (1.0 + weighted);
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
            let saturation = (((COLOR_ONE - old) * weight) + old);
            return colorSetLightness(model, colorSetSaturation(destination, saturation), light);
        }
        case 4u: {
            let saturation = (colorSaturation(model, destination) * colorSaturation(model, source));
            return colorSetLightness(model, colorSetSaturation(destination, saturation), light);
        }
        case 5u: { return colorSetLightness(model, destination, colorLightness(model, source)); }
        case 6u: { return colorAddLightness(model, destination, colorLightness(model, source)); }
        case 7u: { return colorAddLightness(model, destination, (colorLightness(model, source) - COLOR_ONE)); }
        default: {
            let complement = (COLOR_ONE - light);
            return vec3<f32>((light + (source.x * complement)),
                             (light + (source.y * complement)),
                             (light + (source.z * complement)));
        }
    }
}
fn colorRecombine(mode: u32, source: vec3<f32>, destination: vec3<f32>) -> vec3<f32> {
    if (COLOR_BLEND_MODE_A != 0u && mode == COLOR_BLEND_MODE_A) {
        return colorRecombineMode(COLOR_BLEND_MODE_A, source, destination);
    }
    if (COLOR_BLEND_MODE_B != 0u && mode == COLOR_BLEND_MODE_B) {
        return colorRecombineMode(COLOR_BLEND_MODE_B, source, destination);
    }
    return destination;
}
fn colorComponentByte(value: f32) -> u32 {
    if ((value < 0.0)) { return 0u; }
    if (!(value < COLOR_ONE)) { return 255u; }
    let scaled = (value * 255.0);
    let rounded = (scaled + 0.5);
    return u32(rounded);
}
fn coupledRgba8Pixel(base: vec4<u32>, source: vec4<u32>, alpha: u32, mask: u32, mode: u32) -> u32 {
    if (COUPLED_PIXEL_FORMAT == 2u || (COLOR_BLEND_MODE_A == 0u && COLOR_BLEND_MODE_B == 0u)) { return packPixel(base); }
    var destination = base;
    if (mask != 15u && destination.w == 0u) { destination = vec4<u32>(0u); }
    let oldAlpha = destination.w;
    let locked = (mask & 8u) == 0u;
    let nextAlpha = select(alpha + oldAlpha - multiplyByte(alpha, oldAlpha), oldAlpha, locked);
    var mixed = vec3<f32>(0.0);
    if (alpha != 0u && oldAlpha != 0u && (mask & 7u) != 0u) {
        var s: vec3<f32>;
        var d: vec3<f32>;
        for (var i = 0u; i < 3u; i += 1u) {
            s[i] = safeRatio(f32(source[i]), 255.0);
            d[i] = safeRatio(f32(destination[i]), 255.0);
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
fn colorComponentWord(value: f32) -> u32 {
    if ((value < 0.0)) { return 0u; }
    if (!(value < COLOR_ONE)) { return 65535u; }
    let scaled = (value * 65535.0);
    let rounded = (scaled + 0.5);
    return u32(rounded);
}
fn coupledRgba16Pixel(base: vec4<u32>, source: vec4<u32>, alpha: u32, mask: u32, mode: u32) -> vec4<u32> {
    if (COUPLED_PIXEL_FORMAT == 1u || (COLOR_BLEND_MODE_A == 0u && COLOR_BLEND_MODE_B == 0u)) { return base; }
    var destination = base;
    if (mask != 15u && destination.w == 0u) { destination = vec4<u32>(0u); }
    let oldAlpha = destination.w;
    let locked = (mask & 8u) == 0u;
    let nextAlpha = select(alpha + oldAlpha - mul65535(alpha, oldAlpha), oldAlpha, locked);
    var mixed = vec3<f32>(0.0);
    if (alpha != 0u && oldAlpha != 0u && (mask & 7u) != 0u) {
        var s: vec3<f32>;
        var d: vec3<f32>;
        for (var i = 0u; i < 3u; i += 1u) {
            s[i] = safeRatio(f32(source[i]), 65535.0);
            d[i] = safeRatio(f32(destination[i]), 65535.0);
        }
        mixed = colorRecombine(mode, s, d);
    }
    for (var i = 0u; i < 3u; i += 1u) {
        if ((mask & (1u << i)) == 0u) { continue; }
        let product = colorComponentWord(mixed[i]);
        if (locked) {
            if (oldAlpha != 0u) { destination[i] = interpolateWord(destination[i], product, alpha); }
        } else if (nextAlpha != 0u) {
            let weighted = (multiplyThreeWords(65535u - alpha, oldAlpha, destination[i]) +
                multiplyThreeWords(65535u - oldAlpha, alpha, source[i]) + multiplyThreeWords(oldAlpha, alpha, product)) & 65535u;
            destination[i] = divideWord(weighted, nextAlpha) & 65535u;
        }
    }
    destination.w = nextAlpha;
    return destination;
}
