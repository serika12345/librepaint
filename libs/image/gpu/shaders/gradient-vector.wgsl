// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Linear, bi-linear, radial and square gradient coordinates and seam smoothing.

fn gradientPosition(pixel: vec2<i32>) -> Word64 {
    let zero = Word64(0u); let epsilon = Word64(0u, 0x3cb00000u);
    let dx = doubleSubtract(parameters.patternMap1.xy, parameters.patternMap0.xy);
    let dy = doubleSubtract(parameters.patternMap1.zw, parameters.patternMap0.zw);
    let length = doubleSqrt(doubleAdd(doubleMultiply(dx, dx), doubleMultiply(dy, dy)));
    if (GRADIENT_SHAPE < 4u && gradientLess(length, epsilon)) { return zero; }
    let px = doubleSubtract(markerInteger(pixel.x), parameters.patternMap0.xy);
    let py = doubleSubtract(markerInteger(pixel.y), parameters.patternMap0.zw);
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

fn gradientColor(pixel: vec2<i32>) -> vec4<u32> {
    let one = Word64(0u, 0x3ff00000u); let half = Word64(0u, 0x3fe00000u);
    var t = gradientPosition(pixel);
    let dx = doubleSubtract(parameters.patternMap1.xy, parameters.patternMap0.xy);
    let dy = doubleSubtract(parameters.patternMap1.zw, parameters.patternMap0.zw);
    let length = doubleSqrt(doubleAdd(doubleMultiply(dx, dx), doubleMultiply(dy, dy)));
    let threshold = doubleDivide(parameters.patternMap2.xy, length);
    let seamSmoothing = parameters.patternSampling.y == 1u && gradientLess(Word64(0u, 0x3cb00000u), parameters.patternMap2.xy);
    let center = seamSmoothing && GRADIENT_SHAPE != 0u && !gradientLess(threshold, t);
    if (!center) { t = gradientRepeated(t); }
    if ((parameters.patternSampling.x & 1u) != 0u) { t = doubleSubtract(one, t); }
    let reverseThreshold = doubleSubtract(one, threshold);
    if (seamSmoothing && !center && (!gradientLess(threshold, t) || !gradientLess(t, reverseThreshold))) {
        let twice = doubleMultiply(doubleInteger(2u), threshold);
        var s = doubleDivide(doubleSubtract(t, reverseThreshold), twice);
        if (!gradientLess(threshold, t)) { s = doubleAdd(half, doubleDivide(t, twice)); }
        let weight = min(32767u, doubleTruncate(doubleAdd(doubleMultiply(doubleSubtract(one, s), doubleInteger(32767u)), half)));
        return gradientMix(gradientCached(one), gradientCached(Word64(0u)), weight);
    }
    return gradientCached(t);
}
