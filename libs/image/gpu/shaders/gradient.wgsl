// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Frozen gradient resource sampling, repetition, native mixing and quantization.



fn gradientRepeated(value: f32) -> f32 {
    let one = 1.0; let epsilon = 2.220446049250313e-16;
    if (parameters.patternSampling.y == 0u) {
        if ((value < epsilon)) { return 0.0; }
        if (((one - epsilon) < value)) { return one; }
        return value;
    }
    let integral = floor(abs(value));
    if (parameters.patternSampling.y == 1u) {
        var integer = integral;
        if ((value < 0.0)) { integer = (-integer); }
        if ((value < epsilon)) { integer = (integer - one); }
        return (value - integer);
    }
    if (parameters.pattern.y >= 6u) {
        var fraction = (abs(value) - integral);
        let halfIntegral = floor((abs(value) * f32(2u)));
        if (patternCoordinate(halfIntegral, 2u).pixel == 1) { fraction = (one - fraction); }
        return (fraction * f32(2u));
    }
    var fraction = (abs(value) - integral);
    if (patternCoordinate(integral, 2u).pixel == 1) { fraction = (one - fraction); }
    return fraction;
}

fn gradientCached(value: f32) -> vec4<u32> {
    let count = parameters.pattern.x;
    let index = min(count - 1u, u32(max(((value * f32(count - 1u)) + 0.5), 0.0)));
    return nativePixel(patternPixels[index * 2u], patternPixels[index * 2u + 1u], true);
}

fn gradientMix(first: vec4<u32>, second: vec4<u32>, weight: u32) -> vec4<u32> {
    let a = first.w * weight; let b = second.w * (32767u - weight);
    let alpha = a + b;
    var color = vec4<u32>();
    if (alpha != 0u) {
        for (var channel = 0u; channel < 3u; channel++) {
            color[channel] = patternQuotient(((f32(first[channel]) * f32(a)) + (f32(second[channel]) * f32(b))), alpha, 65535u);
        }
        color.w = (alpha + 16383u) / 32767u;
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
        let normalized = safeRatio(f32(color[channel]), 65535.0);
        let delta = ((noise - normalized) * scale);
        let value = ((normalized + delta) * maximum);
        result[channel] = u32(clamp((value + 0.5), 0.0, maximum));
    }
    return result;
}
