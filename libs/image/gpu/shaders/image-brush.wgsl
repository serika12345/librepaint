// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

fn imageByteMultiply(a: u32, b: u32) -> u32 {
    let value = a * b + 128u;
    return (value + (value >> 8u)) >> 8u;
}
fn imageTipTexel(dab: MaskDab, point: vec2<i32>) -> vec2<u32> {
    let size = vec2<i32>(dab.vertical.xy);
    let p = vec2<u32>(clamp(point, vec2<i32>(0), size - vec2<i32>(1)));
    let index = p.y * dab.vertical.x + p.x;
    let word = patternPixels[parameters.textureStorage.y + dab.vertical.z + index / 2u];
    let value = word >> ((index & 1u) * 16u);
    return vec2<u32>(value & 255u, (value >> 8u) & 255u);
}
fn imageWordMultiply(a: u32, b: u32) -> u32 {
    let value = a * b;
    return (value + (value >> 16u) + 32768u) >> 16u;
}
fn imageInterpolatePair(a: u32, b: u32, fraction: u32) -> u32 {
    if (fraction == 0u) { return a; }
    return ((a * (65536u - fraction)) >> 16u) + ((b * fraction) >> 16u);
}
fn imageInterpolate(tl: u32, tr: u32, bl: u32, br: u32, x: u32, y: u32) -> u32 {
    return imageInterpolatePair(imageInterpolatePair(tl, bl, y), imageInterpolatePair(tr, br, y), x);
}
fn imageWordToByte(value: u32) -> u32 {
    let rounded = value + 128u;
    return (rounded - (rounded >> 8u)) >> 8u;
}
fn imageStraightByte(value: u32, alpha: u32) -> u32 {
    let numerator = value * 255u;
    let quotient = numerator / alpha;
    let remainder = numerator % alpha;
    return quotient + u32(remainder * 2u > alpha || (remainder * 2u == alpha && (quotient & 1u) != 0u));
}
fn imageBrushAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    if ((dab.size.w & 512u) != 0u) {
        let texel = imageTipTexel(dab, vec2<i32>(point) + vec2<i32>(1));
        return imageByteMultiply(255u - texel.x, texel.y);
    }
    let row = dab.vertical.w + point.y * 4u;
    let first = geometryWords[row];
    let count = geometryWords[row + 1u];
    if (point.x < first || point.x - first >= count) { return 0u; }
    let chunkIndex = (point.x - first) / 2048u;
    let chunk = first + chunkIndex * 2048u;
    let origin = dab.vertical.w + geometryWords[row + 2u] + chunkIndex * 2u;
    let fx = bitcast<i32>(geometryWords[origin]) + bitcast<i32>(dab.coefficient.x) * i32(point.x - chunk);
    let fy = bitcast<i32>(geometryWords[origin + 1u]) + bitcast<i32>(dab.coefficient.y) * i32(point.x - chunk);
    let p = vec2<i32>(fx >> 16, fy >> 16);
    let tl = imageTipTexel(dab, p); let tr = imageTipTexel(dab, p + vec2<i32>(1, 0));
    let bl = imageTipTexel(dab, p + vec2<i32>(0, 1)); let br = imageTipTexel(dab, p + vec2<i32>(1, 1));
    let x = u32(fx) & 65535u; let y = u32(fy) & 65535u;
    // Native ARGB32 targets interpolate premultiplied 16-bit components.
    let alpha = imageInterpolate(tl.y * 257u, tr.y * 257u, bl.y * 257u, br.y * 257u, x, y);
    if (alpha == 0u) { return 0u; }
    let premultiplied = imageInterpolate(imageWordMultiply(tl.x * 257u, tl.y * 257u),
        imageWordMultiply(tr.x * 257u, tr.y * 257u), imageWordMultiply(bl.x * 257u, bl.y * 257u),
        imageWordMultiply(br.x * 257u, br.y * 257u), x, y);
    let red = min(255u, imageStraightByte(premultiplied, alpha));
    return imageByteMultiply(255u - red, imageWordToByte(alpha));
}
