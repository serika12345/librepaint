// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-FileCopyrightText: Johan Mabille, Sylvain Corlay, Wolf Vollprecht, Martin Renou, QuantStack, Serge Guelton
// SPDX-FileCopyrightText: 2016 NumScale SAS
// SPDX-License-Identifier: GPL-2.0-or-later AND BSD-3-Clause AND BSL-1.0

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
    vertical: vec4<u32>,
    texture: vec4<u32>,
    paperOffset: vec4<i32>,
}

// Standard ellipse and rectangle tips share dab placement and WASH accumulation
// with Gaussian tips. Their independent fades follow the native mask formulas.
fn standardCircleSample(dab: MaskDab, dx: Word64, dy: Word64) -> u32 {
    let one = Word64(0u, 0x3ff00000u);
    let xr = doubleSubtract(doubleMultiply(dab.rotation.xy, dx), doubleMultiply(dab.rotation.zw, dy));
    let yr = doubleAdd(doubleMultiply(dab.rotation.zw, dx), doubleMultiply(dab.rotation.xy, dy));
    var x = doubleMagnitude(xr);
    var y = doubleMagnitude(yr);
    let nx = doubleMultiply(x, dab.radius.xy);
    let ny = doubleMultiply(y, dab.vertical.xy);
    let n = doubleAdd(doubleMultiply(nx, nx), doubleMultiply(ny, ny));
    let rectangle = (dab.size.w & 16u) != 0u;
    if (rectangle) {
        if (!wordLessEqual(nx, one) || !wordLessEqual(ny, one)) { return 255u; }
    } else if (!wordLessEqual(n, one)) { return 255u; }
    if ((dab.size.w & 2u) != 0u) {
        x = doubleAdd(x, one);
        y = doubleAdd(y, one);
    }
    let fx = doubleMultiply(x, dab.radius.zw);
    let fy = doubleMultiply(y, dab.vertical.zw);
    if (rectangle) {
        let xDenominator = doubleSubtract(fx, nx);
        let xNumerator = doubleMultiply(nx, doubleSubtract(fx, one));
        var fade = Word64(0u);
        if (!wordLessEqual(fx, one)) {
            fade = doubleDivide(doubleMultiply(nx, doubleSubtract(fx, one)), xDenominator);
        }
        if (!wordLessEqual(fy, one)) {
            let vertical = doubleDivide(doubleMultiply(ny, doubleSubtract(fy, one)), doubleSubtract(fy, ny));
            // Native zero denominators distinguish a negative infinity from
            // 0/0 when comparing the horizontal and vertical fades.
            var beforeVertical = (xNumerator.y & 0x80000000u) != 0u && any(doubleMagnitude(xNumerator) != Word64(0u));
            if (any(doubleMagnitude(xDenominator) != Word64(0u))) {
                let horizontal = doubleDivide(xNumerator, xDenominator);
                beforeVertical = (horizontal.y & 0x80000000u) != 0u || !wordLessEqual(vertical, horizontal);
            }
            if (beforeVertical) { fade = vertical; }
        }
        return min(255u, doubleTruncate(doubleMultiply(doubleInteger(255u), fade)));
    }
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
fn standardCircleAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32, maskPainting: bool) -> u32 {
    let dx = floatSubtract(bitcast<u32>(f32(point.x)), floatFromDouble(dab.center.xy));
    let dy = floatSubtract(bitcast<u32>(f32(point.y)), floatFromDouble(dab.center.zw));
    let cosine = floatFromDouble(dab.rotation.xy);
    let sine = floatFromDouble(dab.rotation.zw);
    let rectangle = (dab.size.w & 16u) != 0u;
    var x = floatSubtract(floatMultiply(dx, cosine), floatMultiply(sine, dy)) & 0x7fffffffu;
    var y = floatAdd(floatMultiply(dx, sine), floatMultiply(cosine, dy)) & 0x7fffffffu;
    if (rectangle) {
        // Native rectangle rows use a single-rounded coordinate multiply-add;
        // standard circle rows round their products separately.
        x = floatMultiplyAdd(dx, cosine, floatMultiply(sine, dy) ^ 0x80000000u) & 0x7fffffffu;
        y = floatMultiplyAdd(dx, sine, floatMultiply(cosine, dy)) & 0x7fffffffu;
    }
    let coefficient = floatFromDouble(dab.radius.xy);
    let nx = floatMultiply(x, coefficient);
    let ny = floatMultiply(y, floatFromDouble(dab.vertical.xy));
    let n = floatAdd(floatMultiply(nx, nx), floatMultiply(ny, ny));
    if (rectangle) {
        if (nx > 0x3f800000u || ny > 0x3f800000u) { return 0u; }
    } else if (n > 0x3f800000u) { return 0u; }
    if ((dab.size.w & 2u) != 0u) {
        x = floatAdd(x, 0x3f800000u);
        y = floatAdd(y, 0x3f800000u);
    }
    let fade = floatFromDouble(dab.radius.zw);
    let fx = floatMultiply(x, fade);
    let fy = floatMultiply(y, floatFromDouble(dab.vertical.zw));
    var inverse = 0u;
    if (rectangle) {
        let xDenominator = floatSubtract(fx, nx);
        let xNumerator = floatMultiply(nx, floatSubtract(fx, 0x3f800000u));
        if (fx > 0x3f800000u) {
            inverse = floatDivide(floatMultiply(nx, floatSubtract(fx, 0x3f800000u)), xDenominator);
        }
        if (fy > 0x3f800000u) {
            let vertical = floatDivide(floatMultiply(ny, floatSubtract(fy, 0x3f800000u)), floatSubtract(fy, ny));
            var beforeVertical = (xNumerator & 0x80000000u) != 0u && (xNumerator & 0x7fffffffu) != 0u;
            if ((xDenominator & 0x7fffffffu) != 0u) {
                let horizontal = floatDivide(xNumerator & 0x7fffffffu, xDenominator & 0x7fffffffu);
                beforeVertical = ((xNumerator ^ xDenominator) & 0x80000000u) != 0u || horizontal < vertical;
            }
            if (beforeVertical) { inverse = vertical; }
        }
    } else {
        let nf = floatAdd(floatMultiply(fx, fx), floatMultiply(fy, fy));
        if (nf < 0x3f800000u) { return maximum; }
        if (nf == n) { return 0u; }
        inverse = floatDivide(floatMultiply(n, floatSubtract(nf, 0x3f800000u)), floatSubtract(nf, n));
    }
    let alpha = floatMultiply(bitcast<u32>(f32(maximum)), floatSubtract(0x3f800000u, inverse));
    // Native GrayA8 and U16 mask application truncate; RGB8 rounds to nearest-even.
    if (maximum == 65535u || maskPainting) { return u32(bitcast<f32>(alpha)); }
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
fn circleAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32, maskPainting: bool) -> u32 {
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
    // Native GrayA8 and U16 mask application truncate; RGB8 rounds to nearest-even.
    if (maximum == 65535u || maskPainting) { return u32(bitcast<f32>(alpha)); }
    return projectionComponent(alpha);
}

// The native vector Gaussian uses VcExtraMath::erf. Its exp reduction and
// coefficients follow xsimd 14.3's common float kernel (original Boost.SIMD).
// Integer binary32 operations fix the rounding independently of GPU hardware.
fn gaussianNegativeExp(value: u32) -> u32 {
    let magnitude = projectionComponent(floatMultiply(value & 0x7fffffffu, 0x3fb8aa3bu));
    let integral = bitcast<u32>(-f32(magnitude));
    var reduced = floatMultiplyAdd(integral ^ 0x80000000u, 0x3f318000u, value);
    reduced = floatMultiplyAdd(integral ^ 0x80000000u, 0xb95e8083u, reduced);
    var polynomial = floatMultiplyAdd(reduced, 0x3ab778cfu, 0x3c098d8bu);
    polynomial = floatMultiplyAdd(reduced, polynomial, 0x3d2aa957u);
    polynomial = floatMultiplyAdd(reduced, polynomial, 0x3e2aa9a5u);
    polynomial = floatMultiplyAdd(reduced, polynomial, 0x3f000000u);
    let approximate = floatAdd(0x3f800000u, floatMultiplyAdd(polynomial, floatMultiply(reduced, reduced), reduced));
    return u32(i32(approximate) - i32(magnitude) * 8388608);
}
fn gaussianVectorErf(value: u32) -> u32 {
    let magnitude = value & 0x7fffffffu;
    const one = 0x3f800000u;
    let sign = select(value & 0x80000000u, 0u, magnitude == 0u);
    if (magnitude >= 0x4114cccdu) { return one | sign; }
    let t = floatDivide(0x3f800000u, floatAdd(0x3f800000u, floatMultiply(0x3ea7ba05u, magnitude)));
    var polynomial = floatAdd(floatMultiply(0x3f87dc22u, t), 0xbfba00e3u);
    polynomial = floatAdd(floatMultiply(polynomial, t), 0x3fb5f0e3u);
    polynomial = floatAdd(floatMultiply(polynomial, t), 0xbe91a98eu);
    polynomial = floatAdd(floatMultiply(polynomial, t), 0x3e827906u);
    polynomial = floatMultiply(polynomial, t);
    let exponential = gaussianNegativeExp(floatMultiply(magnitude, magnitude) | 0x80000000u);
    return floatSubtract(0x3f800000u, floatMultiply(polynomial, exponential)) ^ sign;
}
fn unaliasedGaussianAlpha(dab: MaskDab, point: vec2<u32>, maximum: u32, maskPainting: bool) -> u32 {
    let dx = floatSubtract(bitcast<u32>(f32(point.x)), floatFromDouble(dab.center.xy));
    let dy = floatSubtract(bitcast<u32>(f32(point.y)), floatFromDouble(dab.center.zw));
    let cosine = floatFromDouble(dab.rotation.xy);
    let sine = floatFromDouble(dab.rotation.zw);
    let xr = floatSubtract(floatMultiply(dx, cosine), floatMultiply(sine, dy));
    let yr = floatMultiply(floatAdd(floatMultiply(dx, sine), floatMultiply(cosine, dy)), dab.vertical.w);
    let distance = floatSqrt(gaussianSquaredDistance(xr, yr));
    if (distance > floatFromDouble(dab.radius.xy)) { return 0u; }
    let argument = floatMultiply(distance, dab.vertical.y);
    var fullFade = floatMultiply(dab.vertical.z, floatSubtract(
        gaussianVectorErf(floatAdd(argument, dab.vertical.x)),
        gaussianVectorErf(floatSubtract(argument, dab.vertical.x))));
    if ((fullFade & 0x80000000u) != 0u) { fullFade = 0u; }
    else if (fullFade > 0x437ef958u) { fullFade = 0x437f0000u; }
    let inverse = floatDivide(floatSubtract(0x437f0000u, fullFade), 0x437f0000u);
    let alpha = floatMultiply(bitcast<u32>(f32(maximum)), floatSubtract(0x3f800000u, inverse));
    if (maximum == 65535u || maskPainting) { return u32(bitcast<f32>(alpha)); }
    return projectionComponent(alpha);
}
