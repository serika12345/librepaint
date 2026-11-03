// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct MaskDab {
    size: vec4<u32>,
    center: vec2<f32>,
    rotation: vec2<f32>,
    radius: vec2<f32>,
    coefficient: vec2<u32>,
    origin: vec2<i32>,
    application: vec4<u32>,
    vertical: vec4<u32>,
    texture: vec4<u32>,
    paperOffset: vec4<i32>,
}

fn standardCircleInverse(dab: MaskDab, delta: vec2<f32>) -> f32 {
    var point = abs(vec2<f32>(
        dab.rotation.x * delta.x - dab.rotation.y * delta.y,
        dab.rotation.y * delta.x + dab.rotation.x * delta.y));
    let normalized = point * vec2<f32>(dab.radius.x, bitcast<f32>(dab.vertical.x));
    let n = dot(normalized, normalized);
    let rectangle = (dab.size.w & 16u) != 0u;
    if (rectangle) {
        if (any(normalized > vec2<f32>(1.0))) { return 1.0; }
    } else if (n > 1.0) { return 1.0; }
    if ((dab.size.w & 2u) != 0u) { point += vec2<f32>(1.0); }
    let faded = point * vec2<f32>(dab.radius.y, bitcast<f32>(dab.vertical.y));
    if (rectangle) {
        var fade = vec2<f32>();
        for (var axis = 0u; axis < 2u; axis++) {
            if (faded[axis] > 1.0 && faded[axis] != normalized[axis]) {
                fade[axis] = normalized[axis] * (faded[axis] - 1.0) / (faded[axis] - normalized[axis]);
            }
        }
        return clamp(max(fade.x, fade.y), 0.0, 1.0);
    }
    let nf = dot(faded, faded);
    if (nf < 1.0) { return 0.0; }
    if (nf == n) { return 1.0; }
    return clamp(n * (nf - 1.0) / (nf - n), 0.0, 1.0);
}

fn smallStandardCircleAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    let side = dab.size.z;
    var total = 0u;
    for (var y = 0u; y < side; y++) {
        for (var x = 0u; x < side; x++) {
            let delta = vec2<f32>(point) + vec2<f32>(f32(x), f32(y)) / f32(side) - dab.center;
            total += u32(255.0 * standardCircleInverse(dab, delta));
        }
    }
    return 255u - total / (side * side);
}
fn brushAlpha(value: f32, maximum: u32, maskPainting: bool) -> u32 {
    let scaled = clamp(value, 0.0, 1.0) * f32(maximum);
    if (maximum == 65535u || maskPainting) { return u32(scaled); }
    return u32(round(scaled));
}
fn standardCircleAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32, maskPainting: bool) -> u32 {
    let inverse = standardCircleInverse(dab, vec2<f32>(point) - dab.center);
    return brushAlpha(1.0 - inverse, maximum, maskPainting);
}
fn smallCircleAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    let side = dab.size.z;
    var total = 0u;
    let coefficient = bitcast<f32>(dab.coefficient.x);
    for (var y = 0u; y < side; y++) {
        for (var x = 0u; x < side; x++) {
            let delta = vec2<f32>(point) + vec2<f32>(f32(x), f32(y)) / f32(side) - dab.center;
            let distance = length(delta);
            if (distance > dab.radius.x) { total += 255u; }
            else if (distance > dab.radius.y) { total += u32((distance - dab.radius.y) * coefficient); }
        }
    }
    return 255u - total / (side * side);
}
fn circleAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32, maskPainting: bool) -> u32 {
    let delta = vec2<f32>(point) - dab.center;
    let rotated = vec2<f32>(dab.rotation.x * delta.x - dab.rotation.y * delta.y,
                             dab.rotation.y * delta.x + dab.rotation.x * delta.y);
    let distance = length(rotated);
    if (distance > dab.radius.x) { return 0u; }
    if (distance <= dab.radius.y) { return maximum; }
    let inverse = (distance - dab.radius.y) * bitcast<f32>(dab.coefficient.x) / 255.0;
    return brushAlpha(1.0 - inverse, maximum, maskPainting);
}

// Five-term approximation of the Gaussian integral, evaluated with native f32.
fn gaussianErf(value: f32) -> f32 {
    let magnitude = abs(value);
    let t = 1.0 / (1.0 + 0.3275911 * magnitude);
    let polynomial = (((((1.061405429 * t - 1.453152027) * t) + 1.421413741) * t
                       - 0.284496736) * t + 0.254829592) * t;
    return sign(value) * (1.0 - polynomial * exp(-magnitude * magnitude));
}
fn unaliasedGaussianAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32, maskPainting: bool) -> u32 {
    let delta = vec2<f32>(point) - dab.center;
    let vertical = bitcast<vec4<f32>>(dab.vertical);
    let rotated = vec2<f32>(dab.rotation.x * delta.x - dab.rotation.y * delta.y,
        (dab.rotation.y * delta.x + dab.rotation.x * delta.y) * vertical.w);
    let distance = length(rotated);
    if (distance > dab.radius.x) { return 0u; }
    let argument = distance * vertical.y;
    var fade = max(0.0, vertical.z * (gaussianErf(argument + vertical.x) - gaussianErf(argument - vertical.x)));
    if (fade > 254.974) { fade = 255.0; }
    return brushAlpha(fade / 255.0, maximum, maskPainting);
}
