// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

fn multiply8(a: u32, b: u32) -> u32 {
    let product = a * b + 128u;
    return (product + (product >> 8u)) >> 8u;
}

fn unpack(pixel: u32) -> vec4<u32> {
    return vec4<u32>(pixel, pixel >> 8u, pixel >> 16u, pixel >> 24u) & vec4<u32>(255u);
}

fn quantize(value: f32) -> u32 {
    let bounded = clamp(value, 0.0, 255.0);
    let lower = u32(floor(bounded));
    let fraction = bounded - f32(lower);
    return lower + select(0u, 1u, fraction > 0.5 || (fraction == 0.5 && (lower & 1u) != 0u));
}

fn over(source: vec4<u32>, destination: vec4<u32>, opacity: u32, coverage: u32) -> vec4<u32> {
    var alpha = f32(source.a) * (f32(opacity) * (1.0 / 255.0));
    alpha *= f32(coverage) * (1.0 / 255.0);
    if (alpha == 0.0) { return destination; }
    let resultAlpha = f32(destination.a) + (255.0 - f32(destination.a)) * alpha * (1.0 / 255.0);
    let blend = alpha / resultAlpha;
    let color = blend * (vec3<f32>(source.rgb) - vec3<f32>(destination.rgb)) + vec3<f32>(destination.rgb);
    return vec4<u32>(quantize(color.r), quantize(color.g), quantize(color.b), quantize(resultAlpha));
}

fn blend(source: vec4<u32>, destination: vec4<u32>, operation: u32, opacity: u32, coverage: u32) -> vec4<u32> {
    if (operation == 2u) {
        let alpha = multiply8(multiply8(source.a, coverage), opacity);
        return vec4<u32>(destination.rgb, multiply8(destination.a, 255u - alpha));
    }
    return over(source, destination, opacity, coverage);
}

fn pack(pixel: vec4<u32>) -> u32 {
    return pixel.r | (pixel.g << 8u) | (pixel.b << 16u) | (pixel.a << 24u);
}
