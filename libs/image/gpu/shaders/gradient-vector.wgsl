// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Linear, bi-linear, radial and square gradient coordinates and seam smoothing.

fn gradientPosition(pixel: vec2<i32>) -> f32 {
    let zero = 0.0; let epsilon = 2.220446049250313e-16;
    let dx = (parameters.patternMap0.z - parameters.patternMap0.x);
    let dy = (parameters.patternMap0.w - parameters.patternMap0.y);
    let length = sqrt(max(((dx * dx) + (dy * dy)), 0.0));
    if (GRADIENT_SHAPE < 4u && (length < epsilon)) { return zero; }
    let px = (f32(pixel.x) - parameters.patternMap0.x);
    let py = (f32(pixel.y) - parameters.patternMap0.y);
    if (GRADIENT_SHAPE == 2u) {
        return (sqrt(max(((px * px) + (py * py)), 0.0)) / length);
    }
    let nx = (dx / length); let ny = (dy / length);
    if (GRADIENT_SHAPE == 3u) {
        if (!(epsilon < length)) { return zero; }
        let first = abs((((-ny) * px) + (nx * py)));
        let second = abs((((-ny) * (-py)) + (nx * px)));
        return (select(second, first, (second <= first)) / length);
    }
    var t = (((px * nx) + (py * ny)) / length);
    if (GRADIENT_SHAPE == 1u && (t < (-epsilon))) { t = (-t); }
    return t;
}

fn gradientColor(pixel: vec2<i32>) -> vec4<u32> {
    let one = 1.0; let half = 0.5;
    var t = gradientPosition(pixel);
    let dx = (parameters.patternMap0.z - parameters.patternMap0.x);
    let dy = (parameters.patternMap0.w - parameters.patternMap0.y);
    let length = sqrt(max(((dx * dx) + (dy * dy)), 0.0));
    let threshold = (parameters.patternMap1.x / length);
    let seamSmoothing = parameters.patternSampling.y == 1u && (2.220446049250313e-16 < parameters.patternMap1.x);
    let center = seamSmoothing && GRADIENT_SHAPE != 0u && !(threshold < t);
    if (!center) { t = gradientRepeated(t); }
    if ((parameters.patternSampling.x & 1u) != 0u) { t = (one - t); }
    let reverseThreshold = (one - threshold);
    if (seamSmoothing && !center && (!(threshold < t) || !(t < reverseThreshold))) {
        let twice = (f32(2u) * threshold);
        var s = ((t - reverseThreshold) / twice);
        if (!(threshold < t)) { s = (half + (t / twice)); }
        let weight = min(32767u, u32(max((((one - s) * f32(32767u)) + half), 0.0)));
        return gradientMix(gradientCached(one), gradientCached(0.0), weight);
    }
    return gradientCached(t);
}
