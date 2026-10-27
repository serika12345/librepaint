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
fn circleAlpha(dab: MaskDab, point: vec2<u32>) -> u32 {
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
    if (distance <= start) { return 255u; }
    let inverse = floatDivide(floatMultiply(floatSubtract(distance, start), floatFromDouble(dab.coefficient.xy)), 0x437f0000u);
    return projectionComponent(floatMultiply(0x437f0000u, floatSubtract(0x3f800000u, inverse)));
}
