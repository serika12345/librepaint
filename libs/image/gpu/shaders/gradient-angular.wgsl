// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Conical, symmetric conical and spiral geometry and seam smoothing.

fn gradientAtan2(y: f32, x: f32) -> f32 {
    if (x == 0.0 && y == 0.0) { return 0.0; }
    return atan2(y, x);
}

fn gradientAngle(px: f32, py: f32, dx: f32, dy: f32) -> f32 {
    let pi = 3.1415927410125732;
    var angle = ((gradientAtan2(py, px) + pi) - (gradientAtan2(dy, dx) + pi));
    if ((angle < 0.0)) { angle = (angle + 6.2831854820251465); }
    return angle;
}

fn gradientPosition(pixel: vec2<i32>) -> f32 {
    let zero = 0.0; let epsilon = 2.220446049250313e-16;
    let dx = (parameters.patternMap0.z - parameters.patternMap0.x);
    let dy = (parameters.patternMap0.w - parameters.patternMap0.y);
    let length = sqrt(max(((dx * dx) + (dy * dy)), 0.0));
    if (GRADIENT_SHAPE < 4u && (length < epsilon)) { return zero; }
    let px = (f32(pixel.x) - parameters.patternMap0.x);
    let py = (f32(pixel.y) - parameters.patternMap0.y);
    if (GRADIENT_SHAPE >= 4u) {
        let angle = gradientAngle(px, py, dx, dy);
        let pi = 3.1415927410125732;
        let one = 1.0;
        let turn = (angle / 6.2831854820251465);
        if (GRADIENT_SHAPE == 4u) { return turn; }
        if (GRADIENT_SHAPE == 5u) {
            if ((angle < pi)) { return (angle / pi); }
            return (one - ((angle - pi) / pi));
        }
        var radius = 0.0;
        if (!(length < epsilon)) {
            radius = (sqrt(max(((px * px) + (py * py)), 0.0)) / length);
        }
        if (GRADIENT_SHAPE == 7u) { return (radius + (one - turn)); }
        return (radius + turn);
    }
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
    let antiAlias = parameters.patternMap1.x;
    let hasAntialias = (2.220446049250313e-16 < antiAlias);
    let shape = GRADIENT_SHAPE;
    let spiralSeam = shape >= 6u && parameters.patternSampling.y == 0u && hasAntialias;
    var threshold = (antiAlias / length);
    var seamSmoothing = parameters.patternSampling.y == 1u && (shape < 4u || shape >= 6u) && hasAntialias;
    var distance = 0.0;
    var seam = 0.0;
    if ((shape == 4u && hasAntialias) || spiralSeam) {
        let px = (f32(pixel.x) - parameters.patternMap0.x);
        let py = (f32(pixel.y) - parameters.patternMap0.y);
        distance = sqrt(max(((px * px) + (py * py)), 0.0));
        var singularity = f32(8u);
        if (spiralSeam) { singularity = (length / f32(32u)); }
        threshold = antiAlias;
        if ((distance < singularity)) { threshold = ((distance * antiAlias) / singularity); }
        if (distance != 0.0) {
            threshold = (threshold / (6.2831854820251465 * distance));
        }
        seamSmoothing = distance != 0.0 && (!spiralSeam || (distance < length));
        if (spiralSeam) { seam = gradientRepeated((gradientAngle(px, py, dx, dy) / 6.2831854820251465)); }
    }
    let center = seamSmoothing && shape >= 1u && shape <= 3u && !(threshold < t);
    if (!center) { t = gradientRepeated(t); }
    if ((parameters.patternSampling.x & 1u) != 0u) { t = (one - t); }
    var color = gradientCached(t);
    if (!spiralSeam) { seam = t; }
    let reverseThreshold = (one - threshold);
    if (seamSmoothing && !center && (!(threshold < seam) || !(seam < reverseThreshold))) {
        let twice = (f32(2u) * threshold);
        var s = ((seam - reverseThreshold) / twice);
        if (!(threshold < seam)) { s = (half + (seam / twice)); }
        let weight = min(32767u, u32(max((((one - s) * f32(32767u)) + half), 0.0)));
        var first = gradientCached(one); var second = gradientCached(0.0);
        if (spiralSeam) {
            if ((parameters.patternSampling.x & 1u) != 0u) {
                distance = (length - distance); first = gradientCached(0.0);
            }
            let radial = gradientCached((distance / length));
            second = radial;
            if (shape == 7u) { second = first; first = radial; }
        }
        color = gradientMix(first, second, weight);
    }
    return color;
}
