// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Gaussian coverage rounds y*y before the single-rounded x*x + y*y sum.
fn gaussianSquaredDistance(x: u32, y: u32) -> u32 {
    let xx = x & 0x7fffffffu;
    let yy = floatMultiply(y, y);
    if (xx == 0u) { return yy; }
    if (yy == 0u) { return floatMultiply(x, x); }
    let product = projectionProduct(projectionMantissa(xx), projectionMantissa(xx));
    let productPower = 2 * projectionPower(xx);
    let yPower = projectionPower(yy);
    // Align the larger term at bit 55, retaining low guard space before a
    // discarded tail becomes sticky. The positive sum fits in 57 bits.
    let productTop = productPower + i32(63u - countLeadingZeros(product.y));
    let power = max(productTop, yPower + 23) - 55;
    var first = product;
    if (productPower < power) { first = wordJam(first, u32(power - productPower)); }
    else { first = wordLeft(first, u32(productPower - power)); }
    var second = Word64(projectionMantissa(yy), 0u);
    if (yPower < power) { second = wordJam(second, u32(power - yPower)); }
    else { second = wordLeft(second, u32(yPower - power)); }
    return projectionFloat(wordAdd(first, second), power);
}

struct MaskDab {
    size: vec4<u32>,
    center: vec4<u32>,
    rotation: vec4<u32>,
    radius: vec4<u32>,
    coefficient: vec4<u32>,
    application: vec4<u32>,
}

// The standard generated circle shares dab placement and WASH accumulation
// with Gaussian tips; its native fade is a rational squared-distance function.
fn standardCircleSample(dab: MaskDab, dx: Word64, dy: Word64) -> u32 {
    let one = Word64(0u, 0x3ff00000u);
    let nx = doubleMultiply(dx, dab.radius.xy);
    let ny = doubleMultiply(dy, dab.radius.xy);
    let n = doubleAdd(doubleMultiply(nx, nx), doubleMultiply(ny, ny));
    if (!wordLessEqual(n, one)) { return 255u; }
    var x = Word64(dx.x, dx.y & 0x7fffffffu);
    var y = Word64(dy.x, dy.y & 0x7fffffffu);
    if ((dab.size.w & 2u) != 0u) {
        x = doubleAdd(x, one);
        y = doubleAdd(y, one);
    }
    let fx = doubleMultiply(x, dab.radius.zw);
    let fy = doubleMultiply(y, dab.radius.zw);
    let nf = doubleAdd(doubleMultiply(fx, fx), doubleMultiply(fy, fy));
    if (!wordLessEqual(one, nf)) { return 0u; }
    let numerator = doubleMultiply(doubleMultiply(doubleInteger(255u), n), doubleSubtract(nf, one));
    return min(255u, doubleTruncate(doubleDivide(numerator, doubleSubtract(nf, n))));
}
fn smallStandardCircleAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    let side = dab.size.z;
    let step = select(Word64(0x55555555u, 0x3fd55555u), Word64(0x55555555u, 0x3fc55555u), side == 6u);
    var total = 0u;
    for (var y = 0u; y < side; y += 1u) {
        for (var x = 0u; x < side; x += 1u) {
            let dx = doubleSubtract(doubleAdd(doubleInteger(point.x), doubleMultiply(doubleInteger(x), step)), dab.center.xy);
            let dy = doubleSubtract(doubleAdd(doubleInteger(point.y), doubleMultiply(doubleInteger(y), step)), dab.center.zw);
            total += standardCircleSample(dab, dx, dy);
        }
    }
    return 255u - total / (side * side);
}
fn standardCircleAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32) -> u32 {
    let dx = floatSubtract(bitcast<u32>(f32(point.x)), floatFromDouble(dab.center.xy));
    let dy = floatSubtract(bitcast<u32>(f32(point.y)), floatFromDouble(dab.center.zw));
    let coefficient = floatFromDouble(dab.radius.xy);
    let nx = floatMultiply(dx, coefficient);
    let ny = floatMultiply(dy, coefficient);
    let n = floatAdd(floatMultiply(nx, nx), floatMultiply(ny, ny));
    if (n > 0x3f800000u) { return 0u; }
    var x = dx & 0x7fffffffu;
    var y = dy & 0x7fffffffu;
    if ((dab.size.w & 2u) != 0u) {
        x = floatAdd(x, 0x3f800000u);
        y = floatAdd(y, 0x3f800000u);
    }
    let fade = floatFromDouble(dab.radius.zw);
    let fx = floatMultiply(x, fade);
    let fy = floatMultiply(y, fade);
    let nf = floatAdd(floatMultiply(fx, fx), floatMultiply(fy, fy));
    if (nf < 0x3f800000u) { return maximum; }
    if (nf == n) { return 0u; }
    let inverse = floatDivide(floatMultiply(n, floatSubtract(nf, 0x3f800000u)), floatSubtract(nf, n));
    let alpha = floatMultiply(bitcast<u32>(f32(maximum)), floatSubtract(0x3f800000u, inverse));
    if (maximum == 65535u) { return u32(bitcast<f32>(alpha)); }
    return projectionComponent(alpha);
}

fn smallCircleAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
    let side = dab.size.z;
    var inverseStep = Word64(0u, 0x3ff00000u);
    if (side == 3u) { inverseStep = Word64(0x55555555u, 0x3fd55555u); }
    if (side == 6u) { inverseStep = Word64(0x55555555u, 0x3fc55555u); }
    var total = 0u;
    for (var y = 0u; y < side; y += 1u) {
        for (var x = 0u; x < side; x += 1u) {
            let dx = doubleSubtract(doubleAdd(doubleInteger(point.x), doubleMultiply(doubleInteger(x), inverseStep)), dab.center.xy);
            let dy = doubleSubtract(doubleAdd(doubleInteger(point.y), doubleMultiply(doubleInteger(y), inverseStep)), dab.center.zw);
            let xr = doubleSubtract(doubleMultiply(dab.rotation.xy, dx), doubleMultiply(dab.rotation.zw, dy));
            let yr = doubleAdd(doubleMultiply(dab.rotation.zw, dx), doubleMultiply(dab.rotation.xy, dy));
            let distance = doubleSqrt(doubleAdd(doubleMultiply(xr, xr), doubleMultiply(yr, yr)));
            if (!wordLessEqual(distance, dab.radius.xy)) { total += 255u; }
            else if (!wordLessEqual(distance, dab.radius.zw)) {
                total += min(255u, doubleTruncate(doubleMultiply(doubleSubtract(distance, dab.radius.zw), dab.coefficient.xy)));
            }
        }
    }
    return 255u - total / (side * side);
}
fn circleAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32) -> u32 {
    let dx = floatSubtract(bitcast<u32>(f32(point.x)), floatFromDouble(dab.center.xy));
    let dy = floatSubtract(bitcast<u32>(f32(point.y)), floatFromDouble(dab.center.zw));
    let cosine = floatFromDouble(dab.rotation.xy);
    let sine = floatFromDouble(dab.rotation.zw);
    let xr = floatSubtract(floatMultiply(dx, cosine), floatMultiply(sine, dy));
    let yr = floatAdd(floatMultiply(dx, sine), floatMultiply(cosine, dy));
    let distance = floatSqrt(gaussianSquaredDistance(xr, yr));
    let radius = floatFromDouble(dab.radius.xy);
    let start = floatFromDouble(dab.radius.zw);
    if (distance > radius) { return 0u; }
    if (distance <= start) { return maximum; }
    let inverse = floatDivide(floatMultiply(floatSubtract(distance, start), floatFromDouble(dab.coefficient.xy)), 0x437f0000u);
    let alpha = floatMultiply(bitcast<u32>(f32(maximum)), floatSubtract(0x3f800000u, inverse));
    if (maximum == 65535u) { return u32(bitcast<f32>(alpha)); }
    return projectionComponent(alpha);
}
