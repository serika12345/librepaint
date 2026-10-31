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
fn imageDoubleLessEqual(a: Word64, b: Word64) -> bool {
    let am = doubleMagnitude(a); let bm = doubleMagnitude(b);
    let an = (a.y & 0x80000000u) != 0u && any(am != Word64(0u));
    let bn = (b.y & 0x80000000u) != 0u && any(bm != Word64(0u));
    if (an != bn) { return an; }
    return select(wordLessEqual(am, bm), wordLessEqual(bm, am), an);
}
fn imageLeftEdge(coefficient: Word64, offsetValue: Word64, size: u32) -> Word64 {
    if (all(doubleMagnitude(coefficient) == Word64(0u))) { return Word64(0u); }
    var edge = Word64(0u);
    if ((coefficient.y & 0x80000000u) != 0u) { edge = doubleInteger(size); }
    return doubleDivide(doubleSubtract(edge, offsetValue), coefficient);
}
fn imageSpanFirst(dab: MaskDab, y: u32) -> u32 {
    let cy = doubleAdd(doubleInteger(y), Word64(0u, 0x3fe00000u));
    let xConstant = doubleAdd(doubleMultiply(dab.rotation.xy, cy), dab.radius.xy);
    let yConstant = doubleAdd(doubleMultiply(dab.rotation.zw, cy), dab.radius.zw);
    let a = imageLeftEdge(dab.center.xy, xConstant, dab.vertical.x);
    let b = imageLeftEdge(dab.center.zw, yConstant, dab.vertical.y);
    var left = Word64(0u);
    if (imageDoubleLessEqual(left, a)) { left = a; }
    if (imageDoubleLessEqual(left, b)) { left = b; }
    if (!imageDoubleLessEqual(left, doubleInteger(dab.size.x))) { return dab.size.x; }
    return doubleTruncate(doubleAdd(left, Word64(0u, 0x3fe00000u)));
}
fn imageSignedFixed(value: Word64) -> i32 {
    let scaled = doubleMultiply(value, doubleInteger(65536u));
    let magnitude = i32(doubleTruncate(doubleMagnitude(scaled)));
    return select(magnitude, -magnitude, (scaled.y & 0x80000000u) != 0u);
}
fn imageInterpolate(tl: u32, tr: u32, bl: u32, br: u32, x: u32, y: u32) -> u32 {
    let left = (tl * (256u - y) + bl * y) >> 8u;
    let right = (tr * (256u - y) + br * y) >> 8u;
    return (left * (256u - x) + right * x) >> 8u;
}
fn imageBrushAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    if ((dab.size.w & 512u) != 0u) {
        let texel = imageTipTexel(dab, vec2<i32>(point) + vec2<i32>(1));
        return imageByteMultiply(255u - texel.x, texel.y);
    }
    let first = imageSpanFirst(dab, point.y);
    if (point.x < first) { return 0u; }
    let chunk = first + ((point.x - first) / 2048u) * 2048u;
    let cx = doubleAdd(doubleInteger(chunk), Word64(0u, 0x3fe00000u));
    let cy = doubleAdd(doubleInteger(point.y), Word64(0u, 0x3fe00000u));
    let sx = doubleAdd(doubleAdd(doubleMultiply(dab.rotation.xy, cy), doubleMultiply(dab.center.xy, cx)), dab.radius.xy);
    let sy = doubleAdd(doubleAdd(doubleMultiply(dab.rotation.zw, cy), doubleMultiply(dab.center.zw, cx)), dab.radius.zw);
    let fx = imageSignedFixed(sx) - 32768 + bitcast<i32>(dab.coefficient.x) * i32(point.x - chunk);
    let fy = imageSignedFixed(sy) - 32768 + bitcast<i32>(dab.coefficient.y) * i32(point.x - chunk);
    let p = vec2<i32>(fx >> 16, fy >> 16);
    let tl = imageTipTexel(dab, p); let tr = imageTipTexel(dab, p + vec2<i32>(1, 0));
    let bl = imageTipTexel(dab, p + vec2<i32>(0, 1)); let br = imageTipTexel(dab, p + vec2<i32>(1, 1));
    let x = (u32(fx) & 65535u) >> 8u; let y = (u32(fy) & 65535u) >> 8u;
    let alpha = imageInterpolate(tl.y, tr.y, bl.y, br.y, x, y);
    if (alpha == 0u) { return 0u; }
    let premultiplied = imageInterpolate(imageByteMultiply(tl.x, tl.y), imageByteMultiply(tr.x, tr.y),
        imageByteMultiply(bl.x, bl.y), imageByteMultiply(br.x, br.y), x, y);
    let red = select((premultiplied * (0x00ff00ffu / alpha) + 32768u) >> 16u, premultiplied, alpha == 255u);
    return imageByteMultiply(255u - red, alpha);
}
