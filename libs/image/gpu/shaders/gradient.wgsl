// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Gradient colors are imported resources. All per-pixel geometry, seam
// interpolation and native-depth quantization belong to canonical GPU pixels.
fn gradientLess(a: Word64, b: Word64) -> bool {
    if (all(doubleMagnitude(a) == Word64(0u)) && all(doubleMagnitude(b) == Word64(0u))) { return false; }
    let negativeA = (a.y & 0x80000000u) != 0u;
    let negativeB = (b.y & 0x80000000u) != 0u;
    if (negativeA != negativeB) { return negativeA; }
    if (negativeA) { return !wordLessEqual(doubleMagnitude(a), doubleMagnitude(b)); }
    return !wordLessEqual(b, a);
}
fn gradientAtan2(y: Word64, x: Word64) -> Word64 {
    let pi = Word64(0x54442d18u, 0x400921fbu);
    let halfPi = Word64(0x54442d18u, 0x3ff921fbu);
    let tinyPi = Word64(0x33145c07u, 0x3ca1a626u);
    let negativeX = (x.y & 0x80000000u) != 0u;
    let negativeY = (y.y & 0x80000000u) != 0u;
    var angle = Word64(0u);
    if (all(doubleMagnitude(y) == Word64(0u))) { angle = select(Word64(0u), pi, negativeX); }
    else if (all(doubleMagnitude(x) == Word64(0u))) { angle = halfPi; }
    else {
        angle = positiveColorAtan(doubleMagnitude(doubleDivide(y, x)));
        if (negativeX) { angle = doubleSubtract(pi, doubleSubtract(angle, tinyPi)); }
    }
    if (negativeY) { return doubleNegate(angle); }
    return angle;
}
fn gradientAngle(px: Word64, py: Word64, dx: Word64, dy: Word64) -> Word64 {
    let pi = Word64(0x54442d18u, 0x400921fbu);
    var angle = doubleSubtract(doubleAdd(gradientAtan2(py, px), pi), doubleAdd(gradientAtan2(dy, dx), pi));
    if (gradientLess(angle, Word64(0u))) { angle = doubleAdd(angle, Word64(0x54442d18u, 0x401921fbu)); }
    return angle;
}
fn gradientPosition(pixel: vec2<i32>) -> Word64 {
    let zero = Word64(0u); let epsilon = Word64(0u, 0x3cb00000u);
    let dx = doubleSubtract(parameters.patternMap1.xy, parameters.patternMap0.xy);
    let dy = doubleSubtract(parameters.patternMap1.zw, parameters.patternMap0.zw);
    let length = doubleSqrt(doubleAdd(doubleMultiply(dx, dx), doubleMultiply(dy, dy)));
    if (GRADIENT_SHAPE < 4u && gradientLess(length, epsilon)) { return zero; }
    let px = doubleSubtract(markerInteger(pixel.x), parameters.patternMap0.xy);
    let py = doubleSubtract(markerInteger(pixel.y), parameters.patternMap0.zw);
    if (GRADIENT_SHAPE >= 4u) {
        let angle = gradientAngle(px, py, dx, dy);
        let pi = Word64(0x54442d18u, 0x400921fbu);
        let one = Word64(0u, 0x3ff00000u);
        let turn = doubleDivide(angle, Word64(0x54442d18u, 0x401921fbu));
        if (GRADIENT_SHAPE == 4u) { return turn; }
        if (GRADIENT_SHAPE == 5u) {
            if (gradientLess(angle, pi)) { return doubleDivide(angle, pi); }
            return doubleSubtract(one, doubleDivide(doubleSubtract(angle, pi), pi));
        }
        var radius = Word64(0u);
        if (!gradientLess(length, epsilon)) {
            radius = doubleDivide(doubleSqrt(doubleAdd(doubleMultiply(px, px), doubleMultiply(py, py))), length);
        }
        if (GRADIENT_SHAPE == 7u) { return doubleAdd(radius, doubleSubtract(one, turn)); }
        return doubleAdd(radius, turn);
    }
    if (GRADIENT_SHAPE == 2u) {
        return doubleDivide(doubleSqrt(doubleAdd(doubleMultiply(px, px), doubleMultiply(py, py))), length);
    }
    let nx = doubleDivide(dx, length); let ny = doubleDivide(dy, length);
    if (GRADIENT_SHAPE == 3u) {
        if (!gradientLess(epsilon, length)) { return zero; }
        let first = doubleMagnitude(doubleAdd(doubleMultiply(doubleNegate(ny), px), doubleMultiply(nx, py)));
        let second = doubleMagnitude(doubleAdd(doubleMultiply(doubleNegate(ny), doubleNegate(py)), doubleMultiply(nx, px)));
        return doubleDivide(select(second, first, wordLessEqual(second, first)), length);
    }
    var t = doubleDivide(doubleAdd(doubleMultiply(px, nx), doubleMultiply(py, ny)), length);
    if (GRADIENT_SHAPE == 1u && gradientLess(t, doubleNegate(epsilon))) { t = doubleNegate(t); }
    return t;
}
fn gradientRepeated(value: Word64) -> Word64 {
    let one = Word64(0u, 0x3ff00000u); let epsilon = Word64(0u, 0x3cb00000u);
    if (parameters.patternSampling.y == 0u) {
        if (gradientLess(value, epsilon)) { return Word64(0u); }
        if (gradientLess(doubleSubtract(one, epsilon), value)) { return one; }
        return value;
    }
    let integral = doubleFloor(doubleMagnitude(value));
    if (parameters.patternSampling.y == 1u) {
        var integer = integral;
        if ((value.y & 0x80000000u) != 0u) { integer = doubleNegate(integer); }
        if (gradientLess(value, epsilon)) { integer = doubleSubtract(integer, one); }
        return doubleSubtract(value, integer);
    }
    if (GRADIENT_SHAPE >= 6u) {
        var fraction = doubleSubtract(doubleMagnitude(value), integral);
        let halfIntegral = doubleFloor(doubleMultiply(doubleMagnitude(value), doubleInteger(2u)));
        if (patternCoordinate(halfIntegral, 2u).pixel == 1) { fraction = doubleSubtract(one, fraction); }
        return doubleMultiply(fraction, doubleInteger(2u));
    }
    var fraction = doubleSubtract(doubleMagnitude(value), integral);
    if (patternCoordinate(integral, 2u).pixel == 1) { fraction = doubleSubtract(one, fraction); }
    return fraction;
}
fn gradientCached(value: Word64) -> vec4<u32> {
    let count = parameters.pattern.x;
    let index = min(count - 1u, doubleTruncate(doubleAdd(
        doubleMultiply(value, doubleInteger(count - 1u)), Word64(0u, 0x3fe00000u))));
    return nativePixel(patternPixels[index * 2u], patternPixels[index * 2u + 1u], true);
}
fn gradientColor(pixel: vec2<i32>) -> vec4<u32> {
    let one = Word64(0u, 0x3ff00000u); let half = Word64(0u, 0x3fe00000u);
    var t = gradientPosition(pixel);
    let dx = doubleSubtract(parameters.patternMap1.xy, parameters.patternMap0.xy);
    let dy = doubleSubtract(parameters.patternMap1.zw, parameters.patternMap0.zw);
    let length = doubleSqrt(doubleAdd(doubleMultiply(dx, dx), doubleMultiply(dy, dy)));
    let antiAlias = parameters.patternMap2.xy;
    let hasAntialias = gradientLess(Word64(0u, 0x3cb00000u), antiAlias);
    let shape = GRADIENT_SHAPE;
    let spiralSeam = shape >= 6u && parameters.patternSampling.y == 0u && hasAntialias;
    var threshold = doubleDivide(antiAlias, length);
    var seamSmoothing = parameters.patternSampling.y == 1u && (shape < 4u || shape >= 6u) && hasAntialias;
    var distance = Word64(0u);
    var seam = Word64(0u);
    if ((shape == 4u && hasAntialias) || spiralSeam) {
        let px = doubleSubtract(markerInteger(pixel.x), parameters.patternMap0.xy);
        let py = doubleSubtract(markerInteger(pixel.y), parameters.patternMap0.zw);
        distance = doubleSqrt(doubleAdd(doubleMultiply(px, px), doubleMultiply(py, py)));
        var singularity = doubleInteger(8u);
        if (spiralSeam) { singularity = doubleDivide(length, doubleInteger(32u)); }
        threshold = antiAlias;
        if (gradientLess(distance, singularity)) { threshold = doubleDivide(doubleMultiply(distance, antiAlias), singularity); }
        if (any(distance != Word64(0u))) {
            threshold = doubleDivide(threshold, doubleMultiply(Word64(0x54442d18u, 0x401921fbu), distance));
        }
        seamSmoothing = any(distance != Word64(0u)) && (!spiralSeam || gradientLess(distance, length));
        if (spiralSeam) { seam = gradientRepeated(doubleDivide(gradientAngle(px, py, dx, dy), Word64(0x54442d18u, 0x401921fbu))); }
    }
    let center = seamSmoothing && shape >= 1u && shape <= 3u && !gradientLess(threshold, t);
    if (!center) { t = gradientRepeated(t); }
    if ((parameters.patternSampling.x & 1u) != 0u) { t = doubleSubtract(one, t); }
    var color = gradientCached(t);
    if (!spiralSeam) { seam = t; }
    let reverseThreshold = doubleSubtract(one, threshold);
    if (seamSmoothing && !center && (!gradientLess(threshold, seam) || !gradientLess(seam, reverseThreshold))) {
        let twice = doubleMultiply(doubleInteger(2u), threshold);
        var s = doubleDivide(doubleSubtract(seam, reverseThreshold), twice);
        if (!gradientLess(threshold, seam)) { s = doubleAdd(half, doubleDivide(seam, twice)); }
        let weight = min(32767u, doubleTruncate(doubleAdd(doubleMultiply(doubleSubtract(one, s), doubleInteger(32767u)), half)));
        var first = gradientCached(one); var second = gradientCached(Word64(0u));
        if (spiralSeam) {
            if ((parameters.patternSampling.x & 1u) != 0u) {
                distance = doubleSubtract(length, distance); first = gradientCached(Word64(0u));
            }
            let radial = gradientCached(doubleDivide(distance, length));
            second = radial;
            if (shape == 7u) { second = first; first = radial; }
        }
        let a = first.w * weight; let b = second.w * (32767u - weight);
        let alpha = a + b;
        color = vec4<u32>();
        if (alpha != 0u) {
            for (var channel = 0u; channel < 3u; channel++) {
                color[channel] = patternQuotient(wordAdd(wordProduct(first[channel], a), wordProduct(second[channel], b)), alpha, 65535u);
            }
            color.w = (alpha + 16383u) / 32767u;
        }
    }
    return color;
}
fn gradientPixel(pixel: vec2<i32>) -> vec4<u32> {
    let color = gradientColor(pixel);
    let wide = parameters.native.z != 0u;
    if ((parameters.patternSampling.x & 2u) == 0u) {
        if (wide) { return color; }
        return (color - (color >> vec4<u32>(8u)) + vec4<u32>(128u)) >> vec4<u32>(8u);
    }
    let noiseIndex = ((u32(pixel.y) & 63u) << 6u) | (u32(pixel.x) & 63u);
    let noise = f32(patternPixels[parameters.pattern.x * 2u + noiseIndex]) * (1.0 / 4096.0) + (1.0 / 8192.0);
    let scale = select(1.0 / 256.0, 1.0 / 65536.0, wide);
    let maximum = select(255.0, 65535.0, wide);
    var result = vec4<u32>();
    for (var channel = 0u; channel < 4u; channel++) {
        let normalized = floatDivide(bitcast<u32>(f32(color[channel])), bitcast<u32>(65535.0));
        let delta = floatMultiply(floatSubtract(bitcast<u32>(noise), normalized), bitcast<u32>(scale));
        let value = floatMultiply(floatAdd(normalized, delta), bitcast<u32>(maximum));
        result[channel] = u32(clamp(bitcast<f32>(floatAdd(value, bitcast<u32>(0.5))), 0.0, maximum));
    }
    return result;
}

